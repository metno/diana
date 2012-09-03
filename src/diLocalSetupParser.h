/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

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
#ifndef diLocalSetupParser_h
#define diLocalSetupParser_h

#include <puTools/miString.h>
#include <puTools/miSetupParser.h>
#include <vector>
#include <map>
#include <diPlotOptions.h>
#include <diCommonTypes.h>
#include <diField/diArea.h>

using namespace std;

class Controller;

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
  static miutil::miString setupFilename;
  static vector<QuickMenuDefs>   quickmenudefs;
  static map<miutil::miString, miutil::miString> basic_values;
  static map<miutil::miString, InfoFile> infoFiles;
  static vector<miutil::miString>        langPaths;

  // parse basic info
  static bool parseBasics(const miutil::miString&);
  // parse section containing colour definitions
  static bool parseColours(const miutil::miString&);
  // parse section containing colour-palette definitions
  static bool parsePalettes(const miutil::miString&);
  // parse section containing fillpattern definitions
  static bool parseFillPatterns(const miutil::miString&);
  // parse section containing linetype definitions
  static bool parseLineTypes(const miutil::miString&);
  // parse section containing definitions of quick-menus
  static bool parseQuickMenus(const miutil::miString&);
  // parse text-information-files
  static bool parseTextInfoFiles(const miutil::miString&);
  // check if fielname exists, if not make directory
  static bool makeDirectory(const miutil::miString& filename, miutil::miString & error);

public:
  LocalSetupParser(){}

  /// recursively parse setupfiles - mainfilename can be changed in the process
  static bool parse( miutil::miString& mainfilename );
  /// get quick menues defined in setup
  static bool getQuickMenus(vector<QuickMenuDefs>& qm);
  /// get list of lists of colours
  static vector< vector<Colour::ColourInfo> > getMultiColourInfo(int multiNum);
  /// get list of InfoFile - used in textview
  static map<miutil::miString,InfoFile> getInfoFiles() {return infoFiles;}
  /// paths to check for language files
  static vector<miutil::miString> languagePaths() {return langPaths;}
  /// Basic types
  static miutil::miString basicValue(const miutil::miString& key) { return basic_values[key];}
  /// Setup filename
  static miutil::miString getSetupFileName() { return setupFilename;}
  static void setSetupFileName(const miutil::miString& sf) { setupFilename=sf;}
};

#endif
