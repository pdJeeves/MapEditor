#include "roommesh.h"
#include "palette16.h"
#include "casttoedges.h"
#include "verticiesfromimage.h"
#include <QPainter>

void RoomMesh::Quad::validate()
{
	if(verticies[rv_topLeft].x() > verticies[rv_topRight].x())
	{
		std::swap(verticies[rv_topLeft   ], verticies[rv_topRight   ]);
		std::swap(verticies[rv_bottomLeft], verticies[rv_bottomRight]);
	}

	min = QPoint(verticies[rv_topLeft].x(), std::min(verticies[rv_topLeft].y(), verticies[rv_topRight].y()));
	max = QPoint(verticies[rv_topRight].x(), std::max(verticies[rv_bottomLeft].y(), verticies[rv_bottomRight].y()));
}

RoomMesh::RoomMesh()
{
	state = 0;
	mode = 0;
}

void RoomMesh::clearSelection()
{
	selectedVerts.clear();
}

void RoomMesh::addQuad(QPoint pos)
{
	if(state != 0)
	{
		return;
	}

	state = 'n';
	mode = 0;

	workingQuads = quads;
	workingQuads.push_back(Quad());
}

void RoomMesh::escape()
{
	state = 0;
	mode = 0;
	workingQuads.clear();
}

void RoomMesh::onMousePress(const VerticiesFromImage &, QPoint pos, QSize)
{
	if(state == 'n')
	{
		if(mode == 0)
		{
			mode = 1;
		}
		else if(mode == 1)
		{
			if(workingQuads.back().verticies[rv_topLeft].x() - workingQuads.back().verticies[rv_topRight].x() != 0)
			{
				quads = std::move(workingQuads);
				quads.back().validate();
			}
			escape();
		}
	}
}

RoomMesh::Quad * RoomMesh::getContainingQuad(QPoint p)
{
	for(auto i = quads.begin(); i != quads.end(); ++i)
	{
		if(i->min.y() <= p.y() && p.y() <= i->max.y()
		&& i->min.x() <= p.x() && p.x() <= i->max.x())
		{
			float t = (p.x() - i->min.x()) * 1.0 / (i->max.x() - i->min.x());

			float y = t * (i->verticies[rv_topRight].y() - i->verticies[rv_topLeft].y()) + i->verticies[rv_topLeft].y();

			if(y > p.y())
			{
				continue;
			}

			y = t * (i->verticies[rv_bottomRight].y() - i->verticies[rv_bottomLeft].y()) + i->verticies[rv_bottomLeft].y();

			if(y > p.y())
			{
				return &(*i);
			}
		}
	}

	return 0L;
}

#if 0

QPoint RoomMesh::snapToQuad(QPoint p)
{
	QPoint p1 = p;
	QPoint p2 = p;
	int d1 = 0;
	int d2 = 0;

	Quad * q1 = getContainingQuad(pos);
	Quad * q2 = q1;

	if(!q1 && !q2)
	{
		return p;
	}

	while(q1 && q2)
	{
		if(q1)
		{
			d1 += p1.x() - q1->min.x();
			p1.setX(q1->min.x());
		}

		if(q2)
		{
			d2 += q2->max.x() - p2.x();
			p2.setX(q2->max.x());
		}

		q1 = getContainingQuad(p1+QPoint(-1, 0));
		q2 = getContainingQuad(p2+QPoint( 1, 0));
	}

	if(q1 && q2)

	return 0L;
}

#endif


void RoomMesh::snapToRoom(int x0, int & x1, int & top, int & bottom)
{

}

void RoomMesh::defineQuad(const LengthEncoded_t & edges, QPoint pos, QSize size)
{
	QPoint delta = pos - lastPos;

	if(mode == 0)
	{
		Quad * q = getContainingQuad(pos);

		if(q)
		{
			if(std::abs(pos.x() - q->verticies[rv_topLeft].x()) < std::abs(pos.x() - q->verticies[rv_topRight].x()))
			{
				pos.setX(q->min.x());
			}
			else
			{
				pos.setX(q->max.x());
			}
		}
	}

	std::pair<int, int> height = CastToEdges::getHeight(edges, pos);

	if(mode == 1)
	{
		int x = pos.x();
		snapToRoom(workingQuads.back().verticies[rv_topLeft].x(), x, height.first, height.second);
		pos.setX(x);
	}

	workingQuads.back().verticies[rv_topRight]	  = QPoint(pos.x(), height.first);
	workingQuads.back().verticies[rv_bottomRight] = QPoint(pos.x(), height.second);

	if(mode == 0)
	{
		workingQuads.back().verticies[rv_topLeft]     = QPoint(pos.x(), height.first);
		workingQuads.back().verticies[rv_bottomLeft]  = QPoint(pos.x(), height.second);
	}
}


void RoomMesh::draw(QPainter & painter, QPoint offset, float scale, QPoint, QSize)
{
	scale = 1/scale;

	std::vector<Quad> & q = workingQuads.size()? workingQuads : quads;

	painter.setPen(QPen(QBrush(Qt::cyan), 2, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));

	for(size_t i = 0; i < q.size(); ++i)
	{
		QPoint verticies[4];

		for(int j = 0; j < 4; ++j)
		{
			verticies[j] = (q[i].verticies[j] - offset)*scale;
		}

		painter.drawPolygon(verticies, 4);
	}
}
