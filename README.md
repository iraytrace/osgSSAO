# osgSSAO
Implementation of screen space ambient occlusion (ssao) using
OpenSceneGraph (OSG).

The application window consists of 3 views.  Two OSG views on the left and
center, and a panel to adjust the screen space ambient occlusion parameters on
the right.

The left-most OSG view is rendered with an OSGWidget which derives from the
legacy QOGLWidget.  This properly displays ssao and is enabled for SSAO at
application startup.

The center OSG view is rendered with an Osg3dSSAOView which derives from a
Osg3dViewWithCamera which in turn derives from the new QOpenGLWidget.  This QT 
widget renders GL into a texture which Qt then composites into the application
window.  This does not properly render the scene.

Inspection of the existing SSAO implementation shows that it does all the work
in PRE- and POST- render passes.  This makes the primary render traversal
of no value.  Ideally, SSAO would render as PRE- render passes and the primary
render traversal would produce the final output.  This would likely solve the
problem with using QOpenGLWidget as well.
