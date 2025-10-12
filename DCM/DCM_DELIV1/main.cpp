#include <QApplication>
#include "mainwindow.h"


// Main application loop
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
