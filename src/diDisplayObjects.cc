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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diDisplayObjects.h"
#include "diDrawingTypes.h"
#include "diKVListPlotCommand.h"
#include "diWeatherFront.h"
#include "diWeatherSymbol.h"
#include "diWeatherArea.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.DisplayObjects"
#include <miLogger/miLogging.h>

using namespace::miutil;
using namespace std;

DisplayObjects::DisplayObjects()
{
  METLIBS_LOG_SCOPE();
  init();
}

void DisplayObjects::init()
{
  METLIBS_LOG_SCOPE();

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

bool DisplayObjects::define(const PlotCommand_cp& pc)
{
  METLIBS_LOG_SCOPE();

  KVListPlotCommand_cp cmd = std::dynamic_pointer_cast<const KVListPlotCommand>(pc);
  if (!cmd)
    return 0;

  init();
  pin = cmd->all();

  for (const miutil::KeyValue& kv : cmd->all()){
    if (kv.key() == "types") {
      setSelectedObjectTypes(kv.value());
    } else {
      const std::string& key = kv.key();
      const std::string& value = kv.value();
      METLIBS_LOG_DEBUG(LOGVAL(key) << LOGVAL(value));
      if (key=="file") {
        int l= value.length();
        int f= value.rfind('.') + 1;
        std::string tstr= value.substr(f,l-f);
        itsTime= timeFromString(tstr);
        autoFile= false;
      } else if (key=="name") {
        objectname = value;
      } else if (key=="time") {
        itsTime = timeFromString(value);
        autoFile= false;
      } else if (key == "timediff") {
        timeDiff = kv.toInt();
      } else if (key=="alpha" || key=="alfa") {
        alpha = int(kv.toDouble()*255);
      } else if (key=="frontlinewidth") {
        newfrontlinewidth = kv.toInt();
      } else if (key=="fixedsymbolsize") {
        fixedsymbolsize= kv.toInt();
      } else if (key=="symbolfilter") {
        const std::vector<std::string> vals = miutil::split(value, ",");
        symbolfilter.insert(symbolfilter.end(), vals.begin(), vals.end());
      }
    }
  }

  defined= true;
  return true;
}

/*********************************************/

bool DisplayObjects::prepareObjects()
{
  METLIBS_LOG_SCOPE();

  approved = false;
  if (!defined)
    return false;

  METLIBS_LOG_DEBUG("...area = " << itsArea.Name());
  METLIBS_LOG_DEBUG("...size of wObjects =  "<< objects.size());
  METLIBS_LOG_DEBUG("...wObjects.objectname = " <<  objectname);
  METLIBS_LOG_DEBUG("...wObjects.time = " << itsTime);
  METLIBS_LOG_DEBUG("...wObjects.filename = " << filename);
  METLIBS_LOG_DEBUG("...autoFile = " << autoFile);

  //loop over all objects
  //set alpha value for objects as requested in objectdialog
  //and set state to passive
  for (ObjectPlot* pobject : objects) {
    pobject->setPlotInfo(pin);
    pobject->setColorAlpha(alpha);
    pobject->setState(ObjectPlot::passive);
    if (newfrontlinewidth)
      pobject->setLineWidth(newfrontlinewidth);
    if (fixedsymbolsize)
      pobject->setSize(fixedsymbolsize);
    if (symbolfilter.size())
      pobject->applyFilters(symbolfilter);
  }

  //read comments file (assume commentfile names can be obtained
  //by replacing "draw" with "comm")
  if (miutil::contains(filename, "draw")){
    std::string commentfilename = filename;
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
    str = objectname + " " + itsTime.format("%D %H:%M", "", true);
    col = Colour("black");
  } else {
    str.erase();
  }
}

bool DisplayObjects::getAnnotations(vector <string>& anno)
{
  if (!isEnabled() or objects.empty())
    return false;
  for (string& a : anno) {
    if (!miutil::contains(a, "table") || miutil::contains(a, "table="))
      continue;
    std::string endString;
    if (miutil::contains(a, ",")) {
      size_t nn = a.find_first_of(",");
      endString = a.substr(nn);
    }
    std::string str;
    for (ObjectPlot* op : objects) {
      if (op->getAnnoTable(str)) {
        str += endString;
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
