#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H
#include "roomsfromimage.h"
#include "mainwindow.h"
#include <QImage>
#include <QThread>
#include <QMutex>

class WorkerThread : public QThread
{
	Q_OBJECT

	MainWindow & main_window;
	QImage & image;
	bool type;

public:
	static int i;
	static bool canceled;

	WorkerThread(MainWindow & main_window, QImage & image, bool type);

	void run() override;

signals:
	void finished();
	void work_finished();
	void taking_item(int);
};

#endif // WORKERTHREAD_H
