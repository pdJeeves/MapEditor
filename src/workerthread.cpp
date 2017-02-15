#include "workerthread.h"

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
		emit taking_item(j);
		mutex.unlock();

		if(canceled)
		{
			break;
		}

		int x = j / main_window.tiles().height();
		int y = j % main_window.tiles().height();

		auto list = getRoomsFromImage(image.copy(QRect(x*256, y*256, 256, 256)));

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
