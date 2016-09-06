#ifndef OSG3DVIEWWITHSSAO_H
#define OSG3DVIEWWITHSSAO_H

#include "Osg3dViewWithCamera.h"
#include "SSAONode.h"

class  Osg3dSSAOView : public Osg3dViewWithCamera
{
    Q_OBJECT
public:
    explicit Osg3dSSAOView(QWidget *parent = 0);

    bool ssaoIsEnabled() const { return m_root->getValue(m_root->getChildIndex(m_ssao)); }
    bool ssaoBlurIsEnabled() const { return m_ssao->IsAOBlurEnabled(); }
    bool ssaoHaloRemovalIsEnabled() const {return m_ssao->IsHaloRemovalEnabled();}
    float ssaoRadius() const { return m_ssao->GetSSAORadius(); }
    float ssaoPower() const { return m_ssao->GetSSAOPower(); }
    float ssaoHaloThreshold() const { return m_ssao->GetHaloTreshold(); }
    unsigned ssaoDisplayMode() const { return m_ssao->GetDisplayMode(); }

signals:
    void ssaoRadiusChanged(float f);
    void ssaoPowerChanged(float f);
    void ssaoHaloThresholdChanged(float f);
    void ssaoHaloRemovalChanged(bool tf);
    void ssaoBlurEnabledChanged(bool tf);
    void ssaoIsEnabledChanged(bool tf);

public slots:
    virtual void setSSAOEnabled(bool tf);
    void setSSAOBlurEnabled(bool tf) { m_ssao->setAOBlurEnabled(tf);
                                     emit ssaoBlurEnabledChanged(tf);}
    void setSSAOHaloRemoval(bool tf) { m_ssao->setHaloRemovalEnabled(tf);
                                     emit ssaoHaloRemovalChanged(tf);}
    void setSSAORadius(float r) { m_ssao->SetSSAORadius(r);
                                emit ssaoRadiusChanged(r);}
    void setSSAOPower(float f) { m_ssao->SetSSAOPower(f);
                               emit ssaoPowerChanged(f);}
    void setSSAOHaloThreshold(float f) { m_ssao->SetHaloTreshold(f);
                                       emit ssaoHaloThresholdChanged(f);}
    void setSSAODisplayMode(SSAONode::DisplayMode mode) { m_ssao->SetDisplayMode(mode); update();}
    /// Render one frame
    virtual void paintGL() override;

    /// Updates OpenGL viewport when window size changes
    virtual void resizeGL( int width, int height ) override;

protected:
    SSAONode *m_ssao;
};

#endif // OSG3DVIEWWITHSSAO_H
