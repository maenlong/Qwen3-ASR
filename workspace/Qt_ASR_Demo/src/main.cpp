#include "mainwindow.h"
#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>

int main(int argc, char *argv[])
{
    // 高分屏适配：随系统缩放（如 4K 屏 150% 时界面同步放大）
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
