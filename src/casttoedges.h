#ifndef CASTTOEDGES_H
#define CASTTOEDGES_H
#include <QPoint>

class VerticiesFromImage;
class QImage;
typedef std::pair<int, int> Run_t;
typedef std::vector<Run_t>  RunList_t;
typedef std::vector<RunList_t> LengthEncoded_t;

struct CastToEdges
{
	static float doCast(const QVector<QPoint> & verts, QPoint r_p, QPointF r_d, float length);

public:
	static float doCast(const VerticiesFromImage & edges, QPoint r_p, QPointF r_d);
	static float doCast(const QImage & image, QPoint r_p, QPointF r_d);

	static float doCast(const LengthEncoded_t & list, QPoint r_p, bool end);
	static std::pair<int, int> getHeight(const LengthEncoded_t & list, QPoint r_p);

};

#endif // CASTTOEDGES_H
