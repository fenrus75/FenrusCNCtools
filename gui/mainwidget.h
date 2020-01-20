#pragma once

#include <QMainWindow>
#include <QApplication>

#include "scene.h"
#include "drawarea.h"

class MainWidget : public QWidget {
	public:
		MainWidget(QWidget *parent = NULL);
		void set_scene(class scene *_scene) { scene = _scene; if (drawarea) drawarea->set_scene(_scene); };
		class scene *get_scene(void) { return scene; };

	private:
		class scene *scene;
		DrawArea * drawarea;

};
