#ifndef ROOMS_FROM_IMAGE_H
#define ROOMS_FROM_IMAGE_H
#include "verticies.h"
#include "room.h"

class RoomsFromImage
{
	static std::list<Verticies> getInverseVerts(const std::list<Verticies> & vertex_data);
	static void insertVertex(std::list<Verticies> & vertex_data, uint16_t color, uint16_t color2, int x, int y, int _y);
	static std::list<Verticies> GenerateEdges(const QImage & image);
	static void optimizeEdges(std::list<Verticies> & vertex_data);
	static std::list<Room> getRoomsFromVertexData(std::list<Verticies> vertex_data);

	static uint16_t getBottomOfRun(const QImage & image, uint8_t color, uint16_t x, uint16_t y);

public:
	static std::list<Room> getRoomsFromImage(const QImage & image);
};

#endif // VERTICIES_H
