#include "ImageWindow.h"

#include <QVBoxLayout>
#include <QKeyEvent>

namespace mib {

ImageWindow::ImageWindow(QtMibController* controller, QWidget* parent)
    : QWidget(parent), controller_(controller) {
    setWindowTitle("Live Feed (Qt)");
    label_ = new QLabel(this);
    label_->setAlignment(Qt::AlignCenter);
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(label_);
    setLayout(layout);

    // Forward frames to label
    connect(controller_, &QtMibController::frameReady, this, &ImageWindow::setImage);
}

void ImageWindow::setImage(const QImage& img) {
    label_->setPixmap(QPixmap::fromImage(img).scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void ImageWindow::keyPressEvent(QKeyEvent* event) {
    int key = event->key();
    int code = 0;
    if (key == Qt::Key_Escape) code = 27;
    else if (key == Qt::Key_Space) code = 32;
    else if (event->text().size() == 1) code = event->text().at(0).toLatin1();
    if (code != 0) emit controller_->sendKey(code);
    QWidget::keyPressEvent(event);
}

void ImageWindow::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) dragStart_ = event->pos();
    QWidget::mousePressEvent(event);
}

void ImageWindow::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QPoint end = event->pos();
        QRect r = QRect(dragStart_, end).normalized();
        if (r.width() > 5 && r.height() > 5) {
            QString roi = QString::number(r.x()) + "," + QString::number(r.y()) + "," +
                          QString::number(r.width()) + "," + QString::number(r.height());
            controller_->setParam("roi", roi);
        }
    }
    QWidget::mouseReleaseEvent(event);
}

} // namespace mib


