#include "workerthread.h"
#include "verticies.h"

int WorkerThread::i = 0;
bool WorkerThread::canceled = 0;

WorkerThread::WorkerThread(MainWindow & main_window, QImage & image, bool type) :
	main_window(main_window),
	image(image),
	type(type)
{
}

void WorkerThread::run()
{
static QMutex mutex;
static int working_threads = 0;
	mutex.lock();
	++working_threads;
	for(; i < main_window.totalTiles(); ++i)
	{
		int j = i;
		emit taking_item(j + image.height());
		mutex.unlock();

		if(canceled)
		{
			break;
		}

		int x = j / main_window.tiles().height();
		int y = j % main_window.tiles().height();

		auto list = RoomsFromImage::getRoomsFromImage(image.copy(QRect(x*PixelsPerTile, y*PixelsPerTile, PixelsPerTile, PixelsPerTile)));

		for(auto i = list.begin(); i != list.end(); ++i)
		{
			if(i->left         == (PixelsPerTile-1)) i->left         = PixelsPerTile;
			if(i->right        == (PixelsPerTile-1)) i->right        = PixelsPerTile;
			if(i->top_left     == (PixelsPerTile-1)) i->top_left     = PixelsPerTile;
			if(i->top_right    == (PixelsPerTile-1)) i->top_right    = PixelsPerTile;
			if(i->bottom_left  == (PixelsPerTile-1)) i->bottom_left  = PixelsPerTile;
			if(i->bottom_right == (PixelsPerTile-1)) i->bottom_right = PixelsPerTile;
#if 0
			i->left         += x*PixelsPerTile;
			i->right        += x*PixelsPerTile;
			i->top_left     += y*PixelsPerTile;
			i->top_right    += y*PixelsPerTile;
			i->bottom_left  += y*PixelsPerTile;
			i->bottom_right += y*PixelsPerTile;
#endif

			i->left         *= 2;
			i->right        *= 2;
			i->top_left     *= 2;
			i->top_right    *= 2;
			i->bottom_left  *= 2;
			i->bottom_right *= 2;
		}

		if(type)
		{
			main_window.rooms[j].splice(main_window.rooms[j].end(), std::move(list));
		}
		else
		{
			main_window.fluids[j].splice(main_window.fluids[j].end(), std::move(list));
		}

		mutex.lock();
	}
	--working_threads;

	if(working_threads == 0)
	{
		emit work_finished();
	}
	mutex.unlock();

	emit finished();
}
