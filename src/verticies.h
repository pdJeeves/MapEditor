#ifndef VERTICIES_H
#define VERTICIES_H
#include <QRgb>
#include <QImage>
#include <vector>
#include <map>

class MemoData;

#define SMOOTHING_Y 24
#define SMOOTHING_X 24


struct Verticies
{
private:
	MemoData * OptimizeLine(std::map<int, MemoData*> & map, const int min, const int max, const int depth);
	const QImage & image;

public:
	Verticies(const QImage & image, int x, int y0, int y1);
	void push_back(int x, int y);
	void push_back(int x, int y0, int y1);

	uint16_t color;
	uint16_t begin;
	uint16_t end;
	uint16_t above[258];
	uint16_t below[258];

	std::vector<uint16_t> optimal;

	void optimize(int depth);
	void smooth();
	int size() const { return end - begin; }

	int getStartingPoint(int x) const;
	int getEndingPoint(int x) const;
	int getHeight(int x) const;
};


struct MemoData
{
private:
	double _error;
	Verticies & verts;

public:
	MemoData(Verticies & verts, int begin, int end);

	MemoData * left;
	MemoData * right;

	int min, max;

	double error(int depth) const;
	double area(int depth) const;
	int cost(int depth) const;
	void getIndicies(int depth) const;
};


#endif // VERTICIES_H
