/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diPattern.cc 3273 2010-05-18 17:32:21Z dages $

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

#include <diPattern.h>

using namespace miutil;

map<std::string,Pattern::PatternInfo> Pattern::pmap;


Pattern::Pattern( const std::string& name, const vector<std::string>& pattern)
{

  if((not name.empty())){
    pmap[name].name    = name;
    pmap[name].pattern = pattern;
  }

}

void Pattern::addPatternInfo(const PatternInfo& pi)
{
  if ( (not pi.name.empty()) ){
    pmap[pi.name] = pi;
  }

}

vector<std::string> Pattern::getPatternInfo(const std::string& name)
{

  if(pmap.count(name)>0)
    return pmap[name].pattern;

  vector<std::string> v;
  return v;

}

vector<Pattern::PatternInfo> Pattern::getAllPatternInfo()
{

  vector<PatternInfo> pattern;

  map<std::string,PatternInfo>::iterator p= pmap.begin();
  for(;p!=pmap.end();p++) {
    pattern.push_back(p->second);
  }

  return pattern;

}
