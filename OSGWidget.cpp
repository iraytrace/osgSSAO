#include "OSGWidget.h"
#include <math.h>
#include <QApplication>
#include <QDesktopWidget>

#include <QtGui/QKeyEvent>
#include <osg/LightModel>
#include <osgViewer/Renderer>
#include <osgGA/TrackballManipulator>
#include <osg/LineWidth>

#include <QAction>

OSGWidget::OSGWidget(QWidget *parent)
    : QGLWidget(parent)
    , m_ssao(new SSAONode(width(), height()))
    , m_root(new osg::Switch)
    , m_scene(new osg::Group)
    , m_cameraModel(new CameraModel)
{
    // Strong focus policy needs to be set to capture keyboard events
    setFocusPolicy(Qt::StrongFocus);

    // Construct the embedded graphics window
    m_osgGraphicsWindow = new osgViewer::GraphicsWindowEmbedded(0,0,width(),height());
    getCamera()->setGraphicsContext(m_osgGraphicsWindow);

    // Set up the camera
    getCamera()->setViewport(new osg::Viewport(0,0,width(),height()));

    // By default draw everthing that has even 1 bit set in the nodemask
    getCamera()->setCullMask( (unsigned)~0 );
    getCamera()->setDataVariance(osg::Object::DYNAMIC);

    // As of July 2010 there wasn't really a good way to multi-thread OSG
    // under Qt so just set the threading model to be SingleThreaded
    setThreadingModel(osgViewer::Viewer::SingleThreaded);

    // draw both sides of polygons
    setLightingTwoSided();

    // Set the minimum size for this viewer window
    setMinimumSize(64, 64);


    SSAONode::buildGraph(m_root, m_scene, m_ssao);

    this->setSSAOEnabled(true);

    this->setSceneData(m_root);

    m_cameraModel->setBoundingNode(m_scene);
    m_cameraModel->computeInitialView();
    connect(m_cameraModel, SIGNAL(changed()), this, SLOT(update()));
}


void OSGWidget::setSSAOEnabled(bool tf)
{
    SSAONode::setSSAOEnabled(m_root, m_scene, m_ssao, tf);

    update();
}

void OSGWidget::setSSAODisplayMode(SSAONode::DisplayMode mode)
{
    m_ssao->SetDisplayMode(mode);
    update();
}

void OSGWidget::paintGL()
{
    // Update the camera
    osg::Camera *cam = this->getCamera();
#ifdef VIEWINGCORE
    const osg::Viewport* vp = cam->getViewport();

    m_viewingCore->setAspect(vp->width() / vp->height());
    cam->setViewMatrix(m_viewingCore->getInverseMatrix());
    cam->setProjectionMatrix(m_viewingCore->computeProjection());

#else

    cam->setCullMask( m_cameraModel->cullMask() );

    //m_cameraModel->setAspect((double)width() / (double)height());

    cam->setViewMatrix(m_cameraModel->getModelViewMatrix() );
    cam->setProjectionMatrix(m_cameraModel->computeProjection());
#endif
    // Let SSAO class know that camera has changed

    m_ssao->updateProjectionMatrix(getCamera()->getProjectionMatrix());

    // Invoke the OSG traversal pipeline
    frame();

    // Start the timer again
    //m_redrawTimer.start();
}








void OSGWidget::setScene(osg::Node *scene)
{
    m_scene->removeChildren(0, m_scene->getNumChildren());
    m_scene->addChild(scene);

//    m_viewingCore->setSceneData(m_scene); // redundant after first
//    m_viewingCore->fitToScreen();
}

void OSGWidget::setLightingTwoSided()
{
    osg::ref_ptr<osg::LightModel> lm = new osg::LightModel;
    lm->setTwoSided(true);
    lm->setAmbientIntensity(osg::Vec4(0.1f,0.1f,0.1f,1.0f));

    osg::StateSet *ss;

    for (int i=0 ; i < 2 ; i++ ) {
        ss = ((osgViewer::Renderer *)getCamera()
              ->getRenderer())->getSceneView(i)->getGlobalStateSet();

        ss->setAttributeAndModes(lm, osg::StateAttribute::ON);
    }
}

void OSGWidget::resizeGL(int width, int height)
{
    m_ssao->Resize(width, height);
    m_osgGraphicsWindow->resized(0,0,width,height);
}


void OSGWidget::setCameraModel(osg::ref_ptr<CameraModel> cameraModel)
{
    disconnect(m_cameraModel, SIGNAL(changed()), this, SLOT(update()));
    m_cameraModel = cameraModel;
    connect(m_cameraModel, SIGNAL(changed()), this, SLOT(update()));
    update();
}
