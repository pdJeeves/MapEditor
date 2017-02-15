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
#include <QImageReader>
#include <QStandardPaths>
#include <QImageWriter>
#include <QProgressDialog>
#include "roomsfromimage.h"
#include "workerthread.h"
#include <squish.h>
#include <QProgressBar>


static void initializeImageFileDialog(QFileDialog &dialog, QFileDialog::AcceptMode acceptMode)
{
    static bool firstDialog = true;

    if (firstDialog) {
        firstDialog = false;
        const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
    }

    QStringList mimeTypeFilters;
    const QByteArrayList supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen
        ? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
    foreach (const QByteArray &mimeTypeName, supportedMimeTypes)
        mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/png");
    if (acceptMode == QFileDialog::AcceptSave)
        dialog.setDefaultSuffix("jpg");
}

bool MainWindow::loadImage(QImage & ret_image)
{
top:
	QFileDialog dialog(this, tr("Open File"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);

	QImage newImage;
	QString name;
	bool accepted;

    while ((accepted = (dialog.exec() == QDialog::Accepted)))
	{
		name = dialog.selectedFiles().first();

		QImageReader reader(name);
		reader.setAutoTransform(true);
		newImage = reader.read();
		if (!newImage.isNull())
		{
			break;
		}

		QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
		tr("Cannot load %1: %2")
		.arg(QDir::toNativeSeparators(name), reader.errorString()));
	}

	if(!accepted)
	{
		return false;
	}

	if(!dimensionCheck(newImage.size()))
	{
		goto top;
	}

	if(dimensions == QSize(-1, -1))
	{
		filename = QFileInfo(name).fileName();
		dimensions = newImage.size();

		rooms.clear();
		rooms.resize(totalTiles());

		fluids.clear();
		fluids.resize(totalTiles());
	}

	if(newImage.size().width() % 256 == 0
	&& newImage.size().height() % 256 == 0)
	{
		ret_image = std::move(newImage);
		return true;
	}

	QImage image((newImage.size().width() + 255) & 0xFF00, (newImage.size().height() + 255) & 0xFF00, QImage::Format_ARGB32);
	image.fill(0);

	for(int y = 0; y < newImage.size().height(); ++y)
	{
		for(int x = 0; x < newImage.size().width(); ++x)
		{
			image.setPixel(x, y, newImage.pixel(x, y));
		}
	}

	ret_image = std::move(image);
	return true;
}

void MainWindow::replaceImage(QImage & image)
{
	QImage newImage;
	loadImage(newImage);

	image = std::move(newImage.convertToFormat(QImage::Format_ARGB32_Premultiplied));

	ui->widget->repaint();
}

void MainWindow::actionImportRooms(bool type)
{
	QImage image;
	if(!loadImage(image))
	{
		return;
	}

	static QVector<QRgb> palette16;

	if(!palette16.size())
	{
		palette16.push_back(0);

		for(uint8_t i = 0; i < 16; ++i)
		{
			palette16.push_back(palette[i]);
		}
	}

	progress = new QProgressDialog(tr("Processing Image..."), "Cancel", 0, totalTiles(), this);

	room_map = new QImage(std::move(image.convertToFormat(QImage::Format_Indexed8, palette16, Qt::ThresholdDither)));

	for(int y = 0; y < image.height(); ++y)
	{
		for(int x = 0; x < image.width(); ++x)
		{
			if(qAlpha(image.pixel(x, y)) < 200)
			{
				room_map->setPixel(x, y, 0);
			}
		}
	}

	WorkerThread::i = 0;
	WorkerThread::canceled = false;
	connect(progress, &QProgressDialog::canceled, this, [this](){ WorkerThread::canceled = true; });


	ui->actionImportRooms->setEnabled(false);

	for(int i = 0; i < 3; ++i)
	{
		WorkerThread *workerThread = new WorkerThread(*this, *room_map, type);
		connect(workerThread, &WorkerThread::taking_item, progress, &QProgressDialog::setValue);
		connect(workerThread, &WorkerThread::finished, workerThread, &WorkerThread::deleteLater);
		connect(workerThread, &WorkerThread::work_finished, this, [this](){ui->actionImportRooms->setEnabled(true); delete progress; delete room_map; });
		workerThread->start();
	}

	ui->actionExport_Parallax_Layer->setEnabled(false);
}

QRgb readColor(FILE * file, int index)
{
	uint8_t b[4];
	switch(index)
	{
	case 0:
		fread(b, 1, 2, file);

		return  ((b[1] & 0x0F) << 28)
			  | ((b[0] & 0xF0) << 16)
			  | ((b[0] & 0x0F) << 12)
			  | ((b[1] & 0xF0));
		break;
	case 1:
		fread(b, 1, 1, file);
		return  QColor::fromHsl(0, 0, b[0], 255).rgba();
	case 2:
		fread(b, 1, 1, file);
		return  0xFF000000
			  | ((b[0] & 0xE0) << 16)
			  | ((b[0] & 0x1C) << 11)
			  | ((b[0] & 0x03) << 6);
		break;
	default:
		fread(b, 1, 4, file);
		return ((((((int) b[3] << 8) | b[0]) << 8) | b[1]) << 8) | b[2];
	}
}


void MainWindow::documentOpen()
{
	QString name;

	do {
		name = QFileDialog::getOpenFileName(this, tr("Open Kreatures Background"), QString(), tr("Kreatures Background (*.bak)"));

		if(name.isEmpty())
		{
			return;
		}
	} while(!openKreaturesFile(name.toStdString(), 17));
}

void MainWindow::openParallaxLayer()
{
	QString name;

	do {
		name = QFileDialog::getOpenFileName(this, tr("Open Kreatures Parallax Layer"), QString(), tr("Kreatures Parallax Layer (*.plx)"));

		if(name.isEmpty())
		{
			return;
		}
	} while(!openKreaturesFile(name.toStdString(), 5));
}


std::vector<QRgb> readTile(FILE * file, int x0, int y0)
{
	uint8_t compression_type;
	uint32_t size;
	fread(&compression_type, 1, 1, file);
	fread(&size, 4, 1, file);
	size = byte_swap(size);

	std::vector<uint8_t> image_data(size, 0);

	for(int l = 0; l < size; )
	{
		uint32_t len;
		fread(&len, 4, 1, file);
		len = byte_swap(len);

		switch(compression_type)
		{
		case 1:
			for(; l+8 <= size && len; len -= 8, l += 8)
			{
				memset(image_data.data() + (l+4), 0xFF, 4);
			}
			break;
		case 3:
			break;
		case 5:
			for(; l+16 <= size && len; len -= 16, l += 16)
			{
				image_data[l+1] = 0x05;
			}
			break;
		}

		l += len;


		if(l >= size)
		{
			break;
		}

		fread(&len, 4, 1, file);
		len = byte_swap(len);
		fread(image_data.data() + l, 1, len, file);
		l += len;
	}

	switch(compression_type)
	{
	default:
		compression_type = squish::kDxt1;
		break;
	case 3:
		compression_type = squish::kDxt3;
		break;
	case 5:
		compression_type = squish::kDxt5;
		break;
	}

	std::vector<QRgb> uncompressed_image(65536);
	squish::DecompressImage((uint8_t*) uncompressed_image.data(), 256, 256, image_data.data(), compression_type);
	return uncompressed_image;
}

uint32_t packBytes(uint32_t a, uint32_t b, uint32_t c,uint32_t d);

bool MainWindow::openKreaturesFile(std::string name, int read_length)
{
	FILE * file = fopen(name.c_str(), "rb");
	if(!file)
	{
		QMessageBox mesg;
		mesg.setText(QObject::tr("Unable to open file '%1' for reading.").arg(filename));
		mesg.exec();
		return false;
	}

	int _one;
	fread(&_one, 4, 1, file);

	if(byte_swap(_one) != 1)
	{
		fclose(file);
		QMessageBox mesg;
		mesg.setText(QObject::tr("File is not a Kreatures background file."));
		mesg.exec();
		documentOpen();
		return false;
	}

	uint16_t width, height;
	uint32_t image_offsets[17];

	fread(&width, 2, 1, file);
	fread(&height, 2, 1, file);
	memset(image_offsets, 0, 4*17);
	fread(image_offsets, 4, read_length, file);

	width = byte_swap(width);
	height = byte_swap(height);

	dimensions = QSize(width, height);
	filepath = name.c_str();
	filename = QFileInfo(filepath).fileName();

	int tiles_x = (width  + 255) / 256;
	int tiles_y = (height + 255) / 256;

	std::vector<uint32_t> tile_offsets(tiles_x*tiles_y);

	int length = tiles_x*tiles_y;
	int mult = 0;

	for(int map = 0; map < 3; ++map)
	{
		for(int channel = 0; channel < 5; ++channel)
		{
			if(image_offsets[map*5 + channel])
			{
				++mult;
			}
		}
	}

	QProgressDialog progress(tr("Loading Kreatures Background..."), 0L, 0, mult*length, this);
	length = 0;

	for(int map = 0; map < 3; ++map)
	{
		for(int channel = 0; channel < 5; ++channel)
		{
			if(!image_offsets[map*5 + channel])
			{
				background[map][channel] = QImage();
				continue;
			}

			background[map][channel] = QImage(tiles_x << 8, tiles_y << 8, QImage::Format_ARGB32);
			background[map][channel].fill(0);

			fseek(file, byte_swap(image_offsets[map*5+channel]), SEEK_SET);
			fread(tile_offsets.data(), 4, tile_offsets.size(), file);

			for(uint16_t tile = 0; tile < tile_offsets.size(); ++tile)
			{
				progress.setValue(++length);

				if(!tile_offsets[tile])
				{
					continue;
				}

				if((length & 0x03) == 0)
				{
					QApplication::processEvents();
				}

				int x0 = (tile / tiles_y) << 8;
				int y0 = (tile % tiles_y) << 8;

				fseek(file, byte_swap(tile_offsets[tile]), SEEK_SET);

				std::vector<QRgb> uncompressed_image(readTile(file, x0, y0));

				for(int l = 0; l < 0x10000; ++l)
				{
					QRgb c = uncompressed_image[l];
					if(c)
					{
						c = packBytes(qRed(c), qGreen(c), qBlue(c), qAlpha(c));

						if(channel >= 2)
						{
							c = qRgba(qBlue(c), qRed(c), 0, 255);
						}
					}

					uint8_t x = l & 0xFF;
					uint8_t y = l >> 8;

					background[map][channel].setPixel(x0 + x, y0 + y, c);
				}
			}
		}
	}

	if(image_offsets[15])
	{
		fseek(file, byte_swap(image_offsets[15]), SEEK_SET);
		rooms.clear();
		rooms.resize(totalTiles());

		for(auto i = 0; i < totalTiles(); ++i)
		{
			uint8_t length;
			fread(&length, 1, 1, file);

			if(length == 0)
			{
				continue;
			}

			for(int j = 0; j < length; ++j)
			{
				Room room;
				fread(&room, sizeof(Room), 1, file);
				rooms[i].push_back(room);
			}
		}
	}

	if(image_offsets[16])
	{
		fseek(file, byte_swap(image_offsets[16]), SEEK_SET);
		fluids.clear();
		fluids.resize(totalTiles());

		for(auto i = 0; i < totalTiles(); ++i)
		{
			uint8_t length;
			fread(&length, 1, 1, file);

			if(length == 0)
			{
				continue;
			}

			for(int j = 0; j < length; ++j)
			{
				Room room;
				fread(&room, sizeof(Room), 1, file);
				fluids[i].push_back(room);
			}
		}
	}

	fclose(file);
	ui->widget->repaint();

	return true;
}
