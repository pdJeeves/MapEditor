#include "airfromrooms.h"
#include <iostream>
#include <cmath>

int AirCastData::doesFollow(const AirCastData & it, bool d) const
{
	if(adj[d] == it.adj[d])
	{
		return 1;
	}

	if((   adj[d] &&    adj[d]->next.empty() && !it.adj[d])
	|| (it.adj[d] && it.adj[d]->prev.empty() && !   adj[d]))
	{
		return 2;
	}

	if(adj[d] && adj[d]->next.size())
	{
		return it.adj[d] == (d?  adj[d]->next.front() : adj[d]->next.back());
	}

	return false;
}

std::vector< std::list<Room> > AirFromRooms::initialPass(const std::vector< std::list<Room> > & current_rooms)
{
	std::vector< std::list<Room> > r(current_rooms.size());

	for(size_t i = 0; i != current_rooms.size(); ++i)
	{
		if(current_rooms[i].empty())
		{
			r[i].emplace_back(0, 256, 0, 0, 256, 256, 0);
			continue;
		}
		else if(current_rooms[i].size() == 1
			&& current_rooms[i].front().isFullRoom())
		{
			continue;
		}

		r[i] = processTile(current_rooms[i]);
	}

	int no_rooms = 0;

	for(auto i = r.begin(); i != r.end(); ++i)
	{
		no_rooms += i->size();
	}

	std::cerr << no_rooms << std::endl;

	return r;
}



std::list<Room> AirFromRooms::processTile(const std::list<Room> & current_rooms)
{
	std::list<Room> r;
	auto casts = getCasts(current_rooms);

	for(auto i = casts.begin(); i != casts.end(); ++i)
	{
		if(i->connect_right)
		{
			findNext(current_rooms, r, casts, i);
		}
	}

	return r;
}

void AirFromRooms::findNext(const std::list<Room> & original, std::list<Room> & rooms, std::list<AirCastData> & casts,  std::list<AirCastData>::iterator i)
{/*
	for(auto j = std::next(i); j != casts.end(); ++j)
	{
		if(j->connect_left
		&& j->x != i->x
		&& i->adj[0] == j->adj[0]
		&& i->adj[1] == j->adj[1])
		{
			AirCastData::insertRoom(rooms, Room(i->x, j->x, i->top, j->top, i->bottom, j->bottom, 0));
			return;
		}
	}*/

	for(auto j = std::next(i); j != casts.end(); ++j)
	{
		if(j->connect_left
		&& j->x != i->x
		&& i->doCastsConnect(rooms, *j))
		{
			break;
		}
	}
}

void AirFromRooms::insertUnique(std::list<AirCastData> & list, AirCastData op, int l_type, int r_type)
{
	op.l_type = l_type;
	op.r_type = r_type;

	for(auto i = list.begin(); i != list.end(); ++i)
	{
		if(i->x != op.x)
		{
			continue;
		}

		if(std::fabs(i->top - op.top) <= 4
		&& std::fabs(i->bottom - op.bottom) <= 4)
		{
			if(i->adj[0] == 0L)
			{
				i->adj[0] = op.adj[0];
			}

			if(i->adj[1] == 0L)
			{
				i->adj[1] = op.adj[1];
			}

			i->top	  = std::min(i->top   , op.top);
			i->bottom = std::max(i->bottom, op.bottom);
			i->l_type = std::max(i->l_type, l_type);
			i->r_type = std::max(i->r_type, r_type);

			i->connect_left |= op.connect_left;
			i->connect_right |= op.connect_right;

/*
			if(i->connect_left != op.connect_left)
			{
				if(i->adj[0] && !op.adj[0] && i->adj[0]->prev.empty()
				|| op.adj[0] && !i->adj[0] && op.adj[0]->prev.empty())

			}*/

			return;
		}
	}

	list.push_back(op);
}


std::list<AirCastData> AirFromRooms::getCasts(const std::list<Room> & current_rooms)
{
	std::list<AirCastData> r;

	int right = 0;
	int left = 256;

	for(auto i = current_rooms.begin(); i != current_rooms.end(); ++i)
	{
		Room a = *i;

		const Room * c1=0L,*c2=0L,*c3=0L,*c4=0L;
		float l1 = 256, l2 = 256, r1 = 256, r2 = 256;

		left  = std::min<int>(left , i->left);
		right = std::max<int>(right, i->right);

		for(auto j = current_rooms.begin(); j != current_rooms.end(); ++j)
		{
			if(i == j)
			{
				continue;
			}

			if(j->doesContain(i->left, i->top_left))
			{
				l1 = 0;
				c1 = &(*j);
			}


			if(j->doesContain(i->left, i->bottom_left))
			{
				l2 = 0;
				c2 = &(*j);
			}

			if(j->doesContain(i->right, i->top_right))
			{
				r1 = 0;
				c3 = &(*j);
			}

			if(j->doesContain(i->right, i->bottom_right))
			{
				r2 = 0;
				c4 = &(*j);
			}

			if(j->left <= i->left && i->left <= j->right)
			{
				if(j->getBottomIntercept(l1, i->left, i->top_left, 0, -1))
				{
					c1 = &(*j);
				}

				if(j->getTopIntercept(l2, i->left, i->bottom_left, 0, 1))
				{
					c2 = &(*j);
				}
			}

			if(j->left <= i->right && i->right <= j->right)
			{
				if(j->getBottomIntercept(r1, i->right, i->top_right, 0, -1))
				{
					c3 = &(*j);
				}

				if(j->getTopIntercept(r2, i->right, i->bottom_right, 0, 1))
				{
					c4 = &(*j);
				}
			}
		}

		l1 = std::max(  0.f, -l1 + i->top_left);
		l2 = std::min(256.f,  l2 + i->bottom_left);
		r1 = std::max(  0.f, -r1 + i->top_right);
		r2 = std::min(256.f,  r2 + i->bottom_right);

		if(i->top_left  - l1 > 4
		|| i->top_right - r1 > 4)
		{
			AirCastData temp(i->left,  l1,i->top_left, c1, &(*i));
			temp.connect_left = false;

			insertUnique(r, temp, i->room_type, -1);

			temp = AirCastData(i->right, r1, i->top_right, c3, &(*i));
			temp.connect_right = false;
			insertUnique(r, temp, -1, i->room_type);
		}
		else
		{
			c1 = &(*i);
			c3 = &(*i);
		}

		if(l2 - i->bottom_left  > 4
		|| r2 - i->bottom_right > 4)
		{
			AirCastData temp(i->left,  i->bottom_left, l2, &(*i), c2);
			temp.connect_left = false;
			insertUnique(r, temp, c2? c2->room_type : 0, -1);

			temp = AirCastData(i->right, i->bottom_right, r2,  &(*i), c4);
			temp.connect_right = false;
			insertUnique(r, temp, -1, c4? c4->room_type : 0);
		}
		else
		{
			c2 = &(*i);
			c4 = &(*i);
		}

		if(i->prev.empty() && i->left > 0)
		{
			AirCastData temp(i->left, l1, l2, c1, c2);
			temp.connect_right = false;
			insertUnique(r, temp, c2? c2->room_type : 0, -1);
		}

		if(i->next.empty() && i->right < 256)
		{
			AirCastData temp(i->right, r1, r2, c3, c4);
			temp.connect_left = false;
			insertUnique(r, temp, -1, c4? c4->room_type : 0);
		}

	}

	if(right < 254)
	{
		insertUnique(r, AirCastData(256,  0, 256, 0L, 0L), 0, 0);
	}
	if(left > 1)
	{
		insertUnique(r, AirCastData(0,  0, 256, 0L, 0L), 0, 0);
	}

	r.sort([](const AirCastData & a, const AirCastData & b) { return a.x < b.x; } );

#if 0
	for(auto i = r.begin(); i != r.end(); ++i)
	{
		std::cerr << i->x << "\t" << i->top << "\t" << i->bottom << "\t" << i->connect_right << "\t" << i->connect_left << std::endl;
	}
#endif

	return r;
}

bool AirCastData::insertRoom(std::list<Room> & rooms, Room it)
{
	for(auto i = rooms.begin(); i != rooms.end(); ++i)
	{
		if(*i == it)
		{
			return true;
		}
	}

	rooms.push_back(it);
	return false;
}

bool AirCastData::doCastsConnect(std::list<Room> & rooms, AirCastData & it)
{
	int i = doesFollow(it);

	switch(i)
	{
	case 0x00:
		return false;
	case 0x11:
		insertRoom(rooms, Room(x, it.x, top, it.top, bottom, it.bottom, 0));
		return true;
	case 0x22:
	//	insertRoom(rooms, Room(x, it.x, 0, 0, 255, 255, 0));
		return false;
	case 0x01:
	case 0x10:
		return false;
	case 0x02:
	case 0x20:
		return false;
	case 0x12:
	case 0x21:
		insertRoom(rooms, Room(x, it.x, top, it.top, bottom, it.bottom, 0));
		return true;
	}

	return false;
}
