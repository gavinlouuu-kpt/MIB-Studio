#include "qt/main_window.h"
#include <QApplication>
#include <opencv2/opencv.hpp>

/**
 * Simple test application to verify Qt integration
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Create main window
    mib::ui::MainWindow mainWindow;
    mainWindow.show();

    // Create a test image
    cv::Mat testImage(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));

    // Draw some shapes on the test image
    cv::circle(testImage, cv::Point(320, 240), 100, cv::Scalar(0, 0, 255), -1);
    cv::rectangle(testImage, cv::Rect(200, 150, 240, 180), cv::Scalar(0, 255, 0), 3);
    cv::putText(testImage, "MIB-Studio Qt Test", cv::Point(150, 400),
                cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);

    // Display the test image
    mainWindow.displayImage(testImage);

    return app.exec();
}