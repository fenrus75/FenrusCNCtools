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
		for (unsigned int i = 0; i < input->poly.size(); i++) {
			unsigned int next = i + 1;
			if (next >= input->poly.size())
				next = 0;
			double X1, Y1, X2, Y2;

			X1 = CGAL::to_double((input->poly)[i].x());
			Y1 = CGAL::to_double((input->poly)[i].y());
			X2 = CGAL::to_double((input->poly)[next].x());
			Y2 = CGAL::to_double((input->poly)[next].y());


			qscene->addLine(mm_to_px(X1), mm_to_px(Y1), mm_to_px(X2), mm_to_px(Y2), pen);
		}

		for (auto input2 : input->children) {
			for (unsigned int i = 0; i < input2->poly.size(); i++) {
				unsigned int next = i + 1;
				if (next >= input2->poly.size())
					next = 0;
				double X1, Y1, X2, Y2;

				X1 = CGAL::to_double((input2->poly)[i].x());
				Y1 = CGAL::to_double((input2->poly)[i].y());
				X2 = CGAL::to_double((input2->poly)[next].x());
				Y2 = CGAL::to_double((input2->poly)[next].y());


				qscene->addLine(mm_to_px(X1), mm_to_px(Y1), mm_to_px(X2), mm_to_px(Y2), pen);

			}
		}
	}
	view = new QGraphicsView(qscene);
	QGridLayout *grid = new QGridLayout(this);
	grid->addWidget(view, 0,0 );
	setLayout(grid);
}