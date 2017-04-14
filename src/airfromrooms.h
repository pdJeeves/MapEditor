#ifndef AIRFROMROOMS_H
#define AIRFROMROOMS_H
#include <vector>
#include <list>
#include "room.h"

struct AirCastData
{
	bool doCastsConnect(std::list<Room> & rooms, AirCastData & it);
	static bool insertRoom(std::list<Room> & rooms, Room it);

	AirCastData(int x, int top, int bottom, const Room * above, const Room * below) :
		x(x),
		top(top),
		bottom(bottom),
		l_type(0),
		r_type(0),
		connect_left(1),
		connect_right(1),
		adj{above,below}
	{
	}

	int x;
	int top;
	int bottom;

	int l_type;
	int r_type;

	bool connect_left;
	bool connect_right;

	const Room * adj[2];

	std::list<AirCastData *> next;
	std::list<AirCastData *> prev;

	inline const Room * above() const { return adj[0]; }
	inline const Room * below() const { return adj[1]; }

	inline bool isSolid() const { return adj[0] == adj[1]; }
	inline int doesFollow(const AirCastData & it) const { return (doesFollow(it, false) << 4) | doesFollow(it, true); }

	int doesFollow(const AirCastData & it, bool d) const;
};

class AirFromRooms
{
	static void findNext(const std::list<Room> & original, std::list<Room> & rooms, std::list<AirCastData> & casts,  std::list<AirCastData>::iterator i);
	static void insertUnique(std::list<AirCastData> & list, AirCastData op, int l_type, int r_type);
	static std::list<AirCastData> getCasts(const std::list<Room> & current_rooms);
	static void addGapRoom(std::list<Room> & rooms, int left, int right);
	static std::list<Room> processTile(const std::list<Room> & current_rooms);
	static std::list<Room> processChamber(const std::list<Room> & current_rooms, Room*left, Room*right, Room chamber, bool direction);

public:
	static std::vector< std::list<Room> > initialPass(const std::vector< std::list<Room> > & current_rooms);
};

#endif // AIRFROMROOMS_H
