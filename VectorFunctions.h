//  This software is covered by the MIT open source license. See misc/MIT.txt

#ifndef VECTORFUNCTIONS_H
#define VECTORFUNCTIONS_H
#include <osg/Vec2d>
#include <osg/Vec3d>
#include <QString>

double preAcosClamp(double val, const double minval, const double maxval);

class VectorFunctions
{
    VectorFunctions() {}
public:
    static void vecFromBallisticAzEl(osg::Vec2d ae,
                                     osg::Vec3d &dir,
                                     osg::Vec3d &up);
    static osg::Vec2d azElFromVec(osg::Vec3d vecDir);
    static bool vecFromString(osg::Vec3d &v, QString str);
};

#endif // VECTORFUNCTIONS_H
