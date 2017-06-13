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
#ifndef diCommandParser_h
#define diCommandParser_h

#include "util/diKeyValue.h"
#include <map>
#include <string>
#include <vector>

/// one element in CommandParser
struct ParsedCommand {
  std::string key;
  int      idNumber;
  std::string allValue;
  std::vector<std::string> strValue;
  std::vector<int>      intValue;
  std::vector<float>    floatValue;
};

/**

  \brief Option handler for FieldDialog

  Splits up a command string in separate elements,
  with a key and one or more strings, floats and ints.
  Changes option elements and makes a complete string again.
*/
class CommandParser {

public:
  // for each key
  enum cmdValueType {
    cmdNoValue,      // key
    cmdString,       // key=string,"string",...
    cmdInt,          // key=1,2,3,4,...
    cmdFloat,        // key=0.2,0.5,1,2,3,4,5,10,...
    cmdUnknown,      // for internal use only!
    cmdSkip          // for internal use only!
  };

  // for all key search
  enum cmdCaseType {
    cmdLowerCase,      // convert keywords (and strValue if cmdNoValue) to lowercase
    cmdUpperCase,      // convert keywords (and strValue if cmdNoValue) to uppercase
    cmdCaseDependant,  // no case conversion (default)
  };

private:
  // Copy members
  void memberCopy(const CommandParser& rhs);

  struct keyDescription {
    cmdValueType valueType;
    std::string     name;      // name (or group name)
    int          idNumber;  // user supplied
  };

  // map<key,keyDescription>
  std::map<std::string,keyDescription> keyDataBase;

  int findKey(std::vector<ParsedCommand>& vpc,
              const std::string& key, bool addkey) const;

public:
  CommandParser();
  CommandParser(const CommandParser &rhs);
  ~CommandParser();

  CommandParser& operator=(const CommandParser &rhs);
  bool operator==(const CommandParser &rhs) const;

  // add key (name not used if cmdValueType==cmdNoValue)
  bool addKey(const std::string& name, const std::string& key,
              int idNumber, cmdValueType valuetype);

  std::vector<ParsedCommand> parse(const std::string& str);

  int findKey(std::vector<ParsedCommand>& vpc,
              const std::string& key) const
    { return findKey(vpc, key, false); }

  bool removeValue(std::vector<ParsedCommand>& vpc,
		   const std::string& key);

  bool replaceValue(std::vector<ParsedCommand>& vpc,
		    const std::string& key,
		    const std::string value, int valueIndex=0) const;

  bool replaceValue(ParsedCommand& pc,
		    const std::string value, int valueIndex=0) const;

  std::string unParse(const std::vector<ParsedCommand>& vpc) const;

  miutil::KeyValue_v toKeyValueList(const std::vector<ParsedCommand>& vpc) const;
  std::vector<ParsedCommand> fromKeyValueList(const miutil::KeyValue_v& kvs) const;

  // static string parsing functions

  static bool isInt(const std::string& s);

  static bool isFloat(const std::string& s);

  static std::vector<std::string> parseString(const std::string& str);

  static std::vector<float> parseFloat(const std::string& str);

  static std::vector<int> parseInt(const std::string& str);

};

#endif
