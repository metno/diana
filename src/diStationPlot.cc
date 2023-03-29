/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2022 met.no

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

#include "diGlUtilities.h"
#include "diPlotModule.h"
#include "diStaticPlot.h"
#include "diStationPlot.h"
#include "diUtilities.h"
#include "util/misc_util.h"

#include <mi_fieldcalc/math_util.h>

#include <puTools/miStringFunctions.h>

#include <boost/algorithm/string.hpp>

#include <QString>

#include <sstream>

#define MILOGGER_CATEGORY "diana.StationPlot"
#include <miLogger/miLogging.h>

using namespace::miutil;

static std::string ddString[16]; // norwegian directions North, NorthNorthEast, NorthEast, EastNorthEast, etc.

static const float RAD_TO_DEG = 180 / M_PI;

//! distance in km
static double distance(const miCoordinates& pos, const Station* s)
{
  return pos.distanceTo(s->pos) / 1000;
}

StationPlot::StationPlot(const std::vector<float> & lons, const std::vector<float> & lats)
{
  init();
  const size_t n = std::min(lons.size(), lats.size());
  for (unsigned int i = 0; i < n; i++) {
    addStation(lons[i], lats[i]);
  }
  defineCoordinates();
}

StationPlot::StationPlot(const std::vector<std::string> & names,
    const std::vector<float> & lons, const std::vector<float> & lats)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  init();
  const size_t n = std::min(names.size(), std::min(lons.size(), lats.size()));
  for (size_t i = 0; i < n; i++) {
    addStation(lons[i], lats[i], names[i]);
  }
  defineCoordinates();
}

StationPlot::StationPlot(const std::vector<stationInfo> & stations)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  init();
  for (const stationInfo& si : stations) {
    addStation(si.lon, si.lat, si.name);
  }
  defineCoordinates();
}

StationPlot::StationPlot(const std::vector<std::string> & names,
    const std::vector<float> & lons, const std::vector<float> & lats,
    const std::vector<std::string>& images)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  init();
  const size_t n = std::min(names.size(), std::min(lons.size(), lats.size()));
  for (size_t i = 0; i < n; i++) {
    addStation(lons[i], lats[i], names[i], images[i]);
  }
  useImage = true;
  defineCoordinates();
}

StationPlot::StationPlot(const std::vector <Station*> &stations)
{
  init();
  for (Station* si : stations)
    addStation(si);

  useImage = false;
  defineCoordinates();
}

StationPlot::StationPlot(const std::string& commondesc, const std::string& common,
    const std::string& description, int from, const std::vector<std::string>& data)
{
  init();

  id = from;
  std::vector<std::string> vcommondesc;
  std::vector<std::string> vcommon;
  std::vector<std::string> vdesc;
  boost::algorithm::split(vcommondesc, commondesc, boost::algorithm::is_any_of(":"));
  boost::algorithm::split(vcommon, common, boost::algorithm::is_any_of(":"));
  boost::algorithm::split(vdesc, description, boost::algorithm::is_any_of(":"));

  if (vcommondesc.size() != vcommon.size()) {
    METLIBS_LOG_ERROR("commondesc:" << commondesc << " and common:" << common
        << " do not match");
    return;
  }

  int num = vcommondesc.size();
  std::map<std::string, int> commonmap;
  for (int i = 0; i < num; i++)
    commonmap[vcommondesc[i]] = i;
  if (commonmap.count("dataset")) {
    name = vcommon[commonmap["dataset"]];
    setPlotName(name);
  }
  if (commonmap.count("showname")) {
    useStationNameNormal = (vcommon[commonmap["showname"]] == "true");
    useStationNameSelected = (vcommon[commonmap["showname"]] == "true"
        || vcommon[commonmap["showname"]] == "selected");
  }
  if (commonmap.count("showtext"))
    showText = (vcommon[commonmap["showtext"]] == "true");
  //obsolete
  if (commonmap.count("normal"))
    useStationNameNormal = (vcommon[commonmap["normal"]] == "true");
  //obsolete
  if (commonmap.count("selected"))
    useStationNameSelected = (vcommon[commonmap["selected"]] == "true");
  if (commonmap.count("image")) {
    imageNormal = imageSelected = vcommon[commonmap["image"]];
    useImage = true;
  }
  if (commonmap.count("icon"))
    iconName = vcommon[commonmap["icon"]];
  if (commonmap.count("annotation"))
    annotation = vcommon[commonmap["annotation"]];

  //decode data
  int ndata = data.size();
  std::map<std::string, int> datamap;
  unsigned int ndesc = vdesc.size();
  for (unsigned int i = 0; i < ndesc; i++)
    datamap[vdesc[i]] = i;
  if (!datamap.count("name") || !datamap.count("lat") || !datamap.count("lon")) {
    METLIBS_LOG_ERROR("diStationPlot:");
    METLIBS_LOG_ERROR(" positions must contain name:lat:lon");
    return;
  }
  std::string stationname;
  float lat, lon;
  int alpha = 255;
  for (int i = 0; i < ndata; i++) {
    std::vector<std::string> token;
    boost::algorithm::split(token, data[i], boost::algorithm::is_any_of(":"));
    if (token.size() != ndesc)
      continue;
    stationname = token[datamap["name"]];
    lat = atof(token[datamap["lat"]].c_str());
    lon = atof(token[datamap["lon"]].c_str());
    if (datamap.count("alpha"))
      alpha = atoi(token[datamap["alpha"]].c_str());
    if (datamap.count("image")) {
      std::string image = token[datamap["image"]];
      addStation(lon, lat, stationname, image, alpha);
      useImage = true;
    } else {
      addStation(lon, lat, stationname);
    }
  }

  defineCoordinates();

  setVisible(false);
}

void StationPlot::init()
{
  // Add a default plot area.
  StationArea area(-90.0, 90.0, -180.0, 180.0);
  stationAreas.push_back(area);

  //coordinates to be plotted
  setVisible(true);
  useImage = true;
  useStationNameNormal = false;
  useStationNameSelected = false;
  showText = true;
  textColour = Colour("white");
  textSize = 16;
  textStyle = "bold";
  name = "vprof";
  id = -1;
  index = -1;
}

StationPlot::~StationPlot()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  diutil::delete_all_and_clear(stations);
}

void StationPlot::addStation(float lon, float lat, const std::string& newname, const std::string& newimage, int alpha)
{
  //at the moment, this should only be called from constructor, since
  //define coordinates must be called to actually plot stations
  std::unique_ptr<Station> newStation(new Station);
  if (not newname.empty()) {
    newStation->name = newname;
  } else {
    miCoordinates coordinates(lon, lat);
    newStation->name = coordinates.str();
  }
  newStation->pos = miCoordinates(lon, lat);
  newStation->alpha = alpha;
  newStation->isSelected = false;
  if (newimage == "HIDE") {
    newStation->isVisible = false;
  } else {
    newStation->isVisible = true;
    newStation->image = newimage;
  }
  newStation->status = Station::noStatus;

  addStation(newStation.release());
}

void StationPlot::addStation(Station* station)
{
  stations.push_back(station);

  // Add the station to the area tree.
  stationAreas[0].addStation(station);
}

void StationPlot::plot(DiGLPainter* gl, PlotOrder zorder)
{
  /* Plot stations at positions xplot,yplot if stations[i]->isVisible
   different plotting options
   if stations[i]->image not empty, plot image
   else if useImage==true, plot imageNormal or imageSelected
   else plot red crosses, yellow circle around selected station
   if useStationNameNormal/useStationNameSelected==true plot name
   */

  METLIBS_LOG_SCOPE(LOGVAL(name));

  if (!isEnabled() || !visible || zorder != PO_LINES)
    return;

  std::map<Station::Status, std::vector<int> > selected; //index of selected stations for each status
  std::map<Station::Status, std::vector<int> > unselected; //index of unselected stations for each status

  //first loop, only unselected stations
  int n = stations.size();
  for (int i = 0; i < n; i++) {
    if (!stations[i]->isVisible)
      continue;
    if (stations[i]->isSelected) {
      selected[stations[i]->status].push_back(i);
      continue;
    }
    unselected[stations[i]->status].push_back(i);
  }

  static Station::Status plotOrder[5] = {Station::noStatus, Station::unknown, Station::working, Station::underRepair, Station::failed};

  for (unsigned int i = 0; i < 5; ++i) {
    for (std::vector<int>::iterator it = unselected[plotOrder[i]].begin(); it != unselected[plotOrder[i]].end(); it++) {
      plotStation(gl, *it);
    }
    for (std::vector<int>::iterator it = selected[plotOrder[i]].begin(); it != selected[plotOrder[i]].end(); it++) {
      plotStation(gl, *it);
    }
  }
}

void StationPlot::plotStation(DiGLPainter* gl, int i)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE(LOGVAL(i));
#endif

  float h = 0; //height for displaying text
  Station* si = stations[i];
  float x = xplot[i];
  float y = yplot[i];
  bool plotted = true;

  if (useImage) {
    // use either si->image or imageNormal/imageSelected
    if (!si->image.empty() && si->image2.empty()) {
      if (si->image == "wind") {
        h = (si->isSelected ? 40 : 30) * getStaticPlot()->getPhysToMapScaleY();
        plotWind(gl, i, x, y);
      } else {
        h = ig.height_(si->image) * getStaticPlot()->getPhysToMapScaleY();
        if (!ig.plotImage(gl, getStaticPlot()->plotArea(), si->image, x, y, true, 1, si->alpha))
          plotted = false;
      }
      if (si->isSelected && si->image != "wind")
        gl->setColour(Colour::RED);
      glPlot(gl, Station::noStatus, x, y);
    } else if (!si->image.empty() && !si->image2.empty()) {
      float h1 = ig.height_(si->image);
      float h2 = ig.height_(si->image2);
      h = std::max(h1, h2) * getStaticPlot()->getPhysToMapScaleY();
      float w1 = ig.width_(si->image);
      float w2 = ig.width_(si->image2);
      gl->setColour(Colour(128, 128, 128)); // grey
      glPlot(gl, Station::noStatus, x, y);
      if (!ig.plotImage(gl, getStaticPlot()->plotArea(), si->image, x - w1 / 2, y, true, 1, si->alpha))
        plotted = false;
      if (!ig.plotImage(gl, getStaticPlot()->plotArea(), si->image2, x + w2 / 2, y, true, 1, si->alpha))
        plotted = false;
      if (si->isSelected)
        gl->setColour(Colour::RED);
      glPlot(gl, Station::noStatus, x, y);
    } else if (!si->isSelected && !imageNormal.empty()) {
      //otherwise plot images for selected/normal stations
      if (!ig.plotImage(gl, getStaticPlot()->plotArea(), imageNormal, x, y, true, 1, si->alpha))
        plotted = false;
      h = ig.height_(imageNormal) * getStaticPlot()->getPhysToMapScaleY();
    } else if (si->isSelected && !imageSelected.empty()) {
      if (!ig.plotImage(gl, getStaticPlot()->plotArea(), imageSelected, x, y, true, 1, si->alpha))
        plotted = false;
      h = ig.height_(imageSelected) * getStaticPlot()->getPhysToMapScaleY();
    } else {
      //if no image plot crosses and circles for selected/normal stations
      //METLIBS_LOG_DEBUG("useImage=false");
      glPlot(gl, Station::failed, x, y, si->isSelected);
    }

    //if something went wrong,
    //plot crosses and circles for selected/normal stations
    if (!plotted) {
      glPlot(gl, Station::failed, x, y, si->isSelected);
    }
  } else if (si->status != Station::noStatus) {
    glPlot(gl, si->status, x, y, si->isSelected);
  }

  if ((useStationNameNormal && !si->isSelected) || (useStationNameSelected && si->isSelected)) {
    float cw, ch;
    gl->setFont(diutil::BITMAPFONT, diutil::F_NORMAL, 10);
    gl->getTextSize(si->name, cw, ch);
    if (useStationNameNormal && !si->isSelected) {
      gl->setColour(Colour::BLACK);
      gl->drawText(si->name, x - cw / 2, y + h / 2, 0.0);
    } else if (useStationNameSelected && si->isSelected) {
      gl->setColour(Colour::WHITE);
      glPlot(gl, Station::noStatus, x, y + h / 2 + ch * 0.1);
      gl->setColour(Colour::BLACK);
      gl->drawText(si->name, x - cw / 2, y + h / 2 + ch * 0.35, 0.0);
    }
  }

  if (showText || si->isSelected) {
    for (const auto& t : si->vsText) {
      float cw, ch;
      gl->setColour(textColour);
      const std::string& text = t.text;
      gl->setFont(diutil::BITMAPFONT, textStyle, textSize);
      gl->getTextSize(text, cw, ch);
      if (t.hAlign == align_center)
        gl->drawText(text, x - cw / 2, y - ch / 4, 0.0);
      else if (t.hAlign == align_top)
        gl->drawText(text, x - cw / 2, y + h / 2, 0.0);
      else if (t.hAlign == align_bottom) {
        if (si->isSelected)
          gl->setColour(Colour::WHITE);
        glPlot(gl, Station::noStatus, x, y - h / 1.9 - ch * 1.0);
        gl->setColour(textColour);
        gl->drawText(text, x - cw / 2, y - h / 1.9 - ch * 0.7, 0.0);
      }
    }
  }
}

std::string StationPlot::getEnabledStateKey() const
{
  return "stationplot-" + getName();
}

void StationPlot::setVisible(bool vis)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  visible = vis;
  unselect();
  if (visible)
    switchProjection();
}

void StationPlot::unselect()
{
  for (Station* si : stations)
    si->isSelected = false;
  index = -1;
}

bool StationPlot::isVisible() const
{
  return visible;
}

const std::string& StationPlot::getPlotName() const
{
  static const std::string EMPTY;
  return isVisible() ? Plot::getPlotName() : EMPTY;
}

const std::string& StationPlot::getIconName() const
{
  return iconName;
}

void StationPlot::defineCoordinates()
{
  //should be called from constructor and when new stations have
  //been added
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  // correct spec. when making Projection for long/lat coordinates
  //positions from lat/lon will be converted in changeprojection
  const size_t npos = stations.size();
  xplot.reset(new float[npos]);
  yplot.reset(new float[npos]);
  switchProjection();
}

void StationPlot::changeProjection(const Area& /*mapArea*/, const Rectangle& /*plotSize*/, const diutil::PointI& /*physSize*/)
{
  switchProjection();
}

bool StationPlot::switchProjection()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE("Change projection to: "<< area << " wind might not be rotated");
#endif

  const size_t npos = stations.size();
  if (npos == 0) {
    return false;
  }

  size_t i = 0;
  for (Station* si : stations) {
    xplot[i] = si->lon();
    yplot[i] = si->lat();
    i += 1;
  }

  if (!getStaticPlot()->GeoToMap(npos, xplot.get(), yplot.get())) {
    METLIBS_LOG_ERROR("getPoints error");
    return false;
  }

  // TODO rotate wind

  return true;
}

std::vector<Station*> StationPlot::getStations() const
{
  return stations;
}

Station* StationPlot::stationAt(int phys_x, int phys_y)
{
  using miutil::square;

  const float radius = 100;
  std::vector<Station*> found = stationsAt(phys_x, phys_y, radius);

  if (found.empty())
    return 0;

  const XY pos = getStaticPlot()->PhysToMap(XY(phys_x, phys_y));

  float min_r = square(radius * getStaticPlot()->getPhysToMapScaleX());
  int min_i = 0;

  // Find the closest station to the point within a given radius.
  for (unsigned int i = 0; i < found.size(); ++i) {
    float sx = found[i]->lon(), sy = found[i]->lat();
    if (getStaticPlot()->GeoToMap(1, &sx, &sy)) {
      float r = square(pos.x() - sx) + square(pos.y() - sy);
      if (r < min_r) {
        min_r = r;
        min_i = i;
      }
    }
  }

  return found[min_i];
}

std::vector<Station*> StationPlot::stationsAt(int phys_x, int phys_y, float radius, bool useAllStations)
{
  const XY pos = getStaticPlot()->PhysToMap(XY(phys_x, phys_y));

  float min_r = miutil::square(radius * getStaticPlot()->getPhysToMapScaleX());

  std::vector<Station*> within;

  float geo_x = pos.x(), geo_y = pos.y();
  if (getStaticPlot()->MapToGeo(1, &geo_x, &geo_y)) {
    std::vector<Station*> found;
    if (useAllStations) {
      found = stations;
    } else {
      found = stationAreas[0].findStations(geo_y, geo_x, radius);
    }

    const auto tf = getStaticPlot()->getMapProjection().transformationFrom(Projection::geographic());
    for (unsigned int i = 0; i < found.size(); ++i) {
      if (found[i]->isVisible) {
        float sx = found[i]->lon(), sy = found[i]->lat();
        if (tf->forward(1, &sx, &sy)) {
          float r = miutil::absval2(pos.x() - sx, pos.y() - sy);
          if (r < min_r) {
            within.push_back(found[i]);
          }
        }
      }
    }
  }

  return within;
}

std::vector<std::string> StationPlot::findStation(int x, int y, bool add)
{
  std::vector<std::string> stationstring;

  if (!visible || !isEnabled())
    return stationstring;

  Station* found = stationAt(x, y);
  if (found && (index < 0 || found != stations[index])) {

    add = found->isSelected || add;
    setSelectedStation(found->name, add);

    for (const Station* si : stations) {
      if (si->isSelected)
        stationstring.push_back(si->name);
    }
  }

  return stationstring;
}

std::vector<std::string> StationPlot::findStations(int x, int y)
{
  std::vector<std::string> stationstring;

  if (!visible || !isEnabled())
    return stationstring;

  std::vector<Station*> found = stationsAt(x, y, 10.0f, true);

  for (unsigned int i = 0; i < found.size(); i++) {
    stationstring.push_back(found[i]->name);
  }

  return stationstring;
}

std::vector<Station*> StationPlot::getSelectedStations() const
{
  std::vector<Station*> selected;
  for (unsigned int i = 0; i < stations.size(); ++i) {
    if (stations[i]->isSelected)
      selected.push_back(stations[i]);
  }
  return stations;
}

void StationPlot::setSelectedStations(const std::vector<std::string>& station)
{
  METLIBS_LOG_SCOPE(LOGVAL(station.size()));

  int n = stations.size();
  for (int j = 0; j < n; j++) {
    stations[j]->isSelected = false;
  }

  int m = station.size();
  for (int j = 0; j < m; j++){
    for (int i = 0; i < n; i++)
      if (stations[i]->name == station[j]) {
        setSelectedStation(i,true);
      }
  }
}

int StationPlot::setSelectedStation(std::string station, bool add)
{
  METLIBS_LOG_SCOPE(LOGVAL(station));

  int n = stations.size();
  for (int i = 0; i < n; i++)
    if (stations[i]->name == station) {
      return setSelectedStation(i);
    }
  return -1;
}

/**
 Selects the station specified by its index \a i. If \a add is true then
 the station will be added to the existing selection; otherwise, the
 existing selection will be cleared before the station is added to it.

 Returns the index of the station if it was added to the selection, or -1
 if it could not be added.
 */
int StationPlot::setSelectedStation(int i, bool add)
{
  METLIBS_LOG_SCOPE(LOGVAL(i));

  int n = stations.size();

  //remove old selections
  if (!add) {
    for (int j = 0; j < n; j++) {
      stations[j]->isSelected = false;
    }
  }

  //select
  if (i < n && i > -1) {
    stations[i]->isSelected = true;
    index = i;

    return i;
  } else {
    index = -1;
  }
  return -1;
}

void StationPlot::getAnnotation(std::string &str, Colour &col) const
{
  if (visible) {
    str = annotation;
    col = Colour("red");
  } else {
    str.erase();
  }
}

void StationPlot::setStationPlotAnnotation(const std::string &str)
{
  annotation = str;
}

void StationPlot::setName(const std::string& nm)
{
  name = nm;
  if (getPlotName().empty())
    setPlotName(name);
}

void StationPlot::setImage(std::string im1)
{
  imageNormal = im1;
  imageSelected = im1;
  useImage = true;
}

void StationPlot::setImage(std::string im1, std::string im2)
{
  imageNormal = im1;
  imageSelected = im2;
  useImage = true;
}

void StationPlot::clearText()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  int n = stations.size();
  for (int i = 0; i < n; i++) {
    stations[i]->vsText.clear();
  }
}

void StationPlot::setUseStationName(bool normal, bool selected)
{
  useStationNameNormal = normal;
  useStationNameSelected = selected;
}


bool StationPlot::stationCommand(const std::string& command,
    const std::vector<std::string>& data, const std::string& misc)
{
  METLIBS_LOG_SCOPE();
  if (command == "changeImageandText") {
    std::vector<std::string> description;
    boost::algorithm::split(description, misc, boost::algorithm::is_any_of(":"));
    int ndesc = description.size();
    std::map<std::string, int> datamap;
    for (int i = 0; i < ndesc; i++)
      datamap[description[i]] = i;
    if (!datamap.count("name")) {
      METLIBS_LOG_ERROR(" StationPlot::stationCommand: missing name of station");
      return false;
    }
    bool chImage, chImage2, chText, defAlign, ch_dd, ch_ff, ch_colour, ch_alpha;;
    chImage = chImage2 = chText = defAlign = ch_dd = ch_ff = ch_colour
        = ch_alpha = false;
    if (datamap.count("image"))
      chImage = true;
    if (datamap.count("image2"))
      chImage2 = true;
    if (datamap.count("text"))
      chText = true;
    if (datamap.count("alignment"))
      defAlign = true;
    if (datamap.count("dd"))
      ch_dd = true;
    if (datamap.count("ff"))
      ch_ff = true;
    if (datamap.count("colour"))
      ch_colour = true;
    if (datamap.count("alpha"))
      ch_alpha = true;

    //decode data
    int n = data.size();
    std::string name, image, image2, text, alignment;
    int dd=0, ff=0, alpha=0;
    Colour colour;
    for (int i = 0; i < n; i++) {
      //       METLIBS_LOG_DEBUG("StationPlot::stationCommand:data:"<<data[i]);
      std::vector<std::string> token;
      boost::algorithm::split(token, data[i], boost::algorithm::is_any_of(":"));
      if (token.size() < description.size()) {
        METLIBS_LOG_ERROR("Description:" << misc << " and data:" << data[i] << " do not match");
        continue;
      }
      name = token[datamap["name"]];
      if (chImage)
        image = token[datamap["image"]];
      if (chImage2)
        image2 = token[datamap["image2"]];
      if (chText)
        text = token[datamap["text"]];
      if (defAlign)
        alignment = token[datamap["alignment"]];
      if (ch_dd)
        dd = atoi(token[datamap["dd"]].c_str());
      if (ch_ff)
        ff = atoi(token[datamap["ff"]].c_str());
      if (ch_colour)
        colour = Colour(token[datamap["colour"]]);
      if (ch_alpha)
        alpha = atoi(token[datamap["alpha"]].c_str());

      if (image == "_")
        image = "";
      if (image2 == "_")
        image2 = "";

      //find station
      int m = stations.size();
      for (int i = 0; i < m; i++) {
        if (stations[i]->name == name) {
          stations[i]->vsText.clear(); //clear text
          if (chImage) {
            if (image == "HIDE") {
              stations[i]->isVisible = false;
            } else {
              stations[i]->isVisible = true;
              stations[i]->image = image;
            }
          }
          stations[i]->image2 = image2;

          if (ch_alpha)
            stations[i]->alpha = alpha;

          if (chText) {
            stationText stext;
            stext.hAlign = align_center; //default
            if (text != "_") { //underscore means no text
              stext.text = text;
              if (alignment == "center")
                stext.hAlign = align_center;
              else if (alignment == "top")
                stext.hAlign = align_top;
              else if (alignment == "bottom")
                stext.hAlign = align_bottom;
              stations[i]->vsText.push_back(stext);
            }
          }

          if (ch_dd && ch_ff) {
            //init ddString
            if (ddString[0].empty()) {
              ddString[0] = "N";
              ddString[1] = "NN\xD8";
              ddString[2] = "N\xD8";
              ddString[3] = "\xD8N\xD8";
              ddString[4] = "\xD8";
              ddString[5] = "\xD8S\xD8";
              ddString[6] = "S\xD8";
              ddString[7] = "SS\xD8";
              ddString[8] = "S";
              ddString[9] = "SSV";
              ddString[10] = "SV";
              ddString[11] = "VSV";
              ddString[12] = "V";
              ddString[13] = "VNV";
              ddString[14] = "NV";
              ddString[15] = "NNV";
            }
            stations[i]->image = "wind";
            stations[i]->isVisible = true;
            //change projection from geo to current
            int num = 1;
            float* xx = new float[num];
            float* yy = new float[num];
            float* u = new float[num];
            float* v = new float[num];
            xx[0] = xplot[i];
            yy[0] = yplot[i];
            u[0] = 0;
            v[0] = 10;
            getStaticPlot()->GeoToMap(num, xx, yy, u, v);
            int angle = (int) (atan2f(u[0], v[0]) * RAD_TO_DEG);
            dd += angle;
            if (dd < 1)
              dd += 360;
            if (dd > 360)
              dd -= 360;
            stations[i]->dd = dd;
            stations[i]->ff = ff;
            stations[i]->north = angle;
            delete[] xx;
            delete[] yy;
            delete[] v;
            delete[] u;
            if (ch_colour)
              stations[i]->colour = colour;
          }
          break;
        }
      }
    }

    return true;

  } else if (command == "annotation" && data.size() > 0) {
    setStationPlotAnnotation(data[0]);
    return true;
  }

  else if (command == "setSelectedStation" && data.size() > 0) {
    setSelectedStation(data[0]);
    return true;
  }

  else if (command == "setSelectedStations" ) {
    setSelectedStations(data);
    return true;
  }

  else if (command == "showPositionName" && data.size() > 0) {
    std::vector<std::string> token;
    boost::algorithm::split(token, data[0], boost::algorithm::is_any_of(":"));
    if (token.size() < 2) // obsolete
      boost::algorithm::split(token, data[0], boost::algorithm::is_any_of(","));
    if (token.size() < 2)
      return false;
    bool normal = false, selected = false;
    if (token[0] == "true")
      normal = true;
    if (token[1] == "true")
      selected = true;
    if (token.size() == 3) {
      if (token[2] == "true")
        useImage = true;
      else
        useImage = false;
    }
    setUseStationName(normal, selected);

    return true;
  }

  else if (command == "showPositionText" && data.size() > 0) {

    std::vector<std::string> description;
    boost::algorithm::split(description, misc, boost::algorithm::is_any_of(":"));
    int ndesc = description.size();
    std::map<std::string, int> datamap;
    for (int i = 0; i < ndesc; i++)
      datamap[description[i]] = i;
    if (!datamap.count("showtext")) {
      METLIBS_LOG_ERROR(" StationPlot::stationCommand: description must contain showtext");
      return false;
    }
    std::vector<std::string> token;
    boost::algorithm::split(token, data[0], boost::algorithm::is_any_of(":"));
    if (token.size() < description.size()) {
      METLIBS_LOG_ERROR("StationPlot::stationCommand: Description:" << misc
          << " and data:" << data[0] << " do not match");
      return false;
    }
    showText = (token[datamap["showtext"]] == "true"
        || token[datamap["showtext"]] == "normal");
    if (datamap.count("colour"))
      textColour = Colour(token[datamap["colour"]]);
    if (datamap.count("size"))
      textSize = atoi(token[datamap["size"]].c_str());
    if (datamap.count("style"))
      textStyle = token[datamap["style"]].c_str();

    return true;
  }

  return false;
}

bool StationPlot::stationCommand(const std::string& command)
{
  if (command == "show") {
    setVisible(true);
  } else if (command == "hide") {
    setVisible(false);
  } else if (command == "unselect") {
    unselect();
  } else if (command == "showPositionName" ) {
    setUseStationName(true, true);
  } else {
    return false;
  }
  return true;
}

void StationPlot::glPlot(DiGLPainter* gl, Station::Status tp, float x, float y, bool selected)
{
  //called from StationPlot::plotStation: Add GL things to plot here.
  const float scale = 1.5*getStaticPlot()->getPhysToMapScaleX();
  float linewidth, r;
  DiGLPainter::GLfloat radius;
  switch (tp) {
  case Station::unknown:
    linewidth = 2;
    r = linewidth * scale;
    //plot grey transparent square
    gl->setColour(Colour(100, 100, 100, 50));
    gl->Enable(DiGLPainter::gl_BLEND);
    gl->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);
    gl->drawRect(true, x - r, y - r, x + r, y + r);
    gl->Disable(DiGLPainter::gl_BLEND);
    break;
  case Station::failed:
    linewidth = 4;
    gl->LineWidth(linewidth);
    r = linewidth * scale;
    //plot crosses
    gl->setColour(Colour::RED);
    gl->drawCross(x, y, r, true);
    break;
  case Station::underRepair:
    linewidth = 4;
    gl->LineWidth(linewidth);
    r = linewidth * scale;
    //plot crosses
    gl->setColour(Colour::YELLOW);
    gl->drawCross(x, y, r, true);
    break;
  case Station::working:
    linewidth = 4;
    gl->LineWidth(linewidth);
    r = linewidth * scale;
    //plot crosses
    gl->setColour(Colour::GREEN);
    gl->drawCross(x, y, r, true);
    break;
  case Station::noStatus:
  default:
    ;
  }

  // Plot a yellow circle if the station is selected.
  if (selected) {
    linewidth = 4;
    gl->LineWidth(linewidth);
    r = linewidth * scale;
    radius = 1.5 * r;
    gl->setColour(Colour::YELLOW);
    gl->drawCircle(false, x, y, radius);
  }
}

void StationPlot::plotWind(DiGLPainter* gl, int ii, float x, float y, bool classic, float scale)
{
  gl->Enable(DiGLPainter::gl_BLEND);
  gl->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);

  Colour colour = stations[ii]->colour;
  colour.set(Colour::alpha, stations[ii]->alpha);

  int dd = stations[ii]->dd;
  int ff = stations[ii]->ff;
  float radius = scale * 14 * getStaticPlot()->getPhysToMapScaleX();

  diutil::GlMatrixPushPop pushpop(gl);
  gl->Translatef(x, y, 0.0);
  gl->Scalef(radius, radius, 0.0);

  if (stations[ii]->isSelected) {
    //compass
    int linewidth = 1;
    gl->LineWidth(linewidth);
    gl->setColour(Colour::BLACK);
    gl->drawCircle(false, 0, 0, 1);

    diutil::GlMatrixPushPop pushpop2(gl);
    gl->Rotatef(-1 * stations[ii]->north, 0.0, 0.0, 1.0);
    gl->Begin(DiGLPainter::gl_LINES);
    gl->Vertex2f(0.1, 1);
    gl->Vertex2f(0.1, 1.5);
    gl->Vertex2f(-0.1, 1.0);
    gl->Vertex2f(-0.1, 1.5);

    gl->Vertex2f(0.0, 1.5);
    gl->Vertex2f(0.4, 1.1);
    gl->Vertex2f(0.0, 1.5);
    gl->Vertex2f(-0.4, 1.1);

    gl->Vertex2f(0.05, -1);
    gl->Vertex2f(0.05, -1.3);
    gl->Vertex2f(-0.05, -1);
    gl->Vertex2f(-0.05, -1.3);
    gl->End();

    for (int i = 0; i < 7; i++) {
      gl->Rotatef(45.0, 0.0, 0.0, 1.0);
      gl->Begin(DiGLPainter::gl_LINES);
      gl->Vertex2f(0, 1.0);
      gl->Vertex2f(0, 1.3);
      gl->End();
    }
  }

  if (ff < 1) {
    gl->Rotatef(45, 0.0, 0.0, 1.0);
  } else {
    gl->Rotatef(-1 * (dd + 180), 0.0, 0.0, 1.0);
  }

  gl->setColour(Colour::BLACK);
  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK,DiGLPainter::gl_FILL);

  // ff in m/s
  if (ff < 1) {
    gl->Begin(DiGLPainter::gl_POLYGON);
    gl->Vertex2f(-0.1, 0.5);
    gl->Vertex2f(-0.1, -0.5);
    gl->Vertex2f(0.1, -0.5);
    gl->Vertex2f(0.1, 0.5);
    gl->End();
    gl->Begin(DiGLPainter::gl_POLYGON);
    gl->Vertex2f(-0.5, 0.1);
    gl->Vertex2f(-0.5, -0.1);
    gl->Vertex2f(0.5, -0.1);
    gl->Vertex2f(0.5, 0.1);
    gl->End();

  } else {
    if (classic) {
      // main body
      gl->setColour(colour);
      gl->Begin(DiGLPainter::gl_POLYGON);
      gl->Vertex2f(-0.1, 0.6);
      gl->Vertex2f(-0.1, -1.0);
      gl->Vertex2f(0.1, -1.0);
      gl->Vertex2f(0.1, 0.6);
      gl->End();
      // arrowhead
      gl->Begin(DiGLPainter::gl_POLYGON);
      gl->Vertex2f(0.5, 0.6);
      gl->Vertex2f(0.0, 1.0);
      gl->Vertex2f(-0.5, 0.6);
      gl->End();

      // 22-40 knots
      if (ff >= 11) {
        gl->Begin(DiGLPainter::gl_POLYGON);
        gl->Vertex2f(-0.1, -1.0);
        gl->Vertex2f(-0.7, -1.4);
        gl->Vertex2f(-0.7, -1.2);
        gl->Vertex2f(-0.1, -0.8);
        gl->End();
      }
      // 41-63 knots
      if (ff >= 21) {
        gl->Begin(DiGLPainter::gl_POLYGON);
        gl->Vertex2f(-0.1, -0.6);
        gl->Vertex2f(-0.7, -1.0);
        gl->Vertex2f(-0.7, -0.8);
        gl->Vertex2f(-0.1, -0.4);
        gl->End();
      }

    } else {
      gl->ShadeModel(DiGLPainter::gl_SMOOTH);
      gl->setColour(colour);
      // arrowhead
      gl->Begin(DiGLPainter::gl_POLYGON);
      gl->Vertex2f(1.0, 0.5);
      gl->Vertex2f(0.0, 1.0);
      gl->Vertex2f(-1.0, 0.5);
      gl->End();
      // main body
      gl->Begin(DiGLPainter::gl_POLYGON);
      gl->Vertex2f(-0.5, 0.5);
      gl->Vertex2f(0.5, 0.5);
      gl->Vertex2f(0.5, -0.3);
      gl->Vertex2f(-0.5, -0.3);
      gl->End();

      gl->Begin(DiGLPainter::gl_POLYGON);
      gl->Vertex2f(0.5, -0.3);
      Colour colour03(colour);
      colour03.setF(Colour::alpha, 0.3);
      gl->setColour(colour03);
      gl->Vertex2f(0.5, -1.0);
      gl->Vertex2f(-0.5, -1.0);
      gl->setColour(colour);
      gl->Vertex2f(-0.5, -0.3);
      gl->End();

      gl->ShadeModel(DiGLPainter::gl_FLAT);
      gl->setColour(Colour::WHITE);
    }
  }

  pushpop.PopMatrix();

  if (ff > 0 && !classic) {
    gl->setFont(diutil::BITMAPFONT, diutil::F_NORMAL, 10);
    float sW, sH;
    const QString ost = QString::number(ff);
    gl->getTextSize(ost, sW, sH);
    float sx = x - 0.45 * sW;
    float sy = y - 0.35 * sH;
    gl->setColour(Colour::WHITE);
    gl->drawText(ost, sx, sy);
  }

  if (stations[ii]->isSelected) {
    //wind direction
    gl->setFont(diutil::BITMAPFONT, diutil::F_NORMAL, 10);
    float sW, sH;
    dd = (dd - stations[ii]->north);
    float df = dd;
    df += 11.25;
    df /= 22.5;
    dd = (int) df;
    if (dd < 0)
      dd += 16;
    if (dd > 15)
      dd -= 16;
    gl->getTextSize(ddString[dd], sW, sH);
    float sx = x - 0.45 * sW;
    float sy = y - 2.35 * sH;
    gl->setColour(Colour::WHITE);
    glPlot(gl, Station::noStatus, x, y - 2.5 * sH);
    gl->setColour(Colour::BLACK);
    gl->drawText(ddString[dd], sx, sy);
  }

  gl->Disable(DiGLPainter::gl_BLEND);
}

StationArea::StationArea(float minLat, float maxLat, float minLon, float maxLon) :
  minLat(minLat), maxLat(maxLat), minLon(minLon), maxLon(maxLon)
{
}

double StationArea::distance(const miCoordinates& pos) const
{
  if (contains(pos))
    return 0;

  const float lat = pos.dLat(), lon = pos.dLon();
  double d = 0;
  if (lat >= minLat && lat < maxLat) {
    // on the left or right
    const miCoordinates left(minLon, lat), right(maxLon, lat);
    d = std::min(pos.distanceTo(left), pos.distanceTo(right));
  } else if (lon >= minLon && lat < maxLon) {
    // on the left or right
    const miCoordinates below(lon, minLat), above(lon, maxLat);
    d = std::min(pos.distanceTo(below), pos.distanceTo(above));
  } else if (lon < minLon && lat < minLat) {
    const miCoordinates corner(minLon, minLat);
    d = pos.distanceTo(corner);
  } else if (lon >= maxLon && lat < minLat) {
    const miCoordinates corner(maxLon, minLat);
    d = pos.distanceTo(corner);
  } else if (lon < minLon && lat >= maxLat) {
    const miCoordinates corner(minLon, maxLat);
    d = pos.distanceTo(corner);
  } else if (lon >= maxLon && lat >= maxLat) {
    const miCoordinates corner(maxLon, maxLat);
    d = pos.distanceTo(corner);
  }
  return d / 1000; // convert from m to km
}

std::vector<Station*> StationArea::findStations(const miCoordinates& pos, float radius) const
{
  std::vector<Station*> all;

  if (!areas.empty()) {
    for (std::vector<StationArea>::const_iterator it = areas.begin(); it != areas.end(); ++it) {
      const double d = it->distance(pos);
      if (d < radius) {
        diutil::insert_all(all, it->findStations(pos, radius));
      }
    }
  } else {
    for (std::vector<Station*>::const_iterator it = stations.begin(); it != stations.end(); ++it) {
      Station* s = *it;
      const double d = ::distance(pos, s);
      if (d < radius)
        all.push_back(s);
    }
  }
  return all;
}

Station* StationArea::findStation(const miCoordinates& pos, float radius) const
{
  std::vector<Station*> found = findStations(pos, radius);
  if (found.empty())
    return 0;

  Station* min_station = found.front();
  if (found.size() > 1) {
    std::vector<Station*>::const_iterator it = found.begin();
    float min_dist = ::distance(pos, *it);
    for (++it; it != found.end(); ++it) {
      const float dist = ::distance(pos, *it);
      if (dist < min_dist) {
        min_dist = dist;
        min_station = *it;
      }
    }
  }
  return min_station;
}

void StationArea::addStation(Station* station)
{
  if (areas.empty()) {
    stations.push_back(station);

    if (stations.size() > 10) {
      // If there are more than 10 stations in the area, split up the area and
      // move each of the stations into the appropriate subarea.
      const float midLat = (minLat + maxLat)/2, midLon = (minLon + maxLon)/2;

      StationArea topLeft(midLat, maxLat, minLon, midLon);
      areas.push_back(topLeft);

      StationArea topRight(midLat, maxLat, midLon, maxLon);
      areas.push_back(topRight);

      StationArea bottomLeft(minLat, midLat, minLon, midLon);
      areas.push_back(bottomLeft);

      StationArea bottomRight(minLat, midLat, midLon, maxLon);
      areas.push_back(bottomRight);

      // Move all the stations into the subareas. May not call "addStation" to avoid infinite recursion if
      // there are more than 10 stations with identical lon and lat.
      for (std::vector<Station*>::const_iterator it = stations.begin(); it != stations.end(); ++it) {
        const size_t a = ((*it)->lat() >= midLat ? 0 : 2) + ((*it)->lon() >= midLon ? 1 : 0);
        areas[a].stations.push_back(*it);
      }

      // Clear the vector of stations in this area.
      stations.clear();
    }
  } else {
    // Find the appropriate subarea to store the station in.
    for (std::vector<StationArea>::iterator it = areas.begin(); it != areas.end(); ++it) {
      if (it->contains(station->lat(), station->lon())) {
        it->addStation(station);
        break;
      }
    }
  }
}
