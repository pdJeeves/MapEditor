#ifndef ROOMMESH_H
#define ROOMMESH_H
#include "casttoedges.h"
#include <QPoint>
#include <QSize>
#include <vector>
#include <array>
#include <list>

class QPainter;
class VerticiesFromImage;
class QImage;

typedef enum {
	rv_topLeft,
	rv_topRight,
	rv_bottomRight,
	rv_bottomLeft,
} RoomVerts;

class RoomMesh
{
	struct Quad
	{
		std::array<QPoint, 4> verticies;
		std::vector<int> adjacentQuads;
		QPoint min, max;

		void validate();
	};

	std::vector<Quad> workingQuads;
	std::vector<Quad> quads;

	std::list<QPoint*> selectedVerts;

	QPoint lastPos;

	void snapToRoom(int x0, int & x1, int & top, int & bottom);
	Quad * getContainingQuad(QPoint p);

public:
	int state;
	int mode;

	RoomMesh();

	void draw(QPainter & painter, QPoint offset, float scale, QPoint pos, QSize size);

	void addQuad(QPoint pos);
	QPoint snapToQuad(QPoint p);


	void defineQuad(const LengthEncoded_t & edges, QPoint pos, QSize size);
	void clearSelection();

	void grab(QPoint);
	void escape();
	void onMousePress(const VerticiesFromImage &, QPoint, QSize);

};

#endif // ROOMMESH_H
