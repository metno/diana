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
#ifndef diSetupParser_h
#define diSetupParser_h

#include <puTools/miString.h>
#include <vector>
#include <map>
#include <diField/diPlotOptions.h>
#include <diCommonTypes.h>
#include <diField/diArea.h>

using namespace std;

class Controller;

/**
   \brief one section in setupfile

   list of strings with references to original linenumbers and filesources
*/
struct SetupSection {
  vector<miString> strlist;
  vector<int> linenum;
  vector<int> filenum;
};

/**
   \brief the setup file parser

   Reads a Diana setup file
   - organized in sections
   - environment/shell variable expansions
   - local variable expansions
   - inclusion of other files
   - rejoin lines ending with /
*/

class SetupParser {
private:
  /// list of setup-filenames
  static vector<miString> sfilename;
  /// Setuptext hashed by Section name
  static map<miString, SetupSection> sectionm;

  static map<miString, miString> substitutions;
  static map<miString, miString> user_variables;
  static map<miString, Filltype> filltypes;
  static vector<QuickMenuDefs>   quickmenudefs;
  static map<miString, miString> basic_values;
  static map<miString, InfoFile> infoFiles;
  static vector<miString>        langPaths;

  // parse basic info
  bool parseBasics(const miString&);
  // parse section containing colour definitions
  bool parseColours(const miString&);
  // parse section containing colour-palette definitions
  bool parsePalettes(const miString&);
  // parse section containing fillpattern definitions
  bool parseFillPatterns(const miString&);
  // parse section containing linetype definitions
  bool parseLineTypes(const miString&);

  // parse section containing definitions of quick-menus
  bool parseQuickMenus(const miString&);
  // parse text-information-files
  bool parseTextInfoFiles(const miString&);
  // expand local variables in string
  bool checkSubstitutions(miString& t);
  // expand environment values in string
  bool checkEnvironment(miString& t);
  // check if fielname exists, if not make directory
  bool makeDirectory(const miString& filename, miString & error);
  /// parse one setupfile
  bool parseFile(const miString& filename,
		 const miString& section,
		 int level);
  /// report an error with filename and linenumber
  void internalErrorMsg(const miString& filename,
			const int linenum,
			const miString& error);


public:
  SetupParser(){}

  /// set user variables
  void setUserVariables(const map<miString,miString> & user_var);
  /// cleans a string
  void cleanstr(miString&);
  /// finds key=value in string
  static void splitKeyValue(const miString& s, miString& key, miString& value);
  /// finds key=v1,v2,v3,... in string
  static void splitKeyValue(const miString& s, miString& key, vector<miString>& value);

  /// recursively parse setupfiles - mainfilename can be changed in the process
  bool parse( miString& mainfilename );
  /// get stringlist for a named section
  bool getSection(const miString&,vector<miString>&);
  /// report an error with line# and sectionname
  void errorMsg(const miString&,const int,const miString&);
  /// report a warning with line# and sectionname
  void warningMsg(const miString&,const int,const miString&);
  /// get quick menues defined in setup
  bool getQuickMenus(vector<QuickMenuDefs>& qm);
  /// get list of lists of colours
  vector< vector<Colour::ColourInfo> > getMultiColourInfo(int multiNum);
  /// get list of InfoFile - used in textview
  map<miString,InfoFile> getInfoFiles() const {return infoFiles;}
  /// paths to check for language files
  vector<miString> languagePaths() const {return langPaths;}
  /// Basic types
  miString basicValue(const miString& key) { return basic_values[key];}
};

#endif
