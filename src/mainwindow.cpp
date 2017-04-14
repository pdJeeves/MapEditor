#include <QDesktopWidget>
#include "mainwindow.h"
#include "viewwidget.h"
#include "ui_mainwindow.h"
#include <QKeyEvent>
#include <QWheelEvent>
#include <QMessageBox>
#include <QFileDialog>
#include "byteswap.h"
#include <QPainter>
#include <iostream>
#include <QImageReader>
#include <QStandardPaths>
#include <QImageWriter>
#include <QProgressDialog>
#include <QCursor>
#include "roomsfromimage.h"
#include <memory>
#include "airfromrooms.h"
#include "airfromroomsnp.h"
#include "linkrooms.h"
#include "joinroomsvertically.h"


QVector<QRgb> MainWindow::palette332;

QRgb MainWindow::palette[16] = {
	0x00000000,
	0xFFFFFFFF,
	0xFFAAAAAA,
	0xFF555555,

	0xFF000000,
	0xFFFF0000,
	0xFFAA0000,
	0xFFFF5500,

	0xFFFFAA00,
	0xFFAAFF00,
	0xFF00FF00,
	0xFF00AA00,

	0xFF00AAFF,
	0xFF0000FF,
	0xFFAA00FF,
	0xFFFF00FF
};

std::ostream & operator<<(std::ostream & o, const QPoint & point)
{
	o << "QPoint(" << point.x() << ", " << point.y() << ")";
	return o;
}

MainWindow::MainWindow(QWidget *parent) :
QMainWindow(parent),
state(0),
left(0),
right(0),
top_left(0),
top_right(0),
bottom_left(0),
bottom_right(0),
mouse_down(true),
newEdge(QKeySequence("n"), this),
extrudeEdge(QKeySequence("e"), this),
grabMode(QKeySequence("g"), this),
escape(Qt::Key_Escape, this),
dimensions(-1, -1),
zoom(1.0),
autosaveTimer(this),
toolGroup(this),
viewGroup(this),
ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	ui->widget->window = this;


	toolGroup.addAction(ui->actionAdd);
	toolGroup.addAction(ui->actionSelect);

	viewGroup.addAction(ui->actionShow_Baked_Map);
	viewGroup.addAction(ui->actionShow_Albedo_Map);
	viewGroup.addAction(ui->actionShow_Normal_Map);
	viewGroup.addAction(ui->actionShow_Microsurface_Map);
	viewGroup.addAction(ui->actionShow_Reflectivity_Map);

	connect(ui->actionNew, &QAction::triggered, this, &MainWindow::documentNew);
	connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::documentOpen);
	connect(ui->actionClose, &QAction::triggered, this, &MainWindow::documentClose);
	connect(ui->actionSave, &QAction::triggered, this, &MainWindow::documentSave);
	connect(ui->actionSave_As, &QAction::triggered, this, &MainWindow::documentSaveAs);

	connect(ui->actionImport_Parallax_Layer, &QAction::triggered, this, &MainWindow::openParallaxLayer );
	connect(ui->actionExport_Parallax_Layer, &QAction::triggered, this, &MainWindow::saveParallaxLayer );

	connect(ui->actionImport_Spr, &QAction::triggered, this, &MainWindow::documentImportSpr );
	connect(ui->actionImport_s16, &QAction::triggered, this, &MainWindow::documentImportS16);
	connect(ui->actionImport_Blk, &QAction::triggered, this, &MainWindow::documentImportBlk);

	connect(ui->actionExit, &QAction::triggered, this, &MainWindow::applicationExit);

	connect(ui->actionUndo, &QAction::triggered, this, &MainWindow::editUndo);
	connect(ui->actionRedo, &QAction::triggered, this, &MainWindow::editRedo);

	connect(ui->actionCut, &QAction::triggered, [this]() { editCopy(); editDelete(); });
	connect(ui->actionCopy, &QAction::triggered, this, &MainWindow::editCopy);
	connect(ui->actionPaste, &QAction::triggered, this, &MainWindow::editPaste);
	connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::editDelete);

	connect(ui->actionRemove_Hidden_Pixels, &QAction::triggered, [this]() { removeHidden(); });

	connect(ui->actionCalculate_CA_Field, &QAction::triggered, [this]()
	{
		LinkRooms::interConnectRooms(rooms);
		std::vector< std::list<Room> > caField = AirFromRooms::initialPass(rooms);

		int height = dimensions.height() % 256;
		int width = dimensions.width() % 256;

		int y = tiles().height()-1;
		for(int x = 0; x < tiles().width(); ++x)
		{
			int i = x*tiles().height() + y;

			for(auto j = caField[i].begin(), k = std::next(j); j != caField[i].end(); j = k++)
			{
				if(j->bottom_left > height)
				{
					j->bottom_left = height;
				}
				if(j->bottom_right > height)
				{
					j->bottom_right = height;
				}

				if(height - j->top_left < 4
				&& height - j->top_right < 4)
				{
					caField[i].erase(j);
				}
			}
		}

		for(int y = 0; y < tiles().height(); ++y)
		{
			int i = (tiles().width()-1)*tiles().height() + y;

			for(auto j = caField[i].begin(), k = std::next(j); j != caField[i].end(); j = k++)
			{
				if(j->right > width)
				{
					j->right = width;
				}

				if(width - j->left  < 4)
				{
					caField[i].erase(j);
				}
			}
		}

		JoinRoomsVertically::linkShafts(caField, tiles());
		CaField = JoinRoomsVertically::toWorldSpace(caField, tiles());
		JoinRoomsVertically::joinPairs(CaField);
//		JoinRoomsVertically::joinPairs(CaField);
	//	AirFromRoomsNP::processTiles(CaField);
	});

	connect(ui->actionShow_Background, &QAction::triggered, [this]() { ui->widget->repaint(); });
	connect(ui->actionShow_Foreground, &QAction::triggered, [this]() { ui->widget->repaint(); });
	connect(ui->actionShow_Cutouts, &QAction::triggered, [this]() { ui->widget->repaint(); });

	connect(ui->actionShow_Baked_Map, &QAction::triggered, [this]() { ui->widget->repaint(); });
	connect(ui->actionShow_Albedo_Map, &QAction::triggered, [this]() { ui->widget->repaint(); });
	connect(ui->actionShow_Normal_Map, &QAction::triggered, [this]() { ui->widget->repaint(); });
	connect(ui->actionShow_Microsurface_Map, &QAction::triggered, [this]() { ui->widget->repaint(); });
	connect(ui->actionShow_Reflectivity_Map, &QAction::triggered, [this]() { ui->widget->repaint(); });

	connect(ui->actionBackgroundBakedMap, &QAction::triggered, [this]() { replaceImage(background[0][0]); });
	connect(ui->actionBackgroundAlbedoMap, &QAction::triggered, [this]() { replaceImage(background[0][1]); });
	connect(ui->actionBackgroundNormalMap, &QAction::triggered, [this]() { replaceImage(background[0][2]); });
	connect(ui->actionBackgroundMicrosurfaceMap, &QAction::triggered, [this]() { replaceImage(background[0][3]); });
	connect(ui->actionBackgroundReflectivityMap, &QAction::triggered, [this]() { replaceImage(background[0][4]); });

	connect(ui->actionForegroundBakedMap, &QAction::triggered, [this]() { replaceImage(background[1][0]); });
	connect(ui->actionForegroundAlbedoMap, &QAction::triggered, [this]() { replaceImage(background[1][1]); });
	connect(ui->actionForegroundNormalMap, &QAction::triggered, [this]() { replaceImage(background[1][2]); });
	connect(ui->actionForegroundMicrosurfaceMap, &QAction::triggered, [this]() { replaceImage(background[1][3]); });
	connect(ui->actionForegroundReflectivityMap, &QAction::triggered, [this]() { replaceImage(background[1][4]); });

	connect(ui->actionCutoutsBakedMap, &QAction::triggered, [this]() { replaceImage(background[2][0]); });
	connect(ui->actionCutoutsAlbedoMap, &QAction::triggered, [this]() { replaceImage(background[2][1]); });
	connect(ui->actionCutoutsNormalMap, &QAction::triggered, [this]() { replaceImage(background[2][2]); });
	connect(ui->actionCutoutsMicrosurfaceMap, &QAction::triggered, [this]() { replaceImage(background[2][3]); });
	connect(ui->actionCutoutsReflectivityMap, &QAction::triggered, [this]() { replaceImage(background[2][4]); });

	connect(ui->actionAdd, &QAction::triggered, [this]() { selectedRooms.clear();; });
	connect(ui->actionSelect, &QAction::triggered, [this]() { state = 0; });
	connect(ui->actionMeld, &QAction::triggered, this, &MainWindow::meld);
	connect(ui->actionRemove_Thin_Rooms, &QAction::triggered, this, &MainWindow::removeThinRooms);

	connect(ui->actionImportRooms, &QAction::triggered, this, [this]() { actionImportRooms(true); } );

	connect(ui->actionAdd_Fluid_Map, &QAction::triggered, this, [this]() { actionImportRooms(false); } );

	connect(ui->actionZoom_Out, &QAction::triggered, [this]() { zoom *= .8; ui->widget->repaint(); });
	connect(ui->actionZoom_In, &QAction::triggered, [this]() { zoom  *= 1/.8;  ui->widget->repaint(); });
	connect(ui->actionActual_Size, &QAction::triggered, [this]() { zoom  = 1.0; ui->widget->repaint(); });

	connect(ui->actionAbout, &QAction::triggered, [this]() { QApplication::aboutQt(); });

	connect(ui->horizontalScrollBar, &QScrollBar::valueChanged, [this](int) { ui->widget->repaint(); });
	connect(ui->verticalScrollBar, &QScrollBar::valueChanged, [this](int) { ui->widget->repaint(); });

	connect(&newEdge, &QShortcut::activated, [this]() { roomMesh.addQuad(getMousePosition()); ui->widget->repaint(); } );
//	connect(&grabMode, &QShortcut::activated, [this]() { roomMesh.grab(getMousePosition()); } );
	connect(&escape, &QShortcut::activated, [this]() { roomMesh.escape(); ui->widget->repaint(); } );

	setGeometry(
	    QStyle::alignedRect(
	        Qt::LeftToRight,
	        Qt::AlignCenter,
	        size(),
	        qApp->desktop()->availableGeometry()
	    )
	);

	connect(&autosaveTimer, &QTimer::timeout, [this]() { if(!filepath.isEmpty()) documentSave(); } );

	autosaveTimer.setInterval(5*60*1000);
	autosaveTimer.setTimerType(Qt::VeryCoarseTimer);
	autosaveTimer.setSingleShot(false);
}

MainWindow::~MainWindow()
{
	delete ui;
}

//------------------------------
// behaviors
//-----------------------------

void MainWindow::documentNew() {}

void MainWindow::removeThinRooms()
{

}

void MainWindow::meld()
{
	for(int x = 0; x < tiles().width()-1; ++x)
	{
		for(int y = 0; y < tiles().height(); ++y)
		{
			int i00 = x*tiles().height() + y;
			int i10 = i00 + tiles().height();

			for(auto i = rooms[i00].begin(); i != rooms[i00].end(); ++i)
			{
				if(i->right == 255)
				{
					for(auto j = rooms[i10].begin(); j != rooms[i10].end(); ++j)
					{
						if(j->left != 0)
						{
							continue;
						}

						if(std::abs((int) i->top_right - j->top_left) < 4)
						{
							i->top_right = j->top_left = (i->top_right + j->top_left) / 2;
						}

						if(std::abs((int) i->bottom_right - j->bottom_left) < 4)
						{
							i->bottom_right = j->bottom_left = (i->bottom_right + j->bottom_left) / 2;
						}
					}
				}
				else
				{
					for(auto j = rooms[i00].begin(); j != rooms[i00].end(); ++j)
					{
						if(j->left != i->right)
						{
							continue;
						}

						if(std::abs((int) i->top_right - j->top_left) < 4)
						{
							i->top_right = j->top_left = (i->top_right + j->top_left) / 2;
						}

						if(std::abs((int) i->bottom_right - j->bottom_left) < 4)
						{
							i->bottom_right = j->bottom_left = (i->bottom_right + j->bottom_left) / 2;
						}
					}
				}
			}
		}
	}
}

void MainWindow::documentClose() {}

bool MainWindow::dimensionCheck(QSize dimension1, bool room_map)
{
	if(dimensions != QSize(-1, -1))
	{
		if((!room_map && dimensions != dimension1)
		|| ( room_map && dimensions != dimension1*2))
		{
			/*
			for(int i = 0; i < 3; ++i)
			{
				for(int j = 0; j < 4; ++j)
				{
					if(i == 0 && j == 3)
					{
						continue;
					}

					if(!background[i][j].isNull())
					{
						goto error;
					}
				}
			}

			return true;

error:

			// */
			QMessageBox mesg;
			mesg.setText(QObject::tr("The given background would not match the current dimensions."));
			mesg.exec();
			return false;
		}
	}

	return true;
}

void MainWindow::applicationExit() {}

void MainWindow::editUndo()
{
	if(commandList.canRollBack())
	{
		selectedRooms.clear();
		commandList.rollBack(this);
		ui->actionUndo->setEnabled(commandList.canRollBack());
		ui->actionRedo->setEnabled(commandList.canRollForward());
		ui->widget->repaint();
	}

}

void MainWindow::editRedo()
{
	if(commandList.canRollForward())
	{
		selectedRooms.clear();
		commandList.rollForward(this);
		ui->actionUndo->setEnabled(commandList.canRollBack());
		ui->actionRedo->setEnabled(commandList.canRollForward());
		ui->widget->repaint();
	}
}

void MainWindow::editCopy() {}
void MainWindow::editPaste() {}

void MainWindow::editDelete()
{
	if(selectedRooms.size())
	{
		auto command = new AggregateCommand();

		for(auto i = selectedRooms.begin(); i != selectedRooms.end(); ++i)
		{
			command->emplace_back(new RemoveRoomCommand(i->first, *(i->second)));
		}

		commandList.push(command, this);

		ui->actionUndo->setEnabled(commandList.canRollBack());
		ui->actionRedo->setEnabled(commandList.canRollForward());
	}
}

//------------------------------
// responses
//-----------------------------

void MainWindow::onMousePress(QPoint pos, QSize size)
{
	roomMesh.onMousePress(roomOutline, getMousePosition(pos, size), dimensions);
}


QPoint MainWindow::getMousePosition(QPoint pos, QSize size)
{
	pos /= zoom;
	size = dimensions - size / zoom;

	pos += QPoint((ui->horizontalScrollBar->value() * size.width()) >> 8,
				  (ui->verticalScrollBar->value()   * size.height()) >> 8);

	pos.setX(std::max(0, std::min(pos.x(), dimensions.width())));
	pos.setY(std::max(0, std::min(pos.y(), dimensions.height())));

	return pos;
}

QPoint MainWindow::getMousePosition()
{
	return getMousePosition(mapFromGlobal(QCursor::pos()), ui->widget->size());
}

bool MainWindow::cropRoom(Room & room, int left, int right, int top_left, int top_right, int bottom_left, int bottom_right)
{
	std::cerr << "\nbefore adjustment:" << std::endl;
	std::cerr << "left         = " << left << std::endl;
	std::cerr << "right        = " << right << std::endl;
	std::cerr << "top_left     = " << top_left << std::endl;
	std::cerr << "top_right    = " << top_right << std::endl;
	std::cerr << "bottom_left  = " << bottom_left << std::endl;
	std::cerr << "bottom_right = " << bottom_right << std::endl;

	double m, b;

	m = (top_right - top_left) / (double) (right - left);
	b = top_left - left*m;

	std::cerr << "\nm = " << m << std::endl;
	std::cerr << "b = " << b << std::endl;

	top_left  = std::max(left, 0)*m + b;
	top_right = std::min(right, 255)*m + b;

	m = (bottom_right - bottom_left) / (double) (right - left);
	b = bottom_left - left*m;

	std::cerr << "\nm = " << m << std::endl;
	std::cerr << "b = " << b << std::endl;

	bottom_left  = std::max(left, 0)*m + b;
	bottom_right = std::min(right, 255)*m + b;

	left = std::max(left, 0);
	right = std::min(right, 255);

	if(top_left <= 0 && top_right <= 0)
	{
		top_left = 0;
		top_right = 0;
	}

	if(bottom_left >= 255 && bottom_right >= 255)
	{
		bottom_left = 0;
		bottom_right = 0;
	}

	std::cerr << "\nafter adjustment:" << std::endl;
	std::cerr << "left         = " << left << std::endl;
	std::cerr << "right        = " << right << std::endl;
	std::cerr << "top_left     = " << top_left << std::endl;
	std::cerr << "top_right    = " << top_right << std::endl;
	std::cerr << "bottom_left  = " << bottom_left << std::endl;
	std::cerr << "bottom_right = " << bottom_right << std::endl;

	room = Room(left, right, top_left, top_right, bottom_left, bottom_right, 0);
	return true;
}

void MainWindow::onMouseRelease(QPoint pos, QSize size)
{
	mouse_down = false;

	size /= zoom;
	pos  /= zoom;

	QSize s0 = dimensions - size;

	pos += QPoint(ui->horizontalScrollBar->value() * s0.width() / 255,
				  ui->verticalScrollBar->value() * s0.height() / 255);

	if(dimensions.width() <= 0 || dimensions.width() <= 0)
	{
		return;
	}

	if(ui->actionSelect->isChecked())
	{
		int x = pos.x() >> 8;
		int y = pos.y() >> 8;

		int i = x*tiles().height() + y;

		if(i > (int) rooms.size())
		{
			return;
		}

		if(!(QApplication::keyboardModifiers() & (Qt::ShiftModifier | Qt::ControlModifier)))
		{
			selectedRooms.clear();
		}


		x = pos.x() & 0xFF;
		y = pos.y() & 0xFF;

		for(auto j = rooms[i].begin(); j != rooms[i].end(); ++j)
		{
			if(j->left >=  x || j->right <= x)
			{
				continue;
			}

			double m, b;

			m = (top_right - top_left) / (double) (right - left);
			b = top_left - left*m;

			if(x*m + b < y)
			{
				continue;
			}

			m = (bottom_right - bottom_left) / (double) (right - left);
			b = bottom_left - left*m;

			if(x*m + b > y)
			{
				continue;
			}

			for(auto k = selectedRooms.begin(); k != selectedRooms.end(); ++k)
			{
				if(k->first == i && k->second == &(*j))
				{
					selectedRooms.erase(k);
					goto end_select;
				}
			}

			selectedRooms.push_back(std::make_pair(i, &(*j)));
		end_select:
			(void)0;
		}

		ui->widget->repaint();
	}
	else if(ui->actionAdd->isChecked())
	{
		if(++state <= 3)
		{
			return;
		}
		state = 0;

		if(left > right)
		{
			std::swap(left, right);
			std::swap(top_left, top_right);
			std::swap(bottom_left, bottom_right);
		}

		if(right - left < 4)
		{
			return;
		}

		if(top_left > bottom_left)
		{
			std::swap(top_left, bottom_left);
		}
		if(top_right > bottom_right)
		{
			std::swap(top_right, bottom_right);
		}


		int min_x = left >> 8;
		int max_x = right >> 8;
		int min_y = std::min(top_left, top_right) >> 8;
		int max_y = std::max(bottom_left, bottom_right) >> 8;

		auto command = new AggregateCommand();
		for(int x = min_x; x <= max_x; ++x)
		{
			for(int y = min_y; y <= max_y; ++y)
			{
				Room room;
				int i = x*tiles().height() + y;
				if(cropRoom(room, left - (x << 8), right - (x << 8), top_left - (y << 8), top_right - (y << 8),  bottom_left - (y << 8),  bottom_right - (y << 8)))
				{
					command->emplace_back(new AddRoomCommand(i, room));
				}
			}
		}

		if(command->size())
		{
			commandList.push(command, this);
			ui->actionUndo->setEnabled(commandList.canRollBack());
			ui->actionRedo->setEnabled(commandList.canRollForward());
		}
		else
		{
			delete command;
		}

		ui->widget->repaint();
	}
}

void MainWindow::onDoubleClick(QPoint pos, QSize size)
{
	ui->actionAdd->setChecked(false);
	ui->actionSelect->setChecked(true);

	onMousePress(pos, size);
	onMouseRelease(pos, size);
}

void MainWindow::onMouseMoveEvent(QPoint pos, QSize size)
{
	if(roomMesh.state == 'n')
	{
		roomMesh.defineQuad(yLines, getMousePosition(pos, size), dimensions);
		ui->widget->repaint();

	}
#if 0
	if(state == 'g')
	{
		pos = getMousePosition(pos, size);
		roomMesh.moveVerticies(roomOutline, pos/zoom, size/zoom, (pos-last_pos)/zoom);
		ui->widget->repaint();
	}


	size /= zoom;
	pos  /= zoom;

	QSize s0 = dimensions - size;

	pos += QPoint(ui->horizontalScrollBar->value() * s0.width() / 255,
				  ui->verticalScrollBar->value() * s0.height() / 255);

	pos.setX(std::max(0, std::min(pos.x(), dimensions.width())));
	pos.setY(std::max(0, std::min(pos.y(), dimensions.height())));

	statusBar()->showMessage(QObject::tr("%1 : %2").arg(pos.x()).arg(pos.y()));

	if(ui->actionAdd->isChecked())
	{
		switch(state)
		{
		case 0:
			left = pos.x();
			top_left = pos.y();
			break;
		case 1:
			bottom_left = pos.y();
			break;
		case 2:
			right = pos.x();
			bottom_right = pos.y();
			break;
		case 3:
			top_right = pos.y();
			break;
		default:
			break;
		}

		if(state > 0)
		{
			ui->widget->repaint();
		}
	}
	else if(ui->actionSelect->isChecked() && mouse_down)
	{
		ui->widget->repaint();
	}
#endif
}

QString MainWindow::getToolTip(QPoint pos, QSize size)
{
	return QString();
}

void MainWindow::removeHidden()
{
	if(background[2][3].isNull())
	{
		return;
	}

	uint32_t value = 0;

	for(int map = 0; map < 2; ++map)
	{
		for(int channel = 0; channel < 4; ++channel)
		{
			if(background[map][channel].isNull())
			{
				continue;
			}

			value += dimensions.height();
		}
	}

	QProgressDialog progress(tr("Removing Hidden Pixels..."), "Cancel", 0, value, this);
	value = 0;

	for(int map = 0; map < 2; ++map)
	{
		for(int channel = 0; channel < 4; ++channel)
		{
			if(background[map][channel].isNull())
			{
				continue;
			}

			for(int y = 0; y < dimensions.height(); ++y)
			{
				for(int x = 0; x < dimensions.width(); ++x)
				{
					if(isTransparent(map, channel, x, y))
					{
						background[map][channel].setPixel(x, y, 0);
					}
				}

				progress.setValue(++value);
				QApplication::processEvents();

				if(progress.wasCanceled())
				{
					return;
				}
			}

		}
	}

}

void MainWindow::drawRooms(const std::vector< std::list<Room> > & rooms, QPainter & painter, QPoint offset, QSize size, Qt::PenStyle line_type)
{
	for(uint16_t i = 0; i < rooms.size(); ++i)
	{
		int x = i / tiles().height() * 256;
		int y = i % tiles().height() * 256;

		if(x + 256 < offset.x() || y + 256 < offset.y()
		|| x > offset.x() + size.width() || y > offset.y() + size.height())
		{
			continue;
		}

		QPoint points[4];
#if 1
		QPoint pos(x, y);
		pos -= offset;
#else
		QPoint pos(-offset);

#endif
		for(auto j = rooms[i].begin(); j != rooms[i].end(); ++j)
		{
			if(j->room_type)
			{
				painter.setPen(QPen(QBrush(palette[j->room_type-1]), 1, line_type));
			}
			else
			{
				painter.setPen(QPen(Qt::cyan, 2, line_type));
			}

			points[0] = pos + QPoint(j->left, j->top_left);
			points[1] = pos + QPoint(j->left, j->bottom_left);
			points[2] = pos + QPoint(j->right, j->bottom_right);
			points[3] = pos + QPoint(j->right, j->top_right);
			painter.drawPolygon(points, 4);
		}
	}
}

void MainWindow::drawRooms(const std::list<Room> & rooms, QPainter & painter, QPoint offset, QSize size, Qt::PenStyle line_type)
{
	QPoint pos(-offset);
	QPoint points[4];

	for(auto j = rooms.begin(); j != rooms.end(); ++j)
	{
		if(j->room_type)
		{
			painter.setPen(QPen(QBrush(palette[j->room_type-1]), 1, line_type));
		}
		else
		{
			painter.setPen(QPen(Qt::cyan, 2, line_type));
		}

		points[0] = pos + QPoint(j->left, j->top_left);
		points[1] = pos + QPoint(j->left, j->bottom_left);
		points[2] = pos + QPoint(j->right, j->bottom_right);
		points[3] = pos + QPoint(j->right, j->top_right);
		painter.drawPolygon(points, 4);
	}
}

void MainWindow::draw(QPainter & painter, QPoint pos, QSize size)
{
	if(dimensions.width() < 0)
	{
		return;
	}

	painter.scale(zoom, zoom);
	pos  /= zoom;
	size /= zoom;

	QSize s0 = dimensions - size;

	QPoint offset(ui->horizontalScrollBar->value() * s0.width() / 255,
				  ui->verticalScrollBar->value() * s0.height() / 255);

	pos += offset;

	pos.setX(std::max(0, std::min(pos.x(), dimensions.width())));
	pos.setY(std::max(0, std::min(pos.y(), dimensions.height())));

	pos -= offset;

// QPoint prev_pos = (last_pos - offset)/zoom;

	for(int i = 0; i < 3; ++i)
	{
		if(!showBackground(i))
		{
			continue;
		}

		if(background[i][showMapping()].isNull())
		{
			continue;
		}

		painter.drawImage(0, 0, background[i][showMapping()], offset.x(), offset.y(), size.width(), size.height());
	}

	roomOutline.draw(painter, offset, pos, size);
	roomMesh.draw(painter, offset, 1.0, pos, size);

	painter.setPen(QPen(Qt::cyan, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));

	drawRooms(rooms  , painter, offset, size, Qt::SolidLine);
	drawRooms(fluids , painter, offset, size, Qt::DashLine);
	drawRooms(CaField, painter, offset, size, Qt::DashDotLine);

/*
	if(state != 0 && ui->actionAdd->isChecked())
	{
		QPoint points[4];
		switch(state)
		{
		default:
			break;
		case 1:
			points[0] = QPoint(left, top_left) - offset;
			points[1] = QPoint(left, bottom_left) - offset;
			painter.drawPolygon(points, 2);
			break;
		case 2:
			points[0] = QPoint(left, top_left) - offset;
			points[1] = QPoint(left, bottom_left) - offset;
			points[2] = QPoint(right, bottom_right) - offset;
			painter.drawPolygon(points, 3);
			break;
		case 3:
			QPoint points[4];
			points[0] = QPoint(left,  std::min(top_left, bottom_left)) - offset;
			points[1] = QPoint(left,  std::max(top_left, bottom_left)) - offset;
			points[2] = QPoint(right, std::max(top_right, bottom_right)) - offset;
			points[3] = QPoint(right, std::min(top_right, bottom_right)) - offset;
			painter.drawPolygon(points, 4);
			break;
		}
	}
	else if(ui->actionSelect->isChecked() && mouse_down)
	{
		painter.setPen(QPen(Qt::white, 1, Qt::DashLine, Qt::FlatCap, Qt::MiterJoin));

		QPoint points[4];
		points[0] = prev_pos;
		points[1] = QPoint(prev_pos.x(), pos.y());
		points[2] = pos;
		points[3] = QPoint(pos.x(), prev_pos.y());
		painter.drawPolygon(points, 4);
	}
	//*/
}

QMenu * MainWindow::showContextMenu(ViewWidget * widget, QPoint pos, QSize size)
{
	return 0L;
}

//------------------------------
// accessors
//-----------------------------

bool MainWindow::addMode()
{
	return ui->actionAdd->isChecked();
}

bool MainWindow::selectMode()
{
	return ui->actionSelect->isChecked();
}

bool MainWindow::showBackground(int i)
{
	switch(i)
	{
	case 0: return ui->actionShow_Background->isChecked();
	case 1: return ui->actionShow_Foreground->isChecked();
	case 2: return ui->actionShow_Cutouts->isChecked();
	}
	return false;
}

int MainWindow::showMapping()
{
	if(ui->actionShow_Baked_Map->isChecked())
	{
		return 0;
	}
	else if(ui->actionShow_Albedo_Map->isChecked())
	{
		return 1;
	}
	else if(ui->actionShow_Normal_Map->isChecked())
	{
		return 2;
	}
	else if(ui->actionShow_Microsurface_Map->isChecked())
	{
		return 3;
	}

	return 4;
}


bool MainWindow::showRooms()
{
	return ui->actionRooms->isChecked();
}

bool MainWindow::event(QEvent * event)
{
	switch(event->type())
	{
	case QEvent::KeyPress:
	case QEvent::KeyRelease:
	{
		QKeyEvent * key = static_cast<QKeyEvent *>(event);

		switch(key->key())
		{
		case Qt::Key_Up:
		case Qt::Key_Down:
			return super::event(event) && ui->verticalScrollBar->event(event);
		case Qt::Key_Left:
		case Qt::Key_Right:
			return super::event(event) && ui->horizontalScrollBar->event(event);
		default:
			return super::event(event);
		}
	}
	case QEvent::Wheel:
	{
		QWheelEvent * wheel = static_cast<QWheelEvent *>(event);

		if(wheel->modifiers() & Qt::ControlModifier)
		{
			if(wheel->orientation() == Qt::Vertical)
			{
				double angle = wheel->angleDelta().y();
				double factor = pow(1.0015, angle);
				zoom *= factor;
				ui->widget->repaint();
			}
		}
		else if(wheel->buttons() != Qt::MidButton)
		{
			if(wheel->orientation() == Qt::Horizontal)
			{
				return super::event(event) && ui->horizontalScrollBar->event(event);
			}
			else
			{
				return super::event(event) && ui->verticalScrollBar->event(event);
			}
		}
	}
	default:
		return super::event(event);
	}
}

