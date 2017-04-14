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
#include <squish.h>
#include <QProgressDialog>


bool MainWindow::isVicintitySolid(int map, int channel, int x0, int y0) const
{
	if(background[map][channel].isNull())
	{
		return false;
	}

	int xN = std::min(x0+2, dimensions.width());
	int yN = std::min(y0+2, dimensions.height());
	x0 = std::max(x0 - 2, 0);
	y0 = std::max(y0 - 2, 0);

	for(int y = y0; y < yN; ++y)
	{
		for(int x = x0; x < xN; ++x)
		{
			if(qAlpha(background[map][channel].pixel(x, y)) != 255)
			{
				return false;
			}
		}
	}

	return true;
}

bool MainWindow::isTransparent(int map, int channel, int x, int y) const
{
	if(qAlpha(background[map][channel].pixel(x, y)) == 0)
	{
		return true;
	}

//farthest ahead
	if(map > 1)
	{
		return false;
	}

	if(isVicintitySolid(2, 3, x, y))
	{
		return true;
	}

//2nd farthest ahead, and nothing ahead of us...
	if(map > 0)
	{
		return false;
	}
//todo: finish this
	return false;
}

void MainWindow::writeColor(FILE * file, QRgb color, int channel) const
{
	uint8_t run[4];

	switch(channel)
	{
	case 0:
		run[0] = (qRed(color) & 0xF0) | (qGreen(color) >> 4);
		run[1] = (qBlue(color) & 0xF0) | (qAlpha(color) >> 4);
		fwrite(run, 1, 2, file);
		break;
	case 1:
		run[0] = qGray(color);
		fwrite(run, 1, 1, file);
		break;
	case 2:
		run[0] = (qRed(color) & 0xE0) | ((qGreen(color) & 0xE0) >> 3) | (qBlue(color) >> 6);
		fwrite(run, 1, 2, file);
		break;
	case 3:
		run[0] = qRed(color);
		run[1] = qGreen(color);
		run[2] = qBlue(color);
		run[3] = qAlpha(color);
		fwrite(run, 1, 4, file);
		break;
	default:
		break;
	}
}

uint32_t MainWindow::runLength(uint32_t i, int x0, int y0, int map, int channel, bool transparent) const
{
	for(uint32_t j = i; j < 0x10000; ++j)
	{
		int x = (j & 0xFF) + x0;
		int y = (j >> 8) + y0;

		if(transparent != !qAlpha(background[map][channel].pixel(x, y)))
		{
			return j - i;
		}
	}

	return 0x10000 - i;
}
#if 1
struct BLOCK_128
{
	uint8_t data[16];

	operator bool() const
	{
		return
		(data[0] | (data[1] & ~0x05) | data[2] | data[3] |
		 data[4] | data[5] | data[6] | data[7] |
		 data[8] | data[9] | data[10] | data[11] |
		 data[12] | data[13] | data[14] | data[15]);
	}
};

struct BLOCK_64
{
	uint8_t data[8];

	operator bool() const
	{
#if 1
		static uint8_t cmp[] = { 0, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF };
		auto r = memcmp(data, cmp, 8);
		return r;
#else
		return true;
	#endif
	}
};
#else
struct BLOCK_128
{
	BLOCK_128() :
	a(0), b(0) {}

	uint64_t a;
	uint64_t b;

	operator bool() const
	{
		return (a|b);
	}
};
#endif

uint8_t scanAlpha(const QImage & image, const uint16_t x, const uint16_t y)
{
	uint8_t min = 255;
	uint8_t max = 0;

	if(qAlpha(image.pixel(x, y)) == qAlpha(image.pixel(x+3, y+3)))
	{
		return qAlpha(image.pixel(x, y)) > 16;
	}

	for(uint16_t _y = y; _y < y+4; ++_y)
	{
		for(uint16_t _x = x; _x < x+4; ++_x)
		{
			uint8_t c = qAlpha(image.pixel(_x, _y));

			min = std::min(min, c);
			max = std::max(max, c);
		}
	}

	if(((min >> 4) == 0 || (max >> 4) == 0x0F))
	{
		return 1;
	}

	if(0x01 > (max - min))
	{
		return 5;
	}

	return 3;
}

uint32_t packBytes(uint32_t a, uint32_t b, uint32_t c,uint32_t d)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return (((((d << 8) | c ) << 8) | b) << 8) | a;
#else
	return (((((a << 8) | b ) << 8) | c) << 8) | d;
#endif
}

#define C16_COMPRESSION

void writeDtx1(FILE * file, std::vector<uint32_t> & uncompressed_image, int width, int height)
{
	uint32_t size = GetStorageRequirements(width, height, squish::kDxt1);

	{
		uint8_t type = 1;
		fwrite(&type, 1, 1, file);
		fwrite(&size, 4, 1, file);
	}

	std::vector<BLOCK_64> blocks(size >> 3);
	squish::CompressImage((uint8_t*) uncompressed_image.data(), width, height, (void*) blocks.data(),
		squish::kDxt1 | squish::kColourIterativeClusterFit);
#if !defined(C16_COMPRESSION)
	uint32_t length = 0;
	fwrite(&length, 4, 1, file);
	fwrite(&size, 4, 1, file);
	fwrite(blocks.data(), 8, blocks.size(), file);
#else
	for(size_t i = 0; i < blocks.size(); )
	{
		uint32_t length;
		for(length = 0; i < blocks.size() && !blocks[i]; ++i, ++length) {}

		length <<= 3;
		length = byte_swap(length);
		fwrite(&length, 4, 1, file);

		if(i >= size)
		{
			break;
		}

		for(length = 0; i+length < blocks.size() && blocks[i+length]; ++length)  {}

		{
			uint32_t len = length << 3;
			len = byte_swap(len);
			fwrite(&len, 4, 1, file);
		}

		fwrite(blocks.data() + i, 8, length, file);
		i += length;
	}
#endif
}

uint32_t MainWindow::writeTile(FILE * file, int x0, int y0, int map, int channel) const
{
	uint16_t compression_1 = 0;
	uint16_t compression_3 = 0;
	uint16_t compression_5 = 0;

	for(int y = 0; y < background[map][channel].height(); y += 4)
	{
		for(int x = 0; x < background[map][channel].width(); x += 4)
		{
			switch(scanAlpha(background[map][channel], x, y))
			{
			case 1:
				++compression_1;
				break;
			case 3:
				++compression_3;
				break;
			case 5:
				++compression_5;
				break;
			}
		}
	}

	if(!(compression_1|compression_3|compression_5))
	{
		return 0;
	}

	uint8_t compression_type = 1;

	if(compression_1 < (compression_3+compression_5)/8)
	{
		if(compression_3 > 8 && compression_3 > compression_5)
		{
			compression_type = 3;
		}
		else if(compression_5 > 8 && compression_5 >= sqrt(compression_3)/2)
		{
			compression_type = 5;
		}
	}

	uint32_t r = byte_swap((uint32_t) ftell(file));
	std::vector<uint32_t> uncompressed_image(65536, 0);

	for(uint32_t i = 0; i < 0x10000; ++i)
	{
		int x = (i & 0xFF);
		int y = (i >> 8);

		auto c = background[map][channel].pixel(x + x0, y + y0);
		if(!qAlpha(c))
		{
			continue;
		}

		if(channel >= 2)
		{
			c = (uint32_t) qRgba(qGreen(c),0 ,qRed(c), 0);
		}

		uncompressed_image[i] = (uint32_t) packBytes(qRed(c), qGreen(c), qBlue(c), qAlpha(c));
	}

	if(compression_type == 1)
	{
		writeDtx1(file, uncompressed_image, 256, 256);
		return r;
	}

	fwrite(&compression_type, 1, 1, file);
	switch(compression_type)
	{
	default:
		compression_type = squish::kDxt3;
		break;
	case 5:
		compression_type = squish::kDxt5;
		break;
	}

	uint32_t size = squish::GetStorageRequirements(256, 256, compression_type);
	{
		uint32_t length = byte_swap(size);
		fwrite(&length, 4, 1, file);
	}

	std::vector<BLOCK_128> blocks(size >> 4);

	squish::CompressImage((uint8_t*) uncompressed_image.data(), 256, 256, (void*) blocks.data(), compression_type | squish::kColourIterativeClusterFit | squish::kWeightColourByAlpha);

#if !defined(C16_COMPRESSION)
	{
	uint32_t length = 0;
	fwrite(&length, 4, 1, file);
	length = byte_swap(size);
	fwrite(&length, 4, 1, file);
	fwrite(blocks.data(), 1, size, file);
	}
#else
	for(size_t i = 0; i < blocks.size(); )
	{
		uint32_t length = 0;
		for(length = 0; i < blocks.size() && !blocks[i]; ++i, ++length) {}

		length <<= 4;
		length = byte_swap(length);
		fwrite(&length, 4, 1, file);

		if(i >= 0x10000)
		{
			break;
		}

		for(length = 0; i+length < blocks.size() && blocks[i+length]; ++length)  {}

		{
			uint32_t len = length << 4;
			len = byte_swap(len);
			fwrite(&len, 4, 1, file);
		}

		fwrite(blocks.data() + i, 16, length, file);
		i += length;
	}
#endif

	return r;

}

void MainWindow::saveParallaxLayer()
{
top:
	QString name = QFileDialog::getSaveFileName(this, tr("Export Parallax Layer"), QString(), tr("Kreatures Parallax Layer (*.plx)"));

	if(name.isEmpty())
	{
		return;
	}

	FILE * file = fopen(name.toStdString().c_str(), "wb");
	if(!file)
	{
		QMessageBox mesg;
		mesg.setText(QObject::tr("Unable to open file '%1' for writing.").arg(filename));
		mesg.exec();
		goto top;
		return;
	}

	uint32_t one = byte_swap((uint32_t) 1);
	fwrite(&one, 4, 1, file);
	uint16_t width  = byte_swap((uint16_t) dimensions.width());
	uint16_t height = byte_swap((uint16_t) dimensions.height());
	fwrite(&width, 2, 1, file);
	fwrite(&height, 2, 1, file);

const static int header_offset = 8;
//seek to where we can start writing rooms

	uint32_t image_offsets[5];
	fseek(file, sizeof(image_offsets), SEEK_CUR);
	std::vector<uint32_t> offsets(totalTiles());

	for(auto j = 0; j < 5; ++j)
	{
		if(background[0][j].isNull())
		{
			image_offsets[j] = 0;
			continue;
		}

		int tiles_written = 0;

		image_offsets[j] = byte_swap((uint32_t) ftell(file));

		fpos_t offset_pos;
		fgetpos(file, &offset_pos);
		fseek(file, offsets.size() * 4, SEEK_CUR);

		QProgressDialog progress(tr("Compressing Parallax Layer %1...").arg(j), 0L, 0, totalTiles(), this);

		for(int x = 0; x < tiles().width(); ++x)
		{
			for(int y = 0; y < tiles().height(); ++y)
			{
				int tile = x*tiles().height() + y;
				offsets[tile] = writeTile(file, x << 8, y << 8, 0, j);

				if(offsets[tile])
				{
					++tiles_written;
				}

				progress.setValue(tile);

				if((tile & 0x03) == 0)
				{
					qApp->processEvents();
				}
			}
		}

		fpos_t tile_pos;
		fgetpos(file, &tile_pos);
		fsetpos(file, &offset_pos);

		if(tiles_written == 0)
		{
			image_offsets[j] = 0;
			continue;
		}

		fwrite(offsets.data(), 4, offsets.size(), file);
		fsetpos(file, &tile_pos);
	}

	fseek(file, header_offset, SEEK_SET);
	fwrite(image_offsets, 1, sizeof(image_offsets), file);

	fclose(file);
}

void MainWindow::saveRooms(FILE * file, std::list<Room> & rooms)
{
	uint16_t length = rooms.size();
	fwrite(&length, 2, 1, file);

	for(auto j = rooms.begin(); j != rooms.end(); ++j)
	{
		fwrite(&(j->left)        , sizeof(j->left)        , 1, file);
		fwrite(&(j->right)       , sizeof(j->right)       , 1, file);
		fwrite(&(j->top_left)    , sizeof(j->top_left)    , 1, file);
		fwrite(&(j->top_right)   , sizeof(j->top_right)   , 1, file);
		fwrite(&(j->bottom_left) , sizeof(j->bottom_left) , 1, file);
		fwrite(&(j->bottom_right), sizeof(j->bottom_right), 1, file);
		fwrite(&(j->room_type)   , sizeof(j->room_type)   , 1, file);
	}
}


void MainWindow::documentSave()
{
	if(filepath.isEmpty())
	{
		documentSaveAs();
		return;
	}

	FILE * file = fopen(filepath.toStdString().c_str(), "wb");
	if(!file)
	{
		QMessageBox mesg;
		mesg.setText(QObject::tr("Unable to open file '%1' for writing.").arg(filename));
		mesg.exec();
		filename.clear();
		documentSaveAs();
		return;
	}

	uint32_t one = byte_swap((uint32_t) 1);
	fwrite(&one, 4, 1, file);
	uint16_t width  = byte_swap((uint16_t) dimensions.width());
	uint16_t height = byte_swap((uint16_t) dimensions.height());
	fwrite(&width, 2, 1, file);
	fwrite(&height, 2, 1, file);

const static int header_offset = 8;
//seek to where we can start writing rooms

	std::array<uint32_t, 17> image_offsets;
	image_offsets.fill(0);
	fseek(file, 4*image_offsets.size(), SEEK_CUR);
	std::vector<uint32_t> offsets(totalTiles());

	for(auto i = 0; i < 3; ++i)
	{
		for(auto j = 0; j < 5; ++j)
		{
			if(background[i][j].isNull())
			{
				image_offsets[i*5 + j] = 0;
				continue;
			}

			int tiles_written = 0;

			image_offsets[i*5 + j] = byte_swap((uint32_t) ftell(file));

			fpos_t offset_pos;
			fgetpos(file, &offset_pos);
			fseek(file, offsets.size() * 4, SEEK_CUR);

			QProgressDialog progress(tr("Compressing Background layer %1...").arg(i*5 + j), 0L, 0, totalTiles(), this);

			for(int x = 0; x < tiles().width(); ++x)
			{
				for(int y = 0; y < tiles().height(); ++y)
				{
					int tile = x*tiles().height() + y;

					offsets[tile] = writeTile(file, x << 8, y << 8, i, j);
					if(offsets[tile])
					{
						++tiles_written;
					}

					progress.setValue(tile);

					if((tile & 0x03) == 0)
					{
						qApp->processEvents();
					}
				}
			}

			fpos_t tile_pos;
			fgetpos(file, &tile_pos);
			fsetpos(file, &offset_pos);

			if(tiles_written == 0)
			{
				image_offsets[i*4 + j] = 0;
				continue;
			}

			fwrite(offsets.data(), 4, offsets.size(), file);
			fsetpos(file, &tile_pos);
		}
	}

	image_offsets[15] = byte_swap((uint32_t) ftell(file));

	for(auto i = 0; i < totalTiles(); ++i)
	{
		uint8_t length = rooms[i].size();
		fwrite(&length, 1, 1, file);

		for(auto j = rooms[i].begin(); j != rooms[i].end(); ++j)
		{
			fwrite(&(*j), sizeof(Room), 1, file);
		}
	}

	image_offsets[16] = byte_swap((uint32_t) ftell(file));

	for(auto i = 0; i < totalTiles(); ++i)
	{
		uint8_t length = fluids[i].size();
		fwrite(&length, 1, 1, file);

		for(auto j = fluids[i].begin(); j != fluids[i].end(); ++j)
		{
			fwrite(&(*j), sizeof(Room), 1, file);
		}
	}

	fseek(file, header_offset, SEEK_SET);
	fwrite(&(image_offsets[0]), 4, image_offsets.size(), file);

	fclose(file);
	autosaveTimer.start();
}

void MainWindow::documentSaveAs()
{
	QString name = QFileDialog::getSaveFileName(this, tr("Export Sprite"), QString(), tr("Freetures Background (*.bak)"));

	if(name.isEmpty())
	{
		return;
	}

	filepath = name;
	filename = QFileInfo(filepath).fileName();
	documentSave();
}
