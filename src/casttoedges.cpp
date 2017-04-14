#include "casttoedges.h"
#include "verticiesfromimage.h"
#include <QImage>


float CastToEdges::doCast(const QVector<QPoint> & verts, QPoint r_p, QPointF r_d, float length)
{
	bool invert = (r_d.x() == 0);

	if(invert)
	{
		r_p = QPoint(r_p.y(), r_p.x());
		r_d = QPoint(r_d.y(), r_d.x());

		if(r_d.x() == 0)
		{
			return length;
		}
	}

	for(int i = 0; i < verts.size()-1; ++i)
	{
		auto s_p = verts[i];
		auto s_d = verts[i+1] - s_p;

		if(invert)
		{
			s_p = QPoint(s_p.y(), s_p.x());
			r_d = QPoint(s_d.y(), s_d.x());
		}

		float T2 = (r_d.x() * (s_p.y()-r_p.y()) + r_d.y()*(r_p.x()-s_p.x())) / (s_d.x() * r_d.y() - s_d.y()*r_d.x());

		if(0 < T2 && T2 <= 1)
		{
			float T1 = (s_p.x()+s_d.x()*T2-r_p.x())/r_d.x();

			if(T1 >= 0 && T1 < length)
			{
				length = T1;
			}
		}
	}

	return length;
}

float CastToEdges::doCast(const VerticiesFromImage & edges, QPoint r_p, QPointF r_d)
{
	float a = 65535;

	for(auto i = edges.lists.begin(); i != edges.lists.end(); ++i)
	{
		if(doCast(i->corners, r_p, r_d, a) < a)
		{
			a = doCast(i->points, r_p, r_d, a);
		}

	}

	return a;
}

float CastToEdges::doCast(const QImage & image, QPoint r_p, QPointF r_d)
{
	QPointF t(r_p);

	while( 0 <= t.x() && t.x() < image.width()
		&& 0 <= t.y() && t.y() < image.height())
	{
		if(qAlpha(image.pixel((int) (t.x() + .5), (int) (t.y() + .5))) > 128)
		{
			break;
		}

		t += r_d;
	}

	t -= r_p;

	return sqrt(t.x()*t.x() + t.y()*t.y());
}

float CastToEdges::doCast(const LengthEncoded_t & list, QPoint r_p, bool end)
{
	if(list.size() <= r_p.x())
	{
		return 0;
	}

	for(auto i = list[r_p.x()].begin(); i != list[r_p.x()].end(); ++i)
	{
		if(r_p.y() <= i->second)
		{
			if(i->first != 0)
			{
				break;
			}

			return end? -r_p.y() : i->second - r_p.y();
		}
		else
		{
			r_p.setY(r_p.y() - i->second);
		}
	}

	return 0;
}

std::pair<int, int> CastToEdges::getHeight(const LengthEncoded_t & list, QPoint r_p)
{
	if(list.size() <= r_p.x())
	{
		return std::make_pair(0, 0);
	}

	int y = 0;

	for(auto i = list[r_p.x()].begin(); i != list[r_p.x()].end(); ++i)
	{
		if(r_p.y() <= i->second)
		{
			if(i->first != 0)
			{
				break;
			}

			return std::make_pair(y, y+i->second);
		}
		else
		{
			r_p.setY(r_p.y() - i->second);
			y += i->second;
		}
	}

	return std::make_pair(0, 0);
}
