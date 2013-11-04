/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diColourShading.h 3906 2012-08-02 17:35:03Z lisbethb $

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
#ifndef diColourShading_h
#define diColourShading_h

#include "diColour.h"

#include <string>
#include <map>

/**
  \brief Colour shading type

  Colour shading definition, vector of colours.
  - static list of defined colour shadings, reachable by name
*/
class ColourShading {
public:

  ///List of colours and name
struct ColourShadingInfo {
  std::vector<Colour> colour;
  std::string name;
};


private:
  std::string name;
  std::vector<Colour> colours;
  static std::map<std::string,ColourShading> pmap;
  static std::vector<ColourShadingInfo> colourshadings;

  void morecols(std::vector<Colour>& vcol, const Colour& col1,
		const Colour& col2, int n);

  // Copy members
  void memberCopy(const ColourShading& rhs);
public:
  ColourShading(){;}
  ColourShading(const std::string&);
  ColourShading(const std::string& name, const std::vector<Colour>&);
  ColourShading(const ColourShading &rhs);
  ~ColourShading();

  ColourShading& operator=(const ColourShading &rhs);
  bool operator==(const ColourShading &rhs) const;

  // static function for static palette-map
  ///define new colour shading
  static void define(const std::string&, const std::vector<Colour>&);

  ///define new colour shading
  static void addColourShadingInfo(const ColourShadingInfo& csi);
  ///return all colour shadings defined
  static std::vector<ColourShadingInfo> getColourShadingInfo(){return colourshadings;}
  /// define ColourShading from colour,colour,colour
  static void defineColourShadingFromString(const std::string& str);
  ///return the colours of name_
  static std::vector<Colour> getColourShading(const std::string& name_)
    {return pmap[name_].colours;}
  ///return n colours, interpolate RGB-values or drop colours if needed
  std::vector<Colour> getColourShading(int n);
  ///return all colours
  std::vector<Colour> getColourShading() const {return colours;}
  std::string Name() const {return name;}
};

#endif
