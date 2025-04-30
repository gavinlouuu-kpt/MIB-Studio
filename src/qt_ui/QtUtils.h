#pragma once

#include <QImage>
#include <QPixmap>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

/**
 * @brief Utilities for converting between OpenCV and Qt image formats
 */
namespace QtUtils {

/**
 * @brief Convert an OpenCV Mat to a QImage
 * @param mat The OpenCV Mat image to convert
 * @return QImage converted from the OpenCV Mat
 */
QImage matToQImage(const cv::Mat& mat);

/**
 * @brief Convert an OpenCV Mat to a QPixmap
 * @param mat The OpenCV Mat image to convert
 * @return QPixmap converted from the OpenCV Mat
 */
QPixmap matToQPixmap(const cv::Mat& mat);

/**
 * @brief Convert a QImage to an OpenCV Mat
 * @param image The QImage to convert
 * @return cv::Mat converted from the QImage
 */
cv::Mat qImageToMat(const QImage& image);

} // namespace QtUtils 