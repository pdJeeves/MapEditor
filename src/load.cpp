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
	ui->actionExport_Parallax_Layer->setEnabled(false);
	return true;
}

void MainWindow::replaceImage(QImage & image, int channel)
{
	QImage newImage;
	loadImage(newImage);

	switch(channel)
	{
	case 0:
		image = std::move(newImage.convertToFormat(QImage::Format_ARGB4444_Premultiplied));
		break;
	case 1:
	{
		image = std::move(QImage(newImage.width(), newImage.height(), QImage::Format_ARGB32_Premultiplied));
		image.fill(0);

		for(int x = 0; x < image.width(); ++x)
		{
			for(int y = 0; y < image.height(); ++y)
			{
				if(qAlpha(newImage.pixel(x, y) > 128))
				{
					auto g = qGray(newImage.pixel(x, y));
					image.setPixel(x, y, ((((0xFF00 | g) << 8) | g) << 8)| g);
				}
			}
		}

		break;
	}
	case 2:
	{
		if(!palette332.size())
		{
			for(int r = 0; r < 8; ++r)
			{
				for(int g = 0; g < 8; ++g)
				{
					for(int b = 0; b < 4; ++b)
					{
						palette332.push_back(qRgba(r << 5, g << 5, b << 6, 0xFF));
					}
				}
			}
		}

		QImage dithered = newImage.convertToFormat(QImage::Format_Indexed8, palette332, Qt::PreferDither);
		dithered = std::move(dithered.convertToFormat(QImage::Format_ARGB4444_Premultiplied));

		for(int x = 0; x < newImage.width(); ++x)
		{
			for(int y = 0; y < newImage.height(); ++y)
			{
				if(qAlpha(newImage.pixel(x, y)) < 128)
				{
					dithered.setPixel(x, y, 0);
					continue;
				}
			}
		}

		image = std::move(dithered);
		break;
	}
	default:
		image = std::move(newImage.convertToFormat(QImage::Format_ARGB32_Premultiplied));
		break;
	}


	ui->widget->repaint();
}

void MainWindow::actionImportRooms()
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
		WorkerThread *workerThread = new WorkerThread(*this, *room_map);
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
	} while(!openKreaturesFile(name.toStdString(), 13));
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
	} while(!openKreaturesFile(name.toStdString(), 4));
}


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
	uint32_t image_offsets[13];

	fread(&width, 2, 1, file);
	fread(&height, 2, 1, file);
	memset(image_offsets, 0, 4*13);
	fread(image_offsets, 4, read_length, file);

	width = byte_swap(width);
	height = byte_swap(height);

	dimensions = QSize(width, height);
	filepath = name.c_str();
	filename = QFileInfo(filepath).fileName();

	int tiles_x = (width  + 255) / 256;
	int tiles_y = (height + 255) / 256;

	std::vector<uint32_t> tile_offsets(tiles_x*tiles_y);

	for(int map = 0; map < 3; ++map)
	{
		for(int channel = 0; channel < 4; ++channel)
		{
			if(!image_offsets[map*4 + channel])
			{
				background[map][channel] = QImage();
				continue;
			}

			background[map][channel] = QImage(tiles_x << 8, tiles_y << 8, QImage::Format_ARGB32);
			background[map][channel].fill(0);

			fseek(file, byte_swap(image_offsets[map*4+channel]), SEEK_SET);
			fread(tile_offsets.data(), 4, tile_offsets.size(), file);

			for(uint16_t tile = 0; tile < tile_offsets.size(); ++tile)
			{
				if(!tile_offsets[tile])
				{
					continue;
				}

				int x0 = (tile / tiles_y) << 8;
				int y0 = (tile % tiles_y) << 8;

				fseek(file, byte_swap(tile_offsets[tile]), SEEK_SET);

				for(int l = 0; l < 0x10000; )
				{
					uint32_t length;
					fread(&length, 4, 1, file);
					l += byte_swap(length);

					if(l >= 0x10000)
					{
						break;
					}

					fread(&length, 4, 1, file);
					length = byte_swap(length);

					for(; length; --length, ++l)
					{
						uint8_t x = l & 0xFF;
						uint8_t y = l >> 8;

						background[map][channel].setPixel(x0 + x, y0 + y, readColor(file, channel));
					}
				}
			}
		}
	}

	if(image_offsets[12])
	{
		fseek(file, byte_swap(image_offsets[12]), SEEK_SET);
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

	fclose(file);
	ui->widget->repaint();

	return true;
}
