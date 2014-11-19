
#ifndef diUtilities_h
#define diUtilities_h 1

#include <puTools/miTime.h>

#include <map>
#include <string>
#include <vector>

class Plot;

namespace diutil {

class was_enabled {
  typedef std::map<std::string, bool> key_enabled_t;
  key_enabled_t key_enabled;
public:
  void save(const Plot* plot, const std::string& key);
  void restore(Plot* plot, const std::string& key) const;
};

template<class C>
void delete_all_and_clear(C& container)
{
  for (typename C::iterator it=container.begin(); it!=container.end(); ++it)
    delete *it;
  container.clear();
}

typedef std::vector<std::string> string_v;

//! glob filenames, return matches, or empty if error
string_v glob(const std::string& pattern, int glob_flags, bool& error);

//! glob filenames, return matches, or empty if error
inline string_v glob(const std::string& pattern, int glob_flags=0)
{ bool error; return glob(pattern, glob_flags, error); }

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

bool startswith(const std::string& txt, const std::string& start);

namespace detail {
void append_chars_split_newline(string_v& lines, const char* buffer, size_t nbuffer);
}

bool getFromFile(const std::string& filename, string_v& lines);
bool getFromHttp(const std::string &url, string_v& lines);

bool getFromAny(const std::string &url_or_filename, string_v& lines);

inline int float2int(float f)
{ return (int)(f > 0.0 ? f + 0.5 : f - 0.5); }

inline int ms2knots(float ff)
{ return (float2int(ff*3600.0/1852.0)); }

inline float knots2ms(float ff)
{ return (ff*1852.0/3600.0); }

/*! replace reftime by refhour and refoffset
  refoffset is 0 today, -1 yesterday etc independent of time of the day
*/
void replace_reftime_with_offset(std::string& pstr, const miutil::miDate& nowdate);

/*! Make list of numbers around 'number'.
 */
std::vector<std::string> numberList(float number, const float* enormal);

} // namespace diutil

#endif // diUtilities_h
