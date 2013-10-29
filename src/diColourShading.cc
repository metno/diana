/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diColourShading.cc 3906 2012-08-02 17:35:03Z lisbethb $

  Copyright (C) 2006-2013 met.no

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

#include <diColourShading.h>

#include <puTools/miStringFunctions.h>

using namespace std;

map<std::string,ColourShading> ColourShading::pmap;
vector<ColourShading::ColourShadingInfo> ColourShading::colourshadings;

ColourShading::ColourShading(const std::string& name_)
{
  std::string lname = miutil::to_lower(name_);
  memberCopy(pmap[lname]);
}

ColourShading::ColourShading(const std::string& name_, const vector<Colour>& colours_)
{
  name = miutil::to_lower(name_);
  colours = colours_;
}

ColourShading::ColourShading(const ColourShading &rhs)
{
  // elementwise copy
  memberCopy(rhs);
}

ColourShading::~ColourShading()
{
}

// Assignment operator
ColourShading& ColourShading::operator=(const ColourShading &rhs){
  if (this != &rhs)
    // elementwise copy
    memberCopy(rhs);
  return *this;
}

bool ColourShading::operator==(const ColourShading &rhs) const
{
  int n = colours.size();
  int m = rhs.colours.size();
  if (n != m)
    return false;
  int i=0;
  while(i<n && colours[i] == rhs.colours[i])
    i++;

  if (i==n)
    return true;

  return false;
}

void ColourShading::memberCopy(const ColourShading& rhs)
{
  // copy members
  colours   = rhs.colours;
  name      = rhs.name;
}

void ColourShading::define(const std::string& name_, const vector<Colour>& colours_)
{
  std::string lname= miutil::to_lower(name_);
  ColourShading p(lname, colours_);
  pmap[lname]= p;
}

void ColourShading::defineColourShadingFromString(const std::string& str)
{
  std::string lname= str;
  vector<Colour> colours;
  vector<std::string> token = miutil::split(str, ",");

  for(size_t j=0; j<token.size(); j++) {
    colours.push_back(Colour(token[j]));
  }
  ColourShading p(lname, colours);
  pmap[lname]= p;
}

vector<Colour> ColourShading::getColourShading(int n)
{
  int ncol = colours.size();

  if (ncol<2)
    return colours;

  vector<Colour> vcol;
  if (n < ncol) { //remove colours

    int step = ncol/n;
    int ex = ncol - step*n;

    for(int i=0; i<ncol-ex; i+=step)
      vcol.push_back(colours[i]);

  } else {  //add colours

    int nadd = n - ncol; //no of col to add
    int step = nadd / (ncol-1);
    int ant = nadd - step*(ncol-1);
    int i=0;
    //add step+1 colours pr colour
    for(; i<ant; i++)
      morecols(vcol,colours[i],colours[i+1],step+1);
    //then add step colours pr colour
    for(; i<ncol-1; i++)
       morecols(vcol,colours[i],colours[i+1],step);
    vcol.push_back(colours[ncol-1]);

  }
  return vcol;
}

void ColourShading::morecols(vector<Colour>& vcol, const Colour& col1,
    const Colour& col2, int n)
{
  //add (n+1) colours to vcol, including col1

  vcol.push_back(col1);

  if(n==0) return;

  int deltaR = (col1.R()-col2.R())/(n+1);
  int deltaG = (col1.G()-col2.G())/(n+1);
  int deltaB = (col1.B()-col2.B())/(n+1);

  uchar_t R=col1.R();
  uchar_t G=col1.G();
  uchar_t B=col1.B();
  uchar_t A=col1.A();

  for(int i=0;i<n;i++){
    R -= deltaR;
    G -= deltaG;
    B -= deltaB;

    Colour c(R,G,B,A);
    vcol.push_back(c);
  }
}

void ColourShading::addColourShadingInfo(const ColourShadingInfo& csi)
{
  if (not csi.name.empty()) {
    for (unsigned int q=0; q<colourshadings.size(); q++)
      if (colourshadings[q].name == csi.name) {
	colourshadings[q] = csi;
	return;
      }
  }

  colourshadings.push_back(csi);
}
