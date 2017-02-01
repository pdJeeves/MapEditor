#include "verticies.h"

static
double __attribute((pure)) getError(const uint16_t * heights, int min, int max)
{
	double m = (heights[max] - heights[min])  / (double) (max - min);
	double b = heights[min] - m * min;
	double error = 0;

	bool r = heights[max] == heights[min] && (heights[max] == 255 || heights[max] == 0);

	for(int x = min; x <= max; ++x)
	{
		auto err = fabs(m*x + b - heights[x]);

		if(r && heights[x] != heights[max])
		{
			err = 256;
		}

		error += err*err;
	}

	return error + (256 - (max - min));
}

static
double __attribute((pure)) getArea(const uint16_t * above, uint16_t * below, int min, int max)
{
	double m1 = (above[max] - above[min])  / (double) (max - min);
	double b1 = above[min] - m1 * min;
	double m2 = (below[max] - below[min])  / (double) (max - min);
	double b2 = below[min] - m2 * min;

	return	((max * m1/2 + b1)*max - (min * m1/2 + b1)*min) -
			((max * m2/2 + b2)*max - (min * m2/2 + b2)*min);
}

MemoData::MemoData(Verticies & verts, int begin, int end)
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
	if(left && depth)
	{
		return left->error(depth-1) + right->error(depth-1);
	}

	return _error;
}

double MemoData::area(int depth) const
{
	if(left && depth)
	{
		return left->area(depth-1) + right->area(depth-1);
	}

	return getArea(verts.above, verts.below, min, max);
}

int MemoData::cost(int depth) const
{
	if(left && depth)
	{
		return left->cost(depth-1) + right->cost(depth-1) + 1;
	}
	return 1;
}

void MemoData::getIndicies(int depth) const
{
	if(left && depth)
	{
		left->getIndicies(depth -1);
		right->getIndicies(depth-1);
	}
	else
	{
		verts.optimal.push_back(min);
		verts.optimal.push_back(max);
	}
}

Verticies::Verticies(const QImage & image, int x, int y0, int y1)
	: image(image),
	color(image.pixelIndex(x, y0)),
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

	for(below[x] = y; below[x] < 256; ++below[x])
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

MemoData * Verticies::OptimizeLine(std::map<int, MemoData*> & memo, const int min, const int max, const int depth)
{
	int key = min << 16 | max;
	auto index = memo.find(key);

	if(index != memo.end())
	{
		return index->second;
	}

	auto data = new MemoData(*this, min, max);
	memo.insert(std::pair<int, MemoData*>(key, data));

	if(data->error(depth) < 8 || depth == 0)
	{
		return data;
	}

	double self_error = data->error(depth) / 2;
	int self_cost = data->cost(depth);
	int self_distance = 65536;

	for(int x = min + 2; x < max; ++x)
	{
		do {
			if(x == max-1)
			{
				break;
			}

			if(above[x] == above[x+1]
			&& above[x] == above[x-1]
			&& below[x] == below[x+1]
			&& below[x] == below[x-1])
			{
				continue;
			}

			break;
		} while(++x < max);

		auto l = OptimizeLine(memo, min, x-1, depth - 1);
		auto r = OptimizeLine(memo, x, max, depth - 1);
		double split_error = l->error(depth) + r->error(depth);
		int    split_cost  = l->cost(depth) + r->cost(depth);
		int	   split_distance = abs((max - x) - (x - min));

		if(split_error < self_error
		|| (split_error == self_error
		&& split_distance < self_distance))
		{
			self_error = split_error;
			self_distance = split_distance;
			self_cost  = split_cost;

			data->left = l;
			data->right = r;
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

void Verticies::optimize(int depth)
{
	smooth();

	if(end - begin < 8)
	{
		return;
	}

	std::map<int, MemoData*> map;
	auto opt = OptimizeLine(map, begin, end, depth);

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
