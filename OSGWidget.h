#ifndef OSGWIDGET_H
#define OSGWIDGET_H

#include <QTimer>
#include <QTime>
#include <QGLWidget>
#include <osgViewer/Viewer>
#include <osg/Switch>
#include "CameraModel.h"
#include "SSAONode.h"

class OSGWidget : public QGLWidget,
    public osgViewer::Viewer
{
    Q_OBJECT

public:
    OSGWidget(QWidget *parent);

    // Overridden from osgViewer::Viewer //////////////////////////////////////
    /// Let others tell what scene graph we should be drawing
    void setScene(osg::Node *scene);

public:
    bool ssaoIsEnabled() const { return m_root->getValue(m_root->getChildIndex(m_ssao)); }
    bool ssaoBlurIsEnabled() const { return m_ssao->IsAOBlurEnabled(); }
    bool ssaoHaloRemovalIsEnabled() const {return m_ssao->IsHaloRemovalEnabled();}
    float ssaoRadius() const { return m_ssao->GetSSAORadius(); }
    float ssaoPower() const { return m_ssao->GetSSAOPower(); }
    float ssaoHaloThreshold() const { return m_ssao->GetHaloTreshold(); }
    unsigned ssaoDisplayMode() const { return m_ssao->GetDisplayMode(); }


public slots:
    void setSSAOEnabled(bool tf);
    void setSSAOBlurEnabled(bool tf) { m_ssao->setAOBlurEnabled(tf);}
    void setSSAOHaloRemoval(bool tf) { m_ssao->setHaloRemovalEnabled(tf);}
    void setSSAORadius(float r) { m_ssao->SetSSAORadius(r);;}
    void setSSAOPower(float f) { m_ssao->SetSSAOPower(f);}
    void setSSAOHaloThreshold(float f) { m_ssao->SetHaloTreshold(f);}
    void setSSAODisplayMode(SSAONode::DisplayMode mode);
    /// Render one frame
    virtual void paintGL() override;

    void setCameraModel(osg::ref_ptr<CameraModel> cameraModel);

private:
    //  SSAO support //////////////////////////////////////

    SSAONode* m_ssao;
    // Helper functions ///////////////////////////////////////////////////////

    /// OSG uses singleSided drawing/display by default.
    /// This is annoying when you are "inside" something and the back wall of
    /// it simply disappears. This gets called to set up to draw both front and
    /// back facing polygons so that the world behaves in a more normal fashion.
    /// Yes it's a performance hit.
    void setLightingTwoSided();

    /// Invoked whenever the window size changes so that the OpenGL viewport
    /// gets updated appropriately
    void resizeGL( int width, int height );

    // Private data ///////////////////////////////////////////////////////////

    osg::ref_ptr<osg::Switch> m_root;
    osg::ref_ptr<osg::Group> m_scene;

    /// OSG graphics window
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> m_osgGraphicsWindow;

    /// Viewing Core --> controls the camera of the osgViewer
    osg::ref_ptr< CameraModel > m_cameraModel;

};

#endif // OSGVIEW_H
