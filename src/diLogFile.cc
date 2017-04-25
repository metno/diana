/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2014 met.no

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

#include "diLogFile.h"

#include "util/charsets.h"

#include <puTools/miStringFunctions.h>
#include <istream>

LogFileIO::Section::Section(const std::string& title)
  : mTitle(title)
{
}

void LogFileIO::Section::addLine(const std::string& line)
{
  mLines.push_back(line);
}

void LogFileIO::Section::addLines(const std::vector<std::string>& lines)
{
  mLines.insert(mLines.end(), lines.begin(), lines.end());
}

bool LogFileIO::read(std::istream& input)
{
  diutil::GetLineConverter convertline("#");
  std::string begin;
  while (convertline(input, begin)) {
    if (begin.empty() || begin[0]=='#')
      continue;

    miutil::trim(begin);
    const size_t bl = begin.length();
    if (bl<3 || begin[0]!='[' || begin[bl-1]!=']') {
      return false;
    }

    std::string end = begin;
    end.insert(1, "/");

    std::string title = begin.substr(1, bl-2);
    Section& section = getSection(title);
    std::string line;
    while (convertline(input, line)) {
      if (line == end)
        break;
      section.addLine(line);
    }
    if (line != end)
      return false;
  }
  return true;
}

void LogFileIO::write(std::ostream& out)
{
  diutil::CharsetConverter_p converter = diutil::findConverter(diutil::CHARSET_INTERNAL(), diutil::CHARSET_WRITE());
  out << "# -*- coding: " << diutil::CHARSET_WRITE() << " -*-" << std::endl;

  for (Section_v::iterator it = mSections.begin(); it != mSections.end(); ++it) {
    const std::string title = converter->convert(it->title());
    out << '[' << title << ']' << std::endl;
    for (size_t i=0; i<it->size(); ++i)
      out << converter->convert(it->at(i)) << std::endl;
    out << "[/" << title << ']' << std::endl;
  }
}

void LogFileIO::clear()
{
  mSections.clear();
}

LogFileIO::Section& LogFileIO::getSection(const std::string& section)
{
  for (Section_v::iterator it = mSections.begin(); it != mSections.end(); ++it) {
    if (it->title() == section)
      return *it;
  }
  mSections.push_back(Section(section));
  return mSections.back();
}

const LogFileIO::Section& LogFileIO::getSection(const std::string& section) const
{
  for (Section_v::const_iterator it = mSections.begin(); it != mSections.end(); ++it) {
    if (it->title() == section)
      return *it;
  }

  static const Section EMPTY("");
  return EMPTY;
}

bool LogFileIO::hasSection(const std::string& section) const
{
  for (Section_v::const_iterator it = mSections.begin(); it != mSections.end(); ++it) {
    if (it->title() == section)
      return true;
  }

  return false;
}
