#include "UiEventWidget.h"
#include <QMouseEvent>
#include "Osg3dSSAOView.h"
#include <QHBoxLayout>
#include <QApplication>

UiEventWidget::UiEventWidget(QWidget *parent)
    : QWidget(parent)
    , m_mouseMode(MM_ORBIT)
    , m_lastFocused(nullptr)
    , m_shouldAutoFocus(true)
    , m_ssaoWidget(new Osg3dSSAOView(this))
{
    QHBoxLayout *horizontalLayout = new QHBoxLayout(this);
    horizontalLayout->addWidget(m_ssaoWidget);
    horizontalLayout->setContentsMargins(0, 0, 0, 0);

    osg::ref_ptr<CameraModel> cameraModel = m_ssaoWidget->cameraModel();
    connect(this, SIGNAL(startOrbit(osg::Vec2d)),
            cameraModel.get(), SLOT(startOrbit(osg::Vec2d)));
    connect(this, SIGNAL(startRotate(osg::Vec2d)),
            cameraModel.get(), SLOT(startRotate(osg::Vec2d)));
    connect(this, SIGNAL(startPan(osg::Vec2d, osg::Vec4d)),
            cameraModel.get(), SLOT(startPan(osg::Vec2d, osg::Vec4d)));
    connect(this, SIGNAL(startDolly(osg::Vec2d)),
            cameraModel.get(), SLOT(startDolly(osg::Vec2d)));
    connect(this, SIGNAL(startZoom(osg::Vec2d)),
            cameraModel.get(), SLOT(startZoom(osg::Vec2d)));

    connect(this, SIGNAL(orbit(osg::Vec2d)),
            cameraModel.get(), SLOT(orbit(osg::Vec2d)));
    connect(this, SIGNAL(rotate(osg::Vec2d)),
            cameraModel.get(), SLOT(rotate(osg::Vec2d)));
    connect(this, SIGNAL(pan(osg::Vec2d)),
            cameraModel.get(), SLOT(pan(osg::Vec2d)));
    connect(this, SIGNAL(dolly(osg::Vec2d)),
            cameraModel.get(), SLOT(dolly(osg::Vec2d)));
    connect(this, SIGNAL(zoom(osg::Vec2d)),
            cameraModel.get(), SLOT(zoom(osg::Vec2d)));

    connect(this, SIGNAL(finishOrbit(osg::Vec2d)),
            cameraModel.get(), SLOT(finishOrbit(osg::Vec2d)));
    connect(this, SIGNAL(finishRotate(osg::Vec2d)),
            cameraModel.get(), SLOT(finishRotate(osg::Vec2d)));
    connect(this, SIGNAL(finishPan(osg::Vec2d)),
            cameraModel.get(), SLOT(finishPan(osg::Vec2d)));
    connect(this, SIGNAL(finishDolly(osg::Vec2d)),
            cameraModel.get(), SLOT(finishDolly(osg::Vec2d)));
    connect(this, SIGNAL(finishZoom(osg::Vec2d)),
            cameraModel.get(), SLOT(finishZoom(osg::Vec2d)));

}

void UiEventWidget::enterEvent(QEvent *event)
{
    if (!m_shouldAutoFocus) return;
    QWidget *w = qApp->focusWidget();  // get currently focused widget

    if (w == this) m_lastFocused = (QWidget *)0;
    else m_lastFocused = w;

    this->setFocus();
}

void UiEventWidget::leaveEvent(QEvent *event)
{
    if (!m_shouldAutoFocus) return;

    this->clearFocus();

    if (m_lastFocused && m_lastFocused != this) {
        m_lastFocused->setFocus();
        m_lastFocused = (QWidget *)0;
    }
}


void UiEventWidget::shootKeyEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Right: // fallthrough
    case Qt::Key_Up:    // fallthrough
    case Qt::Key_Down:  // fallthrough
    case Qt::Key_Left: {
        if (event->modifiers() == Qt::NoModifier)
            emit shotShift( (Qt::Key)event->key());
        else if (event->modifiers() == Qt::ControlModifier)
            emit shotRotate((Qt::Key)event->key());
        break;
    }
    default: break;
    }
}
void UiEventWidget::clipKeyEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Right: // fallthrough
    case Qt::Key_Up:    // fallthrough
    case Qt::Key_Down:  // fallthrough
    case Qt::Key_Left: {
        if (event->modifiers() == Qt::NoModifier)
            emit clipPlaneShift( (Qt::Key)event->key());
        else if (event->modifiers() == Qt::ControlModifier)
            emit clipPlaneRotate((Qt::Key)event->key());
        break;
    }
    default: break;
    }
}

void UiEventWidget::keyPressEvent(QKeyEvent *event)
{
    switch (m_mouseMode) {
    case MouseMode::MM_SHOOT: { shootKeyEvent(event); break; }
    case MouseMode::MM_CUTTINGPLANE: { clipKeyEvent(event); break; }
    default: break;
    }
}

void UiEventWidget::keyReleaseEvent(QKeyEvent *event)
{

}

void UiEventWidget::startPanning(osg::Vec2d ndc, int x, int y)
{
    setCursor(Qt::ClosedHandCursor);
    auto lsi = m_ssaoWidget->intersectUnderCursor(x, y);

    osg::Vec3d pickPoint;
    if (lsi->containsIntersections()) {
        auto intersections = lsi->getIntersections();
        auto intersection = *( intersections.begin() );
        pickPoint = intersection.getWorldIntersectPoint();
    } else {
        pickPoint = m_ssaoWidget->cameraModel()->viewCenter();
    }
    osg::Vec3d viewDir =  m_ssaoWidget->cameraModel()->viewDir();
    osg::Vec4d panPlane = osg::Vec4d( viewDir, -( pickPoint * viewDir ) );

    emit startPan(ndc, panPlane);
}

void UiEventWidget::mousePressEvent(QMouseEvent *event)
{
    osg::Vec2d ndc = m_ssaoWidget->getNormalizedDeviceCoords(
                event->x(), event->y());

    if ( event->buttons() != Qt::LeftButton) return;

    switch (m_mouseMode) {
    case MM_ORBIT: { emit startOrbit(ndc); break; }
    case MM_PAN: { this->startPanning(ndc, event->x(), event->y()); break; }
    case MM_ZOOM: { emit startZoom(ndc); break; }
    case MM_FLY: { emit startDolly(ndc); break; }
    case MM_ROTATE: { emit startRotate(ndc); break; }
    case MM_PICK_CENTER: { emit pickCenter(event->x(), event->y()); break; }
    case MM_SHOOT: { emit shoot(event->x(), event->y()); break;}
    case MM_SELECTOBJECT: {break;}
    case MM_PICK_POINT: { emit pickaPoint( ndc, event->x(), event->y()); break; }

    default: break;
    }

}

void UiEventWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if ( event->button() != Qt::LeftButton) return;
    osg::Vec2d ndc = m_ssaoWidget->getNormalizedDeviceCoords(
                event->x(), event->y());

    switch (m_mouseMode) {
    case MM_ORBIT: {emit finishOrbit(ndc); break; }
    case MM_PAN: { setCursor(Qt::OpenHandCursor);
        emit finishPan(ndc); break; }
    case MM_ZOOM: { emit finishZoom(ndc); break; }
    case MM_FLY: { emit finishDolly(ndc); break; }
    case MM_ROTATE: { emit finishRotate(ndc); break; }
    case MM_PICK_CENTER: {  break; }
    case MM_SELECTOBJECT: break;
    case MM_PICK_POINT:{emit pickaPoint( ndc, event->x(), event->y()); break;}
    case MM_SHOOT:  {break;}
    case MM_MOVE: break;
    }

}

void UiEventWidget::mouseMoveEvent(QMouseEvent *event)
{
    if ( event->buttons() != Qt::LeftButton) return;

    osg::Vec2d ndc = m_ssaoWidget->getNormalizedDeviceCoords(
                event->x(), event->y());

    switch (m_mouseMode) {
    case MM_ORBIT: { emit orbit(ndc); break; }
    case MM_PAN: { emit pan(ndc); break; }
    case MM_ZOOM: { emit zoom(ndc); break; }
    case MM_FLY: { emit dolly(ndc); break; }
    case MM_ROTATE: { emit rotate(ndc); break; }
    case MM_PICK_CENTER: {  break; }
    case MM_SHOOT: { emit shoot(event->x(), event->y()); break;}
    case MM_SELECTOBJECT: break;
    case MM_PICK_POINT:{emit pickaPoint( ndc, event->x(), event->y()); break;}
    case MM_MOVE:
        break;
    }
}

void UiEventWidget::wheelEvent(QWheelEvent *event)
{
    if (event->delta() > 0)
        emit dolly(0.5);
    else
        emit dolly(-0.5);
}

void UiEventWidget::setMouseMode(UiEventWidget::MouseMode mode)
{
    if (m_mouseMode == MM_PAN && mode != MM_PAN)
        setCursor(Qt::ArrowCursor);

    if (m_mouseMode != MM_PAN && mode == MM_PAN)
        setCursor(Qt::OpenHandCursor);

    m_mouseMode = mode;
}

