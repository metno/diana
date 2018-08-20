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

#ifndef DIANA_UTIL_STRING_UTIL_H
#define DIANA_UTIL_STRING_UTIL_H 1

#include <string>
#include <vector>

namespace diutil {

//! remove comment, remove preceding and trailing blanks
void remove_comment_and_trim(std::string& s, const std::string& commentmarker);

//! remove #-comment, remove preceding and trailing blanks
void remove_comment_and_trim(std::string& s);

//! remove quote at start and end if present at both start and end
void remove_quote(std::string& s, char quote = '"');

//! use remove_quote to remove quotes
std::string quote_removed(const std::string& s, char quote = '"');

//! remove char at start and end if present at start and optionally at end
void remove_start_end_mark(std::string& s, char start = '"', char end = '"');

//! use remove_start_end_mark to remove start and end marks
std::string start_end_mark_removed(const std::string& s, char start = '"', char end = '"');

bool startswith(const std::string& txt, const std::string& start);
bool endswith(const std::string& txt, const std::string& end);

void appendText(std::string& text, const std::string& append, const std::string& separator=" ");
std::string appendedText(const std::string& text, const std::string& append, const std::string& separator=" ");

void replace_chars(std::string& txt, const char* replace, const char with);
inline std::string replaced_chars(const std::string& txt, const char* replace, const char with)
{ std::string t(txt); replace_chars(t, replace, with); return t; }

namespace detail {
void append_chars_split_newline(std::vector<std::string>& lines, const char* buffer, size_t nbuffer);
}

} // namespace diutil

#endif // DIANA_UTIL_STRING_UTIL_H
