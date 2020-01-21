#include "toolwidget.h"
#include <QPainter>
#include <QGridLayout>
#include <QLabel>
#include <QSplitter>
#include <QDoubleSpinBox>
#include <QFrame>

#include "tool.h"
extern "C" {
#include "toolpath.h"
}
ToolCheckbox::ToolCheckbox(const QString &text, QWidget *parent) : QCheckBox(text, parent)
{
}

ToolWidget::ToolWidget(QWidget *parent) : QWidget(parent) 
{  
	QVBoxLayout *vbox = new QVBoxLayout(this);
	int tool;
	vbox->setSpacing(1);

	vbox->addWidget(new QLabel("Tools for VCarve"));
	tool = first_tool();
	while (tool >= 0) {
		char str[4096];
		if (!tool_is_vcarve(tool)) {
			tool = next_tool(tool);			
			continue;
		}
		sprintf(str,"#%i - %s", tool, get_tool_name(tool));
		ToolCheckbox *cb = new ToolCheckbox(QString(str), this);
		cb->set_tool_nr(tool);
		if (scene && scene->has_tool(tool)) {
			cb->setCheckState(Qt::Checked);
		}
		boxes.push_back(cb);
		vbox->addWidget(cb);
		tool = next_tool(tool);			
	}

	vbox->addWidget(new QSplitter(this));

	QFrame* line = new QFrame();
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
	vbox->addWidget(line);


	vbox->addWidget(new QLabel("Tools for pocketing"));


	tool = first_tool();
	while (tool >= 0) {
		char str[4096];
		if (tool_is_vcarve(tool)) {
			tool = next_tool(tool);			
			continue;
		}
		sprintf(str,"#%i - %s", tool, get_tool_name(tool));
		ToolCheckbox *cb = new ToolCheckbox(QString(str), this);
		cb->set_tool_nr(tool);
		if (scene && scene->has_tool(tool)) {
			cb->setCheckState(Qt::Checked);
		}
		boxes.push_back(cb);
		vbox->addWidget(cb);
		tool = next_tool(tool);			
	}
	vbox->addWidget(new QSplitter(this));
	line = new QFrame();
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
	vbox->addWidget(line);

	vbox->addWidget(new QLabel("Toolpath modifiers"));

	QCheckBox *cb = new QCheckBox(QString("Add finishing pass"), this);
	vbox->addWidget(cb);
	QHBoxLayout *hbox = new QHBoxLayout();
	cb = new QCheckBox(QString("Cut out outer polygon"), this);
	hbox->addWidget(cb);


	QDoubleSpinBox  * spinbox = new QDoubleSpinBox(this);
	spinbox->setRange(0, 4);
	spinbox->setSingleStep(0.04);


	hbox->addWidget(spinbox);
	hbox->addWidget(new QLabel("inch"));

	vbox->addLayout(hbox);
	
	vbox->addStretch(1);
	setLayout(vbox);	
}


void ToolWidget::tool_click(int press)
{
	Q_UNUSED(press);

	if (!scene)
		return;

	for (unsigned int i = 0; i < boxes.size(); i++) {
		int tool;
		ToolCheckbox *cb = boxes[i];
		tool = cb->get_tool_nr();
		if (cb->isChecked()) {
			scene->push_tool(tool);
	    } else {
			scene->drop_tool(tool);
		}
		
	}	
}