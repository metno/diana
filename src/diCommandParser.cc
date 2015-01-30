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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diCommandParser.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.CommandParser"
#include <miLogger/miLogging.h>

using namespace::miutil;
using namespace std;

// Default constructor
CommandParser::CommandParser()
  :caseType(cmdCaseDependant),commentSearch(false){
}

// Copy constructor
CommandParser::CommandParser(const CommandParser &rhs){
  // elementwise copy
  memberCopy(rhs);
}

// Destructor
CommandParser::~CommandParser(){
}

// Assignment operator
CommandParser& CommandParser::operator=(const CommandParser &rhs){
  if (this == &rhs) return *this;
  // elementwise copy
  memberCopy(rhs);

  return *this;
}

// Equality operator
bool CommandParser::operator==(const CommandParser &rhs) const{
  return false;
}

void CommandParser::memberCopy(const CommandParser& rhs){
  // copy members

  keyDataBase=   rhs.keyDataBase;
  caseType=      rhs.caseType;
  commentSearch= rhs.commentSearch;
}


bool CommandParser::isInt(const std::string& s)
{
  // in contrast to miutil::is_int, this function does not accept
  // spaces around the number

  int i= 0, n= s.length();
  if (n==0) return false;
  if (s[i]=='-' || s[i]=='+'){
    i++;
    if (n==i) return false;
  }

  for (; i<n; i++)
    if (!isdigit(s[i])) return false;

  return true;
}


bool CommandParser::isFloat(const std::string& s)
{
  // in contrast to miutil::is_float, this function does not accept
  // spaces around the number

  bool adot= false;
  int i= 0, n= s.length();
  if (n==0) return false;
  if (s[i]=='-' || s[i]=='+'){
    i++;
    if (n==i) return false;
  }

  int ne= -1;

  for (; i<n; i++){
    if (!isdigit(s[i])){
      // one dot is a enough
      if (s[i]=='.') {
        if (adot) return false;
        adot=true;
      } else if (s[i]=='E' || s[i]=='e') {
	ne= n;
	n= i+1; // this stops looping, too
      } else {
        return false;
      }
    }
  }

  if (ne<0) return true;
  if (n==ne) return false;
  return isInt(s.substr(n,ne-n));
}


vector<std::string> CommandParser::parseString(const std::string& str) {
  vector<std::string> vs;

  size_t i,pos, end= str.length();
  i= str.find_first_not_of(' ',0);

  while (i<end) {
    pos= i;
    if (str[i]=='"') {
      i= str.find_first_of('"',pos+1);
      if (i>end) i= end;
      vs.push_back(str.substr(pos+1,i-pos-1));
      if (i<end-1) i= str.find_first_of(',',i+1);
    } else {
      i= str.find_first_of(',',pos+1);
      if (i>end) i=end;
      vs.push_back(str.substr(pos,i-pos));
    }
    i++;
  }

  return vs;
}


vector<float> CommandParser::parseFloat(const std::string& str) {
  vector<float> vf;

  std::string snum;
  size_t i,pos, end= str.length();
  i= str.find_first_not_of(' ',0);

  bool ok= true;

  while (ok && i<end) {
    pos= i;
    i= str.find_first_of(',',pos+1);
    if (i>end) i=end;
    snum= str.substr(pos,i-pos);
    if (isFloat(snum)) vf.push_back( atof(snum.c_str()) );
    else ok= false;
    i++;
  }
  if (!ok) vf.clear();

  return vf;
}


vector<int> CommandParser::parseInt(const std::string& str) {
  vector<int> vi;

  std::string snum;
  size_t i,pos, end= str.length();
  i= str.find_first_not_of(' ',0);

  bool ok= true;

  while (ok && i<end) {
    pos= i;
    i= str.find_first_of(',',pos+1);
    if (i>end) i=end;
    snum= str.substr(pos,i-pos);
    if (isInt(snum)) vi.push_back( atoi(snum.c_str()) );
    else ok= false;
    i++;
  }
  if (!ok) vi.clear();

  return vi;
}


bool CommandParser::setCaseType(cmdCaseType casetype) {
  // Case (conversion) type for keywords (not values),
  // Cannot be changed unless cmdCaseDependant.

  if (caseType!=casetype) {
    if (keyDataBase.size()>0) return false;
    caseType= casetype;
  }

  return true;
}


bool CommandParser::addKey(const std::string& name, const std::string& key,
	                   int idNumber, cmdValueType valuetype,
		           bool printError ) {
  // add key

  std::string newkey;
  if (!key.empty()) {
    if      (caseType==cmdLowerCase) newkey= miutil::to_lower(key);
    else if (caseType==cmdUpperCase) newkey= miutil::to_upper(key);
    else                             newkey= key;
  } else if (!name.empty()) {
    // key==name
    if      (caseType==cmdLowerCase) newkey= miutil::to_lower(name);
    else if (caseType==cmdUpperCase) newkey= miutil::to_upper(name);
    else                             newkey= name;
  } else {
    if (printError) METLIBS_LOG_ERROR("CommandParser::addKey ERROR: key= "<<key
                          <<"  name= "<<name);
    return false;
  }

  if (keyDataBase.find(newkey)==keyDataBase.end()) {
    keyDataBase[newkey].valueType= valuetype;
    keyDataBase[newkey].name=      name;
    keyDataBase[newkey].idNumber=  idNumber;
  } else {
    if (printError) METLIBS_LOG_ERROR("CommandParser::addKey ERROR: key= "<<key
                          <<"  name= "<<name);
    return false;
  }
  return true;
}


vector<ParsedCommand> CommandParser::parse(const std::string& str) {

  // Purpose:
  // Split string into interesting parts.
  // E.g. add red as a color (string) to the CommandParser's database,
  // When parsing a string like "........ red ......",
  // the parse method will return "colour" with "red" as a value
  // (possibly also a number/index that red has somewhere else)
  //
  // Most tricky string/split handling not implemented yet
  // (like 'key= 1,2, 3 , 5')...
  //
  // Accepted syntax:
  //    "string"    (unquoted string is the same as an unknown key)
  //    key
  //    key=v,v,v,v,v   (one or more v's required if cmdString, cmdInt or cmdFloat)
  //    v formats: int float string,
  //               all v's of the same type,
  //               no spaces allowed,
  //               v's are returned in the dedicated value vector,
  //               if cmdFloat the v's are returned as strings and floats
  //               if cmdInt   the v's are returned as strings, floats and ints
  //
  // Space is the only separator between items.
  //
  // Unknown key, string or "string"  (not in keyDataBAse) is accepted
  // with key=unknown (data returned in the string value vector, element 0).
  // "" is an empty string (key=unknown and no string returned)
  //
  // If unknown key check if int or float values ??? ... later ????

  vector<ParsedCommand> vpc;

  size_t i,pos,end = 0, strlen= str.length();

  if (commentSearch) {
    i= str.find_first_of('#');
    if (i<strlen) strlen=i;
    if (strlen==0) return vpc;
  }

  i= str.find_first_not_of(' ');
  if (i>=strlen) return vpc;

  map<std::string,keyDescription>::iterator pk, pkend= keyDataBase.end();

  std::string tmp,key;
  cmdValueType valueType;

  // very nice always having a space at the end of the string...
  tmp= str.substr(0,strlen) + " ";

  if      (caseType==cmdLowerCase) tmp= miutil::to_lower(tmp);
  else if (caseType==cmdUpperCase) tmp= miutil::to_upper(tmp);

  // assuming: always a space at tmp[strlen], added above
  //           find_first_... returns a large value if char(s) not found
  //           (gcc/g++ returns max unsigned int (max size_t))

  while (i<strlen) {

    ParsedCommand pc;
    pc.idNumber= -1;

    if (tmp[i]=='"') {
      // quoted string ("string"), the quotes are not returned
      pos= ++i;
      i= tmp.find_first_of('"',pos);
      if (i>strlen) i=strlen;
      pc.key="unknown";
      if (i>pos) pc.strValue.push_back(str.substr(pos,i-pos));
      i++;
    } else {
      // key (possibly unknown)
      pos= i;
      i= tmp.find_first_of(" =",pos+1);
      if (i>strlen) i=strlen;
      key= tmp.substr(pos,i-pos);
      // check if defined in keyDataBase
      pk= keyDataBase.find(key);
      if (pk==pkend) {
	// key not found in database
	if (str[i]=='=') {
	  pc.key= key;
	  valueType= cmdUnknown;
	} else {
          pc.key="unknown";
          pc.allValue= key;
          pc.strValue.push_back(key);
	  valueType= cmdNoValue;
	}
      } else {
	// key found in database
	valueType= pk->second.valueType;
	if (valueType==cmdNoValue && tmp[i]=='=') valueType=cmdSkip;
        if (valueType!=cmdNoValue && tmp[i]!='=') valueType=cmdNoValue;
        if (valueType==cmdNoValue) {
          pc.key= pk->second.name;
          pc.allValue= key;
	  pc.idNumber= pk->second.idNumber;
	  pc.strValue.push_back(key);
        } else {
          pc.key= pk->second.name;
          pc.idNumber= pk->second.idNumber;
	}
      }

      if (valueType!=cmdNoValue) {
	pos= i + 1;
	tmp[i]= ',';
	while (i<strlen && tmp[i]==',') {
	  i++;
	  if (tmp[i]=='"') {
	    i= tmp.find_first_of('"',i+1);
            if (i>strlen) i=strlen;
	    end= ++i;
	  } else {
	    end= i= tmp.find_first_of(" ,",i+1);
	  }
        }
	tmp[pos-1]= '=';

	pc.allValue= str.substr(pos,end-pos);

	pc.strValue= parseString(pc.allValue);

        if (valueType!=cmdString)
	  pc.floatValue= parseFloat(pc.allValue);

        if (valueType!=cmdString && valueType!=cmdFloat)
	  pc.intValue= parseInt(pc.allValue);
      }
    }
    vpc.push_back(pc);

    if (i<strlen) i= str.find_first_not_of(' ',i);
  }

  return vpc;
}


int CommandParser::findKey(vector<ParsedCommand>& vpc,
			   const std::string& key, bool addkey) const {
  int n= vpc.size();
  int i= 0;
  while (i<n && vpc[i].key!=key) i++;
  if (i==n){
    if(addkey) { // add new key
      ParsedCommand pc;
      pc.key=key;
      vpc.push_back(pc);
    } else {
      i=-1;
    }
  }

  return i;
}


bool CommandParser::removeValue(vector<ParsedCommand>& vpc,
			         const std::string& key){

  vector<ParsedCommand>::iterator p=vpc.begin(), pend=vpc.end();
  while(p!=pend && p->key!=key) p++;
  if(p!=pend) {
    vpc.erase(p);
    return true;
  }

  return false;
}

bool CommandParser::replaceValue(vector<ParsedCommand>& vpc,
			         const std::string& key,
				 const std::string value, int valueIndex) const {
  int n= findKey(vpc,key,true);

  if (n>=0) return replaceValue(vpc[n],value,valueIndex);

  return false;
}


bool CommandParser::replaceValue(ParsedCommand& pc,
				 const std::string value, int valueIndex) const {

  // valueIndex > existing : append a new value element
  // valueIndex < 0        : value contains all values

  unsigned int n, nstr= pc.strValue.size();

  if (valueIndex<0) {
    // replace all values
    pc.allValue= value;
    pc.strValue= parseString(pc.allValue);
    n= pc.strValue.size();
  } else if (valueIndex<int(nstr)) {
    // replace one value
    pc.strValue[valueIndex]= value;
    n= nstr;
  } else {
    // append a new value
    pc.strValue.push_back(value);
    n= nstr + 1;
  }

  if (valueIndex>=0) {
    // this works losy if the original strings was of type "string"
    // and contained one ore more commas !!!
    pc.allValue.clear();
    for (unsigned int i=0; i<n; i++) {
      if (i>0) pc.allValue+= ',';
      if (pc.strValue[i].find_first_of(' ') < pc.strValue[i].length())
	pc.allValue+= ('"' + pc.strValue[i] + '"');
      else
	pc.allValue+= pc.strValue[i];
    }
  }

  if (pc.floatValue.size()==nstr) {
    pc.floatValue= parseFloat(pc.allValue);
    if (pc.floatValue.size()!=n && nstr>0) return false;
  }

  if (pc.intValue.size()==nstr) {
    pc.intValue= parseInt(pc.allValue);
    if (pc.intValue.size()!=n && nstr>0) return false;
  }

  return true;
}


std::string CommandParser::unParse(const vector<ParsedCommand>& vpc) const {

  std::string str;

  int n, nvpc= vpc.size();

  for (n=0; n<nvpc; n++) {
    if (n>0) str+= (" " + vpc[n].key + "=" + vpc[n].allValue);
    else     str+=       (vpc[n].key + "=" + vpc[n].allValue);
  }

  return str;
}
