
#include "diUtilities.h"

#include "diPlot.h"

#include <QApplication>
#include <QComboBox>

#include <puCtools/glob_cache.h>
#include <puCtools/puCglob.h>
#include <puTools/miStringFunctions.h>

#include <curl/curl.h>

#include <cstring>
#include <fstream>

#define MILOGGER_CATEGORY "diana.Utilities"
#include <miLogger/miLogging.h>

namespace diutil {

void was_enabled::save(const Plot* plot, const std::string& key)
{
  key_enabled[key] = plot->isEnabled();
}

void was_enabled::restore(Plot* plot, const std::string& key) const
{
  key_enabled_t::const_iterator it = key_enabled.find(key);
  if (it != key_enabled.end())
    plot->setEnabled(it->second);
};

// ========================================================================

string_v glob(const std::string& pattern, int glob_flags, bool& error)
{
  glob_t globBuf;
  error = (glob(pattern.c_str(), glob_flags, 0, &globBuf) != 0);

  string_v matches;
  if (not error)
    matches = string_v(globBuf.gl_pathv, globBuf.gl_pathv + globBuf.gl_pathc);

  globfree_cache(&globBuf);
  return matches;
}

// ========================================================================

MapValuePosition mapValuePositionFromText(const std::string& p)
{
  const std::string pos = miutil::to_lower(p);
  if (pos == "left")
    return diutil::map_left;
  if (pos == "bottom")
    return diutil::map_bottom;
  if (pos == "right")
    return diutil::map_right;
  if (pos == "top")
    return diutil::map_top;
  if (pos == "both")
    return diutil::map_all;

  //obsolete syntax
  return (MapValuePosition)(atoi(pos.c_str()) + 1); // +1: MapValuePosition has map_none first
}

// ========================================================================

void xyclip(int npos, const float *x, const float *y, const float xylim[4],
    MapValuePosition anno_position, const std::string& anno, MapValueAnno_v& anno_positions)
{
  // ploting part(s) of the continuous line that is within the given
  // area, also the segments between 'neighboring point' both of which are
  // Outside the area.
  // (Color, line type and thickness must be set in advance)
  //
  // Graphics: OpenGL
  //
  //  input:
  //  ------
  //  x(npos),y(npos): line with 'npos' points (npos>1)
  //  xylim(1-4):      x1,x2,y1,y2 limits of given area

  int nint, nc, n, i, k1, k2;
  float xa, xb, ya, yb, x1, x2, y1, y2;
  float xc[4], yc[4];

  if (npos < 2)
    return;

  xa = xylim[0];
  xb = xylim[1];
  ya = xylim[2];
  yb = xylim[3];

  float xoffset = (xb - xa) / 200.0;
  float yoffset = (yb - ya) / 200.0;

  float xx = 0, yy = 0;
  if (x[0] < xa || x[0] > xb || y[0] < ya || y[0] > yb) {
    k2 = 0;
  } else {
    k2 = 1;
    nint = 0;
    xx = x[0];
    yy = y[0];
  }

  for (n = 1; n < npos; ++n) {
    k1 = k2;
    k2 = 1;

    if (x[n] < xa || x[n] > xb || y[n] < ya || y[n] > yb)
      k2 = 0;

    // check if 'n' and 'n-1' are inside area
    if (k1 + k2 == 2)
      continue;

    // k1+k2=1: point 'n' or 'n-1' are outside
    // k1+k2=0: checks if two neighboring points which are both outside the area
    //          still has a part of the line within the area.

    x1 = x[n - 1];
    y1 = y[n - 1];
    x2 = x[n];
    y2 = y[n];

    // checking if the 'n-1' and 'n' is outside at the same side
    if (k1 + k2 == 0 && ((x1 < xa && x2 < xa) || (x1 > xb && x2 > xb) || (y1
        < ya && y2 < ya) || (y1 > yb && y2 > yb)))
      continue;

    // checks all intersection possibilities
    nc = -1;
    if (x1 != x2) {
      nc++;
      xc[nc] = xa;
      yc[nc] = y1 + (y2 - y1) * (xa - x1) / (x2 - x1);
      if (yc[nc] < ya || yc[nc] > yb || (xa - x1) * (xa - x2) > 0.)
        nc--;
      nc++;
      xc[nc] = xb;
      yc[nc] = y1 + (y2 - y1) * (xb - x1) / (x2 - x1);
      if (yc[nc] < ya || yc[nc] > yb || (xb - x1) * (xb - x2) > 0.)
        nc--;
    }
    if (y1 != y2) {
      nc++;
      yc[nc] = ya;
      xc[nc] = x1 + (x2 - x1) * (ya - y1) / (y2 - y1);
      if (xc[nc] < xa || xc[nc] > xb || (ya - y1) * (ya - y2) > 0.)
        nc--;
      nc++;
      yc[nc] = yb;
      xc[nc] = x1 + (x2 - x1) * (yb - y1) / (y2 - y1);
      if (xc[nc] < xa || xc[nc] > xb || (yb - y1) * (yb - y2) > 0.)
        nc--;
    }

    if (k2 == 1) {
      // first point at a segment within the area
      nint = n - 1;
      xx = xc[0];
      yy = yc[0];
      if ((anno_position == map_all) ||
          (anno_position == map_left   && fabsf(xx - xa) < 0.001) ||
          (anno_position == map_bottom && fabsf(yy - ya) < 0.001))
      {
        anno_positions.push_back(MapValueAnno(anno, xx + xoffset, yy + yoffset));
      }
    } else if (k1 == 1) {
      // last point at a segment within the area
      glBegin(GL_LINE_STRIP);
      glVertex2f(xx, yy);
      for (i = nint + 1; i < n; i++) {
        glVertex2f(x[i], y[i]);
      }
      glVertex2f(xc[0], yc[0]);
      glEnd();
    } else if (nc > 0) {
      // two 'neighboring points' outside the area, but part of the line within
      glBegin(GL_LINE_STRIP);
      glVertex2f(xc[0], yc[0]);
      glVertex2f(xc[1], yc[1]);
      glEnd();
    }
  }

  if (k2 == 1) {
    // last point is within the area
    glBegin(GL_LINE_STRIP);
    glVertex2f(xx, yy);
    for (i = nint + 1; i < npos; i++) {
      glVertex2f(x[i], y[i]);
    }
    glEnd();
  }
}

void xyclip(int npos, const float *x, const float *y, const float xylim[4])
{
  MapValueAnno_v dummy;
  xyclip(npos, x, y, xylim, map_none, std::string(), dummy);
}

bool startswith(const std::string& txt, const std::string& start)
{
  if (start.empty())
    return true;
  if (txt.size() < start.size())
    return false;
  return txt.substr(0, start.size()) == start; // TODO avoid copy
}

bool getFromFile(const std::string& filename, string_v& lines)
{
  std::ifstream file(filename.c_str());
  if (!file)
    return false;

  std::string str;
  while (getline(file, str))
    lines.push_back(str);

  file.close();
  return true;
}

#if 0
static size_t write_dataa(void *buffer, size_t size, size_t nmemb, void *userp)
{
  string tmp =(char *)buffer; // FIXME according to curl doc, buffer is not 0-terminated
  (*(string*) userp)+=tmp.substr(0,nmemb);
  return (size_t)(size *nmemb);
}
bool getFromHttp(const std::string &url, string_v& lines)
{
  CURL *curl = curl_easy_init();
  if (not curl)
    return false;

  string data;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_dataa);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
  const CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  const string_v result = miutil::split(data, "\n");
  lines.insert(lines.end(), result.begin(), result.end());

  return (res == 0);
}
#else

namespace detail {
/* assumes that lines is a non-empty string vector; appends to the
 * last string until '\n', then adds a new string and repeats
 */
void append_chars_split_newline(string_v& lines, const char* buffer, size_t nbuffer)
{
  const char* newline;
  while (nbuffer > 0 and (newline = (const char*)std::memchr(buffer, '\n', nbuffer))) {
    std::string& back = lines.back();
    back.insert(back.end(), buffer, newline);
    nbuffer -= (newline - buffer) + 1;
    buffer = newline + 1;
    lines.push_back(std::string());
  }
  if (nbuffer > 0) {
    std::string& back = lines.back();
    back.insert(back.end(), buffer, buffer + nbuffer);
  }
}
} // namespace detail

/* assumes that userp points to a non-empty string vector */
static size_t curl_write_callback(void *buffer, size_t size, size_t nmemb, void *userp)
{
  const size_t nbuffer = size * nmemb;
  string_v& lines = *reinterpret_cast<string_v*>(userp);
  detail::append_chars_split_newline(lines, (const char*)buffer, nbuffer);
  return nbuffer;
}

bool getFromHttp(const std::string &url_, string_v& lines)
{
  METLIBS_LOG_SCOPE();
  std::string url(url_);

  CURL *easy_handle = curl_easy_init();
  if (not easy_handle)
    return false;

  lines.push_back(std::string());

  const size_t query_start = url.find('?');
  if (url.size() <= 1024 or query_start == std::string::npos) {
    METLIBS_LOG_DEBUG("GET '" << url << "'");
  } else {
    const std::string post_par = url.substr(query_start+1);
    url = url.substr(0, query_start);
    METLIBS_LOG_DEBUG("POST url='" << url << "' par='" << post_par << "'");
    curl_easy_setopt(easy_handle, CURLOPT_POST, 1);
    curl_easy_setopt(easy_handle, CURLOPT_COPYPOSTFIELDS, post_par.c_str()); // make a copy, post_par goes out of scope
  }
  curl_easy_setopt(easy_handle, CURLOPT_URL, url.c_str());

  curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, curl_write_callback);
  curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, &lines);
  const CURLcode res = curl_easy_perform(easy_handle);
  long http_code = 0;
  curl_easy_getinfo(easy_handle, CURLINFO_RESPONSE_CODE, &http_code);
  curl_easy_cleanup(easy_handle);

  METLIBS_LOG_DEBUG(LOGVAL(res) << LOGVAL(http_code));
  return (res == 0) and http_code == 200;
}
#endif

bool getFromAny(const std::string &uof, string_v& lines)
{
  if (startswith(uof, "http://"))
    return getFromHttp(uof, lines);

  if (startswith(uof, "file://"))
    return getFromFile(uof.substr(7), lines);

  return diutil::getFromFile(uof, lines);
}

#if !defined(USE_PAINTGL)
OverrideCursor::OverrideCursor(const QCursor& cursor)
{
  QApplication::setOverrideCursor(cursor);
}

OverrideCursor::~OverrideCursor()
{
  QApplication::restoreOverrideCursor();
}
#endif

// only used from qtQuickMenu -- and from unit test, that's why it's here
void replace_reftime_with_offset(std::string& pstr, const miutil::miDate& nowdate)
{
  const size_t refpos = pstr.find("reftime=");
  if (refpos == std::string::npos)
    return;

  std::string refstr = pstr.substr(refpos+8,19);
  miutil::miTime reftime(refstr);
  if (reftime.undef())
    return;

  const miutil::miDate refdate = reftime.date();
  const miutil::miClock clock(0,0,0);

  const int daydiff = miutil::miTime::hourDiff(miutil::miTime(refdate,clock),miutil::miTime(nowdate,clock))/24;
  const int hour = reftime.hour();
  
  std::string replacement = "refhour=" + miutil::from_number(hour);
  if (daydiff < 0)
    replacement += " refoffset=" + miutil::from_number(daydiff);
  pstr = pstr.substr(0, refpos) + replacement + pstr.substr(refpos+27);
}

std::vector<std::string> numberList(float number, const float* enormal)
{
  int nenormal = 0;
  while (enormal[nenormal] > 0)
    nenormal += 1;

  const float e = number > 0 ? number : 1;
  const float elog = log10f(e);
  const int ielog = int((elog >= 0) ? elog : elog - 0.99999);
  const float ex = powf(10., ielog);
  int n = 0;
  float d = fabsf(e - enormal[0] * ex);
  for (int i = 1; i < nenormal; ++i) {
    const float dd = fabsf(e - enormal[i] * ex);
    if (d > dd) {
      d = dd;
      n = i;
    }
  }

  std::vector<std::string> vnumber;
  const int nupdown = nenormal * 2 / 3;

  for (int i = n - nupdown; i <= n + nupdown; ++i) {
    int j = i / nenormal;
    int k = i % nenormal;
    if (i < 0)
      j--;
    if (k < 0)
      k += nenormal;
    float ex = powf(10., ielog + j);
    vnumber.push_back(miutil::from_number(enormal[k] * ex));
  }
  return vnumber;
}

std::vector<std::string> numberList(QComboBox* cBox, float number, const float* enormal, bool onoff)
{
  std::vector<std::string> numbers = numberList(number, enormal);

  int current = (numbers.size() - 1)/2;
  if (onoff) {
    numbers.insert(numbers.begin(), "off");
    current += 1;
  }
  cBox->clear();
  for (size_t i=0; i<numbers.size(); ++i)
    cBox->addItem(QString::fromStdString(numbers[i]));
  cBox->setCurrentIndex(current);

  return numbers;
}

} // namespace diutil
