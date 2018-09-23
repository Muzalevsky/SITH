#include <QApplication>
#include <QMainWindow>
#include "benchwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    BenchWindow w;
    w.show();
    return a.exec();
}
