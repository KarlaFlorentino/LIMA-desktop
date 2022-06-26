#include "mainwindow.h"
#include <QApplication>
#include <QScreen>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    w.setWindowTitle("L.I.M.A. Leaf Image Measurement and Analysis");

    //w.showMaximized();

    /////////////////////////

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect  screenGeometry = screen->geometry();
    int height = screenGeometry.height();
    int width = screenGeometry.width();
    w.resize(width, height);
    w.move(0,0);
    w.show();

    /////////////////////////

    return a.exec();

}
