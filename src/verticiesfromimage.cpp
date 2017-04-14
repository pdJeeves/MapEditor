#include "verticiesfromimage.h"
#include <QVector>
#include <QPainter>
#include <QProgressDialog>
#include <QApplication>
#include <QMainWindow>
#include <iostream>
#include "palette16.h"

VertexList::VertexList(QRgb color, QVector<QPoint> && verts) :
	color(color),
	points(verts)
{
	QPoint min(points.front());
	QPoint max(points.back());

	for(size_t i = 0; i < points.size(); ++i)
	{
		min.setX(std::min(min.x(), points[i].x()));
		min.setY(std::min(min.y(), points[i].y()));
		max.setX(std::max(max.x(), points[i].x()));
		max.setY(std::max(max.y(), points[i].y()));
	}

	corners.resize(5);

	corners[0] = min;
	corners[1] = QPoint(max.x(), min.y());
	corners[2] = max;
	corners[3] = QPoint(min.x(), max.y());
	corners[4] = min;
}

bool VerticiesFromImage::getMark(const std::vector<uint8_t> & vec, int i)
{
	return vec[i>>3] & (1 << (i&0x07));
}

void VerticiesFromImage::setMark(std::vector<uint8_t> & vec, int i)
{
	vec[i>>3] |= (1 << (i&0x07));
}

VerticiesFromImage::VerticiesFromImage()
{
}

void VerticiesFromImage::prepImage(QImage & image)
{
	for(int y = 0; y < image.height(); ++y)
	{
		for(int x = 0; x < image.width(); ++x)
		{
			if(qAlpha(image.pixel(x,y)) < 128)
			{
				image.setPixel(x,y, 0);
			}
		}
	}

	image = std::move(image.convertToFormat(QImage::Format_Indexed8, Palette16::get(), Qt::ThresholdDither));
}

VerticiesFromImage::VerticiesFromImage(QMainWindow * window, QImage & image)
{
	QProgressDialog progress(window->tr("Finding Verticies..."), "Cancel", 0, image.height(), window);

	prepImage(image);

	std::vector<uint8_t> mark((image.width() * image.height() + 7) >> 3, 0);

	QRgb line_begin  = 0;

//hollow out image
	for(int y = 0; y < image.height() && !progress.wasCanceled(); ++y)
	{
		int p0 = line_begin;
		int p1 = getColor(image, QPoint(0, y));
		int p2;

		line_begin = image.pixelIndex(0, y);

		for(int x = 1; x < image.width(); ++x, p0=p1,p1=p2)
		{
			p2 = getColor(image, QPoint(x, y));

			if(p0 == p2 || p1 == 0)
			{
				continue;
			}

			if(!getMark(mark, y*image.width() + x-1))
			{
				auto a = getVerticies(image, mark, QPoint(x-1, y));

				if(a.size() > 1)
				{
					VertexList run(image.pixel(x, y), std::move(a));
					lists.push_back(run);
				}
			}

		}

		progress.setValue(y);
		qApp->processEvents();
	}
}

QPointF VerticiesFromImage::getNormal(QPoint a)
{
	float n = sqrt(a.x()*a.x() + a.y()*a.y());
	return QPointF(a.x()/n, a.y()/n);
}

QVector<QPoint> VerticiesFromImage::getVerticies(const QImage & image, std::vector<uint8_t> & mark, QPoint p)
{
	auto a = getVertexRun(image, mark, p);

	{
		auto b = getVertexRun(image, mark, p);

		if(b.size() > 1)
		{
			auto N = b.size()-1;
			for(size_t j = 0; j < b.size()/2;  ++j)
			{
				std::swap(b[j], b[N-j]);
			}

			b.pop_front();
			a.append(b);
		}
	}



	return a;
}

QVector<QPoint> VerticiesFromImage::getVertexRun(const QImage & image, std::vector<uint8_t> & mark, QPoint p)
{
	QVector<QPoint> r;
	auto color = image.pixelIndex(p);

	r.push_back(p);
	QPoint delta;

	for(;;)
	{
		setMark(mark, p.y()*image.width() + p.x());

		QRgb p0 = getColor(image, p + toPoint(6));
		QRgb p1 = getColor(image, p + toPoint(7));
		QRgb p2;

		for(int i = 0; i < 8; ++i, p0=p1,p1=p2)
		{
			p2 = getColor(image, p + toPoint(i));

			if(p0 == p2 || p1 != color)
			{
				continue;
			}

			auto point = p + toPoint(i+7);

			if(!getMark(mark,  point.y()*image.width() + point.x())
			&& isContained(image, point))
			{
				p = point;
				goto continue_loop;
			}
		}

		break;

continue_loop:
		if(r.size() > 2)
		{
			if((p - r.back()) == delta)
			{
				r.back() = p;
				continue;
			}
		}

		if(r.size() > 1)
		{
			delta = p - r.back();
		}

		r.push_back(p);
	}

	if((r.front() - r.back()).manhattanLength() < 3)
	{
		r.push_back(r.front());
	}

	return r;
}

QRgb VerticiesFromImage::getColor(QRgb a)
{
	return 0x00FFFFFF & a;
}

QPoint VerticiesFromImage::toPoint(int p)
{
	switch(p % 8)
	{
	case 0: return QPoint(-1,-1);
	case 1: return QPoint( 0,-1);
	case 2: return QPoint( 1,-1);
	case 3: return QPoint( 1, 0);
	case 4: return QPoint( 1, 1);
	case 5: return QPoint( 0, 1);
	case 6: return QPoint(-1, 1);
	case 7: return QPoint(-1, 0);
	}

	return QPoint(0, 0);
}

bool VerticiesFromImage::isContained(const QImage & image, QPoint p)
{
	return 0 <= p.x() && p.x() < image.width()
		&& 0 <= p.y() && p.y() < image.height();
}

QRgb VerticiesFromImage::getColor(const QImage & image, QPoint p)
{
	if(isContained(image, p))
	{
		return image.pixelIndex(p);
	}

	return 0;
}

void VerticiesFromImage::draw(QPainter & painter, QPoint offset, QPoint pos, QSize size)
{
	painter.save();
	painter.translate(-offset);

	for(auto i = lists.begin(); i != lists.end(); ++i)
	{
		painter.setPen(QPen(QBrush(i->color), 2, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
		painter.drawPolyline(i->points);
	}

	painter.restore();
}

