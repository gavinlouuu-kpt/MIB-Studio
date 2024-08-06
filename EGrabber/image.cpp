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
#include <iostream>
#include <conio.h>

class CircularBuffer {
public:
    CircularBuffer(size_t size, size_t imageSize)
        : buffer_(size* imageSize), size_(size), imageSize_(imageSize), head_(0), tail_(0) {}

    void push(const uint8_t* data) {
        std::copy(data, data + imageSize_, buffer_.begin() + (head_ * imageSize_));
        head_ = (head_ + 1) % size_;
        if (head_ == tail_) {
            tail_ = (tail_ + 1) % size_;
        }
    }

    std::vector<uint8_t> get(size_t index) const {
        size_t actualIndex = (tail_ + index) % size_;
        return std::vector<uint8_t>(buffer_.begin() + (actualIndex * imageSize_),
            buffer_.begin() + ((actualIndex + 1) * imageSize_));
    }

    std::vector<uint8_t> getLatest() const {
        size_t latestIndex = (head_ == 0) ? size_ - 1 : head_ - 1;
        return std::vector<uint8_t>(buffer_.begin() + (latestIndex * imageSize_),
            buffer_.begin() + ((latestIndex + 1) * imageSize_));
    }

    size_t getLatestIndex() const {
        return (head_ == 0) ? size_ - 1 : head_ - 1;
    }

    size_t size() const { return size_; }

private:
    std::vector<uint8_t> buffer_;
    size_t size_;
    size_t imageSize_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
};

using namespace Euresys;

void configure_js() {
    EGenTL gentl;
    EGrabber<> grabber(gentl);
    grabber.runScript("./config.js");
}


void printBufferInfo(ScopedBuffer& buffer, size_t i) {
    //// Get and print pixel format
    //uint64_t pixelFormat = buffer.getInfo<uint64_t>(gc::BUFFER_INFO_PIXELFORMAT);
    //std::cout << "Pixel Format: " << std::hex << pixelFormat << std::dec << std::endl;

    //// Get and print buffer width
    //size_t width = buffer.getInfo<size_t>(gc::BUFFER_INFO_WIDTH);
    //std::cout << "Width: " << width << " pixels" << std::endl;

    //// Get and print buffer height
    //size_t height = buffer.getInfo<size_t>(gc::BUFFER_INFO_HEIGHT);
    //std::cout << "Height: " << height << " pixels" << std::endl;

    // Get and print timestamp
    uint64_t timestamp = buffer.getInfo<uint64_t>(gc::BUFFER_INFO_TIMESTAMP);
    std::cout << i << ".Timestamp: " << timestamp << " us" << std::endl;

    // Get and print the base address of the buffer memory
    //uint8_t* baseAddress = buffer.getInfo<uint8_t*>(gc::BUFFER_INFO_BASE);
    //std::cout << i <<". Base Address: " << static_cast<void*>(baseAddress) << std::endl;

    //// Get and print the size of the buffer
    //size_t bufferSize = buffer.getInfo<size_t>(gc::BUFFER_INFO_SIZE);
    //std::cout << "Buffer Size: " << bufferSize << " bytes" << std::endl;
}

void processingThreadTask(std::queue<size_t>& framesToProcess, std::mutex& queueMutex, std::condition_variable& queueCondition, bool& done) {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCondition.wait(lock, [&]() { return !framesToProcess.empty() || done; });

        if (done && framesToProcess.empty()) {
            break;
        }

        size_t frame = framesToProcess.front();
        framesToProcess.pop();
        lock.unlock();

        // Print a message to the console
        std::cout << "Processing frame " << frame << std::endl;

        // Empty processing task
        // Add your processing code here if needed
    }
}

static void sample() {
    EGenTL genTL; // load GenTL producer
    EGrabberDiscovery egrabberDiscovery(genTL);
    egrabberDiscovery.discover();
    EGrabber<CallbackOnDemand> grabber(egrabberDiscovery.cameras(0));

    size_t bufferCount = 5000;
    grabber.reallocBuffers(3); // prepare 3 buffers for internal use

    // Get image dimensions and pixel format from the first buffer
    grabber.start(1);
    ScopedBuffer firstBuffer(grabber);
    size_t width = firstBuffer.getInfo<size_t>(gc::BUFFER_INFO_WIDTH);
    size_t height = firstBuffer.getInfo<size_t>(gc::BUFFER_INFO_HEIGHT);
    uint64_t pixelFormat = firstBuffer.getInfo<uint64_t>(gc::BUFFER_INFO_PIXELFORMAT);
    size_t imageSize = firstBuffer.getInfo<size_t>(gc::BUFFER_INFO_SIZE);
    grabber.stop();

    // Allocate memory for the new buffer to hold 5000 images
    CircularBuffer circularBuffer(bufferCount, imageSize);

    // Synchronization primitives for the producer-consumer pattern
    std::queue<size_t> framesToProcess;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::atomic<bool> done(false);
    std::atomic<bool> paused(false);
    int currentFrameIndex = -1;
    std::mutex currentFrameIndexMutex;

    auto processingThreadTask = [&]() {
        while (!done) {
            if (!paused) {
                std::unique_lock<std::mutex> lock(queueMutex);
                queueCondition.wait(lock, [&]() { return !framesToProcess.empty() || done; });

                if (framesToProcess.empty() && done) break;

                if (!framesToProcess.empty()) {
                    size_t frame = framesToProcess.front();
                    framesToProcess.pop();
                    lock.unlock();

                    auto imageData = circularBuffer.get(frame % bufferCount);
                    std::cout << "Processing frame " << frame << std::endl;
                    // Process imageData here
                }
            }
            cv::waitKey(1);
        }
    };

    auto displayThreadTask = [&]() {
        while (!done) {
            if (paused) {
                std::lock_guard<std::mutex> lock(currentFrameIndexMutex);
                auto imageData = circularBuffer.get(currentFrameIndex);
                cv::Mat image(height, width, CV_8UC1, imageData.data()); // Adjust CV_8UC1 based on pixel format
                cv::imshow("Live Feed", image);
                cv::waitKey(40);
            }
            else {
                std::unique_lock<std::mutex> lock(queueMutex);
                queueCondition.wait(lock, [&]() { return !framesToProcess.empty() || done; });

                if (framesToProcess.empty() && done) break;
                if (!framesToProcess.empty()) {
                    size_t frame = framesToProcess.front();
                    framesToProcess.pop();
                    lock.unlock();
                    auto imageData = circularBuffer.getLatest();
                    cv::Mat image(height, width, CV_8UC1, imageData.data()); // Adjust CV_8UC1 based on pixel format
                    cv::imshow("Live Feed", image);
                    cv::waitKey(1);
                }
            }
        }
    };

    std::thread workerThread(processingThreadTask);
    std::thread displayThread(displayThreadTask);

    grabber.start();
    size_t frameCount = 0;
    while (!done) {  // Run indefinitely until stopped
        if (paused) {
            //currentFrameIndex = circularBuffer.getLatestIndex();
            // if 'a' key is hit minus currentFrameIndex by 1 and the oppose for 'd' key
            if (_kbhit()) {
				int ch = _getch();
				if (ch == 27) {
					std::cout << "ESC pressed. Stopping capture..." << std::endl;
					done = true;
				}
				else if (ch == 32) { // Space bar
					paused = !paused;
                    std::cout << (paused ? "Paused" : "Resumed") << std::endl;
				}
                else if (ch == 97) { // 'a' key
                    std::lock_guard<std::mutex> lock(currentFrameIndexMutex);
                    currentFrameIndex = (currentFrameIndex == 0) ? bufferCount - 1 : currentFrameIndex - 1;
                    std::cout << "a key pressed\nCurrent Frame Index: " << currentFrameIndex << std::endl;
                }
                else if (ch == 100) { // 'd' key
                    std::lock_guard<std::mutex> lock(currentFrameIndexMutex);
                    currentFrameIndex = (currentFrameIndex == bufferCount - 1) ? 0 : currentFrameIndex + 1;
                    std::cout << "d key pressed\nCurrent Frame Index: " << currentFrameIndex << std::endl;
                }
			}
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
        
            ScopedBuffer buffer(grabber);
            printBufferInfo(buffer, frameCount);

            uint8_t* imagePointer = buffer.getInfo<uint8_t*>(gc::BUFFER_INFO_BASE);
            circularBuffer.push(imagePointer);

            {
                std::lock_guard<std::mutex> lock(queueMutex);
                framesToProcess.push(frameCount);
            }
            queueCondition.notify_one();

            frameCount++;

		
        // Check for stop condition (e.g., user input, max frames, etc.)
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 27) {
                std::cout << "ESC pressed. Stopping capture..." << std::endl;
                done = true;
            }
            else if (ch == 32) { // Space bar
                paused = !paused;
                if (paused) {
                    std::lock_guard<std::mutex> lock(currentFrameIndexMutex);
                    currentFrameIndex = circularBuffer.getLatestIndex();
                }
                std::cout << (paused ? "Paused" : "Resumed") << std::endl;
            }
        }
        cv::waitKey(1);
    }

    grabber.stop();
    workerThread.join();
    displayThread.join();

}

int main() {
    try {
        //grab_live();
        sample();
        //high_sample();
    }
    catch (const std::exception& e) {
        std::cout << "error: " << e.what() << std::endl;
    }
    //open_cv_test();
}


