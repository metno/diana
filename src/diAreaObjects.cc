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

#include <diCommonTypes.h>
#include <diDisplayObjects.h>
#include <diDrawingTypes.h>
#include <diWeatherArea.h>
#include <diAreaObjects.h>
//#define DEBUGPRINT

using namespace::miutil;

AreaObjects::AreaObjects(){
#ifdef DEBUGPRINT
  DEBUG_ << "AreaObjects::AreaObjects\n";
#endif
 init();
 currentArea = NULL;
}



void AreaObjects::makeAreas(const miString& name, const miString& icon,
			    const miString& areastring, int id,const Area& area){
#ifdef DEBUGPRINT
  DEBUG_ << "AreaObjects::makeAreas\n";
  DEBUG_ << "name=" << name;
  DEBUG_ << "areastring=" << areastring;
#endif

  //  clear();

  clickSelect=false;
  itsId=id;
  itsName=name;
  iconName=icon;
  currentArea = NULL;
  autozoom = false;

  if (readEditDrawString(areastring,area,true))
    DEBUG_ << "AreaObjects " << itsName << " id=" << itsId << " read OK!";

  else
    DEBUG_ << "Areaobjects not read OK";

  vector <ObjectPlot*>::iterator p = objects.begin();
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


bool AreaObjects::areaCommand(const miString& command,const miString& data){
#ifdef DEBUGPRINT
  DEBUG_ << "Areaobjects::areaCommand";
  DEBUG_ << "command=" << command;
  DEBUG_ << "data=" << data;
#endif

  vector<miString> token = data.split(1,":",true);

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

  vector <ObjectPlot*>::iterator p = objects.begin();
  while (p!= objects.end() && (*p)->getName()!=token[0]) p++;
  if( p== objects.end() ) return false;
  ObjectPlot * pobject = *p;
  //    DEBUG_ <<"pobject->getName():"<<pobject->getName();
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


vector <selectArea> AreaObjects::findAreas(float x, float y, bool newArea){

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


Rectangle AreaObjects::getBoundBox(const miString& name){
  //DEBUG_ << "AreaObjects::getBoundBox " << name;
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



