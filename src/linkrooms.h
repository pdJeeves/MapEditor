#ifndef LINKROOMS_H
#define LINKROOMS_H
#include <QSize>
#include "room.h"
#include <list>

class LinkRooms
{
public:
	static void linkTile(std::list<Room> & rooms);
	static void linkTiles(std::list<Room> & a, std::list<Room> & b, bool vertical);
	static void intraConnectRooms(std::vector<std::list<Room> > & rooms, QSize tiles);
	static void sortConnections(std::vector<std::list<Room> > & rooms);


	static void interConnectRooms(std::vector<std::list<Room> > & rooms);
	static void linkRooms(std::vector<std::list<Room> > & rooms, QSize tiles);

};

#endif // LINKROOMS_H
