#include "Osg3dViewWithCamera.h"

#include <math.h>
#include <QApplication>
#include <QAction>

#include <QtGui/QKeyEvent>
#include <osg/LightModel>
#include <osgViewer/Renderer>
#include <osgGA/TrackballManipulator>
#include <osgUtil/LineSegmentIntersector>
#include <QTextStream>


#include "NodeMask.h"
Osg3dViewWithCamera::Osg3dViewWithCamera(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_scene(new osg::Group)
    , m_root(new osg::Switch)
    , m_currentLineWidth(1.0)
    , m_cameraModel(new CameraModel)
{
    setFocusPolicy(Qt::StrongFocus);

    // Construct the embedded graphics window
    m_osgGraphicsWindow = new osgViewer::GraphicsWindowEmbedded(0,0,width(),height());
    getCamera()->setGraphicsContext(m_osgGraphicsWindow);

    // Set up the camera
    getCamera()->setViewport(new osg::Viewport(0,0,width(),height()));

    // By default draw everthing that has even 1 bit set in the nodemask
    getCamera()->setCullMask( ~(unsigned)0 );
    getCamera()->setDataVariance(osg::Object::DYNAMIC);

    // As of July 2010 there wasn't really a good way to multi-thread OSG
    // under Qt so just set the threading model to be SingleThreaded
    setThreadingModel(osgViewer::Viewer::SingleThreaded);

    // draw both sides of polygons
    setLightingTwoSided();

    // Set the minimum size for this viewer window
    setMinimumSize(64, 64);

    osg::ref_ptr<osg::StateSet> ss = m_scene->getOrCreateStateSet();
    m_point = new osg::Point(m_currentLineWidth);
    ss->setAttribute(m_point);

    m_lineWidth = new osg::LineWidth(m_currentLineWidth);
    ss->setAttribute(m_lineWidth);

    m_root->setNewChildDefaultValue(true);
    m_root->addChild(m_scene);

    this->setSceneData(m_root);

    m_cameraModel->setBoundingNode(m_scene);
    m_cameraModel->computeInitialView();
    connect(m_cameraModel, SIGNAL(changed()), this, SLOT(update()));
}

Osg3dViewWithCamera::~Osg3dViewWithCamera()
{
}

void Osg3dViewWithCamera::paintGL()
{
    // Update the camera
    osg::Camera *cam = this->getCamera();
    //const osg::Viewport* vp = cam->getViewport();

    if (cam->getCullMask() != m_cameraModel->cullMask())
        cam->setCullMask( m_cameraModel->cullMask() );

    m_cameraModel->setAspect((double)width() / (double)height());

    cam->setViewMatrix(m_cameraModel->getModelViewMatrix() );
    cam->setProjectionMatrix(m_cameraModel->computeProjection());

    // Invoke the OSG traversal pipeline
    frame();

    emit updated();
}

void Osg3dViewWithCamera::setCameraModel(osg::ref_ptr<CameraModel> cameraControl)
{
    disconnect(m_cameraModel.get(), SIGNAL(changed()), this, SLOT(update()));
    m_cameraModel = cameraControl;
    m_cameraModel->setBoundingNode(m_scene);
    connect(m_cameraModel.get(), SIGNAL(changed()), this, SLOT(update()));
    update();
}

void Osg3dViewWithCamera::resizeGL(int width, int height)
{
    m_osgGraphicsWindow->resized(0,0,width,height);
}


void Osg3dViewWithCamera::initializeGL()
{
    initializeOpenGLFunctions();
    QString vendor = (const char *)glGetString(GL_VENDOR);
    QString renderer = (const char *)glGetString(GL_RENDERER);
    QString version = (const char *)glGetString(GL_VERSION);
    QString glsl_version = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);

    QString htmlString;
    QTextStream htmlStream(&htmlString);
    htmlStream << "<table>";
    htmlStream << "<tr><td>GL_VENDOR</td><td>";
    htmlStream << vendor << "</td></tr>" ;
    htmlStream << "<tr><td>GL_RENDERER</td><td>";
    htmlStream << renderer << "</td></tr>" ;
    htmlStream << "<tr><td>GL_VERSION   </td><td>";
    htmlStream << version << "</td></tr>" ;
    htmlStream << "<tr><td>GLSL_VERSION </td><td>";
    htmlStream << glsl_version << "</td></tr>" ;
    htmlStream << "</table>";
    htmlStream.flush();
    qApp->setProperty("OpenGLinfoHTML", htmlString );


    QString plainString;
    QTextStream stream(&plainString);
    stream << "GL_VENDOR    : ";
    stream << vendor << endl;
    stream << "GL_RENDERER  : ";
    stream << renderer << endl;
    stream << "GL_VERSION   : ";
    stream << version << endl;
    stream << "GLSL_VERSION : ";
    stream << glsl_version << endl;
    stream.flush();
    qApp->setProperty("OpenGLinfo", plainString);
}

osg::Vec2d Osg3dViewWithCamera::getNormalizedDeviceCoords(const int ix, const int iy)
{
    osg::Vec2d ndc;

    int center = width()/2;
    ndc[0] = ((double)ix - (double)center) / (double)center;
    if (ndc[0] > 1.0) ndc[0] = 1.0;

    center = height()/2;
    int invertedY = height() - iy;
    ndc[1] = ((double)invertedY - (double)center) / (double)center;
    if (ndc[1] > 1.0) ndc[1] = 1.0;

    return ndc;
}

void Osg3dViewWithCamera::addNode(osg::Node *root)
{
    m_scene->addChild(root);
}
void Osg3dViewWithCamera::removeNode(osg::Node *root)
{
    m_scene->removeChild(root);
}
void Osg3dViewWithCamera::clearNodes()
{
    m_scene->removeChildren(0, m_scene->getNumChildren());
}

void Osg3dViewWithCamera::setLightingTwoSided(bool tf)
{
    osg::ref_ptr<osg::LightModel> lm = new osg::LightModel;
    lm->setTwoSided(tf);
    lm->setAmbientIntensity(osg::Vec4(0.1f,0.1f,0.1f,1.0f));

    osg::StateSet *ss;

    for (int i=0 ; i < 2 ; i++ ) {
        ss = ((osgViewer::Renderer *)getCamera()->getRenderer())->
                getSceneView(i)->getGlobalStateSet();

        ss->setAttributeAndModes(lm, osg::StateAttribute::ON);
    }
}


void Osg3dViewWithCamera::setDrawMode(osg::PolygonMode::Mode drawMode)
{
    osg::ref_ptr<osg::StateSet> ss =
            m_scene->getOrCreateStateSet();

    osg::ref_ptr<osg::PolygonMode> pm =
            dynamic_cast<osg::PolygonMode *>
            (ss->getAttribute(osg::StateAttribute::POLYGONMODE));

    if(!pm) {
        pm = new osg::PolygonMode;
        ss->setAttribute(pm.get());
    }

    pm->setMode(osg::PolygonMode::FRONT_AND_BACK, drawMode);

    switch (drawMode) {
    case osg::PolygonMode::LINE:
    case osg::PolygonMode::POINT:
        ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        break;
    default:
        ss->setMode(GL_LIGHTING, osg::StateAttribute::ON);
        break;
    }

    emit drawModeChanged(drawMode);
    update();
}

void Osg3dViewWithCamera::setDrawModeFromQAction()
{
    QAction *a = dynamic_cast<QAction *>(sender());
    if (!a)
        return;
    bool ok = false;
    osg::PolygonMode::Mode drawMode =
            static_cast<osg::PolygonMode::Mode>(a->data().toUInt(&ok));
    if (ok) setDrawMode(drawMode);
}

void Osg3dViewWithCamera::setLineWidth(int size)
{
    m_point->setSize((float)size);
    m_lineWidth->setWidth((float)size);
    emit lineWidthChanged(size);
    update();
}

void printIntersectorDebugging(const int x, const int y, unsigned mask,
                               osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector
                               )
{
    qDebug("intersectUnderCursor(%d %d %s):", x, y, qPrintable(NodeMask::maskToString(mask)));
    auto intersections = intersector->getIntersections();

    for (auto isect=intersections.begin() ; isect != intersections.end() ; isect++) {

        osg::Vec3d pt = (*isect).getWorldIntersectPoint();

        QString path;
        foreach (osg::Node *n, (*isect).nodePath) {
            if (n->getName().size() > 0)
                path.append(QString("/%1(%2)")
                            .arg(n->getName().c_str())
                            .arg(  NodeMask::maskToString(n->getNodeMask()) ));
            else
                path.append(QString("/%1(%2)")
                            .arg(n->className())
                            .arg(  NodeMask::maskToString(n->getNodeMask()) ));
        }

        qDebug("    ipt %10g %10g %10g %s", pt.x(), pt.y(), pt.z(), qPrintable(path));

    }
    qDebug("endintersections:");
}


osg::ref_ptr<osgUtil::LineSegmentIntersector>
 Osg3dViewWithCamera::intersectUnderCursor(const int x, const int y, unsigned mask)
{
    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector =
            new osgUtil::LineSegmentIntersector(osgUtil::Intersector::WINDOW,
                                                x, height()- y);

    osgUtil::IntersectionVisitor intersectVisitor( intersector.get() );
    intersectVisitor.setTraversalMask(mask);

    getCamera()->accept(intersectVisitor);

    return intersector;
}
