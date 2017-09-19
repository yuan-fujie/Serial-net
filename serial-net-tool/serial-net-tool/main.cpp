#include "mainwindow.h"
#include <QApplication>
#include "commonstyle.h"

int main(int argc, char *argv[])
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);//  ”¶∏ﬂ∑÷∆¡
#endif

    QApplication a(argc, argv);
    MainWindow w;

    w.skin_mode = my_skin::white;
    if(w.skin_mode == my_skin::white)
    {
        w.skin_mode = my_skin::black;
        CommonHelper::setStyle(":/style/White/white.qss");
    }
    else if(w.skin_mode == my_skin::black)
    {
        w.skin_mode = my_skin::white;
        CommonHelper::setStyle(":/style/Black/black.qss");
    }
    w.show();


    return a.exec();
}
