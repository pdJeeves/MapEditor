#include "verticies.h"

#ifdef __GNUC__
#define PURE __attribute((pure))
#else
#define PURE
#endif


static
double PURE getError(const uint16_t * heights, int min, int max)
{
	double m = (heights[max] - heights[min])  / (double) (max - min);
	double b = heights[min] - m * min;
	double error = 0;

	for(int x = min; x <= max; ++x)
	{
		auto err = fabs(m*x + b - heights[x]);

		if(heights[x] == 0  || heights[x] == MAX_HEIGHT)
		{
			err *= PixelsPerTile;
		}

		error += err*err;
	}

	return error + (PixelsPerTile - (max - min));
}

static
double PURE getArea(const uint16_t * above, uint16_t * below, int min, int max)
{
	double m1 = (above[max] - above[min])  / (double) (max - min);
	double b1 = above[min] - m1 * min;
	double m2 = (below[max] - below[min])  / (double) (max - min);
	double b2 = below[min] - m2 * min;

	return	((max * m1/2 + b1)*max - (min * m1/2 + b1)*min) -
			((max * m2/2 + b2)*max - (min * m2/2 + b2)*min);
}

MemoData::MemoData(Verticies & verts, int begin, int end, bool error_type)
	: verts(verts)
{
	left = 0L;
	right = 0L;
	min = begin;
	max = end;

	_error = getError(verts.above, min, max) + getError(verts.below, min, max);
}

double MemoData::error(int depth) const
{
	if(left && right && depth)
	{
		return left->error(depth-1) + right->error(depth-1);
	}
	else if(left && !right)
	{
		return left->error(depth) + verts.missing(left->max+1, max);
	}
	else if(!left && right)
	{
		return verts.missing(min, right->min-1) + right->error(depth);
	}
	else
	{
		return _error;
	}
}

double MemoData::area(int depth) const
{
	if(left && depth)
	{
		return left->area(depth-1) + right->area(depth-1);
	}
	else if(left && !right)
	{
		return left->area(depth);
	}
	else if(right && !left)
	{
		return right->area(depth);
	}
	else
	{
		return getArea(verts.above, verts.below, min, max);
	}
}

int MemoData::cost(int depth) const
{
	if(left && right && depth)
	{
		return left->cost(depth-1) + right->cost(depth-1) + 1;
	}
	else if(left && !right)
	{
		return left->cost(depth);
	}
	else if(!left && right)
	{
		return right->cost(depth);
	}
	else
	{
		return 1;
	}
}

void MemoData::getIndicies(int depth) const
{
	if(left && right && depth)
	{
		left->getIndicies(depth -1);
		right->getIndicies(depth-1);
	}
	else if(left && !right)
	{
		left->getIndicies(depth);
	}
	else if(!left && right)
	{
		right->getIndicies(depth);
	}
	else
	{
		verts.optimal.push_back(min);
		verts.optimal.push_back(max);
	}
}

Verticies::Verticies(uint16_t color, uint16_t color2, int x, int y0, int y1)
	: image(image),
	color(color),
	color2(color2),
	begin(x),
	end(x)
{
	memset(above, 0, sizeof(above));
	memset(below, 0, sizeof(below));
	push_back(x, y0, y1);
}

void Verticies::push_back(int x, int y)
{
	above[x] = y;
	end = x;

	for(below[x] = y; below[x] < PixelsPerTile; ++below[x])
	{
		if(image.pixelIndex(x, below[x]) != color)
		{
			break;
		}
	}
}

void Verticies::push_back(int x, int y0, int y1)
{
	above[x] = y0;
	below[x] = y1;
	end = x;
}

MemoData * Verticies::OptimizeLine(std::map<int, MemoData*> & memo, const int min, const int max, const int depth, const bool error_type, const bool at_begin, bool at_end)
{
	int key = min << 16 | max;
	auto index = memo.find(key);

	if(index != memo.end())
	{
		return index->second;
	}

	auto data = new MemoData(*this, min, max, error_type);
	memo.insert(std::make_pair(key, data));

	if(data->error(depth) < 8 || depth == 0)
	{
		return data;
	}

	double self_error = data->error(depth) / 2;
	int self_cost = data->cost(depth);
	int self_distance = PixelsPerTile*PixelsPerTile;

	for(int x = min + 2; x < max; ++x)
	{
		do {
			if(x == max-1)
			{
				break;
			}
#if 1
			if(above[x-1] == above[x+1]
			&& below[x-1] == below[x+1])
			{
				continue;
			}
#endif

			break;
		} while(++x < max);

		auto l = OptimizeLine(memo, min, x-1, depth - 1, error_type, at_begin, false);
		auto r = OptimizeLine(memo, x, max, depth - 1, error_type, false, at_end);
		const double l_error     = l->error(depth);
		const double r_error     = r->error(depth);
		double split_error = l_error + r_error;

		const int l_cost = l->cost(depth);
		const int r_cost = r->cost(depth);

		int	   split_distance = abs((max - x) - (x - min));

		if(split_error < self_error
		|| (split_error == self_error && split_distance < self_distance))
		{
			self_error = split_error;
			self_distance = split_distance;
			self_cost  = l_cost + r_cost;

			data->left = l;
			data->right = r;
		}

		if(at_begin && !at_end)
		{
			const double l_missing = missing(min, x-1);
			split_error = l_missing + r_error;

			if(split_error < self_error
			|| split_error == self_error && split_distance > self_distance)
			{
				self_error = split_error;
				self_distance = split_distance;
				self_cost  = r_cost;

				data->left = 0L;
				data->right = r;
			}
		}
		else if(at_end && !at_begin)
		{
			const double r_missing = missing(x, max);
			split_error = l_error + r_missing;

			if(split_error < self_error
			|| split_error == self_error && split_distance > self_distance)
			{
				self_error = split_error;
				self_distance = split_distance;
				self_cost  = l_cost;

				data->left = l;
				data->right = 0L;
			}
		}
	}

	return data;
}

static
void smooth(uint16_t * heights, int begin, int end)
{
	int x, _x;
	for(x = begin; x < end; ++x)
	{
		if(heights[x] != heights[x+1])
		{
			int N = std::min(end, x + SMOOTHING_X);
			for(_x = x+1; _x <= N; ++_x)
			{
				if(heights[x] == heights[_x])
				{
					for(int i = x; i < _x; ++i)
					{
						heights[i] = heights[x];
					}
					x = _x-1;
					break;
				}
			}
		}
	}
}


void Verticies::smooth()
{/*
	if(begin != 0)
	{
		//lop off edges
		for(; begin < end; ++begin)
		{
			if(below[begin] - above[begin] > SMOOTHING_Y
			|| above[begin] == 0 || below[begin] == 256)
			{
				break;
			}
		}
	}

	if(end != 256-1)
	{
		for(; begin < end; --end)
		{
			if(below[end] - above[end] > SMOOTHING_Y
			|| above[end] == 0 || below[end] == 256)
			{
				break;
			}
		}
	}*/

	::smooth(above, begin, end);
	::smooth(below, begin, end);
}

void Verticies::optimize(int depth, const bool error_type)
{
	smooth();

	if(end - begin < 8)
	{
		return;
	}

	std::map<int, MemoData*> map;
	MemoData * opt = 0L;

	if(optimal.empty())
	{
		opt = OptimizeLine(map, begin, end, depth, error_type, true, true);
	}
	else
	{
		std::list<MemoData*> list;

		if(optimal.front() != begin)
		{
			list.push_back(OptimizeLine(map, begin, optimal.front(), depth, error_type, begin != 0, false));
		}

		for(size_t i = 1; i < optimal.size(); ++i)
		{
			list.push_back(OptimizeLine(map, optimal[i-1], optimal[i], depth, error_type, false, false));
		}

		if(optimal.back() != end)
		{
			list.push_back(OptimizeLine(map, optimal.back(), end, depth, error_type, false, end != 255));
		}

		optimal.clear();

		while(list.size() > 1)
		{
			for(auto i = list.begin(); i != list.end(); ++i)
			{
				auto j = std::next(i);

				if(j != list.end())
				{
					auto data = new MemoData(*this, (*i)->min, (*j)->max,error_type);
					map.insert(std::make_pair(((*i)->min << 16) | (*j)->max, data));

					data->left = *i;
					data->right = *j;

					*i = data;
					list.erase(j);
				}
			}
		}

		opt = list.front();
	}

	opt->getIndicies(depth);

	for(auto i = map.begin(); i != map.end(); ++i)
	{
		delete i->second;
	}
}

int Verticies::getStartingPoint(int x) const
{
	for(uint32_t i = 0; i < optimal.size(); ++i)
	{
		if(optimal[i] == x)
		{
			return i;
		}
		else if(optimal[i] > x)
		{
			return i-1;
		}
	}

	return 0;
}

int Verticies::getEndingPoint(int x) const
{
	for(uint32_t i = 0; i < optimal.size(); ++i)
	{
		if(optimal[i] >= x)
		{
			return i;
		}
	}

	return optimal.size()-1;
}

double Verticies::missing(int min, int max) const
{
	double error = 0;

	for(int x = min; x <= max; ++x)
	{
		auto err = below[x] - above[x];

		if(above[x] == 0  || below[x] == MAX_HEIGHT)
		{
			err = PixelsPerTile;
		}

		error += err;
	}

	return error + (PixelsPerTile - (max - min));
}

int Verticies::getHeight(int x) const
{/*
	for(auto i = 1; i < optimal.size(); ++i)
	{
		if(optimal[i] == x)
		{
			return heights[x];
		}
		else if(optimal[i] > x)
		{
			double m = (heights[optimal[i]] - heights[optimal[i-1]]) / (double) (optimal[i] - optimal[i-1]);
			double b = heights[optimal[i]] - m*optimal[i];

			return m * x + b;
		}
	}
*/
	return x;
}
