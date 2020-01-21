#pragma once

#include <QMainWindow>
#include <QApplication>
#include <QCheckBox>

using namespace std;
class ToolCheckbox : public QCheckBox {
	public:
		ToolCheckbox(const QString &text, QWidget *parent = nullptr);
		void set_tool_nr(int _toolnr) { toolnr = _toolnr; };
		int get_tool_nr(void) { return toolnr; };

	private:
		int toolnr;
};

class ToolWidget : public QWidget {
	Q_OBJECT

	public:
		ToolWidget(QWidget *parent = NULL);
		void set_scene(class scene *_scene) { scene = _scene; };
		class scene *get_scene(void) { return scene; };

	protected:

	private:
		class scene *scene;
		std::vector<class ToolCheckbox*> boxes;

	private slots:
		void tool_click(int);
		
};
