#include "linkrooms.h"
#include <algorithm>
#include <iostream>

void LinkRooms::interConnectRooms(std::vector<std::list<Room> > & rooms)
{
	for(auto i = rooms.begin(); i != rooms.end(); ++i)
	{
		linkTile(*i);
	}
}

void LinkRooms::linkTile(std::list<Room> & rooms)
{
	if(rooms.size() < 2)
	{
		return;
	}

	for(auto i = rooms.begin(); i != rooms.end(); ++i)
	{
		for(auto j = std::next(i); j != rooms.end(); ++j)
		{
			if(i->left > j->right+2
			&& j->left > i->right+2)
			{
				continue;
			}

			if(j->left        <  i->left
			&& j->right+2     >= i->left
			&& i->top_left    <  j->bottom_right
			&& i->bottom_left >  j->top_right)
			{
				if(i->left != j->right)
				{
					j->right = i->left;
				}

				i->prev.push_back(&(*j));
				j->next.push_back(&(*i));
			}
			else if(j->right   >  i->right
			&& i->right+2     >= j->left
			&& i->top_right    <  j->bottom_left
			&& i->bottom_right >  j->top_left)
			{
				if(j->left == i->right)
				{
					i->right = j->left;
				}

				i->next.push_back(&(*j));
				j->prev.push_back(&(*i));
			}
		}

		std::sort(i->prev.begin(), i->prev.end(), [](Room * a, Room * b) { return a->bottom_right < b->bottom_right; } );
		std::sort(i->next.begin(), i->next.end(), [](Room * a, Room * b) { return a->bottom_left  < b->bottom_left ; } );
	}
}

void LinkRooms::intraConnectRooms(std::vector<std::list<Room> > & rooms, QSize tiles)
{
	for(size_t i = 0; i < rooms.size(); ++i)
	{
		int x = i / tiles.height();
		int y = i % tiles.height();

		if(x < tiles.width())
		{
			linkTiles(rooms[i], rooms[i+tiles.height()], false);
		}

		if(y < tiles.height())
		{
			linkTiles(rooms[i], rooms[i+1], true);
		}
	}
}

void LinkRooms::sortConnections(std::vector<std::list<Room> > & rooms)
{
	for(auto i = rooms.begin(); i != rooms.end(); ++i)
	{
		for(auto j = i->begin(); j != i->end(); ++j)
		{
			std::sort(j->above.begin(), j->above.end(), [](Room * a, Room * b) { return a->right < b->right; } );
			std::sort(j->below.begin(), j->next.end(), [](Room * a, Room * b) { return a->right  < b->right ; } );

			std::sort(j->prev.begin(), j->prev.end(), [](Room * a, Room * b) { return a->bottom_right < b->bottom_right; } );
			std::sort(j->next.begin(), j->next.end(), [](Room * a, Room * b) { return a->bottom_left  < b->bottom_left ; } );
		}
	}
}


void LinkRooms::linkTiles(std::list<Room> & a, std::list<Room> & b, bool vertical)
{

	if(vertical)
	{
		for(auto i = a.begin(); i != a.end(); ++i)
		{
			if(i->bottom_left  % 256 != 0
			|| i->bottom_right % 256 != 0)
			{
				continue;
			}

			for(auto j = b.begin(); j != b.end(); ++j)
			{
				if(i->left < j->right
				&& j->left < i->right
				&& j->top_left  == i->bottom_left
				&& j->top_right == i->bottom_right)
				{
					i->below.push_back(&(*j));
					j->above.push_back(&(*i));
				}
			}
		}
	}
	else
	{
		for(auto i = a.begin(); i != a.end(); ++i)
		{
			if(i->right % 256 != 0)
			{
				continue;
			}

			for(auto j = b.begin(); j != b.end(); ++j)
			{
				if(j->left == i->right
				&&(j->bottom_left < i->top_right
				&& i->bottom_right < j->top_left))
				{
					i->next.push_back(&(*j));
					j->prev.push_back(&(*i));
				}
			}
		}
	}
}

void LinkRooms::linkRooms(std::vector<std::list<Room> > & rooms, QSize tiles)
{
	interConnectRooms(rooms);
	intraConnectRooms(rooms, tiles);
	sortConnections(rooms);
}
