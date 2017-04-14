#ifndef DIFFUSIONMAP_H
#define DIFFUSIONMAP_H
#include <array>
#include <QPoint>
#include <QSize>
#include <QImage>

class QProgressDialog;

class DiffusionMapNode
{
	void subdivide(QProgressDialog &, QImage & image);

	DiffusionMapNode(QProgressDialog & progress, QImage &, QPoint pos, QSize size);
	~DiffusionMapNode();


	QPoint position;
	QSize  size;
	QPoint topLeft;
	QPoint topRight;
	QPoint bottomLeft;
	QPoint bottomRight;

	std::array<DiffusionMapNode *, 4> children;
};

class DiffusionMap
{

public:
	DiffusionMap();
};

#endif // DIFFUSIONMAP_H
