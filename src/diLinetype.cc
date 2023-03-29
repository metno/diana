/*
  Diana - A Free Meteorological Visualisation Tool

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

#include "diana_config.h"

#include "diLinetype.h"
#include <puTools/miStringFunctions.h>


std::map<std::string,Linetype> Linetype::linetypes;
std::vector<std::string>       Linetype::linetypeSequence;
Linetype                  Linetype::defaultLinetype;


Linetype::Linetype()
{
  name= "solid";
  stipple= false;
  bmap= 0xFFFF;
  factor= 1;
}


Linetype::Linetype(const std::string& _name)
{
  std::string ltname= miutil::to_lower(_name);
  std::map<std::string,Linetype>::iterator p = linetypes.find(ltname);
  if (p != linetypes.end())
    memberCopy(p->second);
  else
    memberCopy(defaultLinetype);
}


Linetype& Linetype::operator=(const Linetype &rhs)
{
  if (this != &rhs)
    memberCopy(rhs);
  return *this;
}


bool Linetype::operator==(const Linetype &rhs) const
{
  return (bmap==rhs.bmap && factor==rhs.factor);
}


void Linetype::memberCopy(const Linetype& rhs)
{
  name=    rhs.name;
  stipple= rhs.stipple;
  bmap=    rhs.bmap;
  factor=  rhs.factor;
}


// static
void Linetype::init()
{
  linetypes.clear();
  linetypeSequence.clear();
  defaultLinetype= Linetype();
}


// static
void Linetype::define(const std::string& _name,
    short unsigned int _bmap, int _factor)
{
  std::string ltname= miutil::to_lower(_name);
  Linetype lt;
  lt.name=    ltname;
  lt.stipple= (_bmap!=0xFFFF);
  lt.bmap=    _bmap;
  lt.factor=  _factor;

  if (linetypes.find(ltname)==linetypes.end()) {
    linetypeSequence.push_back(ltname);
    if (linetypeSequence.size()==1)
      defaultLinetype= lt;
  }
  linetypes[ltname]= lt;
}
