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

#include <puTools/miString.h>
#include <vector>
#include <map>

using namespace std;

/// one element in CommandParser
struct ParsedCommand {
  miutil::miString key;
  int      idNumber;
  miutil::miString allValue;
  vector<miutil::miString> strValue;
  vector<int>      intValue;
  vector<float>    floatValue;
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
    miutil::miString     name;      // name (or group name)
    int          idNumber;  // user supplied
  };

  // map<key,keyDescription>
  map<miutil::miString,keyDescription> keyDataBase;

  cmdCaseType caseType; // default cmdCaseUndefined, may set this only once,
                        // and before first addKey !

  bool commentSearch; // #comment rest of the input string

public:
  // Constructors
  CommandParser();
  // Copy constructor
  CommandParser(const CommandParser &rhs);
  // Destructor
  ~CommandParser();
  // Assignment operator
  CommandParser& operator=(const CommandParser &rhs);
  // Equality operator
  bool operator==(const CommandParser &rhs) const;

  static bool isInt(const miutil::miString& s);

  static bool isFloat(const miutil::miString& s);

  static vector<miutil::miString> parseString(const miutil::miString& str);

  static vector<float> parseFloat(const miutil::miString& str);

  static vector<int> parseInt(const miutil::miString& str);

  // case (conversion) type for keywords (not values), before first addKey !!!
  bool setCaseType(cmdCaseType casetype);

  // set comment (#) search
  void setCommentSearch(bool on= true);

  // add key (name not used if cmdValueType==cmdNoValue)
  bool addKey(const miutil::miString& name, const miutil::miString& key,
	      int idNumber, cmdValueType valuetype,
	      bool printError= true );

  vector<ParsedCommand> parse(const miutil::miString& str);

  int findKey(vector<ParsedCommand>& vpc,
	      const miutil::miString& key, bool addkey=false) const;


  bool removeValue(vector<ParsedCommand>& vpc,
		   const miutil::miString& key);

  bool replaceValue(vector<ParsedCommand>& vpc,
		    const miutil::miString& key,
		    const miutil::miString value, int valueIndex=0) const;

  bool replaceValue(ParsedCommand& pc,
		    const miutil::miString value, int valueIndex=0) const;

  miutil::miString unParse(const vector<ParsedCommand>& vpc) const;
};

#endif
