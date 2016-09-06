#ifndef CAMERAMODEL_H
#define CAMERAMODEL_H
#include <iostream>
#include <sstream>
#include <QObject>
#include <osg/Object>
#include <osg/Node>
#include <osgUtil/LineSegmentIntersector>

class CameraModel : public QObject, public osg::Object
{
    Q_OBJECT
public:
    explicit CameraModel(QObject *parent = 0);
    CameraModel( const CameraModel& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );

    META_Object(osgwTools,CameraModel)

    // simple accessors ///////////////////////////////////////////////////////

    osg::Vec3d viewUp() const { return m_viewUp; }
    osg::Vec3d viewDir() const { return m_viewDir; }
    osg::Vec3d viewCenter() const { return m_viewCenter; }
    double fovY() const { return m_fovY; }
    double viewDistance() const { return m_viewDistance; }
    double aspect() const { return m_aspect; }
    double trackballRollSensitivity() const { return m_trackballRollSensitivity; }
    bool isOrtho() const { return m_ortho; }
    double orthoTop() const { return m_orthoTop; }
    double orthoBottom() const { return m_orthoBottom; }
    bool viewChangeInProgress() const { return m_viewChangeInProgress; }
    bool dollyCanChangeCenter() const { return m_dollyCanChangeCenter; }
    int dollyCenterChangeThreshold() const { return m_dollyCenterChangeThreshold; }
    int dollyCenterPressure() const { return m_dollyCurrentPressure; }
    unsigned cullMask() const { return m_cullMask; }

    // derive other representations of member variables ////////////////////
    osg::Matrixd computeProjection() const;
    osg::Matrixd getModelViewMatrix() const;
    osg::Matrixd getMatrix() const;
    osg::Vec3d getEyePosition() const;

    double getFovyRadians() const;
    osg::Vec3d getAzElTwist() const;
    osg::Vec3d getYawPitchRoll() const;

signals:
    void changed();
    void cullMaskChanged(unsigned);

public slots:
    void computeInitialView();
    void fitToScreen();

    void setUpAndDir(osg::Vec3d up, osg::Vec3d dir);
    void setViewUp(osg::Vec3d v);
    void setViewDir(osg::Vec3d v);
    void setViewCenter(osg::Vec3d newCenter);
    void setFovY(double fov) { m_fovY = fov; emit changed(); }
    void setViewDistance(double distance);
    void setEyePosition(osg::Vec3d p);
    void setAspect(double a);
    void setOrtho(bool tf);
    void fovYScaleUp();
    void fovYScaleDown();


    void setClampFovyScale(bool clamp, osg::Vec2d range);
    void setViewDirFromAzEl(osg::Vec2d aet);
    void saveView(std::stringstream &stream);
    void loadView(std::stringstream &stream);
    void setTrackballRollSensitivity(double s) { m_trackballRollSensitivity=s; }
    void setOrthoFromQAction();

    void stashView();
    void restoreView();

    // Manipulation operations ///////////////////////////////////////////////

    void startOrbit(osg::Vec2d startingNDC);
    void orbit(osg::Vec2d currentNDC);
    void finishOrbit(osg::Vec2d currentNDC);
    void finishOrbit() {finishOrbit(m_startingNDC);}

    void startRotate(osg::Vec2d startingNDC);
    void rotate(osg::Vec2d currentNDC);
    void finishRotate(osg::Vec2d currentNDC);
    void finishRotate() { finishRotate(m_startingNDC); }

    void startPan(osg::Vec2d startingNDC, osg::Vec4d panPlane);
    void pan(osg::Vec2d currentNDC);
    void finishPan(osg::Vec2d currentNDC);
    void finishPan() { finishPan(m_startingNDC); }

    void startZoom(osg::Vec2d startingNDC);
    void zoom(osg::Vec2d currentNDC);
    void finishZoom(osg::Vec2d currentNDC);
    void finishZoom() { finishZoom(m_startingNDC); }

    /** Dolly the camera forwards and backwards. Changes the view distance.
    This function is a no-op for orthographic projections. */
    void startDolly(osg::Vec2d startingNDC);
    void dolly( const double deltaMovement );
    void dolly(osg::Vec2d currentNDC);
    void finishDolly(osg::Vec2d currentNDC);
    void finishDolly() { finishDolly(m_startingNDC); }
    void setDollyCanChangeCenter(bool tf) { m_dollyCanChangeCenter = tf; emit changed(); }
    void setDollyCenterChangeThreshold(int threshold) { m_dollyCenterChangeThreshold = threshold; emit changed(); }

    void setCullMask(unsigned mask) { m_cullMask = mask; emit cullMaskChanged(m_cullMask); }
    void setCullMaskBits(unsigned mask) { setCullMask(m_cullMask | mask); }
    void clearCullMaskBits(unsigned mask) { setCullMask(m_cullMask & ~mask); }


    void setBoundingNode( osg::ref_ptr< osg::Node > boundNode) { m_boundingNode = boundNode; emit changed(); }
private:
    void applyViewChangeMatrix();


    /// Assure m_viewUp and m_viewDir are orthogonal
    inline void orthoNormalize();
    bool intersectPlaneRay(osg::Vec3d &result, const osg::Vec4d &plane, const osg::Vec3d &p0, const osg::Vec3d &p1);
    void getZNearZFarProj(double &zNear, double &zFar, const osg::Matrixd &projMat);
    osg::Matrixd getOrientationMatrix() const;

    // Data /////////////////////////////////////////

    // basic camera parameters
    osg::Vec3d m_viewUp;
    osg::Vec3d m_viewDir;
    osg::Vec3d m_viewCenter;
    double m_fovY;
    double m_viewDistance;

    // Projection matrix support
    double m_aspect;
    bool m_ortho;
    double m_orthoBottom;
    double m_orthoTop;

    // manipulation parameters
    double m_fovYScaleFactor;
    osg::Vec2d m_clampFovyRange;
    bool m_shouldClampFovYScale;
    double m_trackballRollSensitivity;
    bool m_dollyCanChangeCenter;
    int m_dollyCenterChangeThreshold;
    int m_dollyCurrentPressure;

    osg::Vec4d m_panPlane;

    bool m_viewChangeInProgress;  // is the user in the process of adjusting the view
    osg::Matrixd m_viewChangeMatrix;  // matrix representing the change thus far
    osg::Vec2d m_startingNDC;  // NDC where view change began

    // node for doing scene bound computation.
    // This is necesary for fitToScreen() because nodes such as OriginAxis
    // have AutoTransform nodes which skew the bounding box based upon the
    // results of the last draw.
    osg::ref_ptr< osg::Node > m_boundingNode;

    std::string m_stashedView;

    unsigned m_cullMask;
};

#endif // CAMERACONTROL_H
