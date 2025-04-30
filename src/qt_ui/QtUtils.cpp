#include "qt_ui/QtUtils.h"

namespace QtUtils {

QImage matToQImage(const cv::Mat& mat) {
    // Handle empty matrix
    if (mat.empty()) {
        return QImage();
    }

    // Convert the OpenCV Mat format to QImage format based on channels and depth
    switch (mat.type()) {
        // 8-bit, 1 channel
        case CV_8UC1: {
            QImage image(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_Grayscale8);
            return image.copy(); // Make a deep copy since mat.data may be deleted
        }

        // 8-bit, 3 channels (BGR)
        case CV_8UC3: {
            // Create a deep copy to be sure we don't lose data when returning
            cv::Mat rgbMat;
            cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);
            QImage image(rgbMat.data, rgbMat.cols, rgbMat.rows, static_cast<int>(rgbMat.step), QImage::Format_RGB888);
            return image.copy();
        }

        // 8-bit, 4 channels (BGRA)
        case CV_8UC4: {
            // Create a deep copy to be sure we don't lose data when returning
            cv::Mat rgbaMat;
            cv::cvtColor(mat, rgbaMat, cv::COLOR_BGRA2RGBA);
            QImage image(rgbaMat.data, rgbaMat.cols, rgbaMat.rows, static_cast<int>(rgbaMat.step), QImage::Format_RGBA8888);
            return image.copy();
        }

        default:
            // Unsupported format - convert to 8-bit RGB
            cv::Mat temp;
            mat.convertTo(temp, CV_8UC3);
            return matToQImage(temp);
    }
}

QPixmap matToQPixmap(const cv::Mat& mat) {
    return QPixmap::fromImage(matToQImage(mat));
}

cv::Mat qImageToMat(const QImage& image) {
    // Handle empty image
    if (image.isNull()) {
        return cv::Mat();
    }

    // Convert QImage to OpenCV Mat based on format
    switch (image.format()) {
        // 8-bit, 1 channel
        case QImage::Format_Grayscale8: {
            cv::Mat mat(image.height(), image.width(), CV_8UC1, const_cast<uchar*>(image.bits()), static_cast<size_t>(image.bytesPerLine()));
            return mat.clone(); // Make a deep copy since image.bits() may be deleted
        }

        // 8-bit, 3 channels (RGB)
        case QImage::Format_RGB888: {
            cv::Mat mat(image.height(), image.width(), CV_8UC3, const_cast<uchar*>(image.bits()), static_cast<size_t>(image.bytesPerLine()));
            cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR); // Convert RGB to BGR
            return mat.clone();
        }

        // 8-bit, 4 channels (RGBA)
        case QImage::Format_RGBA8888: {
            cv::Mat mat(image.height(), image.width(), CV_8UC4, const_cast<uchar*>(image.bits()), static_cast<size_t>(image.bytesPerLine()));
            cv::cvtColor(mat, mat, cv::COLOR_RGBA2BGRA); // Convert RGBA to BGRA
            return mat.clone();
        }

        // For other formats, convert to RGB888 then process
        default: {
            QImage convertedImage = image.convertToFormat(QImage::Format_RGB888);
            cv::Mat mat(convertedImage.height(), convertedImage.width(), CV_8UC3, const_cast<uchar*>(convertedImage.bits()), static_cast<size_t>(convertedImage.bytesPerLine()));
            cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR); // Convert RGB to BGR
            return mat.clone();
        }
    }
}

} // namespace QtUtils 