#include "mainwindow.h"
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle("DCM - Qt Skeleton");
    resize(900, 600);

    // Central placeholder widget
    auto* central = new QWidget(this);
    setCentralWidget(central);
}

MainWindow::~MainWindow() = default;
