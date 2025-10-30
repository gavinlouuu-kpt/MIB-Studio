#include <chrono>
#include <iostream>
#include <thread>

#include "mib/api/MibFactory.h"

using namespace std::chrono_literals;

int main() {
    auto controller = mib::createMibController();

    // Usage: mib_cli <image_dir>
    // Keep this lightweight; avoid interactive prompts
    const char* imageDir = nullptr;
#if defined(__linux__) || defined(__APPLE__)
    // On Linux/macOS, argv is typical; but keep non-interactive for now
#endif

    // Try an environment variable first to avoid hardcoding
    imageDir = std::getenv("MIB_IMAGE_DIR");
    if (!imageDir) {
        std::cerr << "MIB_IMAGE_DIR not set; CLI will exit without running.\n";
        return 0;
    }

    controller->setParam("image_dir", imageDir);
    controller->start();

    // Run briefly to demonstrate pipeline without hanging
    std::this_thread::sleep_for(3s);
    controller->stop();
    return 0;
}


