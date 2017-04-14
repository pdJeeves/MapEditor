#include "joinroomsvertically.h"
#include "linkrooms.h"
#include <climits>
#include <stdexcept>

void JoinRoomsVertically::linkShafts(std::vector<std::list<Room> > & rooms, QSize tiles)
{
	for(int y = tiles.height()-1; y > 0; --y)
	{
		for(int x = 0; x < tiles.width(); ++x)
		{
			int index = x*tiles.height() + y;

			for(auto i = rooms[index].begin(); i != rooms[index].end(); ++i)
			{
				if(i->top_left != 0 || i->top_right != 0)
				{
					continue;
				}

				for(int y2 = y-1; y2 >= 0; --y2)
				{
					int index2   = x*tiles.height() + y2;
					int distance = (y - y2)*256;
					int limit	 = ((y2+1) - y) * 256;

					if(i->top_left != limit || i->top_right != limit)
					{
						break;
					}

					for(auto j = rooms[index2].begin(), k = std::next(j); j != rooms[index2].end(); j = k++)
					{
						if(j->bottom_left  != i->top_left  + distance
						|| j->bottom_right != i->top_right + distance
						|| j->room_type
						)
						{
							continue;
						}

						if(i->left < j->left && j->left < i->right)
						{
							int y = i->getBottomIntersection(j->left);
							rooms[index].insert(i, Room(i->left, j->left, 0, 0, i->bottom_left, y, i->room_type));
							i->left = j->left;
							i->bottom_left = y;
						}

						if(i->left == j->left)
						{
							if(i->right > j->right)
							{
								j->bottom_left	= i->bottom_left + distance;
								j->bottom_right = i->getBottomIntersection(j->right) + distance;
								i->bottom_left	= j->bottom_right - distance;
								i->left			= j->right;
							}
							else if(i->right < j->right)
							{
								i->top_left  = j->top_left - distance;
								i->top_right = j->getTopIntersection(i->right) - distance;
								j->top_left = i->top_right + distance;
								j->left     = i->right;
								break;
							}
							else
							{
								i->top_left  = j->top_left  - distance;
								i->top_right = j->top_right - distance;
								rooms[index-1].erase(j);
								continue;
							}
						}
					}
				}
			}
		}
	}
}


void JoinRoomsVertically::linearize(Room & room)
{
	if(room.prev.size() > 1)
	{
		auto list = std::move(room.prev);

		for(auto i = list.begin(); i != list.end(); ++i)
		{
			linearize(**i);
			(*i)->next.clear();
		}
	}

	if(room.next.size() > 1)
	{
		auto list = std::move(room.next);

		for(auto i = list.begin(); i != list.end(); ++i)
		{
			linearize(**i);
			(*i)->prev.clear();
		}
	}
}

std::list<Room> JoinRoomsVertically::toWorldSpace(std::vector<std::list<Room> > & rooms, QSize tiles)
{
	std::list<Room> r;

	for(size_t i = 0; i < rooms.size(); ++i)
	{
		int x = i / tiles.height() * 256;
		int y = i % tiles.height() * 256;

		for(auto j = rooms[i].begin(); j != rooms[i].end(); ++j)
		{
			r.emplace_back(
				j->left         + x,
				j->right        + x,
				j->top_left     + y,
				j->top_right    + y,
				j->bottom_left  + y,
				j->bottom_right + y,
				j->room_type);

			r.back().original.push_back(r.back());
		}
	}

	r.sort([](const Room & a, const Room & b) { return a.left < b.left; });
	//initialization may be lost in sorting

	LinkRooms::linkTile(r);

	for(auto i = r.begin(); i != r.end(); ++i)
	{
		i->itr = i;
		linearize(*i);
	}

	return r;
}

std::list<Room>::iterator JoinRoomsVertically::findNext(std::list<Room> & list, std::list<Room>::iterator i)
{
	if(i != list.end() && i->next.size() == 1)
	{
		return i->next.front()->room_type == i->room_type? i->next.front()->itr : list.end();
	}

	return list.end();
}

std::list<Room>::iterator JoinRoomsVertically::findPrev(std::list<Room> & list, std::list<Room>::iterator i)
{
	if(i != list.end() && i->prev.size() == 1)
	{
		return i->prev.front()->room_type == i->room_type? i->prev.front()->itr : list.end();
	}

	return list.end();
}

bool JoinRoomsVertically::intersectTest(Room A, Room B)
{
	Room rooms[2] = {A, B};

	for(int i = 0; i < 2; ++i)
	{
		double _m[2];
		double _b[2];
		double y[2];

		_m[0] = (rooms[1-i].top_right - rooms[1-i].top_left) / (double) (rooms[1-i].right - rooms[1-i].left);
		_m[1] = (rooms[1-i].bottom_right - rooms[1-i].bottom_left) / (double) (rooms[1-i].right - rooms[1-i].left);

		_b[0] = rooms[1-i].top_left - _m[0]*rooms[1-i].left;
		_b[1] = rooms[1-i].bottom_left - _m[1]*rooms[1-i].right;


		if(rooms[i].left <= rooms[1-i].left && rooms[1-i].left <= rooms[i].right)
		{
			y[0] = rooms[1-i].left * _m[0] + _b[0];
			y[1] = rooms[1-i].left * _m[1] + _b[1];

			if((y[0] < rooms[1-i].top_left    && rooms[1-i].top_left    < y[1])
			|| (y[0] < rooms[1-i].bottom_left && rooms[1-i].bottom_left < y[1])
			|| (y[0] > rooms[1-i].top_left    && rooms[1-i].bottom_left > y[1]))
			{
				return false;
			}
		}

		if(rooms[i].left <= rooms[1-i].right && rooms[1-i].right <= rooms[i].right)
		{
			y[0] = rooms[1-i].right * _m[0] + _b[0];
			y[1] = rooms[1-i].right * _m[1] + _b[1];

			if((y[0] < rooms[1-i].top_right    && rooms[1-i].top_right    < y[1])
			|| (y[0] < rooms[1-i].bottom_right && rooms[1-i].bottom_right < y[1])
			|| (y[0] > rooms[1-i].top_right    && rooms[1-i].bottom_right > y[1]))
			{
				return false;
			}
		}
	}

	return true;
}

bool JoinRoomsVertically::checkPair(Room it, std::list<Room> & list, std::list<Room>::iterator i, std::list<Room>::iterator j)
{
	for(auto k = list.begin(); k != list.end(); ++k)
	{
		if(k == i
		|| k == j
		|| k->right <= it.left)
		{
			continue;
		}

		if(k->left >= it.right)
		{
			break;
		}

		if(intersectTest(it, *k))
		{
			return false;
		}
	}

	return true;
}


bool JoinRoomsVertically::getOptimal(Room & r, float & error, Room & a, Room & b)
{
	int begin = a.left;
	int end = b.right;

	std::vector<QPoint> top;
	std::vector<QPoint> bottom;

	std::vector<Room> original;

	for(auto i = a.original.begin(); i != a.original.end(); ++i)
	{
		original.push_back(*i);
	}

	for(auto i = b.original.begin(); i != b.original.end(); ++i)
	{
		original.push_back(*i);
	}

	for(auto i = original.begin(); i != original.end(); ++i)
	{
		top.emplace_back(i->left, i->top_left);
		top.emplace_back(i->right, i->top_right);

		bottom.emplace_back(i->left, i->bottom_left);
		bottom.emplace_back(i->right, i->bottom_right);
	}

	QPoint min(65535, 65535);
	QPoint max(0, 0);

	for(size_t i = 0; i < top.size(); ++i)
	{
		if(min.y() > top[i].y())
		{
			min = top[i];
		}

		if(max.y() < bottom[i].y())
		{
			max = bottom[i];
		}
	}

	double m1,b1,e1,m2,b2,e2;

	if(getOptimalLine(top, bottom, min, m1, b1,e1, true)
	&& getOptimalLine(bottom, top, max, m2, b2,e2, false))
	{
		r = Room(begin, end, begin*m1 + b1, end*m1 + b1, begin*m2 + b2, end*m2 + b2, a.room_type);
		r.original = std::move(original);
		error = (e1 + e2)*(end-begin);
		return true;
	}

	return false;
}

bool JoinRoomsVertically::getOptimalLine(std::vector<QPoint> & r, std::vector<QPoint> & other, QPoint intercept, double & m, double & b, double & error, bool direction)
{
	error = INT_MAX;
	bool retn = false;

	for(size_t i = 0; i < r.size(); ++i)
	{
		double m1,b1;

		if(r[i] == intercept)
		{
			m1 = 0;
			b1 = intercept.y();
		}
		else
		{
			m1 = (intercept.y() - r[i].y()) / (double) (intercept.x() - r[i].x());
			b1 = intercept.y() - intercept.x()*m1;
		}

		double err = 0;

		for(size_t i = 0; i < r.size(); ++i)
		{
			double y = r[i].x()*m1 + b1;
			double diff = direction? r[i].y() - y: y - r[i].y();

			if(diff < -1
			/*
			|| ( direction && other[i].y() < y)
			|| ( !direction && other[i].y() > y) // */
			)
			{
				err = INT_MAX;
				break;
			}

			err += diff*diff;
		}

		if(err < error)
		{
			error = err;
			m     = m1;
			b     = b1;
			retn     = true;
		}
	}

	return retn;
}

void JoinRoomsVertically::joinPairs(std::list<Room> & list)
{
	std::list<Room>::iterator rooms[4];
	Room					  temp[3];
	float					  error[3];
	bool					  result[3];


	for(auto i = list.begin(); i != list.end(); ++i)
	{
		rooms[0] = findPrev(list, i);
		rooms[1] = i;
		rooms[2] = findNext(list, rooms[1]);
		rooms[3] = findNext(list, rooms[2]);

		for(int i = 0; i < 3; ++i)
		{
			error[i]  = INT_MAX;
			result[i] = false;

			if(rooms[i] != list.end() && rooms[i+1] != list.end())
			{
				result[i] = getOptimal(temp[i], error[i], *(rooms[i]), *(rooms[i+1]));

				if(result[i])
				{
			//		result[i] = checkPair(temp[i], list, rooms[i], rooms[i+1]);

					if(!result[i])
					{
						error[i] = INT_MAX;
					}


				}
			}
		}

		if(result[1]
		&& (temp[1].right - temp[1].left) < 300
		&& error[0] > error[1]
		&& error[2] > error[1]
		)
		{
			*i = temp[1];
			i->itr = i;

			i->next = std::move(rooms[2]->next);

			if(i->next.size())
			{
				i->next.front()->prev.front() = &(*i);
			}

			list.erase(rooms[2]);
		}
		else if(result[0]
		&& temp[0].right - temp[0].left < 300
		&& error[1] > error[0])
		{
			*i = temp[0];
			i->itr = i;

			i->prev = std::move(rooms[0]->prev);

			if(i->prev.size())
			{
				i->prev.front()->next.front() = &(*i);
			}

			list.erase(rooms[0]);
		}
	}

}
