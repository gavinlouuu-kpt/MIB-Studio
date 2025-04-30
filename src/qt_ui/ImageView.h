#pragma once

#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QPaintEvent>
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPoint>
#include <opencv2/core.hpp>

/**
 * @brief Custom Qt widget for displaying and interacting with OpenCV images
 */
class ImageView : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit ImageView(QWidget* parent = nullptr);

    /**
     * @brief Set the image from an OpenCV Mat
     * @param mat OpenCV Mat image
     */
    void setImage(const cv::Mat& mat);

    /**
     * @brief Set the image from a QImage
     * @param image Qt image
     */
    void setImage(const QImage& image);

    /**
     * @brief Get the current zoom level
     * @return Current zoom level (1.0 = 100%)
     */
    double getZoomLevel() const;

    /**
     * @brief Set the zoom level
     * @param level Zoom level (1.0 = 100%)
     */
    void setZoomLevel(double level);

    /**
     * @brief Reset view to original size and position
     */
    void resetView();

    /**
     * @brief Enable or disable zooming and panning
     * @param enable True to enable, false to disable
     */
    void setInteractionEnabled(bool enable);

signals:
    /**
     * @brief Signal emitted when the zoom level changes
     * @param level New zoom level
     */
    void zoomChanged(double level);

    /**
     * @brief Signal emitted when the user clicks on a point in the image
     * @param imagePoint Point in image coordinates
     */
    void imageClicked(QPoint imagePoint);

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QPixmap m_pixmap;           // The image to display
    double m_zoomLevel = 1.0;   // Current zoom level
    QPoint m_panOffset;         // Current panning offset
    QPoint m_lastPanPos;        // Last mouse position for panning
    bool m_isPanning = false;   // Whether we're currently panning
    bool m_interactionEnabled = true; // Whether zooming and panning is enabled
}; 