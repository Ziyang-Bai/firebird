#ifndef LCDWIDGET_H
#define LCDWIDGET_H

#include <QGraphicsView>
#include <QKeyEvent>
#include <QTimer>

class LCDWidget : public QWidget
{
    Q_OBJECT

public:
    LCDWidget(QWidget *parent, Qt::WindowFlags f = Qt::WindowFlags());

public slots:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *e) override;
    void hideEvent(QHideEvent *e) override;
    void closeEvent(QCloseEvent *e) override;

protected:
    virtual void paintEvent(QPaintEvent *) override;

signals:
    void closed();

private:
    QTimer refresh_timer;
    qreal touch_x = 0.0;
    qreal touch_y = 0.0;
    bool touch_contact = false;
    bool touch_down = false;
};

#endif // LCDWIDGET_H
