#include "airfromroomsnp.h"
#include "linkrooms.h"
#include <climits>

bool AirFromRoomsNP::getOptimalLine(std::vector<QPoint> & r, std::vector<QPoint> & other, QPoint intercept, double & m, double & b, bool direction)
{
	double error = INT_MAX;
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

			if(diff < -1 || y < 0 || y > 256
			/*
			|| ( direction && other[i].y() < y)
			|| ( !direction && other[i].y() > y) // */
			)
			{
				err = error;
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

bool AirFromRoomsNP::getOptimal(Room & result, std::vector<Room*> r, int begin, int end)
{
	if(r.size() == 1)
	{
		result = Room(*(r.front()));
		return true;
	}

	QPoint min(255, 255);
	QPoint max(  0,   0);

	std::vector<QPoint> top;
	std::vector<QPoint> bottom;

	for(auto i = r.begin(); i != r.end() && (*i)->left < end; ++i)
	{
		if((*i)->right < begin)
		{
			continue;
		}

		if(begin <= (*i)->left && (*i)->left <= end)
		{
			top.push_back(QPoint((*i)->left, (*i)->top_left));
			bottom.push_back(QPoint((*i)->left, (*i)->bottom_left));
		}
		else if((*i)->left < begin && begin < (*i)->right)
		{
			top.push_back(QPoint(begin, (*i)->getTopIntersection(begin)));
			bottom.push_back(QPoint(begin, (*i)->getBottomIntersection(begin)));
		}

		if(min.y() > top.back().y())
		{
			min = top.back();
		}
		if(max.y() < bottom.back().y())
		{
			max = bottom.back();
		}

		if(begin <= (*i)->right && (*i)->right <= end)
		{
			top.push_back(QPoint((*i)->right, (*i)->top_right));
			bottom.push_back(QPoint((*i)->right, (*i)->bottom_right));
		}
		else if((*i)->left < end && end < (*i)->right)
		{
			top.push_back(QPoint(begin, (*i)->getTopIntersection(end)));
			bottom.push_back(QPoint(begin, (*i)->getBottomIntersection(end)));
		}

		if(min.y() > top.back().y())
		{
			min = top.back();
		}
		if(max.y() < bottom.back().y())
		{
			max = bottom.back();
		}
	}

	double m1,b1,m2,b2;

	if(getOptimalLine(top, bottom, min, m1, b1, true)
	&& getOptimalLine(bottom, top, max, m2, b2, false))
	{
		result = Room(begin, end, begin*m1 + b1, end*m1 + b1, begin*m2 + b2, end*m2 + b2, r.front()->room_type);
		return true;
	}

	return false;
}

std::vector<std::vector<Room*> > AirFromRoomsNP::getRoomLists(std::list<Room> & rooms)
{
	std::vector<std::vector<Room*> > r;

	for(auto i = rooms.begin(); i != rooms.end(); ++i)
	{
		if(i->prev.size() == 1
		&& i->prev.front()->next.size() == 1)
		{
			continue;
		}

		std::vector<Room*> temp;
		temp.push_back(&(*i));

		while(temp.back()->next.size() == 1
		   && temp.back()->next.front()->room_type  == temp.back()->room_type
		   && temp.back()->next.front()->prev.size() == 1)
		{
			temp.push_back(temp.back()->next.front());
		}

		r.push_back(temp);
	}

	return r;
}

void AirFromRoomsNP::calculateLine(float & m, float & b, int x1, int x2, int y1, int y2)
{
	m = (x1 - x2) / (double) (y1 - y2);
	b = y1 - m*x1;
}

bool AirFromRoomsNP::processRoomLists(std::list<Room> & rooms, const std::vector<std::vector<Room*> > & lists)
{
	std::list<Room> r;

	for(auto i = lists.begin(); i != lists.end(); ++i)
	{
		Room result;
		if(!getOptimal(result, *i, (*i).front()->left, (*i).back()->right))
		{
			return false;
		}

		r.push_back(result);
	}
/*
	for(auto i = r.begin(); i != r.end(); ++i)
	{
		float y;
		float m[2][2];
		float b[2][2];

		calculateLine(m[0][0], b[0][0], i->left, i->top_left   , i->right, i->top_right);
		calculateLine(m[0][1], b[0][1], i->left, i->bottom_left, i->right, i->bottom_right);

		for(auto j = std::next(i); j != r.end(); ++i)
		{
			if(i->right <= j->left
			|| j->right <= i->left)
			{
				continue;
			}

			y = j->left * m[0][1] + b[0][1];

			if(j->top_left < y && y < j->bottom_left)
			{
				return false;
			}

			y = j->right * m[0][1] + b[0][1];

			if(j->top_right < y && y < j->bottom_right)
			{
				return false;
			}

//			calculateLine(m[1][0], b[1][0], j->left, j->top_left   , j->right, j->top_right);
//			calculateLine(m[1][1], b[1][1], j->left, j->bottom_left, j->right, j->bottom_right);
		}
	}
*/
	rooms = std::move(r);

	return true;
}

void AirFromRoomsNP::processTiles(std::vector<std::list<Room> > & rooms)
{
	LinkRooms::interConnectRooms(rooms);

	for(auto i = rooms.begin(); i != rooms.end(); ++i)
	{
		std::vector<std::vector<Room *> > r = getRoomLists(*i);
		processRoomLists(*i, r);
	}
}

