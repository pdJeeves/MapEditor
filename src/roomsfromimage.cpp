#include "roomsfromimage.h"
#include "room.h"
#include <QPoint>
#include <QImage>
#include <QRgb>
#include <list>
#include <map>

uint16_t RoomsFromImage::getBottomOfRun(const QImage & image, uint8_t color, uint16_t x, uint16_t y)
{
	uint16_t _y = y;
	for(_y = y; _y < image.height(); ++_y)
	{
		if(image.pixelIndex(x, _y) != color)
		{
			break;
		}
	}

	if(_y == image.height())
	{
		return _y - 1;
	}

	int N = std::min(_y + SMOOTHING_Y, image.height());
	for(uint16_t z = _y; z < N; ++z)
	{
		if(image.pixelIndex(x, z) == color)
		{
			return getBottomOfRun(image, color, x, z);
		}
	}

	return _y;
}


void RoomsFromImage::insertVertex(std::list<Verticies> & vertex_data, uint16_t color, uint16_t color2, int x, int y, int _y)
{
	if(y == _y)
	{
		return;
	}

	auto best = vertex_data.end();
	int overlap = 0;

	for(auto i = vertex_data.begin(); i != vertex_data.end(); ++i)
	{
		if( color != i->color
		|| color2 != i->color2
		|| i->end < x-1
		|| i->above[x-1] > _y
		|| i->below[x-1] <  y)
		{
			continue;
		}

		if(i->end == x-1)
		{
			auto o = std::min<int>(i->below[x-1], _y) - std::max<int>(i->above[x-1], y);

			if(o > overlap)
			{
				overlap = o;
				best = i;
			}
		}
		else if(i->end == x)
		{
			auto o0 = std::min<int>(i->below[x-1], _y) - std::max<int>(i->above[x-1], y);
			auto o1 = std::min<int>(i->below[x-1], i->below[x]) - std::max<int>(i->above[x-1], i->above[x]);

			if(o0 > o1 && o0 > overlap)
			{
				overlap = o0;
				best = i;
			}
		}
	}

	if(best == vertex_data.end())
	{
		vertex_data.emplace_back(color, color2, x, y, _y);
	}
	else if(best->end == x-1)
	{
		best->push_back(x, y, _y);
	}
	else
	{
		vertex_data.emplace_back(color, color2, x, best->above[x], best->below[x]);

		best->above[x] = y;
		best->below[x] = _y;
	}
}

std::list<Verticies> RoomsFromImage::GenerateEdges(const QImage & image)
{
	std::list<Verticies> vertex_data;

	for(uint16_t x = 0; x < image.width(); ++x)
	{
		uint16_t color = 0, prev_color = 0;

		for(uint16_t y = 0; y < image.height(); ++y, prev_color = color)
		{
			color = image.pixelIndex(x, y);

			if(color == 0 || color == prev_color)
			{
				continue;
			}

			uint16_t _y = getBottomOfRun(image, color, x, y);

			insertVertex(vertex_data, color, color, x, y, _y);

			y = _y-1;
		}
	}

	return vertex_data;
}

/* using transparency for room maps not actually possible because different maps could be used in one metaroom */
std::list<Verticies> RoomsFromImage::getInverseVerts(const std::list<Verticies> & vertex_data)
{
	std::list<Verticies> r;

	for(int x = 0; x < PixelsPerTile; ++x)
	{
		int color1 = 0;

		for(int y = 0; y < PixelsPerTile; )
		{
			auto best1 = vertex_data.end();
			int _y = 128;

			for(auto i = vertex_data.begin(); i != vertex_data.end(); ++i)
			{
				if(i->begin <= x && x <= i->end)
				{
					int t = i->above[x] - y;
					if(0 <= t && t < _y)
					{
						_y = t;
						best1 = i;
					}
				}
			}

			if(best1 == vertex_data.end())
			{
				insertVertex(r, color1, 0, x, y, PixelsPerTile);
				break;
			}

			if(y - _y > 1)
			{
				insertVertex(r, color1, best1->color, x, y, _y);
			}

			color1 = best1->color;
			y = best1->below[x]+1;
		}
	}

	return r;
}

void  RoomsFromImage::optimizeEdges(std::list<Verticies> & vertex_data)
{
	for(auto i = vertex_data.begin(); i != vertex_data.end(); ++i)
	{
		for(auto j = vertex_data.begin(); j != vertex_data.end(); ++j)
		{
			if(j->begin < i->begin && i->begin < j->end)
			{
				auto o = std::min<int>(i->below[i->begin], j->below[i->begin-1])
					   - std::max<int>(i->above[i->begin], j->above[i->begin-1]);
/*
 				if(o > 0)
				{
					i->below[i->begin-1] = i->below[i->begin];
					i->above[i->begin-1] = i->above[i->begin];
					i->begin            -= 1;
				}*/
			}

			if(j->begin < i->end && i->end < j->end)
			{
				auto o = std::min<int>(i->below[i->end], j->below[i->end+1])
					   - std::max<int>(i->above[i->end], j->above[i->end+1]);

				if(o > 0)
				{
					i->below[i->end+1] = i->below[i->end];
					i->above[i->end+1] = i->above[i->end];
					i->end          += 1;
				}
			}
		}
	}

	for(auto i = vertex_data.begin(), j = std::next(i); i != vertex_data.end(); i = j++)
	{
		i->optimize(8, true);

		if(i->optimal.empty())
		{
			vertex_data.erase(i);
			continue;
		}
#if 0
		for(size_t j = 0; j < i->optimal.size(); j += 2)
		{
//MELD
			/*
			if(j+2 < i->optimal.size())
			{
				auto r = i->optimal[j+1];

				if(i->above[r] != 0)
				{
					if(abs((int) i->above[r] - i->above[i->optimal[j+2]]) <= 4)
					{
						i->above[r] = i->above[i->optimal[j+2]];
					}
				}

				if(i->below[r] != 255)
				{
					if(abs((int) i->below[r] - i->below[i->optimal[j+2]]) <= 4)
					{
						i->below[r] = i->below[i->optimal[j+2]];
					}
				}
			}
*/
//THIN_MAN
			/*
			if(j+3 < i->optimal.size())
			{
				if(i->optimal[j+1] - i->optimal[j+3] <= 8)
				{
					i->optimal.erase(i->optimal.begin()+(j+1), i->optimal.begin()+(j+3));
					continue;
				}
			}

			if(0 < j && j+5 < i->optimal.size())
			{
				if(i->optimal[j+1] - i->optimal[j+2] <= 4)
				{
					i->optimal.erase(i->optimal.begin()+(j+1), i->optimal.begin()+(j+3));
				}
			}*/
		}
	#endif
	}
}


std::list<Room> RoomsFromImage::getRoomsFromVertexData(std::list<Verticies> vertex_data)
{
	std::list<Room> rooms;

	for(auto i = vertex_data.begin(); i != vertex_data.end(); ++i)
	{
#if ROOM_LINKED_LIST
		Room * prev = 0L;
#endif

		for(size_t j = 0; j < i->optimal.size(); j += 2)
		{
			auto l = i->optimal[j];
			auto r = i->optimal[j+1];

			rooms.emplace_back(l, j + 2 == i->optimal.size()? r : r + 1, i->above[l], i->above[r], i->below[l], i->below[r], i->color);
#if ROOM_LINKED_LIST
			rooms.back().prev = prev;

			if(prev)
			{
				prev->next = &(rooms.back());
			}
#endif
		}
	}

	return rooms;
}

std::list<Room> RoomsFromImage::getRoomsFromImage(const QImage & image)
{
	auto vert_data = GenerateEdges(image);
	optimizeEdges(vert_data);
	return getRoomsFromVertexData(vert_data);
}

