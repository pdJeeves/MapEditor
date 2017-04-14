#ifndef PALETTE16_H
#define PALETTE16_H
#include <QColor>
#include <QVector>

struct Palette16
{
	static const QVector<QRgb> & get();
};

#endif // PALETTE16_H
