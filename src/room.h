#ifndef ROOM_H
#define ROOM_H
#include <utility>
#include <vector>
#include <cstdint>
#include <cstring>
#include <list>

#ifdef __GNUC__
#define PACK(a, b) a __attribute__((__packed__)) b
#define PACK_TEMPLATE(t, a, b) template< t > a __attribute__((__packed__)) b
#define UNPACK
#else
#define PACK   __pragma(pack(push, 1));  a b
#define PACK_TEMPLATE(t, a, b)  __pragma(pack(push, 1)); template< t > a b
#define UNPACK __pragma(pack(pop));
#endif

template<typename T>
struct Room_t
{
	Room_t()
	{
		memset(this, 0, sizeof(Room_t<T>));
	}
	Room_t(T left, T right, T top_left, T top_right, T bottom_left, T bottom_right, T room_type) :
		left(left),
		right(right),
		top_left(top_left),
		top_right(top_right),
		bottom_left(bottom_left),
		bottom_right(bottom_right),
		room_type(room_type)
	{
	}

	T left, right, top_left, top_right, bottom_left, bottom_right, room_type;

	static inline bool getIntercept(float & r, int r_px, int r_py, int r_dx, int r_dy, int s_px, int s_py, int s_dx, int s_dy)
	{
		s_dy -= s_py;
		s_dx -= s_px;

		float t = (s_dy*r_dx - s_dx*r_dy);

		if(t)
		{
			t = (r_dy*(s_px-r_px) + r_dx*(r_py-s_py)) / t;

			if(0 <= t && t <= 1)
			{
				t = (s_py+s_dy*t-r_py)/r_dy;

				if(t > 0 && t < r)
				{
					r = t;
					return true;
				}
			}
		}

		return false;
	}

	static inline float getIntersection(float x, int r_px, int r_py, float r_dx, float r_dy)
	{
		if(x <= r_px)
		{
			return r_py;
		}
		else if(x >= r_dx)
		{
			return r_dy;
		}

		r_dx -= r_px;
		r_dy -= r_py;

		x = (x - r_px) / r_dx;
		return x*r_dy + r_py;
	}

	//convert to double for math
	inline double width() const
	{
		return right - left;
	}

	inline float getBottomIntersection(int x) const
	{
		if(this == 0) { return 0; }
		return getIntersection(x, left, bottom_left, right, bottom_right);
	}

	inline float getTopIntersection(int x) const
	{
		if(this == 0) { return 255; }
		return getIntersection(x, left, top_left, right, top_right);
	}

	inline bool getTopIntercept(float & r, int r_px, int r_py, int r_dx, int r_dy) const
	{
		if(this == 0) { return 255; }
		return getIntercept(r, r_px, r_py, r_dx, r_dy, left, top_left, right, top_right);
	}

	inline bool getBottomIntercept(float & r, int r_px, int r_py, int r_dx, int r_dy) const
	{
		if(this == 0) { return 0; }
		return getIntercept(r, r_px, r_py, r_dx, r_dy, left, bottom_left, right, bottom_right);
	}

	inline bool doesContain(int x, int y) const
	{
		return left < x && x < right && getTopIntersection(x) < y && y < getBottomIntersection(x);
	}

	bool operator==(const Room_t<T> & it) const
	{
		return memcmp(this, &it, sizeof(Room_t<T>)) == 0;
	}
	bool operator!=(const Room_t<T> & it) const
	{
		return !(*this == it);
	}

};

class Room : public Room_t<int>
{
typedef Room_t<int> super;
public:
	template<class...Args>
	Room(Args...args) : super(std::forward<Args>(args)...) {}
	Room(const Room & it) : super(it) {}
	Room(Room && it) : super(it) {}

	const Room & operator=(const Room & it)
	{
		*(dynamic_cast<Room_t<int>*>(this)) = it;
		return *this;
	}

	std::list<Room>::iterator itr;

	std::vector<Room *> above;
	std::vector<Room *> below;
	std::vector<Room *> prev;
	std::vector<Room *> next;
	std::vector<Room> original;

	bool isFullRoom() const
	{
		return (left | top_left | top_right) == 0
			&& right == 255
			&& bottom_left == 255
			&& bottom_right == 255;
	}
};

#endif // ROOM_H
