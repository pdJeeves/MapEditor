#ifndef JOINROOMSVERTICALLY_H
#define JOINROOMSVERTICALLY_H
#include <QPoint>
#include <QSize>
#include <list>
#include <vector>
#include "room.h"

class JoinRoomsVertically
{
public:
	static std::list<Room> toWorldSpace(std::vector<std::list<Room> > & rooms, QSize tiles);

	static std::list<Room>::iterator findPrev(std::list<Room> & list, std::list<Room>::iterator i);
	static std::list<Room>::iterator findNext(std::list<Room> & list, std::list<Room>::iterator i);

	static bool intersectTest(Room A, Room B);
	static bool checkPair(Room it, std::list<Room> & list, std::list<Room>::iterator i, std::list<Room>::iterator j);

	static bool getOptimal(Room & r, float & error, Room & a, Room & b);

	static bool getOptimalLine(std::vector<QPoint> & r, std::vector<QPoint> & other, QPoint intercept, double & m, double & b, double & error, bool direction);

	static void linearize(Room & room);
	static void joinPairs(std::list<Room> & list);

	static void linkShafts(std::vector<std::list<Room> > & rooms, QSize tiles);
};

#endif // JOINROOMSVERTICALLY_H
