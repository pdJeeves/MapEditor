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

uint32_t MainWindow::writeTile(FILE * file, int x0, int y0, int map, int channel) const
{
	uint32_t r = byte_swap((uint32_t) ftell(file));
	uint32_t length;

	for(uint32_t i = 0; i < 0x10000;)
	{
		length = runLength(i, x0, y0, map, channel, true);

		if(length == 0x10000)
		{
			return 0;
		}

		i += length;
		length = byte_swap(length);
		fwrite(&length, 4, 1, file);

		if(i >= 0x10000)
		{
			break;
		}

		length = runLength(i, x0, y0, map, channel, false);
		length = byte_swap(length);
		fwrite(&length, 4, 1, file);
		length = byte_swap(length);

		for(; length != 0; --length, ++i)
		{
			int x = (i & 0xFF) + x0;
			int y = (i >> 8) + y0;

			writeColor(file, background[map][channel].pixel(x, y), channel);
		}
	}

	return r;
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

	uint32_t image_offsets[13];
	fseek(file, sizeof(image_offsets), SEEK_CUR);
	std::vector<uint32_t> offsets(totalTiles());

	for(auto i = 0; i < 3; ++i)
	{
		for(auto j = 0; j < 4; ++j)
		{
			if(background[i][j].isNull())
			{
				image_offsets[i*4 + j] = 0;
				continue;
			}

			int tiles_written = 0;

			image_offsets[i*4 + j] = byte_swap((uint32_t) ftell(file));

			fpos_t offset_pos;
			fgetpos(file, &offset_pos);
			fseek(file, offsets.size() * 4, SEEK_CUR);

			for(int x = 0; x < tiles().width(); ++x)
			{
				for(int y = 0; y < tiles().height(); ++y)
				{
					offsets[x*tiles().height() + y] = writeTile(file, x << 8, y << 8, i, j);
					if(offsets[x*tiles().height() + y])
					{
						++tiles_written;
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

	image_offsets[12] = byte_swap((uint32_t) ftell(file));

	for(auto i = 0; i < totalTiles(); ++i)
	{
		uint8_t length = rooms[i].size();
		fwrite(&length, 1, 1, file);

		for(auto j = rooms[i].begin(); j != rooms[i].end(); ++j)
		{
			fwrite(&(*j), sizeof(Room), 1, file);
		}
	}

	fseek(file, header_offset, SEEK_SET);
	fwrite(image_offsets, 1, sizeof(image_offsets), file);

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
