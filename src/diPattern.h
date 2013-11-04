/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diPattern.h 3779 2012-01-23 09:44:53Z davidb $

  Copyright (C) 2006 met.no

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
#ifndef diPattern_h
#define diPattern_h

#include <map>
#include <string>
#include <vector>

/**
  \brief Pattern type

  static list of defined sets of patterns, reachable by name
*/
class Pattern {
public:

  /// Pattern data as strings
  struct PatternInfo {
    std::vector<std::string> pattern;
    std::string name;
  };

private:
  typedef std::map<std::string,PatternInfo> pmap_t;
  static pmap_t pmap;

  // Copy members
  void memberCopy(const Pattern& rhs);

public:
  Pattern(const std::string& name, const std::vector<std::string>& pattern);

  // static functions for static pattern-map
  /// add a new PatternInfo
  static void addPatternInfo(const PatternInfo& pi);
  /// return patterns
  static std::vector<std::string> getPatternInfo(const std::string& name);
  /// return all PatternInfos
  static std::vector<PatternInfo> getAllPatternInfo();
};

#endif
