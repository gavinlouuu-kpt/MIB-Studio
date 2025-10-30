#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "mib/api/IMibController.h"
#include "mib/api/Frame.h"

#include "CircularBuffer/CircularBuffer.h"
#include "image_processing/image_processing.h"
#include <opencv2/imgproc.hpp>

namespace mib {

class MibController final : public IMibController {
public:
    MibController() = default;
    ~MibController() override { stop(); }

    void start() override {
        if (running_.exchange(true)) return;

        if (imageDirectory_.empty()) {
            publishError(1, "image_dir not set; call setParam('image_dir', <path>) before start");
            running_ = false;
            return;
        }

        try {
            // Init params and buffers
            params_ = initializeImageParams(imageDirectory_);
            cameraBuffer_ = std::make_unique<CircularBuffer>(params_.bufferCount, params_.imageSize);
            circularBuffer_ = std::make_unique<CircularBuffer>(params_.bufferCount, params_.imageSize);
            processingBuffer_ = std::make_unique<CircularBuffer>(params_.bufferCount, params_.imageSize);
            loadImages(imageDirectory_, *cameraBuffer_, true);

            // Shared state
            shared_ = std::make_unique<SharedResources>();
            initializeMockBackgroundFrame(*shared_, params_, *cameraBuffer_);
            shared_->roi = cv::Rect(0, 0, static_cast<int>(params_.width), static_cast<int>(params_.height));
            shared_->saveDirectory = selectSaveDirectory("config.json");

            // Threads
            threads_.emplace_back(simulateCameraThread, std::ref(*cameraBuffer_), std::ref(*shared_), std::ref(params_));
            threads_.emplace_back(processingThreadTask,
                                  std::ref(shared_->processingQueueMutex), std::ref(shared_->processingQueueCondition),
                                  std::ref(shared_->framesToProcess), std::ref(*processingBuffer_),
                                  params_.width, params_.height, std::ref(*shared_));
            threads_.emplace_back(resultSavingThread, std::ref(*shared_), std::cref(shared_->saveDirectory));

            // Dispatcher: feed frames from camera to processing/display queues (no UI)
            threads_.emplace_back([this]() { dispatcherLoop(); });

            // Publisher: emit frames to observers from validFramesQueue
            threads_.emplace_back([this]() { publisherLoop(); });

            publishStatus("started");
        } catch (const std::exception& e) {
            publishError(2, std::string("start failed: ") + e.what());
            running_ = false;
        }
    }

    void stop() override {
        if (!running_.exchange(false)) return;
        if (shared_) {
            shared_->done = true;
            // Wake all waiters
            shared_->validFramesCondition.notify_all();
            shared_->displayQueueCondition.notify_all();
            shared_->processingQueueCondition.notify_all();
            shared_->savingCondition.notify_all();
            shared_->scatterDataCondition.notify_all();
            shared_->triggerCondition.notify_all();
            shared_->manualTriggerCondition.notify_all();
        }
        for (auto& t : threads_) {
            if (t.joinable()) t.join();
        }
        threads_.clear();
        cameraBuffer_.reset();
        circularBuffer_.reset();
        processingBuffer_.reset();
        shared_.reset();
        publishStatus("stopped");
    }

    void setParam(const std::string& key, const std::string& value) override {
        if (key == "image_dir") {
            imageDirectory_ = value;
            return;
        }
        if (key == "roi" && shared_) {
            int x = 0, y = 0, w = 0, h = 0;
            if (std::sscanf(value.c_str(), "%d,%d,%d,%d", &x, &y, &w, &h) == 4) {
                std::lock_guard<std::mutex> lock(shared_->roiMutex);
                shared_->roi = cv::Rect(x, y, w, h);
                shared_->displayNeedsUpdate = true;
            }
            return;
        }
        // Extend with other parameters as needed
    }

    void onKey(int keyCode) override {
        if (!shared_ || !circularBuffer_) return;
        handleKeypress(keyCode, *circularBuffer_, params_.bufferCount, params_.width, params_.height, *shared_);
    }

    void subscribe(Observer* observer) override {
        if (!observer) return;
        std::lock_guard<std::mutex> lock(observersMutex_);
        observers_.push_back(observer);
    }

    void unsubscribe(Observer* observer) override {
        std::lock_guard<std::mutex> lock(observersMutex_);
        observers_.erase(std::remove(observers_.begin(), observers_.end(), observer), observers_.end());
    }

private:
    void dispatcherLoop() {
        size_t lastProcessedFrame = 0;
        while (running_ && shared_ && !shared_->done) {
            if (shared_->paused) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            size_t latestFrame = shared_->latestCameraFrame.load(std::memory_order_acquire);
            if (latestFrame != lastProcessedFrame) {
                const uint8_t* imageData = cameraBuffer_->getPointer(latestFrame);
                if (imageData != nullptr) {
                    circularBuffer_->push(imageData);
                    processingBuffer_->push(imageData);
                    {
                        std::lock_guard<std::mutex> processingLock(shared_->processingQueueMutex);
                        shared_->framesToProcess.push(latestFrame);
                    }
                    shared_->processingQueueCondition.notify_one();
                    lastProcessedFrame = latestFrame;
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    void publisherLoop() {
        while (running_ && shared_ && !shared_->done) {
            std::unique_lock<std::mutex> lock(shared_->validFramesMutex);
            shared_->validFramesCondition.wait_for(lock, std::chrono::milliseconds(50), [this]() {
                return !running_ || (shared_ && (shared_->done || shared_->newValidFrameAvailable));
            });
            if (!running_ || !shared_ || shared_->done) break;

            if (shared_->newValidFrameAvailable && !shared_->validFramesQueue.empty()) {
                auto vf = shared_->validFramesQueue.front();
                shared_->newValidFrameAvailable = false;
                lock.unlock();

                // Build composite BGR image with overlay and ROI
                cv::Mat displayImage;
                cv::cvtColor(vf.originalImage, displayImage, cv::COLOR_GRAY2BGR);
                if (shared_->overlayMode) {
                    cv::Mat mask = (vf.processedImage > 0);
                    cv::Mat overlay = cv::Mat::zeros(displayImage.size(), CV_8UC3);
                    cv::Scalar overlayColor = determineOverlayColor(vf.result, vf.result.isValid);
                    const double opacity = 0.3;
                    overlay.setTo(overlayColor, mask);
                    cv::addWeighted(displayImage, 1.0, overlay, opacity, 0, displayImage);
                }
                {
                    std::lock_guard<std::mutex> roiLock(shared_->roiMutex);
                    cv::rectangle(displayImage, shared_->roi, cv::Scalar(0, 255, 0), 1);
                }

                // Prepare frame buffer (BGR)
                std::vector<uint8_t> buffer(static_cast<size_t>(displayImage.total() * displayImage.channels()));
                if (displayImage.isContinuous()) {
                    std::memcpy(buffer.data(), displayImage.data, buffer.size());
                } else {
                    size_t rowBytes = static_cast<size_t>(displayImage.cols * displayImage.channels());
                    for (int r = 0; r < displayImage.rows; ++r) {
                        std::memcpy(buffer.data() + r * rowBytes, displayImage.ptr(r), rowBytes);
                    }
                }

                Frame f{};
                f.data = buffer.data();
                f.sizeBytes = buffer.size();
                f.width = displayImage.cols;
                f.height = displayImage.rows;
                f.format = PixelFormat::BGR8;
                f.timestampNs = static_cast<std::uint64_t>(vf.timestamp) * 1000000ULL; // ms → ns

                notifyFrame(f, buffer);
            }
        }
    }

    void notifyFrame(const Frame& frame, const std::vector<uint8_t>& backing) {
        (void)backing; // backing ensures lifetime until after observers return
        std::lock_guard<std::mutex> lock(observersMutex_);
        for (auto* obs : observers_) {
            obs->onFrame(frame);
        }
    }

    void publishStatus(const std::string& message) {
        std::lock_guard<std::mutex> lock(observersMutex_);
        for (auto* obs : observers_) {
            obs->onStatus(message);
        }
    }

    void publishError(int code, const std::string& message) {
        std::lock_guard<std::mutex> lock(observersMutex_);
        for (auto* obs : observers_) {
            obs->onError(code, message);
        }
    }

    std::atomic<bool> running_{false};
    std::mutex observersMutex_;
    std::vector<Observer*> observers_;

    std::string imageDirectory_;
    ImageParams params_{};
    std::unique_ptr<CircularBuffer> cameraBuffer_;
    std::unique_ptr<CircularBuffer> circularBuffer_;
    std::unique_ptr<CircularBuffer> processingBuffer_;
    std::unique_ptr<SharedResources> shared_;
    std::vector<std::thread> threads_;
};

} // namespace mib

// Factory
namespace mib {
std::unique_ptr<IMibController> createMibController() {
    return std::unique_ptr<IMibController>(new MibController());
}
} // namespace mib


