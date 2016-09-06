//  This software is covered by the MIT open source license. See misc/MIT.txt

#include "VectorFunctions.h"
#include <osg/Matrixd>
#include <QRegExp>
#include <QStringList>


void VectorFunctions::vecFromBallisticAzEl(osg::Vec2d ae,
                                                 osg::Vec3d &dir,
                                                 osg::Vec3d &up)
{
    osg::Vec3d xAxis(1.0, 0.0, 0.0);
    osg::Vec3d yAxis(0.0, 1.0, 0.0);
    osg::Vec3d zAxis(0.0, 0.0, 1.0);
    dir = osg::Vec3d(-1.0, 0.0, 0.0);
    up = zAxis;

    osg::Matrixd mat;
    mat.makeRotate(osg::DegreesToRadians(-ae[1]), yAxis);
    dir = dir * mat;
    up = up * mat;

    mat.makeRotate(osg::DegreesToRadians(ae[0]), zAxis);
    dir = dir * mat;
    up = up * mat;
}
osg::Vec2d VectorFunctions::azElFromVec(osg::Vec3d vecDir)
{
    const double twoPi( 2. * osg::PI );
    osg::Vec3d xAxis(1.0, 0.0, 0.0);
    osg::Vec3d yAxis(0.0, 1.0, 0.0);
    osg::Vec3d zAxis(0.0, 0.0, 1.0);

    vecDir.normalize();
    osg::Vec3d rightHandDirection = vecDir ^ zAxis;

    osg::Vec3d viewDirInYXPlane = zAxis ^ rightHandDirection;
    viewDirInYXPlane.normalize();

    double cosAngleRadiansInPlane = preAcosClamp(viewDirInYXPlane * xAxis, -1.0, 1.0);
    double azimuthInRadians = acos(cosAngleRadiansInPlane);

    bool pointsTowardsNegativeYAxis = (viewDirInYXPlane * yAxis) < 0.0;
    if ( pointsTowardsNegativeYAxis )
        azimuthInRadians = twoPi - azimuthInRadians;

    double cosAngleRadiansOfElevation = preAcosClamp(vecDir * viewDirInYXPlane, -1.0, 1.0);

    double elevationInRadians = acos(cosAngleRadiansOfElevation);

    bool pointsTowardsZAxis = (vecDir * zAxis) >= 0.0;
    if (pointsTowardsZAxis) elevationInRadians *= -1.0;



    return osg::Vec2d(osg::RadiansToDegrees( azimuthInRadians ),
                      osg::RadiansToDegrees(elevationInRadians));

}

bool VectorFunctions::vecFromString(osg::Vec3d &v, QString str)
{
    static const char *floatRegExpPattern = "([+-]?([0-9]*[\\.])?[0-9]+)";
    QRegExp m_regExp(floatRegExpPattern);
    QStringList tokens;

    for (int pos = 0;
         (pos = m_regExp.indexIn(str, pos)) != -1 ;
         pos += m_regExp.matchedLength()) {
        tokens << m_regExp.cap(1);
    }
    if (tokens.size() != 3) return false;
    v = osg::Vec3d(tokens[0].toDouble(),
                 tokens[1].toDouble(),
                 tokens[2].toDouble() );
    return true;
}

double preAcosClamp(double val, const double minval, const double maxval)
{
    if (val < minval) {
        if ( (minval-val) > 1.0) { qDebug("gross clamping error"); abort(); }
        val = minval;
    }
    else if (val > maxval) {
        if ( (val - maxval) > 1.0) { qDebug("gross clamping error"); abort(); }
        val = maxval;
    }
    return val;
}
