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

#include "diCommandParser.h"

#define MILOGGER_CATEGORY "diana.CommandParser"
#include <miLogger/miLogging.h>

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
      // one dot is enough
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

std::vector<std::string> CommandParser::parseString(const std::string& str)
{
  std::vector<std::string> vs;

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
