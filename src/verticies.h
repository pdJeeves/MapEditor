#ifndef VERTICIES_H
#define VERTICIES_H
#include <QRgb>
#include <QImage>
#include <vector>
#include <map>

class MemoData;

#define SMOOTHING_Y 12
#define SMOOTHING_X 12

#define PixelsPerTile 128
#define MAX_HEIGHT    (PixelsPerTile-1)

class Verticies
{
private:
	MemoData * OptimizeLine(std::map<int, MemoData*> & map, const int min, const int max, const int depth, const bool error_type, const bool at_begin, bool at_end);
	const QImage & image;

public:
	Verticies(uint16_t color, uint16_t color2, int x, int y0, int y1);
	void push_back(int x, int y);
	void push_back(int x, int y0, int y1);

	uint16_t color;
	uint16_t color2;
	uint16_t begin;
	uint16_t end;
	uint16_t above[PixelsPerTile+2];
	uint16_t below[PixelsPerTile+2];

	std::vector<uint16_t> optimal;

	void optimize(int depth, const bool error_type);
	void smooth();
	int size() const { return end - begin; }

	int getStartingPoint(int x) const;
	int getEndingPoint(int x) const;
	int getHeight(int x) const;
	double missing(int min, int max) const;
};


class MemoData
{
private:
	double _error;
	Verticies & verts;

public:
	MemoData(Verticies & verts, int begin, int end, bool error_type);

	MemoData * left;
	MemoData * right;

	int min, max;

	double error(int depth) const;
	double area(int depth) const;
	int cost(int depth) const;
	void getIndicies(int depth) const;
};


#endif // VERTICIES_H
