
#ifndef diVcrossUtil_h
#define diVcrossUtil_h 1

#include <QtCore/QSizeF>

#include <iosfwd>
#include <set>
#include <string>
#include <vector>

class FontManager;

namespace VcrossUtil {

void xyclip(int npos, float *x, float *y, float xylim[4]);

std::vector <std::string> split(const std::string,const char,const char);

void plotArrow(const float& x0, const float& y0, const float& dx, const float& dy, bool thinArrows);

float exnerFunction(float p);
float exnerFunctionInverse(float e);

float coriolisFactor(float lat /* radian */);

float pressureFromHeight(float height);
float heightFromPressure(float pressure);

void writeLonEW(std::ostream& out, float lon);
void writeLatNS(std::ostream& out, float lat);

void updateMaxStringWidth(FontManager* fp, float& w, const std::string& txt);
QSizeF charSize(FontManager* fp, const char* txt);
inline QSizeF charSize(FontManager* fp, const std::string& txt)
{ return charSize(fp, txt.c_str()); }

bool parseNameAndArgs(const std::string& def, std::string& func, std::vector<std::string>& vars);

inline bool step_index(int& idx, int step, int max)
{
  int oidx = idx;
  idx = (idx + step + max) % max;
  return idx != oidx;
}

template<typename T>
void set_insert(std::set<T>& s, const std::vector<T>& v)
{
  s.insert(v.begin(), v.end());
}

template<typename T>
void from_set(std::vector<T>& v, const std::set<T>& s)
{
  v.clear();
  v.insert(v.end(), s.begin(), s.end());
}

template<typename T1, typename T2>
void minimize(T1& a, const T2& b)
{
  if (b < a)
    a = b;
}

template<typename T1, typename T2>
void maximize(T1& a, const T2& b)
{
  if (b > a)
    a = b;
}

template<typename T1, typename T2>
void minimaximize(T1& mi, T1& ma, const T2& b)
{
  if (b > ma)
    ma = b;
  if (b < mi)
    mi = b;
}

template<typename T1>
bool value_between(const T1& v, const T1& lim0, const T1& lim1)
{
  if (lim0 <= lim1)
    return lim0 <= v and v <= lim1;
  else
    return lim1 <= v and v <= lim0;
}

template<typename T1>
T1 constrain_value(const T1& v, const T1& lim0, const T1& lim1)
{
  if (lim0 <= lim1) {
    if (v < lim0)
      return lim0;
    else if (lim1 < v)
      return lim1;
    else
      return v;
  } else {
    if (v < lim1)
      return lim1;
    else if (lim0 < v)
      return lim0;
    else
      return v;
  }
}

} // namespace VcrossUtil

#endif // diVcrossUtil_h
