#include "mainwindow.h"

#include "unistd.h"
#include <QApplication>

#include <QSplashScreen>
#include <QLabel>
#include <QMovie>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //软件开场动画
    QPixmap pixmap(":/myImage/image/loading.gif");
    QSplashScreen splash(pixmap);
    QLabel label(&splash);
    QMovie mv(":/myImage/image/loading.gif");

    label.setMovie(&mv);
    mv.start();
    splash.show();
    splash.setCursor(Qt::BlankCursor);
    for(int i=0; i<3000; i+=mv.speed())
    {
        QCoreApplication::processEvents();
        usleep(500*static_cast<useconds_t>(mv.speed()));
    }

    MainWindow w;
    w.show();
    splash.close();

    return a.exec();
}
