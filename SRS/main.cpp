#include <QtGui/QApplication>
#include <QtGui>

#include "mainwindow.h"

#include <iostream>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon("/SRSDCS.icns"));
    MainWindow w;
    w.show();
    return a.exec();
}
