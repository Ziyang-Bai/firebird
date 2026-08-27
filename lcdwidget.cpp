#include "lcdwidget.h"
#include "qmlbridge.h"
#include "qtframebuffer.h"

LCDWidget::LCDWidget(QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f)
{
    setMinimumSize(320, 240);

    connect(&refresh_timer, SIGNAL(timeout()), this, SLOT(update()));

    refresh_timer.setInterval(1000 / 30); // 30 fps
}

void LCDWidget::mousePressEvent(QMouseEvent *event)
{
    touch_x = qBound<qreal>(0.0, static_cast<qreal>(event->x()) / width(), 1.0);
    touch_y = qBound<qreal>(0.0, static_cast<qreal>(event->y()) / height(), 1.0);
    touch_contact = true;
    touch_down = event->button() == Qt::RightButton;
    the_qml_bridge->setTouchpadState(touch_x, touch_y, touch_contact, touch_down);
}

void LCDWidget::mouseReleaseEvent(QMouseEvent *event)
{
    touch_x = qBound<qreal>(0.0, static_cast<qreal>(event->x()) / width(), 1.0);
    touch_y = qBound<qreal>(0.0, static_cast<qreal>(event->y()) / height(), 1.0);
    if(event->button() == Qt::RightButton)
        touch_down = touch_contact = false;
    else
        touch_contact = false;
    the_qml_bridge->setTouchpadState(touch_x, touch_y, touch_contact, touch_down);
}

void LCDWidget::mouseMoveEvent(QMouseEvent *event)
{
    touch_x = qBound<qreal>(0.0, static_cast<qreal>(event->x()) / width(), 1.0);
    touch_y = qBound<qreal>(0.0, static_cast<qreal>(event->y()) / height(), 1.0);
    the_qml_bridge->setTouchpadState(touch_x, touch_y, touch_contact, touch_down);
}

void LCDWidget::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);

    refresh_timer.start();
}

void LCDWidget::hideEvent(QHideEvent *e)
{
    QWidget::hideEvent(e);

    refresh_timer.stop();
}

void LCDWidget::closeEvent(QCloseEvent *e)
{
    QWidget::closeEvent(e);

    emit closed();
}

void LCDWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    paintFramebuffer(&painter);
}
