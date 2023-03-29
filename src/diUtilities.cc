/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2022 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "diUtilities.h"

#include "util/string_util.h"

#include "diField/diRectangle.h"

#include <puCtools/glob_cache.h>
#include <puCtools/puCglob.h>
#include <puTools/miStringFunctions.h>

#include <curl/curl.h>

#include <cstring>
#include <fstream>
#include <random>

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

int find_index(bool repeat, int available, int i)
{
  if (repeat) {
    i %= available;
    if (i<0)
      i += available;
    return i;
  }
  else if (i<=0)
    return 0;
  else if (i<available)
    return i;
  else
    return available-1;
}

std::string get_uuid()
{

  static std::random_device dev;
  static std::mt19937 rng(dev());
  std::uniform_int_distribution<int> dist(0, 15);

  const char v[16 + 1] = "0123456789abcdef";
  const bool dash[16] = {0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0};

  std::string uuid(2 * 16 + 4, '-');
  for (int i = 0, j = 0; i < 16; i += 1, j += 2) {
    if (dash[i])
      j += 1;
    uuid[j + 0] = v[dist(rng)];
    uuid[j + 1] = v[dist(rng)];
  }
  return uuid;
}

bool getFromFile(const std::string& filename, string_v& lines)
{
  std::ifstream file(filename);
  if (!file)
    return false;

  std::string str;
  while (getline(file, str))
    lines.push_back(str);

  return true;
}

bool getFromFile(const std::string& filename, std::string& all)
{
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  if (!file)
    return false;

  auto size = file.tellg();
  file.seekg(0, std::ios::beg);
  all = std::string(size, '\0');
  return !!file.read(&all[0], size);
}

namespace detail {
/* assumes that lines is a non-empty string vector; appends to the
 * last string until '\n', then adds a new string and repeats
 */
void append_chars_split_newline(string_v& lines, const char* buffer, size_t nbuffer)
{
  const char* newline;
  while (nbuffer > 0 and (newline = (const char*)std::memchr(buffer, '\n', nbuffer))) {
    const size_t count = newline - buffer;
    lines.back().append(buffer, count);
    nbuffer -= count + 1;
    buffer = newline + 1;
    lines.push_back(std::string());
  }
  if (nbuffer > 0) {
    lines.back().append(buffer, buffer + nbuffer);
  }
}

/* assumes that userp points to string */
size_t curl_write_single(char* buffer, size_t size, size_t nmemb, void* userp)
{
  const size_t nbuffer = size * nmemb;
  std::string& text = *reinterpret_cast<std::string*>(userp);
  text.append((const char*)buffer, nbuffer);
  return nbuffer;
}

/* assumes that userp points to a non-empty string vector */
size_t curl_write_split(char* buffer, size_t size, size_t nmemb, void* userp)
{
  const size_t nbuffer = size * nmemb;
  string_v& lines = *reinterpret_cast<string_v*>(userp);
  detail::append_chars_split_newline(lines, (const char*)buffer, nbuffer);
  return nbuffer;
}

bool doGetFromHttp(const std::string& url_, curl_write_callback cb, void* cb_data)
{
  METLIBS_LOG_SCOPE();
  std::string url(url_);

  CURL *easy_handle = curl_easy_init();
  if (not easy_handle)
    return false;


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

  const std::string uuid = get_uuid();
  struct curl_slist* chunk = NULL;
  const std::string x_request_id = "X-Request-Id: " + uuid;
  chunk = curl_slist_append(chunk, x_request_id.c_str());
  curl_easy_setopt(easy_handle, CURLOPT_HTTPHEADER, chunk);

  curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, cb);
  curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, cb_data);
  const CURLcode res = curl_easy_perform(easy_handle);
  long http_code = 0;
  if (res == CURLE_OK)
    curl_easy_getinfo(easy_handle, CURLINFO_RESPONSE_CODE, &http_code);
  curl_easy_cleanup(easy_handle);
  curl_slist_free_all(chunk);

  METLIBS_LOG_DEBUG(LOGVAL(res) << LOGVAL(http_code) << LOGVAL(uuid));
  return (res == CURLE_OK) and http_code == 200;
}

bool is_http_url(const std::string& uof)
{
  return (diutil::startswith(uof, "http://") || diutil::startswith(uof, "https://"));
}

bool is_file_url(const std::string& uof)
{
  return diutil::startswith(uof, "file://");
}
} // namespace detail

bool getFromHttp(const std::string& url_, string_v& lines)
{
  lines.push_back(std::string());
  return detail::doGetFromHttp(url_, detail::curl_write_split, &lines);
}

bool getFromHttp(const std::string& url_, std::string& all)
{
  return detail::doGetFromHttp(url_, detail::curl_write_single, &all);
}

bool getFromAny(const std::string &uof, string_v& lines)
{
  if (detail::is_http_url(uof))
    return getFromHttp(uof, lines);

  if (detail::is_file_url(uof))
    return getFromFile(uof.substr(7), lines);

  return diutil::getFromFile(uof, lines);
}

bool getFromAny(const std::string& uof, std::string& all)
{
  if (detail::is_http_url(uof))
    return getFromHttp(uof, all);

  if (detail::is_file_url(uof))
    return getFromFile(uof.substr(7), all);

  return diutil::getFromFile(uof, all);
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
