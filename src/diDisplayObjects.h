/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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
#ifndef _diDisplayObjects_h
#define _diDisplayObjects_h

#include "diObjectPlot.h"
#include "diWeatherObjects.h"
#include "diAreaBorder.h"
#include "diUndoFront.h"
#include "diMapMode.h"
#include "diColour.h"

#include <diField/diArea.h>
#include <diField/diGridConverter.h>

#include <map>
#include <vector>

/**
  \brief WeatherObjects to be displayed
*/
class DisplayObjects : public WeatherObjects {
public:
  DisplayObjects();

  /// initialize class variables to false/zero
  void init();
  /// parse std::string pi to define what objects to display
  bool define(const std::string& pi);
  /// prepares object plots by setting colour and alpha value, apply symbolfilter etc.
  bool prepareObjects();
  /// returns a string with objectname and time
  void getObjAnnotation(std::string &str, Colour &col);
  /// adds annotation tables from each objectplot (relevant for shapefiles)
  bool getAnnotations(std::vector<std::string>&);

  /// returns a string with object name and possibly time
  std::string getName() const;

  /// return the object name
  const std::string& getObjectName() const
    { return objectname; }

  /// returns plotInfo string
  const std::string& getPlotInfo() const
    { return pin; }

  bool isDefined() const
    { return defined; }

  bool isAutoFile() const
    { return autoFile; }

  void setAutoFile(bool a)
    { autoFile = a; }

  bool isApproved() const
    { return approved; }

  void setApproved(bool a)
    { approved = a; }

  int getTimeDiff() const
    { return timeDiff; }

private:
  bool autoFile;  //!< read new files
  bool approved;  //!< objects approved for plotting
  int timeDiff;
  std::string objectname;
  bool defined;

  std::map<std::string,bool> useobject;

  std::string pin;
  int alpha;
  int newfrontlinewidth;
  int fixedsymbolsize;
  std::vector <std::string> symbolfilter;
};

#endif
