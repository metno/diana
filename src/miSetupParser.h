/*
  Copyright (C) 2013 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef MISETUPPARSER_H
#define MISETUPPARSER_H

#include "util/diKeyValue.h"
#include <map>
#include <string>
#include <vector>

namespace miutil {

struct SetupSection {
  std::vector<std::string> strlist;
  std::vector<int> linenum;
  std::vector<int> filenum;
  void clear();
};

class SetupParser {
private:
  typedef std::vector<std::string> string_v;
  typedef std::map<std::string,std::string> string_string_m;

  /// list of setup-filenames
  string_v sfilename;
  /// Setuptext hashed by Section name
  std::map<std::string,SetupSection> sectionm;

  string_string_m substitutions;
  string_string_m user_variables;

  /// report an error with filename and linenumber
  static void internalErrorMsg(const std::string& filename,
      int linenum, const std::string& error);

  void writeMsg(const std::string& filename, int linenum, const std::string& msg,
      const std::string& Error);

  // expand local variables in string
  bool checkSubstitutions(std::string& t) const;

  bool substitute(std::string& t, bool environment) const;
  void substituteAll(std::string& s) const;

  bool parseFile(const std::string& mainfilename);
  bool parseFile(const std::string& filename,
      const std::string& section, int level);

  SetupParser();
  SetupParser& operator=(const SetupParser&);

public:
  /// set user variables
  static void setUserVariables(const string_string_m& user_var);

  /// replace or add user variables
  static void replaceUserVariables(const std::string& key, const std::string& value);

  // expand environment values in string
  static bool checkEnvironment(std::string& t);

  /// get list of substitutions (user variables
  static const string_string_m& getUserVariables();

  /// cleans a string
  static void cleanstr(std::string&);

  /// recursively parse setupfiles
  static bool parse(const std::string& mainfilename);

  /// get stringlist for a named section
  static bool getSection(const std::string&,std::vector<std::string>&);

  /// clear the section map sectionm (now used in tsData, ptGribStream)
  static void clearSect();

  /// report an error with line# and sectionname
  static void errorMsg(const std::string&, int, const std::string&);

  /// report a warning with line# and sectionname
  static void warningMsg(const std::string&, int, const std::string&);

public:
  static SetupParser* instance();
  static void destroy();

  /// finds key=value in string
  static void splitKeyValue(const std::string& s, std::string& key,
      std::string& value, bool keepCase = false);

  static KeyValue splitKeyValue(const std::string& s, bool keepCase = false);

  static KeyValue_v splitManyKeyValue(const std::string& line, bool keepCase = false);

  /// finds key=v1,v2,v3,... in string
  static void splitKeyValue(const std::string& s, std::string& key,
      std::vector<std::string>& value);

private:
  static SetupParser* self;
};

} // namespace miutil

#endif
