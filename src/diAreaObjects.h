/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2019 met.no

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
#ifndef _diAreaObjects_h
#define _diAreaObjects_h

#include "diWeatherObjects.h"

#include "diAreaTypes.h"

#include <string>
#include <vector>

/**
  \brief WeatherObjects with special commands

 The AreaObjects class is designed to hold a vector of WeatherArea
 objects. Each instance of the class willhave an icon, and specific
 commands like"show" and "select" can be applied. It is also possible
 to find which areas a point is inside, with the findAreas function.
*/
class AreaObjects : public WeatherObjects {
public:
  AreaObjects();
  ~AreaObjects();

  /// assign name,icon and id, and read areastring to make WeatherArea objects
  void makeAreas(const std::string& name, const std::string& icon, const std::string& areastring, int id);
  /// apply a command
  bool areaCommand(const std::string& command, const std::vector<std::string>& data);

  /// returns id
  int getId() const { return itsId; }

  /// returns name
  const std::string& getName() const
    { return itsName; }

  /// sets icon   
  void setIcon(const std::string& icon)
    { iconName = icon; }

  /// gets icon 
  const std::string& getIcon()
    { return iconName; }

private:
  int itsId;
  std::string itsName;
  std::string iconName;
};

#endif
