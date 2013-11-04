/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

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

#include <diDisplayObjects.h>
#include <diDrawingTypes.h>
#include <diWeatherArea.h>
#include <diAreaObjects.h>

#include <puTools/miStringFunctions.h>

//#define DEBUGPRINT
#define MILOGGER_CATEGORY "diana.AreaObjects"
#include <miLogger/miLogging.h>

using namespace std;

AreaObjects::AreaObjects()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
 init();
 currentArea = NULL;
}



void AreaObjects::makeAreas(const std::string& name, const std::string& icon,
    const std::string& areastring, int id,const Area& area)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG("name=" << name);
  METLIBS_LOG_DEBUG("areastring=" << areastring);
#endif

  //  clear();

  clickSelect=false;
  itsId=id;
  itsName=name;
  iconName=icon;
  currentArea = NULL;
  autozoom = false;

  if (readEditDrawString(areastring,area,true))
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


bool AreaObjects::areaCommand(const std::string& command, const std::string& data)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG("command=" << command);
  METLIBS_LOG_DEBUG("data=" << data);
#endif

  std::vector<std::string> token = miutil::split(data, 1,":",true);

  bool on=false;
  if((token.size()==2 && token[1]=="on") || data=="on")
    on=true;

  if (command=="clickselect"){
    clickSelect = on;
    return true;
  }
  if (command=="autozoom"){
    autozoom = on;
    return true;
  }

  std::vector<ObjectPlot*>::iterator p = objects.begin();
  while (p!= objects.end() && (*p)->getName()!=token[0]) p++;
  if( p== objects.end() ) return false;
  ObjectPlot * pobject = *p;
  //    METLIBS_LOG_DEBUG("pobject->getName():"<<pobject->getName());
  if (command=="show"){
    pobject->setVisible(on);
  }else if (command=="select"){
    pobject->setSelected(on);
    if(on) currentArea = pobject;
    else if(currentArea == pobject)currentArea=NULL;
  }else if (command=="setcolour" && token.size()==2){
    pobject->setObjectRGBColor(token[1]);
  }else if (command=="delete"){
    objects.erase(p);
  }

  return true;
}


vector <selectArea> AreaObjects::findAreas(float x, float y, bool newArea)
{
  vector <selectArea> vsA;

  //return first area, not selected and (x,y) inside
  if(newArea){
    //select by click disabled
    if(!clickSelect) return vsA;

    //nothing new
    if(currentArea && currentArea->isInsideArea(x,y)){
      return vsA;
    }

    vector <ObjectPlot*>::iterator p = objects.begin();
    while (p!= objects.end()){
      ObjectPlot * pobject = *p;
      if (pobject->visible() &&
	  !pobject->selected() &&
	  pobject->isInsideArea(x,y)){
	selectArea sA;
	sA.name=pobject->getName();
	sA.selected=pobject->selected();
	sA.id=itsId;
	vsA.push_back(sA);
	break;
      }
      p++;
    }
  return vsA;

  }

  //return areas if selected or (x,y) inside
  vector <ObjectPlot*>::iterator p = objects.begin();
  while (p!= objects.end()){
    ObjectPlot * pobject = *p;
    if (pobject->selected() || pobject->isInsideArea(x,y) ){
      selectArea sA;
      sA.name=pobject->getName();
      sA.selected=pobject->selected();
      sA.id=itsId;
      vsA.push_back(sA);
    }
    p++;
  }
  return vsA;
}


Rectangle AreaObjects::getBoundBox(const std::string& name){
  //METLIBS_LOG_DEBUG("AreaObjects::getBoundBox " << name);
  Rectangle box;
  vector <ObjectPlot*>::iterator p = objects.begin();
  while (p!= objects.end()){
    ObjectPlot * pobject = *p;
    if (pobject->getName()==name){
      box=pobject->getBoundBox();
      break;
    }
    p++;
  }
  return box;
}
