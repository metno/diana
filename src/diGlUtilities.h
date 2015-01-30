
#ifndef diGlUtilities_h
#define diGlUtilities_h 1

#include <string>
#include <vector>

class Plot;

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

void xyclip(int npos, const float *x, const float *y, const float xylim[4],
    MapValuePosition anno_position, const std::string& anno, MapValueAnno_v& anno_positions);

void xyclip(int npos, const float *x, const float *y, const float xylim[4]);

} // namespace diutil

#endif // diGlUtilities_h
