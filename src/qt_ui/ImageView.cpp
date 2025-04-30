#include "qt_ui/ImageView.h"
#include "qt_ui/QtUtils.h"
#include <QTransform>

ImageView::ImageView(QWidget* parent)
    : QWidget(parent)
{
    // Enable mouse tracking for hover effects
    setMouseTracking(true);
    // Set focus policy to allow wheel events
    setFocusPolicy(Qt::StrongFocus);
    // Set size policy for the widget
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ImageView::setImage(const cv::Mat& mat)
{
    // Convert OpenCV Mat to QPixmap using our utility function
    m_pixmap = QtUtils::matToQPixmap(mat);
    // Update the widget
    update();
}

void ImageView::setImage(const QImage& image)
{
    // Convert QImage to QPixmap
    m_pixmap = QPixmap::fromImage(image);
    // Update the widget
    update();
}

double ImageView::getZoomLevel() const
{
    return m_zoomLevel;
}

void ImageView::setZoomLevel(double level)
{
    // Limit zoom level to a reasonable range
    m_zoomLevel = qBound(0.1, level, 10.0);
    // Emit signal to notify listeners
    emit zoomChanged(m_zoomLevel);
    // Update the widget
    update();
}

void ImageView::resetView()
{
    // Reset zoom and pan
    m_zoomLevel = 1.0;
    m_panOffset = QPoint(0, 0);
    // Update the widget
    update();
    // Emit signal for zoom change
    emit zoomChanged(m_zoomLevel);
}

void ImageView::setInteractionEnabled(bool enable)
{
    m_interactionEnabled = enable;
}

void ImageView::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    
    // Initialize painter
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    
    // If no image, fill with background color
    if (m_pixmap.isNull()) {
        painter.fillRect(rect(), palette().window());
        return;
    }
    
    // Calculate scaled dimensions
    QSize scaledSize = m_pixmap.size() * m_zoomLevel;
    
    // Center the image in the widget
    QPoint centerOffset((width() - scaledSize.width()) / 2, 
                        (height() - scaledSize.height()) / 2);
    
    // Apply pan offset
    QPoint finalOffset = centerOffset + m_panOffset;
    
    // Draw the image
    painter.drawPixmap(QRect(finalOffset, scaledSize), m_pixmap);
}

void ImageView::wheelEvent(QWheelEvent* event)
{
    if (!m_interactionEnabled) {
        event->ignore();
        return;
    }
    
    // Get current mouse position
    QPoint mousePos = event->position().toPoint();
    
    // Calculate zoom center in image coordinates
    QSize scaledSize = m_pixmap.size() * m_zoomLevel;
    QPoint centerOffset((width() - scaledSize.width()) / 2, 
                        (height() - scaledSize.height()) / 2);
    QPoint imagePos = (mousePos - centerOffset - m_panOffset) / m_zoomLevel;
    
    // Calculate zoom factor (120 is a standard wheel step)
    double zoomFactor = 1.0 + (event->angleDelta().y() / 1200.0);
    
    // Calculate new zoom level
    double newZoomLevel = m_zoomLevel * zoomFactor;
    
    // Apply zoom
    setZoomLevel(newZoomLevel);
    
    // Adjust pan offset to keep mousePos over the same image pixel
    QSize newScaledSize = m_pixmap.size() * m_zoomLevel;
    QPoint newCenterOffset((width() - newScaledSize.width()) / 2, 
                          (height() - newScaledSize.height()) / 2);
    QPoint newPanOffset = mousePos - newCenterOffset - (imagePos * m_zoomLevel);
    
    m_panOffset = newPanOffset;
    
    // Update the widget
    update();
    
    // Accept the event
    event->accept();
}

void ImageView::mousePressEvent(QMouseEvent* event)
{
    if (!m_interactionEnabled) {
        event->ignore();
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        // Start panning
        m_isPanning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        
        // Calculate click position in image coordinates
        QSize scaledSize = m_pixmap.size() * m_zoomLevel;
        QPoint centerOffset((width() - scaledSize.width()) / 2, 
                           (height() - scaledSize.height()) / 2);
        QPoint imagePos = (event->pos() - centerOffset - m_panOffset) / m_zoomLevel;
        
        // Emit signal with clicked position
        emit imageClicked(imagePos);
    }
    
    event->accept();
}

void ImageView::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_interactionEnabled) {
        event->ignore();
        return;
    }
    
    if (m_isPanning) {
        // Update pan offset
        QPoint delta = event->pos() - m_lastPanPos;
        m_panOffset += delta;
        m_lastPanPos = event->pos();
        
        // Update the widget
        update();
    }
    
    event->accept();
}

void ImageView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_isPanning) {
        // Stop panning
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
    
    event->accept();
} 