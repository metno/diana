/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "diDisplayObjects.h"
#include "diDrawingTypes.h"
#include "diObjectsPlotCommand.h"
#include "diWeatherArea.h"
#include "diWeatherFront.h"
#include "diWeatherSymbol.h"
#include "util/misc_util.h"
#include "util/string_util.h"
#include "util/time_util.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.DisplayObjects"
#include <miLogger/miLogging.h>

using namespace::miutil;

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

  ObjectsPlotCommand_cp cmd = std::dynamic_pointer_cast<const ObjectsPlotCommand>(pc);
  if (!cmd) {
    init();
    return false;
  }

  objectname = cmd->objectname;
  if (!cmd->objecttypes.empty())
    setSelectedObjectTypes(cmd->objecttypes);
  if (!cmd->file.empty()) {
    const int l = cmd->file.length();
    const int f = cmd->file.rfind('.') + 1;
    itsTime = miutil::timeFromString(cmd->file.substr(f, l - f));
    autoFile = false;
  }
  if (!cmd->time.undef()) {
    itsTime = cmd->time;
    autoFile = false;
  }
  timeDiff = cmd->timeDiff;
  alpha = cmd->alpha;
  newfrontlinewidth = cmd->newfrontlinewidth;
  fixedsymbolsize = cmd->fixedsymbolsize;
  symbolfilter = cmd->symbolfilter;

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
    pobject->setPlotInfo(miutil::KeyValue_v());
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

void DisplayObjects::getObjAnnotation(std::string &str, Colour &col)
{
  if (approved) {
    str = objectname;
    if (!itsTime.undef())
      diutil::appendText(str, itsTime.format("%D %H:%M", "", true));
    col = Colour("black");
  } else {
    str.erase();
  }
}

void DisplayObjects::getDataAnnotations(std::vector<std::string>& anno) const
{
  if (!isEnabled() or objects.empty())
    return;
  for (std::string& a : anno) {
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
}

/*********************************************/

std::string DisplayObjects::getName() const
{
  std::string name;
  if (approved) {
    name = objectname;
    if (!autoFile && !itsTime.undef())
      diutil::appendText(name, itsTime.isoTime());
  }
  return name;
}
