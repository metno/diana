/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017 met.no

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

#include "string_util.h"

#include "puTools/miStringFunctions.h"

namespace diutil {

void remove_comment_and_trim(std::string& s, const std::string& commentmarker)
{
  std::string::size_type p;
  if ((p = s.find(commentmarker)) != std::string::npos)
    s.erase(p);

  miutil::remove(s, '\n');
  miutil::trim(s);
}

void remove_comment_and_trim(std::string& s)
{
  remove_comment_and_trim(s, "#");
}

void remove_quote(std::string& s, char quote)
{
  const size_t l = s.length();
  if (l >= 2 && s[0] == quote && s[l - 1] == quote) {
    s = s.substr(1, l - 2);
  }
}

std::string quote_removed(const std::string& s, char quote)
{
  std::string r = s;
  remove_quote(r, quote);
  return r;
}

void remove_start_end_mark(std::string& s, char start, char end)
{
  const size_t l = s.length();
  if (l >= 1 && s[0] == start) {
    if (l >= 2 && s[l - 1] == end)
      s = s.substr(1, l - 2);
    else
      s = s.substr(1, l - 1);
  }
}

std::string start_end_mark_removed(const std::string& s, char start, char end)
{
  std::string r = s;
  remove_start_end_mark(r, start, end);
  return r;
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

void appendText(std::string& text, const std::string& append, const std::string& separator)
{
  if (append.empty())
    return;
  if (!text.empty())
    text += separator;
  text += append;
}

std::string appendedText(const std::string& text, const std::string& append, const std::string& separator)
{
  std::string t(text);
  appendText(t, append, separator);
  return t;
}

void replace_chars(std::string& txt, const char* replace_these, const char replace_with)
{
  size_t pos = 0;
  while (pos < txt.size()
      && (pos = txt.find_first_of(replace_these, pos)) != std::string::npos)
  {
    txt[pos] = replace_with;
    pos += 1;
  }
}

} // namespace diutil
