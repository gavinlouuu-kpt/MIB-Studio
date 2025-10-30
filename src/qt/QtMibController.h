#pragma once

#include <QObject>
#include <QString>
#include <QImage>

#include "mib/api/IMibController.h"

namespace mib {

class QtMibController : public QObject {
    Q_OBJECT
public:
    explicit QtMibController(IMibController* controller, QObject* parent = nullptr);
    ~QtMibController() override;

public slots:
    void start();
    void stop();
    void setParam(const QString& key, const QString& value);
    void sendKey(int keyCode);

signals:
    void frameReady(const QImage& image);
    void statusChanged(const QString& message);
    void errorOccurred(const QString& message);

private:
    struct ObserverImpl;
    IMibController* controller_;
    ObserverImpl* observer_;
};

} // namespace mib


