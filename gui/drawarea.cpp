#include "drawarea.h"
#include <QPainter>

DrawArea::DrawArea(QWidget *parent)
    : QWidget(parent) {  }


void DrawArea::paintEvent(QPaintEvent *e)  
{
	Q_UNUSED(e);
  
	QPainter qp(this);
	drawContent(&qp);
}

void DrawArea::drawContent(QPainter *qp)
{
	QPen pen(Qt::black, 2, Qt::SolidLine);  
	qp->setPen(pen);
	qp->drawLine(1,1,50,50);
}