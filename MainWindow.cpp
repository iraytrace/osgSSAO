#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "SSAONode.h"
#include "Osg3dSSAOView.h"

#include <QSettings>
#include <QFileDialog>
#include <QFileInfo>

#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_world(new osg::Group)

{
    ui->setupUi(this);
    Osg3dSSAOView *ssaoView = ui->uiEventWidget->ssaoView();

    applicationSetup();
    connectHandlers(ssaoView);
    setupOSGWidget(ssaoView);
    setupSSAOWidget(ssaoView);
    setMouseModeOrbit();

    m_world->addChild(buildScene());
    ui->osgWidget->setScene(m_world);
    ui->uiEventWidget->ssaoView()->addNode(m_world);
    ui->uiEventWidget->ssaoView()->cameraModel()->fitToScreen();
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::applicationSetup()
{
    // set up application name
    QFileInfo applicationFile(QApplication::applicationFilePath());

    // These allow us to simply construct a "QSettings" object without arguments
    qApp->setOrganizationDomain("org.vissimlab");
    qApp->setApplicationName(applicationFile.baseName());
    qApp->setOrganizationName("LeeButler");
    qApp->setApplicationVersion(__DATE__ __TIME__);


    QActionGroup *group = new QActionGroup(this);
    group->addAction(ui->actionOrbit);
    group->addAction(ui->actionPan);
    group->addAction(ui->actionZoom);
    group->addAction(ui->actionRotate);
}

void MainWindow::connectHandlers(Osg3dSSAOView *ssaoView)
{
    connect(ui->actionOrbit, SIGNAL(triggered()), this, SLOT(setMouseModeOrbit()) );
    connect(ui->actionPan, SIGNAL(triggered()), this, SLOT(setMouseModePan()) );
    connect(ui->actionRotate, SIGNAL(triggered()), this, SLOT(setMouseModeRotate()) );
    connect(ui->actionZoom, SIGNAL(triggered()), this, SLOT(setMouseModeZoom()) );

    connect(ui->radiusEdit, SIGNAL(editingFinished()),
            this, SLOT(setRadius()));
    connect(ui->thresholdEdit, SIGNAL(editingFinished()),
            this, SLOT(setThreshold()));
    connect(ui->powerEdit, SIGNAL(editingFinished()),
            this, SLOT(setPower()));
    connect(ui->displayModeCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(handleDisplayModeCombo()));
    connect(ui->aoBlurr, SIGNAL(toggled(bool)),
            this, SLOT(ssaoBlur(bool)));
    connect(ui->haloRemoval, SIGNAL(toggled(bool)),
            this, SLOT(ssaoHalo(bool)));
    connect(ui->ssaoCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(setSSAOEnabled(bool)));

    ui->powerEdit->setText(QString("%1").arg(ssaoView->ssaoPower()));
    ui->radiusEdit->setText(QString("%1").arg(ssaoView->ssaoRadius() ));
    ui->thresholdEdit->setText(QString("%1").arg(ssaoView->ssaoHaloThreshold() ));

    ui->haloRemoval->setChecked(ssaoView->ssaoHaloRemovalIsEnabled());
    ui->aoBlurr->setChecked(ssaoView->ssaoBlurIsEnabled());
    ui->displayModeCombo->setCurrentIndex(ssaoView->ssaoDisplayMode());

}

void MainWindow::setupOSGWidget(Osg3dSSAOView *ssaoView)
{
    ui->osgWidget->setSSAOPower(ssaoView->ssaoPower());
    ui->osgWidget->setSSAORadius(ssaoView->ssaoRadius());
    ui->osgWidget->setSSAOHaloThreshold(ssaoView->ssaoHaloThreshold());

    ui->osgWidget->setSSAOHaloRemoval(ssaoView->ssaoHaloRemovalIsEnabled());
    ui->osgWidget->setSSAOBlurEnabled(ssaoView->ssaoBlurIsEnabled());
    ui->osgWidget->setSSAODisplayMode((SSAONode::DisplayMode)ssaoView->ssaoDisplayMode());
    ui->osgWidget->setCameraModel(ssaoView->cameraModel());

    ui->osgWidget->setSSAOEnabled(true);
}

void MainWindow::setupSSAOWidget(Osg3dSSAOView *ssaoView)
{
    connect(ui->haloRemoval, SIGNAL(toggled(bool)),
            ssaoView, SLOT(setSSAOHaloRemoval(bool)));
    connect(ui->aoBlurr, SIGNAL(toggled(bool)),
            ssaoView, SLOT(setSSAOBlurEnabled(bool)));
    connect(ui->ssaoCheckBox, SIGNAL(toggled(bool)),
            ssaoView, SLOT(setSSAOEnabled(bool)));
}
osg::ref_ptr<osg::Geode> MainWindow::buildAxes()
{
    osg::ref_ptr<osg::Geometry> m_axisGeom = new osg::Geometry();
    // allocate verticies
    osg::ref_ptr<osg::Vec3Array> m_axisVerts = new osg::Vec3Array;
    m_axisVerts->push_back(osg::Vec3(-1.0, 0.0, 0.0));
    m_axisVerts->push_back(osg::Vec3(0.0, 0.0, 0.0));
    m_axisVerts->push_back(osg::Vec3(1.0, 0.0, 0.0));
    m_axisVerts->push_back(osg::Vec3(0.0, -1.0, 0.0));
    m_axisVerts->push_back(osg::Vec3(0.0, 0.0, 0.0));
    m_axisVerts->push_back(osg::Vec3(0.0, 1.0, 0.0));
    m_axisVerts->push_back(osg::Vec3(0.0, 0.0, -1.0));
    m_axisVerts->push_back(osg::Vec3(0.0, 0.0, 0.0));
    m_axisVerts->push_back(osg::Vec3(0.0, 0.0, 1.0));

    m_axisGeom->setVertexArray(m_axisVerts);
    m_axisGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 2));
    m_axisGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 1, 2));
    m_axisGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 3, 2));
    m_axisGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 4, 2));
    m_axisGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 6, 2));
    m_axisGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 7, 2));

    // allocate colors
    osg::Vec4Array* colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(.35f,0.0f,0.0f,1.0f));
    colors->push_back(osg::Vec4(1.0f,0.0f,0.0f,1.0f));
    colors->push_back(osg::Vec4(0.0f,.35f,0.0f,1.0f));
    colors->push_back(osg::Vec4(0.0f,1.0f,0.0f,1.0f));
    colors->push_back(osg::Vec4(0.0f,0.0f,.35f,1.0f));
    colors->push_back(osg::Vec4(0.0f,0.0f,1.0f,1.0f));

    m_axisGeom->setColorArray(colors);
    m_axisGeom->setColorBinding(osg::Geometry::BIND_PER_PRIMITIVE_SET);
    m_axisGeom->setDataVariance(osg::Object::DYNAMIC);

    // generate the Geode
    osg::ref_ptr<osg::Geode> m_axisGeode = new osg::Geode();
    m_axisGeode->addDrawable(m_axisGeom);
    //m_axisGeode->setNodeMask( MASK_AXIS );
    m_axisGeode->setName("Axis");

    //turn off lighting so we always see the line color
    osg::StateSet *ss = m_axisGeode->getOrCreateStateSet();
    ss->setMode( GL_LIGHTING, osg::StateAttribute::OFF );

    // Set the linewidth a little wider so we can see the thing
    osg::LineWidth *lineWidth = new osg::LineWidth;
    lineWidth->setWidth( 2.0 );
    ss->setAttributeAndModes(lineWidth);

    return m_axisGeode;
}


// TODO: Remove
double drand48()
{
    return ((double)rand()/(double)RAND_MAX);
}
double randColor()
{
    return drand48() * 0.875 + 0.125;
}
osg::ref_ptr<osg::Node> MainWindow::buildScene()
{
    osg::ref_ptr<osg::Group> node = new osg::Group;

    osg::ref_ptr<osg::Geode> geode = buildAxes();
    node->addChild(geode);
#if 1
    geode = new osg::Geode;
#define RANDCOORD ((drand48() - 0.5) * 2.0)

    float boxDimen = 30.0;
    float sphMin = 0.125;
    float sphMax = 4;

    for (int i=0 ; i < 5000 ; i++ ) {
        float x = boxDimen * RANDCOORD;
        float y = boxDimen * RANDCOORD;
        float z = boxDimen * RANDCOORD;
        float radius =  sphMin + (drand48() * (sphMax-sphMin));

        osg::ShapeDrawable *sd = new osg::ShapeDrawable(new osg::Box(osg::Vec3(x, y, z), radius));
        sd->setColor(osg::Vec4(randColor(), randColor(), randColor(), 1.0));
        geode->addDrawable(sd);
    }

    node->addChild(geode);
    osgDB::writeNodeFile(*node, "testScene.osg");
#endif

    return node;
}

void MainWindow::on_actionOpen_triggered()
{

    QSettings settings;
    // if there is a current workFlowController, we need

    QString fileName = QFileDialog::getOpenFileName(this, "Select File",
             settings.value("currentDirectory").toString(),
             "OSG File (*.osg *.ive *.osgt *.osgb *.obj *.ply)");

    if (fileName.isEmpty() || fileName.isNull())
        return;

    osg::ref_ptr<osg::Node> loaded = osgDB::readNodeFile(fileName.toStdString());

    if (!loaded.valid()) return;

    const osg::BoundingSphere &bs = loaded->getBound();
    if (bs.radius() <= 0.0) return;

    m_world->removeChildren(0, m_world->getNumChildren());

    m_world->addChild(loaded);

    QFileInfo fi(fileName);
    settings.setValue("currentDirectory", fi.absolutePath());

    ui->uiEventWidget->ssaoView()->cameraModel()->fitToScreen();
}

void MainWindow::setMouseModeOrbit()
{
    ui->actionOrbit->setChecked(true);
    ui->uiEventWidget->setMouseMode(UiEventWidget::MM_ORBIT);
}

void MainWindow::setMouseModePan()
{   
    ui->actionPan->setChecked(true);
    ui->uiEventWidget->setMouseMode(UiEventWidget::MM_PAN);
}

void MainWindow::setMouseModeRotate()
{    
    ui->actionRotate->setChecked(true);
    ui->uiEventWidget->setMouseMode(UiEventWidget::MM_ROTATE);
}

void MainWindow::setMouseModeZoom()
{    
    ui->actionZoom->setChecked(true);
    ui->uiEventWidget->setMouseMode(UiEventWidget::MM_ZOOM);
}

void MainWindow::handleDisplayModeCombo()
{
    ui->osgWidget->setSSAODisplayMode(
                (SSAONode::DisplayMode)
                ui->displayModeCombo->currentIndex()
                );
    ui->uiEventWidget->ssaoView()->setSSAODisplayMode(
                (SSAONode::DisplayMode)
                ui->displayModeCombo->currentIndex()
                );

    ui->osgWidget->update();
    ui->uiEventWidget->ssaoView()->update();
}

void MainWindow::ssaoHalo(bool tf)
{
    ui->osgWidget->setSSAOHaloRemoval(tf);
    ui->uiEventWidget->ssaoView()->setSSAOHaloRemoval(tf);
    ui->osgWidget->update();
    ui->uiEventWidget->ssaoView()->update();
}

void MainWindow::ssaoBlur(bool tf)
{
    ui->osgWidget->setSSAOBlurEnabled(tf);
    ui->uiEventWidget->ssaoView()->setSSAOBlurEnabled(tf);
    ui->osgWidget->update();
    ui->uiEventWidget->ssaoView()->update();
}

void MainWindow::setSSAOEnabled(bool tf)
{
    ui->displayModeCombo->setEnabled(tf);
    ui->osgWidget->setSSAOEnabled(tf);
    ui->uiEventWidget->ssaoView()->setSSAOEnabled(tf);
}

void MainWindow::setRadius()
{
    float radius = ui->radiusEdit->text().toFloat();

    ui->osgWidget->setSSAORadius(radius);
    ui->uiEventWidget->ssaoView()->setSSAORadius(radius);

    ui->osgWidget->update();
    ui->uiEventWidget->ssaoView()->update();
}

void MainWindow::setThreshold()
{
    float threshold = ui->thresholdEdit->text().toFloat();
    ui->osgWidget->setSSAOHaloThreshold(threshold);
    ui->uiEventWidget->ssaoView()->setSSAOHaloThreshold(threshold);
    ui->osgWidget->update();
    ui->uiEventWidget->ssaoView()->update();
}

void MainWindow::setPower()
{
    float power = ui->powerEdit->text().toFloat();
    ui->osgWidget->setSSAOPower(power);
    ui->uiEventWidget->ssaoView()->setSSAOPower(power);
    ui->osgWidget->update();
    ui->uiEventWidget->ssaoView()->update();
}
