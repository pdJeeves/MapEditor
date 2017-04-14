#include "palette16.h"

const QVector<QRgb> & Palette16::get()
{
	static const QRgb palette[] = {
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

	static QVector<QRgb> vec;

	if(!vec.size())
	{
		for(size_t i = 0; i < sizeof(palette)/sizeof(QRgb); ++i)
		{
			vec.push_back(palette[i]);
		}
	}

	return vec;
}
