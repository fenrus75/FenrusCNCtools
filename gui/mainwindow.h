#pragma once

#include <QMainWindow>
#include <QApplication>

#include "scene.h"
#include "mainwidget.h"

class MainWindow : public QMainWindow {
	public:
		MainWindow(QWidget *parent = NULL);

		void set_scene(class scene *_scene) { scene = _scene; if (mainwidget) mainwidget->set_scene(_scene);};
		class scene *get_scene(void) { return scene; };

	private:
		class scene *scene;
		MainWidget *mainwidget;
};
