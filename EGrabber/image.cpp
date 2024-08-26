#include <iostream>  
#include <filesystem>
#include <EGrabber.h>
#include <opencv2/opencv.hpp>
#include <FormatConverter.h>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <conio.h>
#include <chrono>
#include <iomanip>

class CircularBuffer {
public:
    CircularBuffer(size_t size, size_t imageSize)
        : buffer_(size* imageSize), size_(size), imageSize_(imageSize), head_(0), count_(0) {}

    void push(const uint8_t* data) {
        std::copy(data, data + imageSize_, buffer_.begin() + (head_ * imageSize_));
        head_ = (head_ + 1) % size_;
        if (count_ < size_) count_++;
    }

    std::vector<uint8_t> get(size_t index) const {
        if (index >= count_) throw std::out_of_range("Index out of range");
        size_t actualIndex = (head_ - 1 - index + size_) % size_;
        return std::vector<uint8_t>(buffer_.begin() + (actualIndex * imageSize_),
            buffer_.begin() + ((actualIndex + 1) * imageSize_));
    }

    const uint8_t* getPointer(size_t index) const {
        if (index >= count_) throw std::out_of_range("Index out of range");
        size_t actualIndex = (head_ - 1 - index + size_) % size_;
        return buffer_.data() + (actualIndex * imageSize_);
    }

    size_t size() const { return count_; }
    bool isFull() const { return count_ == size_; }

    // Iterator class for CircularBuffer
    class Iterator {
    public:
        Iterator(const CircularBuffer& buffer, size_t index) : buffer_(buffer), index_(index) {}

        std::vector<uint8_t> operator*() const { return buffer_.get(index_); }
        Iterator& operator++() { ++index_; return *this; }
        bool operator!=(const Iterator& other) const { return index_ != other.index_; }

    private:
        const CircularBuffer& buffer_;
        size_t index_;
    };

    // Methods to support range-based for loop
    Iterator begin() const { return Iterator(*this, 0); }
    Iterator end() const { return Iterator(*this, count_); }

private:
    std::vector<uint8_t> buffer_;
    size_t size_;
    size_t imageSize_;
    size_t head_;
    size_t count_;
};

// General template for toString
template <typename T>
inline std::string toString(const T& v) {
    std::stringstream ss;
    ss << v;
    return ss.str();
}

// Specialization for std::string
template <>
inline std::string toString<std::string>(const std::string& v) {
    return v;
}

using namespace Euresys;


struct GrabberParams {
    size_t width;
    size_t height;
    uint64_t pixelFormat;
    size_t imageSize;
    size_t bufferCount;
};

struct SharedResources {
    std::atomic<bool> done{ false };
    std::atomic<bool> paused{ false };
    std::atomic<int> currentFrameIndex{ -1 };
    std::atomic<bool> displayNeedsUpdate{ false };
    std::atomic<size_t> frameRateCount{ 0 };
    std::queue<size_t> framesToProcess;
    std::queue<size_t> framesToDisplay;
    std::mutex displayQueueMutex;
    std::mutex processingQueueMutex;
    std::condition_variable displayQueueCondition;
    std::condition_variable processingQueueCondition;

};


void configure(EGrabber<CallbackOnDemand>& grabber) {
    grabber.setInteger<RemoteModule>("Width", 512);
    grabber.setInteger<RemoteModule>("Height", 96);
    grabber.setInteger<RemoteModule>("AcquisitionFrameRate", 4700);
    // ... (other configuration settings)
}

void configure_js() {
    Euresys::EGenTL gentl;
    Euresys::EGrabber<> grabber(gentl);
    grabber.runScript("config.js");
}

GrabberParams initializeGrabber(EGrabber<CallbackOnDemand>& grabber) {
    grabber.reallocBuffers(3);
    grabber.start(1);
    ScopedBuffer firstBuffer(grabber);

    GrabberParams params;
    params.width = firstBuffer.getInfo<size_t>(gc::BUFFER_INFO_WIDTH);
    params.height = firstBuffer.getInfo<size_t>(gc::BUFFER_INFO_HEIGHT);
    params.pixelFormat = firstBuffer.getInfo<uint64_t>(gc::BUFFER_INFO_PIXELFORMAT);
    params.imageSize = firstBuffer.getInfo<size_t>(gc::BUFFER_INFO_SIZE);
    params.bufferCount = 5000;  // You can adjust this as needed

    grabber.stop();
    return params;
}

void processFrame(const std::vector<uint8_t>& imageData, size_t width, size_t height) {
    // Create OpenCV Mat from the image data
    cv::Mat original(static_cast<int>(height), static_cast<int>(width), CV_8UC1, const_cast<uint8_t*>(imageData.data()));


    // Create binary image
    cv::Mat binary;
    cv::threshold(original, binary, 25, 255, cv::THRESH_BINARY);

    // Add any additional processing steps here
    // For example, you might want to analyze the binary image,
    // perform feature detection, or apply other CV algorithms
}

void processingThreadTask(
    std::atomic<bool>& done,
    std::atomic<bool>& paused,
    std::mutex& processingQueueMutex,
    std::condition_variable& processingQueueCondition,
    std::queue<size_t>& framesToProcess,
    const CircularBuffer& circularBuffer,
    size_t width,
    size_t height
) {
    auto lastPrintTime = std::chrono::steady_clock::now();
    int frameCount = 0;

    while (!done) {
        std::unique_lock<std::mutex> lock(processingQueueMutex);
        processingQueueCondition.wait(lock, [&]() {
            return !framesToProcess.empty() || done || paused;
            });

        if (done) break;

        if (!framesToProcess.empty() && !paused) {
            size_t frame = framesToProcess.front();
            framesToProcess.pop();
            lock.unlock();

            auto imageData = circularBuffer.get(0);

            // Create OpenCV Mat from the image data
            //cv::Mat original(height, width, CV_8UC1, imageData.data());

            //// Create binary image
            //cv::Mat binary;
            //cv::threshold(original, binary, 25, 255, cv::THRESH_BINARY);
            processFrame(imageData, width, height);


            // Increment frame count
            ++frameCount;

            // Check if 5 seconds have passed
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastPrintTime).count();
            if (elapsedTime >= 5) {
                // Calculate frames per second
                double fps = static_cast<double>(frameCount) / elapsedTime;
                std::cout << "Frames processed per second: " << fps << std::endl;

                // Reset frame count and update last print time
                frameCount = 0;
                lastPrintTime = currentTime;
            }
        }
        else {
            lock.unlock();
        }

        // Short sleep to reduce CPU usage when not actively processing
        // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // If you've created a window, make sure to destroy it:
    // cv::destroyWindow("Binary Feed");
}

void onTrackbar(int pos, void* userdata) {
    auto* shared = static_cast<SharedResources*>(userdata);
    shared->currentFrameIndex = pos;
    shared->displayNeedsUpdate = true;
}

void displayThreadTask(
    const std::atomic<bool>& done,
    const std::atomic<bool>& paused,
    const std::atomic<int>& currentFrameIndex,
    std::atomic<bool>& displayNeedsUpdate,
    std::queue<size_t>& framesToDisplay,
    std::mutex& displayQueueMutex,
    const CircularBuffer& circularBuffer,
    size_t width,
    size_t height,
    size_t bufferCount,
    SharedResources& shared
) {
    const std::chrono::duration<double> frameDuration(1.0 / 25.0); // 25 FPS
    auto nextFrameTime = std::chrono::steady_clock::now();

    cv::namedWindow("Live Feed", cv::WINDOW_NORMAL);
    cv::resizeWindow("Live Feed", width, height);

    int trackbarPos = 0;
    cv::createTrackbar("Frame", "Live Feed", &trackbarPos, bufferCount - 1, onTrackbar, &shared);


    while (!done) {
        if (!paused) {
            

            std::unique_lock<std::mutex> lock(displayQueueMutex);
            auto now = std::chrono::steady_clock::now();
            if (now >= nextFrameTime) {
                if (!framesToDisplay.empty()) {
                    size_t frame = framesToDisplay.front();
                    framesToDisplay.pop();
                    lock.unlock();

                    auto imageData = circularBuffer.get(0);
                    cv::Mat image(height, width, CV_8UC1, imageData.data());
                    cv::imshow("Live Feed", image);
                    cv::waitKey(1); // Process GUI events

                    nextFrameTime += std::chrono::duration_cast<std::chrono::steady_clock::duration>(frameDuration);
                    if (nextFrameTime < now) {
                        nextFrameTime = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(frameDuration);
                    }
                }
                else {
                    lock.unlock();
                }
            }
            else {
                lock.unlock();
                cv::waitKey(1); // Process GUI events
            }
        }
        else {
            if (displayNeedsUpdate) {
                
                int index = currentFrameIndex;
                if (index >= 0 && index < circularBuffer.size()) {
                    auto imageData = circularBuffer.get(index);

                    cv::Mat image(height, width, CV_8UC1, imageData.data());

                    if (!image.empty()) {
                        cv::imshow("Live Feed", image);
                        cv::setTrackbarPos("Frame", "Live Feed", index);
                        std::cout << "Displaying frame: " << index << std::endl;
                    }
                    else {
                        std::cout << "Failed to create image from buffer" << std::endl;
                    }
                }
                else {
                    std::cout << "Invalid frame index: " << index << std::endl;
                }
                displayNeedsUpdate = false;
            }
            cv::waitKey(1);
        }
    }
}

void keyboardHandlingThread(
    std::atomic<bool>& done,
    std::atomic<bool>& paused,
    std::atomic<int>& currentFrameIndex,
    std::atomic<bool>& displayNeedsUpdate,
    EGrabber<CallbackOnDemand>& grabber,
    const CircularBuffer& circularBuffer,
    size_t bufferCount
) {
    while (!done) {
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 27) { // ESC key
                std::cout << "ESC pressed. Stopping capture..." << std::endl;
                done = true;
            }
            else if (ch == 32) { // Space bar
                paused = !paused;
                if (paused) {
                    uint64_t fr = grabber.getInteger<StreamModule>("StatisticsFrameRate");
                    uint64_t dr = grabber.getInteger<StreamModule>("StatisticsDataRate");
                    std::cout << "EGrabber Frame Rate: " << fr << " FPS" << std::endl;
                    std::cout << "EGrabber Data Rate: " << dr << " MB/s" << std::endl;
                    grabber.stop();
                    currentFrameIndex = 0;
                    std::cout << "Paused" << std::endl;
                    displayNeedsUpdate = true;
                }
                else {
                    grabber.start();
                    std::cout << "Resumed" << std::endl;
                }
            }
            else if (ch == 97) { // 'a' key - move to older frame
                if (currentFrameIndex < circularBuffer.size() - 1) {
                    currentFrameIndex++;
                    std::cout << "a key pressed\nMoved to older frame. Current Frame Index: " << currentFrameIndex << std::endl;
                    displayNeedsUpdate = true;
                }
                else {
                    std::cout << "Already at oldest frame." << std::endl;
                }
            }
            else if (ch == 100) { // 'd' key - move to newer frame
                if (currentFrameIndex > 0) {
                    currentFrameIndex--;
                    std::cout << "d key pressed\nMoved to newer frame. Current Frame Index: " << currentFrameIndex << std::endl;
                    displayNeedsUpdate = true;
                }
                else {
                    std::cout << "Already at newest frame." << std::endl;
                }
            }
            else if (ch == 115) { // 's' key - save the current frame
                std::filesystem::path outputDir = "output";
                if (!std::filesystem::exists(outputDir)) {
                    std::filesystem::create_directory(outputDir);
                }

                size_t frameCount = circularBuffer.size();
                std::cout << "Saving " << frameCount << " frames..." << std::endl;

                for (size_t i = 0; i < frameCount; ++i) {
                    auto imageData = circularBuffer.get(frameCount - 1 - i);  // Start from oldest frame
                    cv::Mat image(512, 512, CV_8UC1, imageData.data());

                    std::ostringstream oss;
                    oss << "frame_" << std::setw(5) << std::setfill('0') << i << ".png";
                    std::string filename = oss.str();

                    std::filesystem::path fullPath = outputDir / filename;

                    cv::imwrite(fullPath.string(), image);
                    std::cout << "Saved frame " << (i + 1) << "/" << frameCount << ": " << filename << "\r" << std::flush;
                }

                std::cout << "\nAll frames saved in the 'output' directory." << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Small sleep to reduce CPU usage
    }
}

void sample(EGrabber<CallbackOnDemand>& grabber, const GrabberParams& params, CircularBuffer& circularBuffer, SharedResources& shared) {
    std::thread processingThread(processingThreadTask,
        std::ref(shared.done), std::ref(shared.paused),
        std::ref(shared.processingQueueMutex), std::ref(shared.processingQueueCondition),
        std::ref(shared.framesToProcess), std::ref(circularBuffer),
        params.width, params.height);
    std::thread displayThread(displayThreadTask,
        std::ref(shared.done), std::ref(shared.paused), std::ref(shared.currentFrameIndex),
        std::ref(shared.displayNeedsUpdate), std::ref(shared.framesToDisplay),
        std::ref(shared.displayQueueMutex), std::ref(circularBuffer),
        params.width, params.height, params.bufferCount, std::ref(shared));
    std::thread keyboardThread(keyboardHandlingThread,
        std::ref(shared.done), std::ref(shared.paused), std::ref(shared.currentFrameIndex),
        std::ref(shared.displayNeedsUpdate), std::ref(grabber),
        std::ref(circularBuffer), params.bufferCount);

    grabber.start();
    size_t frameCount = 0;
    uint64_t lastFrameId = 0;
    uint64_t duplicateCount = 0;
    while (!shared.done) {
        if (shared.paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            cv::waitKey(1);
            continue;
        }

        ScopedBuffer buffer(grabber);
        uint8_t* imagePointer = buffer.getInfo<uint8_t*>(gc::BUFFER_INFO_BASE);
        uint64_t frameId = buffer.getInfo<uint64_t>(gc::BUFFER_INFO_FRAMEID);
        uint64_t timestamp = buffer.getInfo<uint64_t>(gc::BUFFER_INFO_TIMESTAMP);
        bool isIncomplete = buffer.getInfo<bool>(gc::BUFFER_INFO_IS_INCOMPLETE);
        size_t sizeFilled = buffer.getInfo<size_t>(gc::BUFFER_INFO_SIZE_FILLED);

        if (!isIncomplete) {
            if (frameId <= lastFrameId) {
                ++duplicateCount;
                std::cout << "Duplicate frame detected: FrameID=" << frameId << ", Timestamp=" << timestamp << std::endl;
            }
            else {
                circularBuffer.push(imagePointer);
                {
                    std::lock_guard<std::mutex> displayLock(shared.displayQueueMutex);
                    std::lock_guard<std::mutex> processingLock(shared.processingQueueMutex);
                    shared.framesToProcess.push(frameCount);
                    shared.framesToDisplay.push(frameCount);
                }
                shared.displayQueueCondition.notify_one();
                shared.processingQueueCondition.notify_one();
                frameCount++;
            }
            lastFrameId = frameId;
        }
        else {
            std::cout << "Incomplete frame received: FrameID=" << frameId << std::endl;
        }
    }

    grabber.stop();

    processingThread.join();
    displayThread.join();
    keyboardThread.join();
}


int main() {
    try {
        
        
        EGenTL genTL;
        EGrabberDiscovery egrabberDiscovery(genTL);
        egrabberDiscovery.discover();
        EGrabber<CallbackOnDemand> grabber(egrabberDiscovery.cameras(0));
        //grabber.runScript("config.js");

        
        configure(grabber);
        GrabberParams params = initializeGrabber(grabber);

        CircularBuffer circularBuffer(params.bufferCount, params.imageSize);
        SharedResources shared;

        sample(grabber, params, circularBuffer, shared);
    }
    catch (const std::exception& e) {
        std::cout << "error: " << e.what() << std::endl;
    }
    return 0;
}