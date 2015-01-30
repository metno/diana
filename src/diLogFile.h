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

#ifndef DILOGFILEIO_HH
#define DILOGFILEIO_HH 1

#include <iosfwd>
#include <string>
#include <vector>

class LogFileIO {
public:
  class Section {
  public:
    Section(const std::string& title);

    const std::string& title() const
      { return mTitle; }

    void addLine(const std::string& line);
    void addLines(const std::vector<std::string>& lines);

    size_t size() const
      { return mLines.size(); }

    const std::string& at(size_t index) const
      { return mLines.at(index); }

    const std::string& operator[](int index) const
      { return at(index); }
    
    const std::vector<std::string>& lines() const
      { return mLines; }
    
  private:
    std::string mTitle;
    std::vector<std::string> mLines;
  };

  bool read(std::istream& input);

  void write(std::ostream& out);

  void clear();

  Section& getSection(const std::string& section);

  const Section& getSection(const std::string& section) const;

  bool hasSection(const std::string& section) const;

private:
  typedef std::vector<Section> Section_v;
  Section_v mSections;
};

#endif // DILOGFILE_HH
