#pragma once

#include <QMainWindow>
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>

#include "scene.h"

class DrawArea : public QWidget {
	public:
		DrawArea(QWidget *parent = NULL);
		void set_scene(class scene *_scene) { scene = _scene; create_qgraphicsscene_from_scene();};
		class scene *get_scene(void) { return scene; };

	protected:
//		void paintEvent(QPaintEvent *event);


	private:
		void create_qgraphicsscene_from_scene(void);
		QGraphicsScene * qscene;
		QGraphicsView * view;
		class scene *scene;
		

};
