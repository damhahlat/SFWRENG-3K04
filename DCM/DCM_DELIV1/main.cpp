#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QMessageBox>

#include "loginwindow.h"
#include "mainwindow.h"
#include "database.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Consistent, modern Fusion style
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    // Soft light theme with readable text
    QPalette pal = app.palette();
    pal.setColor(QPalette::Window, QColor(248, 248, 248));
    pal.setColor(QPalette::Base, QColor(255, 255, 255));
    pal.setColor(QPalette::Button, QColor(245, 245, 245));
    pal.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
    pal.setColor(QPalette::WindowText, Qt::black);
    pal.setColor(QPalette::Text, Qt::black);
    pal.setColor(QPalette::ButtonText, Qt::black);
    pal.setColor(QPalette::ToolTipText, Qt::black);
    pal.setColor(QPalette::PlaceholderText, QColor(120, 120, 120));
    pal.setColor(QPalette::Highlight, QColor(30, 144, 255));
    pal.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(pal);

    // Set application details for consistent AppData paths
    QCoreApplication::setOrganizationName("McMaster");
    QCoreApplication::setApplicationName("PacemakerDCM");

    // Initialize the database
    QString err;
    if (!Database::init(&err)) {
        QMessageBox::critical(nullptr, "Database error", "Failed to open DB:\n" + err);
        return 1;
    }

    // Login dialog
    LoginWindow login;
    if (login.exec() != QDialog::Accepted)
        return 0;

    // Load user info
    const QString user = login.username();
    const int userId = Database::userId(user);

    // Launch the main window
    MainWindow w(userId, user);
    w.show();

    return app.exec();
}
