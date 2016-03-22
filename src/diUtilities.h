
#ifndef diUtilities_h
#define diUtilities_h 1

#include <puTools/miTime.h>

#include <map>
#include <string>
#include <vector>

class Plot;
class Rectangle;

namespace diutil {

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

//! return adjusted i so that it is between 0 and available; if repeat, return i % available
int find_index(bool repeat, int available, int i);

//! glob filenames, return matches, or empty if error
inline string_v glob(const std::string& pattern, int glob_flags=0)
{ bool error; return glob(pattern, glob_flags, error); }

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
