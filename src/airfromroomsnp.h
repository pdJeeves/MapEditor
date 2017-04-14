#ifndef AIRFROMROOMSNP_H
#define AIRFROMROOMSNP_H
#include "room.h"
#include <QPoint>
#include <QSize>
#include <vector>
#include <list>

class AirFromRoomsNP
{
	static void calculateLine(float & m, float & b, int x1, int x2, int y1, int y2);
	static bool getOptimalLine(std::vector<QPoint> & r, std::vector<QPoint> & other, QPoint intercept, double & m, double & b, bool direction);
	static bool getOptimal(Room & result, std::vector<Room*> r, int begin, int end);
	static std::vector<std::vector<Room*> > getRoomLists(std::list<Room> &rooms);
	static bool processRoomLists(std::list<Room> & rooms, const std::vector<std::vector<Room*> > & lists);

public:
	static void processTiles(std::vector<std::list<Room> > &rooms);
};

#endif // AIRFROMROOMSNP_H
