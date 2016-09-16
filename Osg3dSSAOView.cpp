#include "Osg3dSSAOView.h"

Osg3dSSAOView::Osg3dSSAOView(QWidget *parent)
    : Osg3dViewWithCamera(parent)
    , m_ssao(new SSAONode(width(), height()))
{

    m_root->removeChild(m_scene); // un-do the Osg3dViewWithCamera setup
    SSAONode::buildGraph(m_root, m_scene, m_ssao);
}

void Osg3dSSAOView::setSSAOEnabled(bool tf)
{
    SSAONode::setSSAOEnabled(m_root, m_scene, m_ssao, tf);

    emit ssaoIsEnabledChanged(tf);
    update();
}

struct CameraPreDrawCallback : public osg::Camera::DrawCallback
{
	CameraPreDrawCallback(int fboInt) :
	_fboInt(fboInt)
	{
	}

	virtual void operator () (const osg::Camera& /*camera*/) const
	{
		QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
		f->glBindFramebuffer(GL_FRAMEBUFFER, _fboInt);
	}

	int _fboInt;
};

void Osg3dSSAOView::paintGL()
{
    // Update the camera
    osg::Camera *cam = this->getCamera();

        cam->setCullMask( m_cameraModel->cullMask() );

    m_cameraModel->setAspect((double)width() / (double)height());

    cam->setViewMatrix(m_cameraModel->getModelViewMatrix() );
    cam->setProjectionMatrix(m_cameraModel->computeProjection());

	int fboInt = defaultFramebufferObject();
	cam->setPreDrawCallback(new CameraPreDrawCallback(fboInt));

    // Let SSAO class know that camera has changed
    m_ssao->updateProjectionMatrix(getCamera()->getProjectionMatrix());

    // Invoke the OSG traversal pipeline
    frame();
}

void Osg3dSSAOView::resizeGL(int width, int height)
{
    m_ssao->Resize(width, height);
    m_osgGraphicsWindow->resized(0,0,width,height);
}
