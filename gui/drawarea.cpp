#include "drawarea.h"
#include <QPainter>
#include <QGridLayout>

#include "toolpath.h"

DrawArea::DrawArea(QWidget *parent)
    : QWidget(parent) {  }


#if 0
void DrawArea::paintEvent(QPaintEvent *e)  
{
	Q_UNUSED(e);
  
	QPainter qp;

	qp.begin(this);
	qp.setRenderHint(QPainter::Antialiasing);
//	qp.setRenderHint(QPainter::HighQualityAntialiasing);

	drawContent(&qp);
	qp.end();
}
#endif
void DrawArea::create_qgraphicsscene_from_scene(void)
{
	if (!scene)
		return;
	if (!qscene)
		qscene = new QGraphicsScene;


	QPen pen(Qt::black, 2, Qt::SolidLine);  

	for (auto input : scene->shapes) {
		QPolygonF *newpoly = new(QPolygonF);

		for (unsigned int i = 0; i < input->poly.size(); i++) {
			double X1, Y1;

			X1 = CGAL::to_double((input->poly)[i].x());
			Y1 = -CGAL::to_double((input->poly)[i].y());

			*newpoly << QPointF(mm_to_px(X1), mm_to_px(Y1));
		}
		qscene->addPolygon(*newpoly, pen);

		for (auto input2 : input->children) {
			QPolygonF *newpoly = new(QPolygonF);
			for (unsigned int i = 0; i < input2->poly.size(); i++) {
				double X1, Y1;

				X1 = CGAL::to_double((input2->poly)[i].x());
				Y1 = -CGAL::to_double((input2->poly)[i].y());
				*newpoly << QPointF(mm_to_px(X1), mm_to_px(Y1));

			}
			qscene->addPolygon(*newpoly, pen);
		}
	}
	view = new QGraphicsView(qscene);
	QGridLayout *grid = new QGridLayout(this);
	grid->addWidget(view, 0,0 );
	setLayout(grid);
}