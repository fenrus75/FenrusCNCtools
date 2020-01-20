#include <QApplication>
#include <QWidget>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
    
    QApplication app(argc, argv);

    MainWindow window;

//    window.resize(250, 150);
    window.setWindowTitle("CNC CAM GUI");
    window.show();

    return app.exec();
}
