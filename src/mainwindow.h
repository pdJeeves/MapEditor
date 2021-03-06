#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QTimer>
#include <QActionGroup>
#include <QMainWindow>
#include <QShortcut>
#include <QImage>
#include "room.h"
#include "commandlist.h"
#include "casttoedges.h"
#include "verticiesfromimage.h"
#include "roommesh.h"

namespace Ui {
class MainWindow;
}

class ViewWidget;
class QProgressDialog;

class MainWindow : public QMainWindow
{
typedef QMainWindow super;
	Q_OBJECT

	static void saveRooms(FILE * file, std::list<Room> & rooms);
	void mergeRoomMaps();
	void drawRooms(const std::list<Room> & rooms, QPainter & painter, QPoint offset, QSize size, Qt::PenStyle line_type);
	void drawRooms(const std::vector< std::list<Room> > & rooms, QPainter & painter, QPoint offset, QSize size, Qt::PenStyle line_type);

	bool cropRoom(Room &room, int left, int right, int top_left, int top_right, int bottom_left, int bottom_right);
	uint32_t writeTile(FILE * file, int x0, int y0, int map, int channel) const;
	uint32_t runLength(uint32_t i, int x0, int y0, int map, int channel, bool transparent) const;
	void writeColor(FILE * file, QRgb color, int channel) const;
	bool isTransparent(int map, int channel, int x, int y) const;
	bool isVicintitySolid(int map, int channel, int x0, int y0) const;

	bool loadImage(QImage & image, bool room_map = false);
	bool dimensionCheck(QSize dimension1, bool room_map = false);

	QPoint last_pos;
	int state, left, right, top_left, top_right, bottom_left, bottom_right;
	bool mouse_down;
	QProgressDialog * progress;
	QImage * room_map;

	static QVector<QRgb> palette332;
	static QRgb palette[16];

	QImage roomMap;
	QImage summedRoomMap;
	LengthEncoded_t xLines;
	LengthEncoded_t yLines;
	VerticiesFromImage roomOutline;
	RoomMesh roomMesh;

	QShortcut newEdge;
	QShortcut extrudeEdge;
	QShortcut grabMode;
	QShortcut escape;

	QPoint getMousePosition(QPoint pos, QSize size);
	QPoint getMousePosition();

public:
	std::list<std::pair<int, Room *> > selectedRooms;
	CommandList commandList;

	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	QSize dimensions;
	inline QSize tiles() const { return QSize((dimensions.width() + 255) / 256, (dimensions.height() + 255) / 256); }
	inline int totalTiles() const { return tiles().width() * tiles().height(); }
	QImage background[3][5];
	std::vector<std::vector<uint8_t> >	compressedBackground[3][5];

	QString filename;
	QString filepath;

	float zoom;
	QTimer autosaveTimer;
	std::vector< std::list<Room> > rooms;
	std::vector< std::list<Room> > fluids;
	std::list<Room> CaField;

	void onMousePress(QPoint pos, QSize size);
	void onMouseRelease(QPoint pos, QSize size);
	void onDoubleClick(QPoint pos, QSize size);
	void onMouseMoveEvent(QPoint pos, QSize size);
	QString getToolTip(QPoint pos, QSize size);

	void replaceImage(QImage & image);
	void draw(QPainter & painter, QPoint pos, QSize size);

	QMenu * showContextMenu(ViewWidget * widget, QPoint pos, QSize size);

	bool event(QEvent * event) override;

private:
	bool openKreaturesFile(std::string name, int read_length);

	void documentNew();
	void documentOpen();
	void documentClose();
	void documentSave();
	void documentSaveAs();

	void openParallaxLayer();
	void saveParallaxLayer();

	void documentImportSpr();
	void documentImportS16();
	void documentImportBlk();

	void applicationExit();

	void editUndo();
	void editRedo();
	void editCopy();
	void editPaste();
	void editDelete();

	void actionImportRooms(bool type);
	void removeThinRooms();
	void meld();
	void removeHidden();

//accessors
	bool selectMode();
	bool addMode();

	bool showBackground(int i);
	int showMapping();

	bool showRooms();

private:
	QActionGroup toolGroup;
	QActionGroup viewGroup;
	Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
