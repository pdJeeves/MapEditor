#ifndef VERTICIESFROMIMAGE_H
#define VERTICIESFROMIMAGE_H
#include <QImage>
#include <QPoint>
#include <QSize>
#include <QVector>

class QMainWindow;

struct VertexList
{
	VertexList(QRgb color, QVector<QPoint> && points);

	QRgb color;
	QVector<QPoint> points;
	QVector<QPoint> corners;
};

class VerticiesFromImage
{
	static QRgb getColor(QRgb a);
	static QPoint toPoint(int p);
	static bool isContained(const QImage & image, QPoint p);
	static QRgb getColor(const QImage & image, QPoint offset);
	static QVector<QPoint> getVerticies(const QImage &image, std::vector<uint8_t> & mark, QPoint p);
	static QVector<QPoint> getVertexRun(const QImage &image, std::vector<uint8_t> & mark, QPoint p);
	static bool getMark(const std::vector<uint8_t> & vec, int i);
	static void setMark(std::vector<uint8_t> &vec, int i);
	static QPointF getNormal(QPoint a);

public:
	VerticiesFromImage();
	VerticiesFromImage(QMainWindow *window, QImage & image);
	std::vector<VertexList> lists;

	void draw(QPainter & painter, QPoint offset, QPoint pos, QSize size);
	static void prepImage(QImage & image);

};

#endif // VERTICIESFROMIMAGE_H
