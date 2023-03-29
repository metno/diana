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

#include "diana_config.h"

#include <diDisplayObjects.h>
#include <diDrawingTypes.h>
#include <diWeatherArea.h>
#include <diAreaObjects.h>

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.AreaObjects"
#include <miLogger/miLogging.h>


AreaObjects::AreaObjects()
{
  METLIBS_LOG_SCOPE();
 init();
}

AreaObjects::~AreaObjects()
{
}

void AreaObjects::makeAreas(const std::string& name, const std::string& icon, const std::string& areastring, int id)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(name));
  METLIBS_LOG_DEBUG(LOGVAL(areastring));

  //  clear();
  itsId=id;
  itsName=name;
  iconName=icon;

  if (readEditDrawString(areastring, true))
    METLIBS_LOG_DEBUG("AreaObjects " << itsName << " id=" << itsId << " read OK!");
  else
    METLIBS_LOG_DEBUG("Areaobjects not read OK");

  std::vector <ObjectPlot*>::iterator p = objects.begin();
  while (p!= objects.end()){
    ObjectPlot * pobject = *p;
    pobject->setColorAlpha(100);
    pobject->setState(ObjectPlot::passive);
    pobject->setVisible(false);
    if (pobject->getXYZsize()<3) //not a real area
      p=objects.erase(p);
    else
      p++;
  }

}

bool AreaObjects::areaCommand(const std::string& command, const std::vector<std::string>& data)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(command));
  METLIBS_LOG_DEBUG(LOGVAL(data.size()));

  bool on = false;
  if ((data.size() == 2 || data.size() == 1) && data.back() == "on")
    on = true;

  if (data.empty())
    return false;
  std::vector<ObjectPlot*>::iterator p = objects.begin();
  while (p!= objects.end() && (*p)->getName()!=data[0])
    p++;
  if (p == objects.end())
    return false;
  ObjectPlot * pobject = *p;
  //    METLIBS_LOG_DEBUG("pobject->getName():"<<pobject->getName());
  if (command=="show"){
    pobject->setVisible(on);
  }else if (command=="select"){
    pobject->setSelected(on);
  } else if (command=="setcolour" && data.size()==2) {
    pobject->setObjectRGBColor(data[1]);
  } else if (command=="delete") {
    objects.erase(p);
  }

  return true;
}

