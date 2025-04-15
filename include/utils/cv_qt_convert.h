#ifndef MIB_STUDIO_CV_QT_CONVERT_H
#define MIB_STUDIO_CV_QT_CONVERT_H

#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc.hpp>
#include <QImage>

namespace mib
{
    namespace utils
    {

        /**
         * @brief Convert OpenCV Mat to Qt QImage
         * @param mat OpenCV Mat image
         * @return QImage representation of the Mat
         */
        inline QImage matToQImage(const cv::Mat &mat)
        {
            // Handle different OpenCV formats and convert to QImage
            switch (mat.type())
            {
            case CV_8UC1:
            {
                // Grayscale image
                return QImage(mat.data, mat.cols, mat.rows,
                              static_cast<int>(mat.step), QImage::Format_Grayscale8)
                    .copy();
            }
            case CV_8UC3:
            {
                // BGR image - convert to RGB
                cv::Mat rgb;
                cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
                return QImage(rgb.data, rgb.cols, rgb.rows,
                              static_cast<int>(rgb.step), QImage::Format_RGB888)
                    .copy();
            }
            case CV_8UC4:
            {
                // BGRA image - convert to RGBA
                cv::Mat rgba;
                cv::cvtColor(mat, rgba, cv::COLOR_BGRA2RGBA);
                return QImage(rgba.data, rgba.cols, rgba.rows,
                              static_cast<int>(rgba.step), QImage::Format_RGBA8888)
                    .copy();
            }
            default:
                // Unsupported format, return empty image and log error
                // In a real application, you would want to handle this more gracefully
                return QImage();
            }
        }

        /**
         * @brief Convert Qt QImage to OpenCV Mat
         * @param image Qt QImage
         * @return cv::Mat representation of the QImage
         */
        inline cv::Mat qImageToMat(const QImage &image)
        {
            switch (image.format())
            {
            case QImage::Format_Grayscale8:
            {
                return cv::Mat(image.height(), image.width(), CV_8UC1,
                               const_cast<uchar *>(image.bits()),
                               static_cast<size_t>(image.bytesPerLine()))
                    .clone();
            }
            case QImage::Format_RGB888:
            {
                cv::Mat mat(image.height(), image.width(), CV_8UC3,
                            const_cast<uchar *>(image.bits()),
                            static_cast<size_t>(image.bytesPerLine()));
                cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
                return mat.clone();
            }
            case QImage::Format_RGBA8888:
            {
                cv::Mat mat(image.height(), image.width(), CV_8UC4,
                            const_cast<uchar *>(image.bits()),
                            static_cast<size_t>(image.bytesPerLine()));
                cv::cvtColor(mat, mat, cv::COLOR_RGBA2BGRA);
                return mat.clone();
            }
            default:
                // Convert unsupported formats to RGB888 and then convert
                return qImageToMat(image.convertToFormat(QImage::Format_RGB888));
            }
        }

    } // namespace utils
} // namespace mib

#endif // MIB_STUDIO_CV_QT_CONVERT_H