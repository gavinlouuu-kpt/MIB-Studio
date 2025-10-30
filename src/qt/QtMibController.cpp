#include "QtMibController.h"

#include <QImage>
#include <QByteArray>

namespace mib {

namespace {
QImage toQImage(const Frame& frame) {
    switch (frame.format) {
        case PixelFormat::MONO8: {
            return QImage(frame.data, frame.width, frame.height, frame.width,
                          QImage::Format_Grayscale8).copy();
        }
        case PixelFormat::RGB8: {
            return QImage(frame.data, frame.width, frame.height, frame.width * 3,
                          QImage::Format_RGB888).copy();
        }
        case PixelFormat::BGR8: {
            QImage bgr(frame.data, frame.width, frame.height, frame.width * 3,
                       QImage::Format_BGR888);
            return bgr.copy();
        }
        default:
            return {};
    }
}
} // namespace

struct QtMibController::ObserverImpl : public Observer {
    explicit ObserverImpl(QtMibController* owner) : owner(owner) {}
    void onFrame(const Frame& frame) override {
        emit owner->frameReady(toQImage(frame));
    }
    void onStatus(const std::string& message) override {
        emit owner->statusChanged(QString::fromStdString(message));
    }
    void onError(int /*code*/, const std::string& message) override {
        emit owner->errorOccurred(QString::fromStdString(message));
    }
    QtMibController* owner;
};

QtMibController::QtMibController(IMibController* controller, QObject* parent)
    : QObject(parent), controller_(controller), observer_(new ObserverImpl(this)) {
    if (controller_) {
        controller_->subscribe(observer_);
    }
}

QtMibController::~QtMibController() {
    if (controller_ && observer_) {
        controller_->unsubscribe(observer_);
    }
    delete observer_;
}

void QtMibController::start() {
    if (controller_) controller_->start();
}

void QtMibController::stop() {
    if (controller_) controller_->stop();
}

void QtMibController::setParam(const QString& key, const QString& value) {
    if (controller_) controller_->setParam(key.toStdString(), value.toStdString());
}

void QtMibController::sendKey(int keyCode) {
    if (controller_) controller_->onKey(keyCode);
}

} // namespace mib


