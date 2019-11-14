/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2019 met.no

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

#include "diAreaObjectsCluster.h"

#include "diPlotElement.h"
#include "diPlotModule.h"
#include "util/misc_util.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.AreaObjectsCluster"
#include <miLogger/miLogging.h>

static const std::string AREAOBJECTS = "AREAOBJECTS";

AreaObjectsCluster::AreaObjectsCluster()
    : PlotCluster(AREAOBJECTS)
{
}

AreaObjectsCluster::~AreaObjectsCluster()
{
}

void AreaObjectsCluster::plot(DiGLPainter* gl, PlotOrder zorder)
{
  METLIBS_LOG_WARN("plotareaobj");
  for (AreaObjects& ao : vareaobjects)
    ao.plot(gl, zorder);
}

void AreaObjectsCluster::addPlotElements(std::vector<PlotElement>& pel)
{
  for (size_t j = 0; j < vareaobjects.size(); j++) {
    AreaObjects& ao = vareaobjects[j];
    const std::string& nm = ao.getName();
    if (!nm.empty()) {
      std::string str = nm + "# " + miutil::from_number(int(j));
      pel.push_back(PlotElement(AREAOBJECTS, str, ao.getIcon(), ao.isEnabled()));
    }
  }
}

bool AreaObjectsCluster::enablePlotElement(const PlotElement& pe)
{
  if (pe.type != AREAOBJECTS)
    return false;

  for (size_t i = 0; i < vareaobjects.size(); i++) {
    AreaObjects& ao = vareaobjects[i];
    const std::string& nm = ao.getName();
    if (!nm.empty()) {
      std::string str = nm + "# " + miutil::from_number(int(i));
      if (str == pe.str) {
        if (ao.isEnabled() != pe.enabled) {
          ao.enable(pe.enabled);
          return true;
        } else {
          break;
        }
      }
    }
  }
  return false;
}

void AreaObjectsCluster::changeProjection(const Area& mapArea, const Rectangle& plotSize)
{
  PlotCluster::changeProjection(mapArea, plotSize);
  for (AreaObjects& ao : vareaobjects)
    ao.changeProjection(mapArea, plotSize);
}

void AreaObjectsCluster::makeAreaObjects(std::string name, std::string areastring, int id)
{
  METLIBS_LOG_SCOPE(LOGVAL(name) << LOGVAL(areastring) << LOGVAL(id));
  //name can be name:icon
  std::vector<std::string> tokens = miutil::split(name, ":");
  std::string icon;
  if (tokens.size() > 1) {
    icon = tokens[1];
    name = tokens[0];
  }

  //check if dataset with this id/name already exist
  areaobjects_v::iterator it =
      std::find_if(vareaobjects.begin(), vareaobjects.end(), [&](const AreaObjects& ao) { return id == ao.getId() && name == ao.getName(); });
  if (it == vareaobjects.end()) {
    // not found, add new at end
    it = vareaobjects.insert(it, AreaObjects());
    it->changeProjection(currentMapArea(), currentPlotSize());
  }
  it->makeAreas(name, icon, areastring, id);
}

void AreaObjectsCluster::areaObjectsCommand(const std::string& command, const std::string& dataSet,
    const std::vector<std::string>& data, int id)
{
  const bool is_delete_command = (command == "delete" && (data.empty() || (data.size() == 1 && data.front() == "all")));
  for (areaobjects_v::iterator it = vareaobjects.begin(); it != vareaobjects.end(); /*nothing*/) {
    AreaObjects& ao = *it;
    if ((id == -1 || id == ao.getId()) && (dataSet == "all" || dataSet == ao.getName())) {
      if (is_delete_command) {
        it = vareaobjects.erase(it);
      } else {
        ++it;
        ao.areaCommand(command, data);
      }
    } else {
      ++it;
    }
  }
}

bool AreaObjectsCluster::hasData()
{
  return !vareaobjects.empty();
}
