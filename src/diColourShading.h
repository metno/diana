/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diColourShading.h 3906 2012-08-02 17:35:03Z lisbethb $

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
#ifndef diColourShading_h
#define diColourShading_h

#include "diColour.h"

#include <puTools/miString.h>

#include <map>



using namespace std;

/**

  \brief Colour shading type

  Colour shading definition, vector of colours.
  - static list of defined colour shadings, reachable by name

*/
class ColourShading {
public:

  ///List of colours and name
struct ColourShadingInfo {
  vector<Colour> colour;
  miutil::miString name;
};


private:
  miutil::miString name;
  vector<Colour> colours;
  static map<miutil::miString,ColourShading> pmap;
  static vector<ColourShadingInfo> colourshadings;

  void morecols(vector<Colour>& vcol, const Colour& col1,
		const Colour& col2, int n);

  // Copy members
  void memberCopy(const ColourShading& rhs);
public:
  // Constructors
  ColourShading(){;}
  ColourShading(const miutil::miString);
  ColourShading(const miutil::miString& name, const vector<Colour>&);
  ColourShading(const ColourShading &rhs);
  // Destructor
  ~ColourShading();

  // Assignment operator
  ColourShading& operator=(const ColourShading &rhs);
  // Equality operator
  bool operator==(const ColourShading &rhs) const;

  // static function for static palette-map
  ///define new colour shading
  static void define(const miutil::miString,
		     const vector<Colour>&);

  ///define new colour shading
  static void addColourShadingInfo(const ColourShadingInfo& csi);
  ///return all colour shadings defined
  static vector<ColourShadingInfo> getColourShadingInfo(){return colourshadings;}
  /// define ColourShading from colour,colour,colour
  static void defineColourShadingFromString(const miutil::miString str);
  ///return the colours of name_
  static vector<Colour> getColourShading(miutil::miString name_)
  {return pmap[name_].colours;}
  ///return n colours, interpolate RGB-values or drop colours if needed
  vector<Colour> getColourShading(int n);
  ///return all colours
  vector<Colour> getColourShading() const {return colours;}
  miutil::miString Name() const {return name;}

};



#endif




