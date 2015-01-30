/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diLinetype.cc 3273 2010-05-18 17:32:21Z dages $

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

#include "diLinetype.h"
#include <puTools/miStringFunctions.h>

using namespace std;

map<std::string,Linetype> Linetype::linetypes;
vector<std::string>       Linetype::linetypeSequence;
Linetype               Linetype::defaultLinetype;


// Default constructor
Linetype::Linetype()
{
  name= "solid";
  stipple= false;
  bmap= 0xFFFF;
  factor= 1;
}


// Constructor
Linetype::Linetype(const std::string& _name)
{
  std::string ltname= miutil::to_lower(_name);
  map<std::string,Linetype>::iterator p;
  if ((p=linetypes.find(ltname))!=linetypes.end())
    memberCopy(p->second);
  else
    memberCopy(defaultLinetype);
}


// Assignment operator
Linetype& Linetype::operator=(const Linetype &rhs){
  if (this != &rhs)
    // elementwise copy
    memberCopy(rhs);

  return *this;
}


// Equality operator
bool Linetype::operator==(const Linetype &rhs) const{
  return (bmap==rhs.bmap && factor==rhs.factor);
}


void Linetype::memberCopy(const Linetype& rhs){
  // copy members
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


// static
vector<std::string> Linetype::getLinetypeInfo(){
  vector<std::string> vs;
  map<std::string,Linetype>::iterator p, pend= linetypes.end();
  int n= linetypeSequence.size();
  for (int i=0; i<n; i++) {
    if ((p=linetypes.find(linetypeSequence[i]))!=pend) {
      std::string str16= "                ";
      for (int k=0; k<16; k++)
        if ((p->second.bmap & (1 << (15-k)))!=0) str16[k]='-';
      vs.push_back(p->first + '[' + str16 + ']');
    }
  }
  return vs;
}

void Linetype::getLinetypeInfo(vector<std::string>& name,
			       vector<std::string>& pattern)
{
  map<std::string,Linetype>::iterator p, pend= linetypes.end();
  int n= linetypeSequence.size();
  for (int i=0; i<n; i++) {
    if ((p=linetypes.find(linetypeSequence[i]))!=pend) {
      std::string str16= "                ";
      for (int k=0; k<16; k++)
        if ((p->second.bmap & (1 << (15-k)))!=0) str16[k]='-';
      name.push_back( p->first );
      pattern.push_back( str16 );
    }
  }
  return;
}
