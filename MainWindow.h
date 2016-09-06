#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "UiEventWidget.h"
#include <osg/Group>
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
public slots:
    void on_actionOpen_triggered();
    void setMouseModeOrbit();
    void setMouseModePan();
    void setMouseModeRotate();
    void setMouseModeZoom();
    void handleDisplayModeCombo();

    void ssaoHalo(bool tf);
    void ssaoBlur(bool tf);
    void setSSAOEnabled(bool tf);

    void setRadius();
    void setThreshold();
    void setPower();


private:
    void applicationSetup();
    osg::ref_ptr<osg::Geode> buildAxes();
    osg::ref_ptr<osg::Node> buildScene();

    void setupOSGWidget(Osg3dSSAOView *ssaoView);
    void setupSSAOWidget(Osg3dSSAOView *ssaoView);
    void connectHandlers(Osg3dSSAOView *ssaoView);

    Ui::MainWindow *ui;
    osg::ref_ptr<osg::Group> m_world;


};

#endif // MAINWINDOW_H
