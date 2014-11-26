/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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
#ifndef diLocalSetupParser_h
#define diLocalSetupParser_h

#include "diCommonTypes.h"

#include <vector>
#include <map>

/**
   \brief the setup file parser

   Reads a Diana setup file
   - organized in sections
   - environment/shell variable expansions
   - local variable expansions
   - inclusion of other files
   - rejoin lines ending with /
*/

class LocalSetupParser {
private:
  static std::string setupFilename;
  static std::vector<QuickMenuDefs>   quickmenudefs;
  static std::map<std::string, std::string> basic_values;
  static std::map<std::string, InfoFile> infoFiles;
  static std::vector<std::string>        langPaths;

  // parse basic info
  static bool parseBasics(const std::string&);
  // parse section containing colour definitions
  static bool parseColours(const std::string&);
  // parse section containing colour-palette definitions
  static bool parsePalettes(const std::string&);
  // parse section containing fillpattern definitions
  static bool parseFillPatterns(const std::string&);
  // parse section containing linetype definitions
  static bool parseLineTypes(const std::string&);
  // parse section containing definitions of quick-menus
  static bool parseQuickMenus(const std::string&);
  // parse text-information-files
  static bool parseTextInfoFiles(const std::string&);
  // check if fielname exists, if not make directory
  static bool makeDirectory(const std::string& filename, std::string & error);

public:
  LocalSetupParser(){}

  /// recursively parse setupfiles - mainfilename can be changed in the process
  static bool parse(std::string& mainfilename);
  /// get quick menues defined in setup
  static bool getQuickMenus(std::vector<QuickMenuDefs>& qm);
  /// get list of lists of colours
  static std::vector< std::vector<Colour::ColourInfo> > getMultiColourInfo(int multiNum);
  /// get list of InfoFile - used in textview
  static const std::map<std::string, InfoFile> getInfoFiles() {return infoFiles;}
  /// paths to check for language files
  static const std::vector<std::string>& languagePaths() {return langPaths;}
  /// Basic types
  static const std::string& basicValue(const std::string& key) { return basic_values[key];}
  /// Setup filename
  static const std::string& getSetupFileName() { return setupFilename;}
  static void setSetupFileName(const std::string& sf) { setupFilename=sf;}
};

#endif
