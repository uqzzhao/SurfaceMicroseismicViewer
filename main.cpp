
#include "wviewer.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QObject>

#include <QPixmap>
#include <QSplashScreen>
#include <QElapsedTimer>



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //设置程序启动画面
    QPixmap pixmap(":/resources/start.png");
    QSplashScreen splash(pixmap);
    splash.show();

    // 设置画面停留时间为3秒
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed()<3000)
    {
        QCoreApplication::processEvents();
    }
    WViewer w;
    w.setWindowTitle(QObject::tr("Surface Microseismic Viewer"));
    w.show();
    w.move ((QApplication::desktop()->width() - w.width())/2,
            (QApplication::desktop()->height() - w.height())/2);
    splash.finish(&w);

    return a.exec();
}
