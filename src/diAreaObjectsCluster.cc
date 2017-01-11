#include "diAreaObjectsCluster.h"

#include "diPlotModule.h"

#include <puTools/miStringFunctions.h>

AreaObjectsCluster::AreaObjectsCluster(PlotModule* plot)
  : plot_(plot)
{
}

AreaObjectsCluster::~AreaObjectsCluster()
{
}

void AreaObjectsCluster::plot(DiGLPainter* gl, Plot::PlotOrder zorder)
{
  for (size_t i = 0; i < vareaobjects.size(); i++) {
    vareaobjects[i].changeProjection(plot_->getMapArea()); // TODO move this somewhere else
    vareaobjects[i].plot(gl, zorder);
  }
}

void AreaObjectsCluster::addPlotElements(std::vector<PlotElement>& pel)
{
  for (size_t j = 0; j < vareaobjects.size(); j++) {
    AreaObjects& ao = vareaobjects[j];
    const std::string& nm = ao.getName();
    if (!nm.empty()) {
      std::string str = nm + "# " + miutil::from_number(int(j));
      pel.push_back(PlotElement("AREAOBJECTS", str, ao.getIcon(), ao.isEnabled()));
    }
  }
}

bool AreaObjectsCluster::enablePlotElement(const PlotElement& pe)
{
  if (pe.type != "AREAOBJECTS")
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

void AreaObjectsCluster::makeAreaObjects(std::string name, std::string areastring, int id)
{
  //   METLIBS_LOG_DEBUG("makeAreas:"<<n);
  //   METLIBS_LOG_DEBUG("name:"<<name);
  //   METLIBS_LOG_DEBUG("areastring:"<<areastring);
  //name can be name:icon
  std::vector<std::string> tokens = miutil::split(name, ":");
  std::string icon;
  if (tokens.size() > 1) {
    icon = tokens[1];
    name = tokens[0];
  }

  //check if dataset with this id/name already exist
  areaobjects_v::iterator it = vareaobjects.begin();
  while (it != vareaobjects.end() && (id != it->getId() || name != it->getName()))
    ++it;
  if (it != vareaobjects.end()) { //add new areas and replace old areas
    it->makeAreas(name, icon, areastring, id, plot_->getMapArea());
    return;
  }

  //make new dataset
  AreaObjects new_areaobjects;
  new_areaobjects.makeAreas(name, icon, areastring, id, plot_->getMapArea());
  vareaobjects.push_back(new_areaobjects);
}

void AreaObjectsCluster::areaObjectsCommand(const std::string& command, const std::string& dataSet,
    const std::vector<std::string>& data, int id)
{
  //   METLIBS_LOG_DEBUG("AreaObjectsCluster::areaCommand");
  //   METLIBS_LOG_DEBUG("id=" << id);
  //   METLIBS_LOG_DEBUG("command=" << command);
  //   METLIBS_LOG_DEBUG("data="<<data);

  int n = vareaobjects.size();
  for (int i = 0; i < n && i > -1; i++) {
    if ((id == -1 || id == vareaobjects[i].getId())
        && (dataSet == "all" || dataSet == vareaobjects[i].getName()))
    {
      if (command == "delete" && (data.empty() || (data.size() == 1 && data.front() == "all"))) {
        vareaobjects.erase(vareaobjects.begin() + i);
        i--;
        n = vareaobjects.size();
      } else {
        vareaobjects[i].areaCommand(command, data);
        //zoom to selected area
        if (command == "select" && vareaobjects[i].autoZoom()) {
          if (data.size() == 2 && data[1] == "on") {
            plot_->setMapAreaFromMap(vareaobjects[i].getBoundBox(data[0])); // TODO try to move this away from here
          }
        }
      }
    }
  }
}

std::vector<selectArea> AreaObjectsCluster::findAreaObjects(int x, int y, bool newArea)
{
  //METLIBS_LOG_DEBUG("AreaObjectsCluster::findAreas"  << x << " " << y);
  float xm = 0, ym = 0;
  plot_->PhysToMap(x, y, xm, ym);
  std::vector<selectArea> vsA;
  int n = vareaobjects.size();
  for (int i = 0; i < n; i++) {
    if (!vareaobjects[i].isEnabled())
      continue;
    std::vector<selectArea> sub_vsA;
    sub_vsA = vareaobjects[i].findAreas(xm, ym, newArea);
    vsA.insert(vsA.end(), sub_vsA.begin(), sub_vsA.end());
  }
  return vsA;
}
