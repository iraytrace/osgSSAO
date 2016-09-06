#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMenu>

#include <osgViewer/Viewer>
#include <osg/Group>
#include <osg/Switch>
#include <osg/PolygonMode>
#include <osg/Point>
#include <osg/LineWidth>
#include <osgUtil/LineSegmentIntersector>

#include "CameraModel.h"


///
/// \brief The Osg3dViewBase class
///
/// This inherits from QOpenGLWidget which renders to a texture and can be
/// easily unit tested, whereas the (deprecated) QGLWidget does not and is
/// not reliably unit-testable.
class  Osg3dViewWithCamera :
        public QOpenGLWidget,
        protected QOpenGLFunctions,
        public osgViewer::Viewer
{
    Q_OBJECT
public:
    Osg3dViewWithCamera(QWidget *parent=nullptr);
    ~Osg3dViewWithCamera();

    unsigned getCameraMask() const { return getCamera()->getCullMask(); }

    const QString & getGlInfo() const { return m_glInfo; }

    /// Get the Normalized Device Coordinates for a given X,Y pixel location
    osg::Vec2d getNormalizedDeviceCoords(const int ix, const int iy);


    osg::ref_ptr<CameraModel> cameraModel() const { return m_cameraModel; }
    void setCameraModel(osg::ref_ptr<CameraModel> cameraModel);
    osg::ref_ptr<osgUtil::LineSegmentIntersector>
        intersectUnderCursor(const int x, const int y, unsigned mask=~0);
public slots:

    /// Let others tell what scene graph we should be drawing
    void addNode(osg::Node *n);
    void removeNode(osg::Node *n);
    void clearNodes();

    /// OSG uses singleSided drawing/display by default.
    /// This is annoying when you are "inside" something and the back wall of
    /// it simply disappears. The constructor calls this to set up to
    /// draw both front and back facing polygons so that the world behaves
    /// in a more normal fashion.
    /// Yes it's a performance hit.
    void setLightingTwoSided(bool tf=true);

    // These support changing the display between drawing in FILL/Line/Point
    // mode and setting the line/point size when in those modes
    void setDrawMode(osg::PolygonMode::Mode drawMode);
    void setDrawModeFromQAction();
    void setLineWidth(int size);


signals:
    void updated();
    void lineWidthChanged(int);
    void drawModeChanged(osg::PolygonMode::Mode drawMode);

protected: // These are from QOpenGLWidget.  There are implementations here,
           // but they can be overridden

    /// Render one frame
    virtual void paintGL() override;

    /// Updates OpenGL viewport when window size changes
    virtual void resizeGL( int width, int height ) override;

    virtual void initializeGL() override;



    /// This is the scene model that will be drawn.  Callers can add/remove
    /// items to/from this with addNode() removeNode() and clearNodes().
    /// This allows per-widget drawing modes when sharing a scene graph
    /// across multiple widgets/osgViewers.
    osg::ref_ptr< osg::Group > m_scene;

    /// This is the root of the everything that will be drawn.  It allows
    /// per-widget elements and drawing modes.  This is a switch so that
    /// it is easy to toggle between different rendering mechanisms.
    osg::ref_ptr< osg::Switch > m_root;

    /// OSG graphics window
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> m_osgGraphicsWindow;

    //Variables for controlling the size of points and lines
    osg::ref_ptr<osg::Point> m_point;
    osg::ref_ptr<osg::LineWidth> m_lineWidth;
    float m_currentLineWidth;

    /// CameraModel --> controls the camera of the osgViewer
    osg::ref_ptr< CameraModel > m_cameraModel;

    QString m_glInfo;
    QList<QMenu *> m_menus;

};

