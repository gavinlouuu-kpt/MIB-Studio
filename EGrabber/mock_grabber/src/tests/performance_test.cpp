#include "image_processing.h"
#include "CircularBuffer.h"
#include <iostream>
#include <chrono>

int main()
{
    try
    {
        // Setup similar to main_test.cpp
        // ...

        auto start = std::chrono::high_resolution_clock::now();

        // Run your performance test here
        // For example, process a fixed number of frames and measure the time

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Performance test elapsed time: " << elapsed.count() << " seconds" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cout << "error: " << e.what() << std::endl;
    }
    return 0;
}