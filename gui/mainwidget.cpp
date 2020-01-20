#include "mainwidget.h"
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>


MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent) {
    
  QHBoxLayout *hbox = new QHBoxLayout(this);
  QVBoxLayout *left = new QVBoxLayout();
  QVBoxLayout *right = new QVBoxLayout();




  hbox->addLayout(left);

  hbox->addLayout(right);


  setLayout(hbox);
}