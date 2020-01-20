#pragma once

#include <QMainWindow>
#include <QApplication>

#include "scene.h"

class DrawArea : public QWidget {
	public:
		DrawArea(QWidget *parent = NULL);
		void set_scene(class scene *_scene) { scene = _scene; };
		class scene *get_scene(void) { return scene; };

	protected:
		void paintEvent(QPaintEvent *event);
	    void drawContent(QPainter *qp);


	private:
		class scene *scene;

};
