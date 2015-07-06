
#ifndef diGlUtilities_h
#define diGlUtilities_h 1

#include <QPolygonF>

#include <string>
#include <vector>

class Plot;
class DiGLPainter;
class DiPainter;

namespace diutil {

enum MapValuePosition {
  map_none, map_left, map_bottom, map_right, map_top, map_all
};

//! convert position "bottom", left" etc to MapValuePosition
MapValuePosition mapValuePositionFromText(const std::string& p);

/// Lat/Lon Value Annotation with position on map
struct MapValueAnno {
  std::string t;
  float x;
  float y;
  MapValueAnno(const std::string& tt, float xx, float yy)
    : t(tt), x(xx), y(yy) { }
};
typedef std::vector<MapValueAnno> MapValueAnno_v;

inline size_t index(int width, int ix, int iy)
{
  return iy * width + ix;
}

bool is_undefined(float v);

class PolylinePainter {
public:
  PolylinePainter(DiPainter* p)
    : mPainter(p) { }
  void reserve(size_t s)
    { mPolyline.reserve(s); }

  PolylinePainter& add(float vx, float vy)
    { mPolyline << QPointF(vx, vy); return *this; }

  PolylinePainter& add(const float* x, const float* y, size_t idx)
    { return add(x[idx], y[idx]); }

  PolylinePainter& addValid(float vx, float vy);

  PolylinePainter& addValid(const float* x, const float* y, size_t idx)
    { return addValid(x[idx], y[idx]); }

  void draw();

private:
  DiPainter* mPainter;
  QPolygonF mPolyline;
};

void xyclip(int npos, const float *x, const float *y, const float xylim[4],
    MapValuePosition anno_position, const std::string& anno,
    MapValueAnno_v& anno_positions, DiGLPainter* gl);

void xyclip(int npos, const float *x, const float *y, const float xylim[4],
    DiGLPainter* gl);

} // namespace diutil

#endif // diGlUtilities_h
