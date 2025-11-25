// Import some header files and libraries
#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QMessageBox>

#include "loginwindow.h"
#include "mainwindow.h"
#include "database.h"

// Main function.
int main(int argc, char *argv[]) {

    // Just creates the app.
    QApplication app(argc, argv);

    // Applies a colourscheme to the app.
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    // Colourscheme which makes our app smell like isopropyl alcohol
    QPalette pal;
    pal.setColor(QPalette::Window, QColor(245,245,247));
    pal.setColor(QPalette::WindowText, Qt::black);
    pal.setColor(QPalette::Base, Qt::white);
    pal.setColor(QPalette::AlternateBase, QColor(240,240,240));
    pal.setColor(QPalette::ToolTipBase, Qt::white);
    pal.setColor(QPalette::ToolTipText, Qt::black);
    pal.setColor(QPalette::Text, Qt::black);
    pal.setColor(QPalette::Button, QColor(245,245,247));
    pal.setColor(QPalette::ButtonText, Qt::black);
    pal.setColor(QPalette::Highlight, QColor(0,120,215));
    pal.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(pal);

    // Start DB
    QString err;
    if (!Database::init(&err)) {
        QMessageBox::critical(nullptr, "DCM", "Failed to open DB: " + err);
        return 1;
    }

    // Login
    LoginWindow login;
    if (login.exec() != QDialog::Accepted)
        return 0;

    // Get the user and the userID.
    const QString user = login.username();
    const int userId = Database::userId(user);

    // Run the mainwindow if login was successful.
    MainWindow w(userId, user);
    w.show();

    return app.exec();
}
