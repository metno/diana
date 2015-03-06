
#include "diUtilities.h"

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

string_v glob(const std::string& pattern, int glob_flags, bool& error)
{
  glob_t globBuf;
  error = (glob(pattern.c_str(), glob_flags, 0, &globBuf) != 0);

  string_v matches;
  if (not error)
    matches = string_v(globBuf.gl_pathv, globBuf.gl_pathv + globBuf.gl_pathc);

  globfree(&globBuf);
  return matches;
}

static bool startsOrEndsWith(const std::string& txt, const std::string& sub,
    int startcompare)
{
  if (sub.empty())
    return true;
  if (txt.size() < sub.size())
    return false;
  return txt.compare(startcompare, sub.size(), sub) == 0;
}

bool startswith(const std::string& txt, const std::string& start)
{
  return startsOrEndsWith(txt, start, 0);
}

bool endswith(const std::string& txt, const std::string& end)
{
  return startsOrEndsWith(txt, end,
      ((int)txt.size()) - ((int)end.size()));
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

} // namespace diutil
