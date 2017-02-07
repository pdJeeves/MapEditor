#include "roomsfromimage.h"
#include "room.h"
#include <QPoint>
#include <QImage>
#include <QRgb>
#include <list>
#include <map>

uint16_t get_bottom_of_run(const QImage & image, uint8_t color, uint16_t x, uint16_t y)
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
			return get_bottom_of_run(image, color, x, z);
		}
	}

	return _y;
}

std::list<Verticies> GenerateEdges(const QImage & image)
{

	std::list<Verticies> vertex_data;

	for(uint16_t x = 0; x < image.width(); ++x)
	{
		for(uint16_t y = 0; y < image.height(); ++y)
		{
			auto color = image.pixelIndex(x, y);

			if(color == 0 || (y != 0 && color == image.pixelIndex(x, y-1)))
			{
				continue;
			}

			uint16_t _y = get_bottom_of_run(image, color, x, y);			

			if(_y == y)
			{
				continue;
			}

			auto best = vertex_data.end();
			int overlap = 0;

			for(auto i = vertex_data.begin(); i != vertex_data.end(); ++i)
			{
				if(color != i->color
				|| i->end < x-1
				|| i->above[x-1] > _y
				|| i->below[x-1] <  y)
				{
					continue;
				}

				if(i->end == x-1)
				{
					auto o = std::min(i->below[x-1], _y) - std::max(i->above[x-1], y);

					if(o > overlap)
					{
						overlap = o;
						best = i;
					}
				}
				else if(i->end == x)
				{
					auto o0 = std::min(i->below[x-1], _y) - std::max(i->above[x-1], y);
					auto o1 = std::min(i->below[x-1], i->below[x]) - std::max(i->above[x-1], i->above[x]);

					if(o0 > o1 && o0 > overlap)
					{
						overlap = o0;
						best = i;
					}
				}
			}

			if(best == vertex_data.end())
			{
				vertex_data.emplace_back(image, x, y, _y);
			}
			else if(best->end == x-1)
			{
				best->push_back(x, y, _y);

/*
				best->push_back(x,
					(y == 0 || image.pixelIndex(x, _y-1) == 0)? y : best->above[x-1],
					image.pixelIndex(x, _y) == 0? _y : best->below[x-1]);
*/
			}
			else
			{
				vertex_data.emplace_back(image, x, best->above[x], best->below[x]);

				best->above[x] = y;
				best->below[x] = _y;
			}

			y = _y-1;
		}
	}

	for(auto i = vertex_data.begin(); i != vertex_data.end(); ++i)
	{
		i->optimize(8);
	}

	return vertex_data;
}

std::list<Room> getRoomsFromVertexData(std::list<Verticies> vertex_data)
{
	std::list<Room> rooms;

	for(auto above = vertex_data.begin(); above != vertex_data.end(); ++above)
	{
		if(above->optimal.empty())
		{
			continue;
		}

		if((above->optimal.front() != 0 && above->optimal.back() != 256)
		&& above->optimal.back() - above->optimal.front() <= 32)
		{
			continue;
		}

		for(uint i = 0; i < above->optimal.size(); i += 2)
		{
			auto l = above->optimal[i];
			auto r = above->optimal[i+1];

			if(i+2 < above->optimal.size())
			{
				if(above->above[r] != 0)
				{
					if(abs(above->above[r] - above->above[above->optimal[i+2]]) <= 5)
					{
						above->above[r] = above->above[above->optimal[i+2]];
					}
				}

				if(above->below[r] != 255)
				{
					if(abs(above->below[r] - above->below[above->optimal[i+2]]) <= 5)
					{
						above->below[r] = above->below[above->optimal[i+2]];
					}
				}

				if(r - l <= 4)
				{
					above->above[l] = above->above[above->optimal[i+2]] ;
					above->below[l] = above->below[above->optimal[i+2]] ;
					above->optimal[i+2] = l;
					continue;
				}
			}

			rooms.emplace_back(l, i + 2 == above->optimal.size()? r : r + 1, above->above[l], above->above[r], above->below[l], above->below[r], above->color);
		}
	}

	return rooms;
}

std::list<Room> getRoomsFromImage(const QImage & image)
{
	return getRoomsFromVertexData(GenerateEdges(image));
}


