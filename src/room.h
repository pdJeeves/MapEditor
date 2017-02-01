#ifndef ROOM_H
#define ROOM_H
#include <cstdint>
#include <cstring>

struct __attribute((packed)) Room
{
	Room()
	{
		memset(this, 0, sizeof(Room));
	}
	Room(uint8_t left, uint8_t right, uint8_t top_left, uint8_t top_right, uint8_t bottom_left, uint8_t bottom_right, uint8_t room_type) :
		left(left),
		right(right),
		top_left(top_left),
		top_right(top_right),
		bottom_left(bottom_left),
		bottom_right(bottom_right),
		room_type(room_type)
	{
	}

	uint8_t left, right, top_left, top_right, bottom_left, bottom_right, room_type;

	bool operator==(const Room & it) const
	{
		return memcmp(this, &it, sizeof(Room)) == 0;
	}
	bool operator!=(const Room & it) const
	{
		return !(*this == it);
	}
};

#endif // ROOM_H
