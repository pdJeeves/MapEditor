#include "viewwidget.h"
#include <QMenu>
#include "mainwindow.h"
#include <QMouseEvent>
#include <QToolTip>
#include <QPainter>

ViewWidget::ViewWidget(QWidget *parent) :
	super(parent)
{
	setMouseTracking(true);
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
	        this, SLOT(ShowContextMenu(const QPoint &)));
}


void ViewWidget::mousePressEvent(QMouseEvent * event)
{
	if(event->button() == Qt::LeftButton)
	{
		window->onMousePress(event->pos(), size());
	}
}

void ViewWidget::mouseReleaseEvent(QMouseEvent * event)
{
	if(event->button() == Qt::LeftButton)
	{
		window->onMouseRelease(event->pos(), size());
	}
}

void ViewWidget::mouseDoubleClickEvent(QMouseEvent * event)
{
	window->onDoubleClick(event->pos(), size());
}

void ViewWidget::mouseMoveEvent(QMouseEvent * event)
{
	last_pos = event->pos();
	window->onMouseMoveEvent(event->pos(), size());
}

void ViewWidget::wheelEvent(QWheelEvent * event)
{
	window->event(event);
}

void ViewWidget::keyPressEvent(QKeyEvent * event)
{
	window->event(event);
}

void ViewWidget::keyReleaseEvent(QKeyEvent * event)
{
	window->event(event);
}

void ViewWidget::ShowContextMenu(const QPoint & pos)
{
	QMenu * menu = window->showContextMenu(this, pos, size());

	if(menu)
	{
		 menu->popup(mapToGlobal(pos));
	}
}

bool ViewWidget::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
	   QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);

	   QString string = window->getToolTip(helpEvent->pos(), size());

	   if(!string.isEmpty())
	   {
		   QToolTip::showText(helpEvent->globalPos(), string);
	   }
	   else
	   {
		   QToolTip::hideText();
		   event->ignore();
	   }

	   return true;
   }

   return super::event(event);
}

void ViewWidget::paintEvent(QPaintEvent * event)
{
static int grid_size = 8;
static QBrush background(Qt::white);
static QBrush square_color(Qt::gray);

	QPainter painter;
	painter.begin(this);

	painter.fillRect(event->rect(), background);
	for(int x = 0, column = 0; x < event->rect().width(); x += grid_size, ++column)
	{
		for(int y = column & 0x01? grid_size : 0; y < event->rect().height(); y += grid_size*2)
		{
			painter.fillRect(QRect(x, y, grid_size, grid_size), square_color);
		}
	}

	window->draw(painter, last_pos, size());
	painter.end();
}
