#pragma once

#include <QWidget>
#include <QLabel>
#include <QImage>
#include <QMouseEvent>

#include "QtMibController.h"

namespace mib {

class ImageWindow : public QWidget {
    Q_OBJECT
public:
    explicit ImageWindow(QtMibController* controller, QWidget* parent = nullptr);
    ~ImageWindow() override = default;

public slots:
    void setImage(const QImage& img);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QtMibController* controller_;
    QLabel* label_;
    QPoint dragStart_;
};

} // namespace mib


