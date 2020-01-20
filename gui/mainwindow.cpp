#include "mainwindow.h"
#include "mainwidget.h"
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    
  QAction *quit = new QAction("&Quit", this);

  QMenu *file;
  file = menuBar()->addMenu("&File");
  file->addAction(quit);

  connect(quit, &QAction::triggered, qApp, QApplication::quit);

  MainWidget *mainwidget = new MainWidget(this);

  setCentralWidget(mainwidget);




  statusBar()->showMessage("Ready");

}