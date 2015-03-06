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
#include <diWeatherFront.h>
#include <diWeatherSymbol.h>
#include <diWeatherArea.h>

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.DisplayObjects"
#include <miLogger/miLogging.h>

using namespace::miutil;
using namespace std;

DisplayObjects::DisplayObjects()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  
  init();
}

void DisplayObjects::init()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  defined=false;
  approved=false;
  objectname=std::string();
  alpha = 255;
  newfrontlinewidth=0;  // might be changed by OKString
  fixedsymbolsize=0; // might be changed by OKString
  symbolfilter.clear(); // might be changed by OKString
  autoFile=true;
  clear();
}


/*********************************************/

bool DisplayObjects::define(const std::string& pi)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  init();
  pin=pi;
  vector<std::string> tokens= miutil::split_protected(pi, '"','"');
  int i,n= tokens.size();
  if (n<2) return false;

  std::string token;
  vector<std::string> stokens;

  for (i=0; i<n; i++){
    token= miutil::to_lower(tokens[i]);
    if (miutil::contains(token, "types=")){
      setSelectedObjectTypes(token);
    } else {
      std::string key, value;
      vector<std::string> stokens= miutil::split(tokens[i], 0, "=");
      if ( stokens.size()==2) {
	key = miutil::to_lower(stokens[0]);
	value = stokens[1];
#ifdef DEBUGPRINT
	METLIBS_LOG_DEBUG("key,value" << key << " " << value);
#endif
	if ( key=="file") {
	  int l= value.length();
	  int f= value.rfind('.') + 1;
	  std::string tstr= value.substr(f,l-f);
	  itsTime= timeFromString(tstr);
	  autoFile= false;
	} else if ( key=="name") {
	  if (value[0]=='"'){
 	    objectname = value.substr(1,value.length()-2);
 	  }
	  else
	    objectname = value;
	} else if ( key=="time") {
	  itsTime = timeFromString(value);
	  autoFile= false;
	} else if (key == "timediff" ) {
	  timeDiff = atoi(value.c_str());
	} else if ( key=="alpha" || key=="alfa") {
	  alpha = (int) (atof(value.c_str())*255);
 	} else if ( key=="frontlinewidth") {
 	  newfrontlinewidth = atoi(value.c_str());
 	} else if ( key=="fixedsymbolsize") {
 	  fixedsymbolsize= atoi(value.c_str());
 	} else if ( key=="symbolfilter") {
	  vector <std::string> vals=miutil::split(value, ",");
	  for (unsigned int i=0;i<vals.size();i++)
	    symbolfilter.push_back(vals[i]);
	}
      }
    }
  }

  defined= true;
  return true;
}

/*********************************************/

bool DisplayObjects::prepareObjects()
{
  approved = false;
  if (!defined)
    return false;

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("DisplayObjects::prepareObjects");
  METLIBS_LOG_DEBUG("...area = " << itsArea.Name());
  METLIBS_LOG_DEBUG("...size of wObjects =  "<< objects.size());
  METLIBS_LOG_DEBUG("...wObjects.objectname = " <<  objectname);
  METLIBS_LOG_DEBUG("...wObjects.time = " << itsTime);
  METLIBS_LOG_DEBUG("...wObjects.filename = " << filename);
  METLIBS_LOG_DEBUG("...autoFile = " << autoFile);
#endif


  //loop over all objects
  //set alpha value for objects as requested in objectdialog
  //and set state to passive
  vector <ObjectPlot*>::iterator p = objects.begin();
  while (p!= objects.end()){
    ObjectPlot * pobject = *p;
    pobject->setPlotInfo(pin);
    pobject->setColorAlpha(alpha);
    pobject->setState(ObjectPlot::passive);
    if (newfrontlinewidth)
      pobject->setLineWidth(newfrontlinewidth);
    if (fixedsymbolsize)
      pobject->setSize(fixedsymbolsize);
    if (symbolfilter.size())
      pobject->applyFilters(symbolfilter);
    p++;
  }

  //read comments file (assume commentfile names can be obtained
  //by replacing "draw" with "comm")
  std::string commentfilename = filename;
  if (miutil::contains(commentfilename, "draw")){
    miutil::replace(commentfilename, "draw","comm");
    readEditCommentFile(commentfilename);
  }

  approved = true;
  return true;
}


/*********************************************/

void DisplayObjects::getObjAnnotation(string &str, Colour &col)
{
  if (approved) {
    str = objectname + " " + itsTime.format("%D %H:%M");
    Colour c("black");
    col = c;
  }
  else
    str.erase();
}


bool DisplayObjects::getAnnotations(vector <string>& anno)
{
  if (!isEnabled() or objects.empty())
    return false;
  for (size_t i=0; i<anno.size(); i++) {
    if (!miutil::contains(anno[i], "table") || miutil::contains(anno[i], "table="))
      continue;
    std::string endString;
    if(miutil::contains(anno[i], ",")) {
      size_t nn = anno[i].find_first_of(",");
      endString = anno[i].substr(nn);
    }
    std::string str;
    for (size_t j=0; j<objects.size(); j++) {
      if (objects[j]->getAnnoTable(str)){
	str+=endString;
	anno.push_back(str);
      }
    }
  }
  return true;
}


/*********************************************/

std::string DisplayObjects::getName() const
{
  std::string name;
  if (approved) {
    name = objectname;
    if (!autoFile)
      name += " " + itsTime.isoTime();
  }
  return name;
}
