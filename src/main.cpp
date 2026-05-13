/**
 * Simulytix — Arduino Circuit Simulator
 * Main entry point using Qt.
 */

#include <QApplication>
#include <QStyleFactory>
#include "MainWindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    // Set application properties
    app.setApplicationName("Simulytix");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("Simulytix");

    // Create and show main window
    MainWindow window;
    window.show();

    return app.exec();
}
