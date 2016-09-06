#ifndef SHOTSELWIDGET_H
#define SHOTSELWIDGET_H

#include <QWidget>
#include "CameraModel.h"
class Osg3dSSAOView;

class UiEventWidget : public QWidget
{
    Q_OBJECT
public:
    enum MouseMode {
        MM_ORBIT = 1<<0,
        MM_PAN = 1<<1,
        MM_ZOOM = 1<<2 ,
        MM_FLY = 1<<3,
        MM_ROTATE = 1<<4,
        MM_PICK_CENTER = 1<<5,
        MM_PICK_POINT = 1<<6,
        MM_SELECTOBJECT = 1<<7,
        MM_SHOOT = 1<<8,
        MM_MOVE = 1<<9,
        MM_CUTTINGPLANE = 1<<10,
        MM_NONE = 0
    };
    explicit UiEventWidget(QWidget *parent = 0);
    MouseMode mouseMode() const { return m_mouseMode; }
    bool autoFocus() const { return m_shouldAutoFocus; }
    Osg3dSSAOView *ssaoView() const { return m_ssaoWidget; }

signals:
    void handleCloseEvent(QCloseEvent *event);
    void startOrbit(osg::Vec2d);
    void orbit(osg::Vec2d);
    void finishOrbit(osg::Vec2d);
    void startRotate(osg::Vec2d);
    void rotate(osg::Vec2d);
    void finishRotate(osg::Vec2d);

    void startPan(osg::Vec2d, osg::Vec4d);
    void pan(osg::Vec2d);
    void finishPan(osg::Vec2d);

    void startZoom(osg::Vec2d);
    void zoom(osg::Vec2d);
    void finishZoom(osg::Vec2d);

    void startDolly(osg::Vec2d);
    /** Dolly the camera forwards and backwards. Changes the view distance.
    This function is a no-op for orthographic projections. */
    void dolly(double);
    void dolly(osg::Vec2d);
    void finishDolly(osg::Vec2d);

    void pickaPoint(osg::Vec2d, int, int);
    void pickCenter(int, int);
    void shoot(int, int);

    void shotShift(Qt::Key);
    void shotRotate(Qt::Key);

    void clipPlaneShift(Qt::Key);
    void clipPlaneRotate(Qt::Key);

public slots:
    void closeEvent(QCloseEvent *event) { emit handleCloseEvent(event); }

    virtual void enterEvent(QEvent *event);
    virtual void leaveEvent(QEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);

    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void wheelEvent(QWheelEvent *event) override;

    void setMouseMode(MouseMode mode);
    void setAutoFocus(bool tf) { m_shouldAutoFocus = tf; }

private:
    void shootKeyEvent(QKeyEvent *event);
    void clipKeyEvent(QKeyEvent *event);
    MouseMode m_mouseMode;

    /// since we always want the 3D view to have focus when entered
    /// it is polite to remember who had the focus when we stole it
    /// and restore it when the cursor leaves the window
    QWidget *m_lastFocused;
    bool m_shouldAutoFocus;

    Osg3dSSAOView *m_ssaoWidget;
    void startPanning(osg::Vec2d ndc, int x, int y);
};

#endif // SHOTSELWIDGET_H
