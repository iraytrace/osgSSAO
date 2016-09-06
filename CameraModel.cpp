#include "CameraModel.h"
#include <osgUtil/LineSegmentIntersector>
#include <QAction>
#include <osg/io_utils>

CameraModel::CameraModel(QObject *parent)
    : osg::Object()
    , QObject(parent)
    , m_viewUp(osg::Vec3d(0., 0., 1.))
    , m_viewDir(osg::Vec3d(0., 1., 0.))
    , m_viewCenter(osg::Vec3d( 0., 0., 0.))
    , m_fovY(30.0)
    , m_viewDistance(20.0)
    , m_aspect(1.0)
    , m_ortho(false)
    , m_orthoBottom(0.0)
    , m_orthoTop(0.0)

    , m_fovYScaleFactor(1.1)
    , m_clampFovyRange( osg::Vec2d( 5.0, 160.0 ) )
    , m_shouldClampFovYScale(true)
    , m_trackballRollSensitivity(1.3)
    , m_dollyCanChangeCenter(true)
    , m_dollyCenterChangeThreshold(10)
    , m_dollyCurrentPressure(0)
    , m_panPlane( osg::Vec4d(0.0, 0.0, 0.0, 0.0) )
    , m_viewChangeInProgress(false)
    , m_viewChangeMatrix(osg::Matrixd::identity())
    , m_startingNDC( osg::Vec2d( 0.0, 0.0) )
    , m_stashedView("")
    , m_cullMask(~0)
{

}

CameraModel::CameraModel(const CameraModel &rhs, const osg::CopyOp &copyop)
    : osg::Object(rhs, copyop)
    , QObject()
    , m_viewUp(rhs.m_viewUp)
    , m_viewDir(rhs.m_viewDir)
    , m_viewCenter(rhs.m_viewCenter)
    , m_fovY(rhs.m_fovY)
    , m_viewDistance(rhs.m_viewDistance)
    , m_aspect(rhs.m_aspect)
    , m_ortho(rhs.m_ortho)
    , m_orthoBottom(rhs.m_orthoBottom)
    , m_orthoTop(rhs.m_orthoTop)
    , m_fovYScaleFactor(rhs.m_fovYScaleFactor)
    , m_clampFovyRange(rhs.m_clampFovyRange)
    , m_shouldClampFovYScale(rhs.m_shouldClampFovYScale)
    , m_trackballRollSensitivity(rhs.m_trackballRollSensitivity)
    , m_dollyCanChangeCenter(rhs.m_dollyCanChangeCenter)
    , m_dollyCenterChangeThreshold(rhs.m_dollyCenterChangeThreshold)
    , m_dollyCurrentPressure(0)
    , m_panPlane(rhs.m_panPlane)
    , m_viewChangeInProgress(false)
    , m_stashedView(rhs.m_stashedView)
    , m_startingNDC(rhs.m_startingNDC)
    , m_cullMask(rhs.m_cullMask)

{

}

void CameraModel::computeInitialView()
{
    m_viewUp = osg::Vec3d(0., 0., 1.);
    m_viewDir = osg::Vec3d(0., 1., 0.);

    fitToScreen();
}

double CameraModel::getFovyRadians() const
{
    return( osg::DegreesToRadians( m_fovY ) );
}
#include "VectorFunctions.h"
osg::Vec3d CameraModel::getAzElTwist() const
{
#if 0

    const double twoPi( 2. * osg::PI );
    osg::Vec3d xAxis(1.0, 0.0, 0.0);
    osg::Vec3d yAxis(0.0, 1.0, 0.0);
    osg::Vec3d zAxis(0.0, 0.0, 1.0);

    osg::Vec3d rightHandDirection = m_viewDir ^ zAxis;

    osg::Vec3d viewDirInYXPlane = zAxis ^ rightHandDirection;
    viewDirInYXPlane.normalize();

    double cosAngleRadiansInPlane = viewDirInYXPlane * xAxis;
    double azimuthInRadians = acos(cosAngleRadiansInPlane);

    bool pointsTowardsNegativeYAxis = (ViewDirInXYPlane * yAxis) < 0.0;
    if ( pointsTowardsNegativeYAxis )
        azimuthInRadians = twoPi - azimuthInRadians;


    double cosAngleRadiansOfElevation = m_viewDir * viewDirInYXPlane;
    double elevationInRadians = acos(cosAngleRadiansOfElevation);


    return osg::Vec3d(osg::RadiansToDegrees( azimuthInRadians ),
                      osg::RadiansToDegrees(elevationInRadians),
                      0.0);
#else
    abort();
#endif
}

osg::Vec3d CameraModel::getYawPitchRoll() const
{
    osg::Vec3d xAxis(1.0, 0.0, 0.0);
    osg::Vec3d yAxis(0.0, 1.0, 0.0);
    osg::Vec3d zAxis(0.0, 0.0, 1.0);
    const osg::Vec3d viewDirXBaseUp( m_viewDir ^ zAxis );
    const double twoPi( 2. * osg::PI );


    // Yaw

    // Compute view direction, projected into plane defined by base up.
    // TBD what if _viewDir and _baseUp are coincident?
    osg::Vec3d projectedDir = zAxis ^ viewDirXBaseUp;
    projectedDir.normalize();
    // Is the vector pointing to the left of north, or to the right?
    //osg::Vec3d right = xAxis ^ zAxis;
    const double dotDirRight = projectedDir * -yAxis;
    // Dot product of two unit vectors is the cosine of the angle between them.
    const double dotDirNorth = preAcosClamp(projectedDir * xAxis, -1.0, 1.0);
    double yawRad = acos( dotDirNorth );
    if( dotDirRight > 0. )
        yawRad = osg::PI + ( osg::PI - yawRad );

    if( yawRad == twoPi )
        yawRad = 0.;
    double yaw = osg::RadiansToDegrees( yawRad );


    // Pitch

    const double dotDirUp = m_viewDir * zAxis;
    const double dotUpUp = preAcosClamp(m_viewUp * zAxis, -1.0, 1.0);
    double pitchRad = acos( osg::absolute< double >( dotUpUp ) );
    if( dotDirUp < 0. )
        pitchRad *= -1.;
    double pitch = osg::RadiansToDegrees( pitchRad );


    // Roll

    // Compute base up projected onto plane defined by view direction.
    // TBD what if _viewDir and _baseUp are coincident?
    osg::Vec3d projectedBaseUp = viewDirXBaseUp ^ m_viewDir;
    projectedBaseUp.normalize();
    // Is the view up vector pointing to the left of the projected base up, or to the right?
    osg::Vec3d right = m_viewDir ^ projectedBaseUp;
    const double dotUpRight = m_viewUp * right;
    // Dot product of two unit vectors is the cosine of the angle between them.
    const double dotUp = preAcosClamp(projectedBaseUp * m_viewUp, -1.0, 1.0);
    double rollRad = acos( dotUp );
    if( dotUpRight > 0. )
        rollRad = osg::PI + ( osg::PI - rollRad );

    if( rollRad == twoPi )
        rollRad = 0.;
    double roll = osg::RadiansToDegrees( rollRad );

    return osg::Vec3d(yaw, pitch, roll);
}

osg::Matrixd CameraModel::computeProjection() const
{
    if( !( m_boundingNode.valid() ) ) {
        osg::notify( osg::WARN ) << "CameraModel::computeProjection: _scene == NULL." << std::endl;
        return( osg::Matrixd::identity() );
    }

    // TBD do we really want eyeToCenter to be a vector
    // to the *bound* center, or to the *view* center?
    const osg::BoundingSphere& bs = m_boundingNode->getBound();

    const osg::Vec3d eyeToCenter( bs._center - getEyePosition() );
    if( m_ortho ) {
        double zNear = eyeToCenter.length() - bs._radius;
        double zFar = eyeToCenter.length() + bs._radius;

        const double xRange = m_aspect * ( m_orthoTop - m_orthoBottom );
        const double right = xRange * .5;

        return( osg::Matrixd::ortho( -right, right, m_orthoBottom, m_orthoTop, zNear, zFar ) );
    } else {
        double zNear = eyeToCenter.length() - bs._radius;
        double zFar = zNear + ( bs._radius * 2. );
        if( zNear < 0. ) {
            zNear = zFar / 2000.; // Default z ratio.
        }
        return( osg::Matrixd::perspective( m_fovY, m_aspect, zNear, zFar ) );
    }
}

osg::Matrixd CameraModel::getModelViewMatrix() const
{
    osg::Matrixd m;
    m.invert( getMatrix() );
    return( m );
}

osg::Matrixd CameraModel::getMatrix() const
{
    const osg::Vec3d& d = m_viewDir;
    const osg::Vec3d& u = m_viewUp;
    osg::Vec3d r = d ^ u;
    const osg::Vec3d p = getEyePosition();

    osg::Matrixd m = osg::Matrixd(
                         r[0], r[1], r[2], 0.0,
                         u[0], u[1], u[2], 0.0,
                         -d[0], -d[1], -d[2], 0.0,
                         p[0], p[1], p[2], 1.0 );

    return( m );
}

osg::Vec3d CameraModel::getEyePosition() const
{
    return( m_viewCenter - ( m_viewDir * m_viewDistance ) );
}


void CameraModel::setEyePosition(osg::Vec3d p)
{
    osg::Vec3d dir = p - m_viewCenter;

    double viewDistance = dir.normalize();

    if (viewDistance == m_viewDistance && dir == m_viewDir)
        return;

    m_viewDistance = viewDistance;
    m_viewDir = dir;

    emit changed();
}

void CameraModel::setAspect(double a)
{
    if (m_aspect == a) return;
    m_aspect = a;
    emit changed();
}

void CameraModel::setOrtho(bool tf)
{
    if (tf == m_ortho) return;

    m_ortho = tf;
    emit changed();
}
void CameraModel::setOrthoFromQAction()
{
    QAction *a = dynamic_cast<QAction *>(sender());
    if (a)
        setOrtho( a->data().toBool() );
}
void CameraModel::fovYScaleUp()
{
    m_fovY *= m_fovYScaleFactor;
    if( m_shouldClampFovYScale ) {
        m_fovY = osg::clampBelow< double >( m_fovY, m_clampFovyRange.y() );
    }

    m_orthoBottom *= m_fovYScaleFactor;
    m_orthoTop *= m_fovYScaleFactor;

    emit changed();
}

void CameraModel::fovYScaleDown()
{
    const double factor( 1.0 / m_fovYScaleFactor );
    m_fovY *= factor;
    if( m_shouldClampFovYScale ) {
        m_fovY = osg::clampAbove< double >( m_fovY, m_clampFovyRange.x() );
    }

    m_orthoBottom *= factor;
    m_orthoTop *= factor;
    emit changed();
}

void CameraModel::setClampFovyScale(bool clamp, osg::Vec2d range)
{
    m_shouldClampFovYScale = clamp;
    m_clampFovyRange = range;

    if (m_shouldClampFovYScale) {
        m_fovY = osg::clampBetween< double >( m_fovY,
                                              m_clampFovyRange.x(),
                                              m_clampFovyRange.y() );
    }
    emit changed();
}


void CameraModel::setViewDirFromAzEl(osg::Vec2d aet)
{
    osg::Vec3d xAxis(1.0, 0.0, 0.0);
    osg::Vec3d yAxis(0.0, 1.0, 0.0);
    osg::Vec3d zAxis(0.0, 0.0, 1.0);

    osg::Matrixd mat;
    mat.makeRotate(osg::DegreesToRadians(aet[1]), yAxis);
    m_viewUp = zAxis * mat;
    m_viewDir = xAxis * mat;

    mat.makeRotate(osg::DegreesToRadians(aet[0]+180.0), zAxis);
    m_viewUp = m_viewUp * mat;
    m_viewDir = m_viewDir * mat;

    emit changed();
}

void CameraModel::saveView(std::stringstream &stream)
{
    stream << m_viewUp.x() << " "
           << m_viewUp.y() << " "
           << m_viewUp.z() << " "
           << m_viewDir.x() << " "
           << m_viewDir.y() << " "
           << m_viewDir.z() << " "
           << m_viewCenter.x() << " "
           << m_viewCenter.y() << " "
           << m_viewCenter.z() << " ";
    stream << m_viewDistance << " ";
    stream << m_fovY << " ";

    stream << (m_ortho?"true ":"false ");

    stream << m_aspect << " "
           << m_orthoBottom << " "
           << m_orthoTop << " ";
}

void CameraModel::loadView(std::stringstream &stream)
{
    stream >> m_viewUp.x() >> m_viewUp.y() >> m_viewUp.z();
    stream >> m_viewDir.x() >> m_viewDir.y() >> m_viewDir.z();
    stream >> m_viewCenter.x() >> m_viewCenter.y() >> m_viewCenter.z();
    stream >> m_viewDistance;
    stream >> m_fovY;
    std::string orthoString;
    stream >> orthoString;
    m_ortho = (orthoString == "true");
    stream >> m_aspect;
    stream >> m_orthoBottom;
    stream >> m_orthoTop;
}



void CameraModel::stashView()
{
    std::stringstream ss;
    saveView(ss);
    ss.flush();

    m_stashedView = ss.str();
}

void CameraModel::restoreView()
{
    if (m_stashedView.size() < 0) return;

    std::stringstream ss(m_stashedView);
    loadView(ss);
}

void CameraModel::startOrbit(osg::Vec2d startingNDC)
{
    m_startingNDC = startingNDC;
    m_viewChangeInProgress = true;
}

void CameraModel::orbit(osg::Vec2d currentNDC)
{
    osg::Vec2d deltaNDC = currentNDC - m_startingNDC;

    const osg::Matrixd orientMat = getOrientationMatrix();

    // Take the spin direction 'dir' and rotate it 90 degrees
    // to get our base axis (still in the window plane).
    // Simultaneously convert to current view space.
    osg::Vec2d screenAxis( -deltaNDC[ 1 ], deltaNDC[ 0 ] );
    const osg::Vec3d baseAxis = osg::Vec3d( screenAxis[ 0 ], screenAxis[ 1 ], 0. ) * orientMat;
    osg::Vec3d dir3 = osg::Vec3d( deltaNDC[ 0 ], deltaNDC[ 1 ], 0. ) * orientMat;
    dir3.normalize();

    // The distance from center, along with the roll sensitivity,
    // tells us how much to rotate the baseAxis (ballTouchAngle) to get
    // the actual ballAxis.
    const double distance = m_startingNDC.length();
    const double rotationDir( ( screenAxis * m_startingNDC > 0. ) ? -1. : 1. );
    const double ballTouchAngle = rotationDir * m_trackballRollSensitivity * distance;
    osg::Vec3d ballAxis = baseAxis * osg::Matrixd::rotate( ballTouchAngle, dir3 );
    ballAxis.normalize();

    osg::Matrixd m = osg::Matrixd::rotate( -( deltaNDC.length() ), ballAxis );

    // Re-orient the basis.
    m_viewDir = m_viewDir * m;
    m_viewUp = m_viewUp * m;

    orthoNormalize();

    m_startingNDC = currentNDC;

    emit changed();
}

void CameraModel::finishOrbit(osg::Vec2d currentNDC)
{
    m_viewChangeInProgress = false;
    orbit(currentNDC);
}


osg::Matrixd CameraModel::getOrientationMatrix() const
{
    const osg::Vec3d& d = m_viewDir;
    const osg::Vec3d& u = m_viewUp;
    osg::Vec3d r = d ^ u;

    osg::Matrixd m = osg::Matrixd(
                         r[0], r[1], r[2], 0.0,
                         u[0], u[1], u[2], 0.0,
                         -d[0], -d[1], -d[2], 0.0,
                         0.0, 0.0, 0.0, 1.0 );
    return( m );
}


void CameraModel::startRotate(osg::Vec2d startingNDC)
{
    m_startingNDC = startingNDC;
    m_viewChangeInProgress = true;
}

void CameraModel::rotate(osg::Vec2d currentNDC)
{
    osg::Vec2d deltaNDC = currentNDC - m_startingNDC;


    // Compute m_viewChangeMatrix here
    // Position is constant in 1st person view. Obtain it (for later use)
    // *before* we alter the _viewDir.
    const osg::Vec3d position = getEyePosition();

    // Compute rotation matrix.
    osg::Vec3d cross = m_viewDir ^ m_viewUp;
    osg::Matrix m = osg::Matrix::rotate( deltaNDC[ 0 ], m_viewUp ) *
                    osg::Matrix::rotate( -deltaNDC[ 1 ], cross );

    // Re-orient the basis.
    m_viewDir = m_viewDir * m;
    m_viewUp = m_viewUp * m;
    orthoNormalize();

    // Compute the new view center.
    m_viewCenter = position + ( m_viewDir * m_viewDistance );

    m_startingNDC = currentNDC;
    emit changed();
}

void CameraModel::finishRotate(osg::Vec2d currentNDC)
{
    rotate(currentNDC);

    m_viewChangeInProgress = false;
    emit changed();
}

void CameraModel::getZNearZFarProj(double &zNear, double &zFar, const osg::Matrixd &projMat)
{
    if( m_ortho ) {
        double l, r, b, t;
        projMat.getOrtho( l, r, b, t, zNear, zFar );
    } else {
        double fovy, aspect;
        projMat.getPerspective( fovy, aspect, zNear, zFar );
    }
}

bool CameraModel::intersectPlaneRay( osg::Vec3d& result, const osg::Vec4d& plane, const osg::Vec3d& p0, const osg::Vec3d& p1 )
{
    osg::Vec3d planeNormal = osg::Vec3d( plane[ 0 ], plane[ 1 ], plane[ 2 ] );

    osg::notify( osg::DEBUG_FP ) << "    p0 " << p0 << std::endl;
    osg::notify( osg::DEBUG_FP ) << "    p1 " << p1 << std::endl;
    const osg::Vec3d vDir = p1 - p0;
    const double dotVd = vDir * planeNormal;
    osg::notify( osg::DEBUG_FP ) << "  dotVd " << dotVd << std::endl;
    if( dotVd == 0. ) {
        osg::notify( osg::WARN ) << "ViewingCore::intersectPlaneRay: No plane intersection." << std::endl;
        return( false );
    }
    double length = -( planeNormal * p0 + plane[ 3 ] ) / dotVd;
    osg::notify( osg::DEBUG_FP ) << "  length " << length << std::endl;
    result = p0 + ( vDir * length );
    osg::notify( osg::DEBUG_FP ) << "    intersection point " << result << std::endl;
    return( true );
}
void CameraModel::startPan(osg::Vec2d startingNDC, osg::Vec4d panPlane)
{
    m_startingNDC = startingNDC;
    m_viewChangeInProgress = true;
    m_panPlane = panPlane;
}
void CameraModel::pan(osg::Vec2d currentNDC)
{
    // Get the view volume far plane value, and the distance from
    // the near to far plane.
    double zNear, zFar;
    osg::Vec2d deltaNDC = m_startingNDC - currentNDC;

    osg::Matrixd p = computeProjection();

    getZNearZFarProj(zNear, zFar, p);

    const double distance = zFar - zNear;

    // Create two points, both in NDC space, and lying on the far plane at the back
    // of the view volume. One is the xy origin, the other with the passed xy parameters.
    osg::Vec4d farPoint0 = osg::Vec4d( 0., 0., 1., 1. );
    osg::Vec4d farPoint1 = osg::Vec4d( deltaNDC.x(), deltaNDC.y(), 1., 1. );
    if( !m_ortho ) {
        // Not ortho, so w != 1.0. Multiply by the far plane distance.
        // This yields values in clip coordinates.
        farPoint0 *= zFar;
        farPoint1 *= zFar;
    }

    // Get inverse view & proj matrices to back-transform the
    // two clip coord far points into world space.
    osg::Matrixd v = getMatrix();

    osg::Matrixd invProjMat;
    invProjMat.invert( p );

    osg::Vec4d wc0 = farPoint0 * invProjMat * v;
    osg::Vec4d wc1 = farPoint1 * invProjMat * v;

    // Intersect the two world coord points with the pan plane.
    osg::Vec3d result0, result1;
    osg::Vec3d p1( wc0.x(), wc0.y(), wc0.z() );
    osg::Vec3d p0 = m_ortho ? p1 - ( m_viewDir * distance ) : getEyePosition();
    intersectPlaneRay( result0, m_panPlane, p0, p1 );
    p1 = osg::Vec3d( wc1.x(), wc1.y(), wc1.z() );
    p0 = m_ortho ? p1 - ( m_viewDir * distance ) : getEyePosition();
    intersectPlaneRay( result1, m_panPlane, p0, p1 );

    // Subtract the two plane intersection points to get the delta world coord
    // motion and move the view center accordingly.
    osg::Vec3d delta = result1 - result0;
    osg::notify( osg::DEBUG_FP ) << "    delta " << delta << std::endl;
    m_viewCenter += delta;

    m_startingNDC = currentNDC;
//    m_viewChangeMatrix.makeTranslate(delta);

    emit changed();
}

void CameraModel::finishPan(osg::Vec2d currentNDC)
{
    m_viewChangeInProgress = false;
    pan(currentNDC);
}

void CameraModel::startZoom(osg::Vec2d startingNDC)
{
    m_startingNDC = startingNDC;
    m_viewChangeInProgress = true;
}

void CameraModel::zoom(osg::Vec2d currentNDC)
{
    if (m_startingNDC.y() == currentNDC.y())
        return;

    if (currentNDC.y() > m_startingNDC.y()) {
        fovYScaleUp();
    } else {
        fovYScaleDown();
    }
    m_startingNDC = currentNDC;
    emit changed();
}

void CameraModel::finishZoom(osg::Vec2d currentNDC)
{
    m_viewChangeInProgress = false;
    zoom(currentNDC);
}

void CameraModel::startDolly(osg::Vec2d startingNDC)
{
    m_startingNDC = startingNDC;
    m_viewChangeInProgress = true;
    m_dollyCurrentPressure = 0;
}
void CameraModel::dolly(const double deltaMovement)
{
    if( m_ortho || ! m_boundingNode.valid() )
        // No dolly in ortho mode
        return;

    // Scale based on model size. TBD this should be under
    // app control so that it can be disabled if desired.
    const osg::BoundingSphere& bs = m_boundingNode->getBound();
    double scale( bs._radius * .5 );
    if( m_viewDistance > bs._radius )
        scale *= ( m_viewDistance / bs._radius );

    double newViewDistance = m_viewDistance + ( deltaMovement * scale );

    if (newViewDistance >= 1.0) {
        if (newViewDistance > m_viewDistance)
            m_dollyCurrentPressure = 0;

        m_viewDistance = newViewDistance;

    } else if (m_dollyCanChangeCenter) {
        m_dollyCurrentPressure++;
        if (m_dollyCurrentPressure > m_dollyCenterChangeThreshold) {
            double centerDistanceToMove = 1 - newViewDistance;
            m_viewCenter = m_viewCenter + (m_viewDir * centerDistanceToMove);
            m_viewDistance = 1.;
        }
    }
    emit changed();
}


void CameraModel::dolly(osg::Vec2d currentNDC)
{
    if (currentNDC.y() > m_startingNDC.y()) {
        dolly(0.5);
    } else if (currentNDC.y() < m_startingNDC.y()) {
        dolly (-0.5);
    } else {
        emit changed();  // Odd but needed in case of m_ViewChangeInProgress
    }
    m_startingNDC = currentNDC;
}

void CameraModel::finishDolly(osg::Vec2d currentNDC)
{
    m_viewChangeInProgress = false;
    dolly(currentNDC);
}


void CameraModel::orthoNormalize()
{
    osg::Vec3d cross = m_viewDir ^ m_viewUp;
    m_viewUp = cross ^ m_viewDir;
    m_viewDir.normalize();
    m_viewUp.normalize();
}

void CameraModel::fitToScreen()
{
    if (!m_boundingNode.valid()) {
        qDebug("CameraModel::fitToScreen() invalid boundingNode");
        return;
    }
    const osg::BoundingSphere &bs = m_boundingNode->getBound();
    m_viewCenter = bs.center();

    double sceneRadius = bs.radius();
    if (sceneRadius <= 0.0) sceneRadius = 10.0;

    // tan( fovy/2. ) = bs.radius / distance
    // Solve for distance:
    // distance = bs.radius / tan( fovy/2. )
    m_fovY = 30;
    float distance = sceneRadius /
            tan( osg::DegreesToRadians( m_fovY/2. ) );

    m_viewDistance = distance;

    m_orthoTop = tan( getFovyRadians() * 0.5 ) * m_viewDistance;
    m_orthoBottom = -m_orthoTop;

    emit changed();
}


void CameraModel::setUpAndDir(osg::Vec3d up, osg::Vec3d dir)
{
    m_viewDir = dir;
    m_viewUp = up;
    orthoNormalize();
    emit changed();
}

void CameraModel::setViewUp(osg::Vec3d v)
{
    osg::Matrixd mat;
    mat.makeRotate(m_viewUp, v);

    m_viewDir = m_viewDir * mat;
    m_viewUp = v;
    emit changed();
}

void CameraModel::setViewDir(osg::Vec3d v)
{
    osg::Matrixd mat;
    mat.makeRotate(m_viewDir, v);

    m_viewUp = m_viewUp * mat;
    m_viewDir = v;
    emit changed();
}

void CameraModel::setViewCenter(osg::Vec3d newCenter)
{
    const osg::Vec3d lastEyePosition = getEyePosition();

    m_viewCenter = newCenter;
    m_viewDistance = (lastEyePosition - m_viewCenter).length();

    emit changed();
}

void CameraModel::setViewDistance(double distance)
{
    if (m_viewDistance == distance) return;
    m_viewDistance = distance;
    emit changed();
}


