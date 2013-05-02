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

map<miString,Pattern::PatternInfo> Pattern::pmap;


Pattern::Pattern( const miString& name, const vector<miString>& pattern)
{

  if(name.exists()){
    pmap[name].name    = name;
    pmap[name].pattern = pattern;
  }

}

void Pattern::addPatternInfo(const PatternInfo& pi)
{
  if ( pi.name.exists() ){
    pmap[pi.name] = pi;
  }

}

vector<miString> Pattern::getPatternInfo(const miString& name)
{

  if(pmap.count(name)>0)
    return pmap[name].pattern;

  vector<miString> v;
  return v;

}

vector<Pattern::PatternInfo> Pattern::getAllPatternInfo()
{

  vector<PatternInfo> pattern;

  map<miString,PatternInfo>::iterator p= pmap.begin();
  for(;p!=pmap.end();p++) {
    pattern.push_back(p->second);
  }

  return pattern;

}
