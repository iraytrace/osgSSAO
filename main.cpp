#include "MainWindow.h"
#include <QApplication>

#include <QFile>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // If resources are in shared library, Make all resources loaded
    // see https://wiki.qt.io/QtResources
    // and http://doc.qt.io/qt-5/resources.html
    // Q_INIT_RESOURCE(shaders);



    MainWindow w;
    w.show();

    return a.exec();
}
