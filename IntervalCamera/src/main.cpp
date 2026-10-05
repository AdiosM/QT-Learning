#include "camerawindow.h"
#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("IntervalCamera");
    QCoreApplication::setApplicationName("IntervalCamera");
    CameraWindow window;
    window.show();
    return app.exec();
}
