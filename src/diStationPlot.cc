/*
 Diana - A Free Meteorological Visualisation Tool

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

#include "diStationPlot.h"
#include "diPlotModule.h"

#include <boost/algorithm/string.hpp>

#define MILOGGER_CATEGORY "diana.StationPlot"
#include <miLogger/miLogging.h>

using namespace::miutil;
using namespace std;

//  static members
std::string StationPlot::ddString[16];

StationPlot::StationPlot(const vector<float> & lons, const vector<float> & lats)
{
  init();
  unsigned int n = lons.size();
  if (n > lats.size())
    n = lats.size();
  for (unsigned int i = 0; i < n; i++) {
    addStation(lons[i], lats[i]);
  }
  defineCoordinates();
}

StationPlot::StationPlot(const vector<std::string> & names,
    const vector<float> & lons, const vector<float> & lats)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  init();
  unsigned int n = names.size();
  if (n > lons.size())
    n = lons.size();
  if (n > lats.size())
    n = lats.size();
  for (unsigned int i = 0; i < n; i++) {
    addStation(lons[i], lats[i], names[i]);
  }
  defineCoordinates();
}

StationPlot::StationPlot(const vector<std::string> & names,
    const vector<float> & lons, const vector<float> & lats,
    const vector<std::string>& images)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  init();
  unsigned int n = names.size();
  if (n > lons.size())
    n = lons.size();
  if (n > lats.size())
    n = lats.size();
  for (unsigned int i = 0; i < n; i++) {
    addStation(lons[i], lats[i], names[i], images[i]);
  }
  useImage = true;
  defineCoordinates();
}

StationPlot::StationPlot(const vector <Station*> &stations)
{
  init();
  for (unsigned int i = 0; i < stations.size(); ++i)
    addStation(stations[i]);

  useImage = false;
  defineCoordinates();
}

StationPlot::StationPlot(const string& commondesc, const string& common,
    const string& description, int from, const vector<string>& data)
{
  init();

  id = from;
  vector<string> vcommondesc;
  vector<string> vcommon;
  vector<string> vdesc;
  boost::algorithm::split(vcommondesc, commondesc, boost::algorithm::is_any_of(":"));
  boost::algorithm::split(vcommon, common, boost::algorithm::is_any_of(":"));
  boost::algorithm::split(vdesc, description, boost::algorithm::is_any_of(":"));

  if (vcommondesc.size() != vcommon.size()) {
    METLIBS_LOG_ERROR("commondesc:" << commondesc << " and common:" << common
        << " do not match");
    return;
  }

  int num = vcommondesc.size();
  map<std::string, int> commonmap;
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
  if (commonmap.count("priority"))
    priority = atoi(vcommon[commonmap["priority"]].c_str());

  //decode data
  int ndata = data.size();
  map<std::string, int> datamap;
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
    vector<string> token;
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

  hide();
}

void StationPlot::init()
{
  // Add a default plot area.
  StationArea area(-90.0, 90.0, -180.0, 180.0);
  stationAreas.push_back(area);

  //coordinates to be plotted
  show();
  useImage = true;
  useStationNameNormal = false;
  useStationNameSelected = false;
  showText = true;
  textColour = Colour("white");
  textSize = 16;
  textStyle = "bold";
  name = "vprof";
  id = -1;
  priority = 1;
  index = -1;
  editIndex = -1;
}

StationPlot::~StationPlot()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  int n = stations.size();
  for (int i = 0; i < n; i++) {
    delete stations[i];
  }
  stations.clear();
  stationAreas.clear();
}

void StationPlot::addStation(float lon, float lat, const std::string& newname,
    const std::string& newimage, int alpha, float scale)
{
  //at the moment, this should only be called from constructor, since
  //define coordinates must be called to actually plot stations
  Station * newStation = new Station;
  if (not newname.empty()) {
    newStation->name = newname;
  } else {
    miCoordinates coordinates(lon, lat);
    newStation->name = coordinates.str();
  }
  newStation->lon = lon;
  newStation->lat = lat;
  newStation->alpha = alpha;
  newStation->scale = scale;
  newStation->isSelected = false;
  if (newimage == "HIDE") {
    newStation->isVisible = false;
  } else {
    newStation->isVisible = true;
    newStation->image = newimage;
  }
  newStation->edit = false;
  newStation->status = Station::noStatus;

  addStation(newStation);
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

  if (!isEnabled() || !visible || zorder != LINES)
    return;

  map<Station::Status, vector<int> > selected; //index of selected stations for each status
  map<Station::Status, vector<int> > unselected; //index of unselected stations for each status

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
    for (vector<int>::iterator it = unselected[plotOrder[i]].begin(); it != unselected[plotOrder[i]].end(); it++) {
      plotStation(gl, *it);
    }
    for (vector<int>::iterator it = selected[plotOrder[i]].begin(); it != selected[plotOrder[i]].end(); it++) {
      plotStation(gl, *it);
    }
  }
}

void StationPlot::plotStation(DiGLPainter* gl, int i)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE(LOGVAL(i));
#endif

  float h = 0, w = 0; //height for displaying text
  float x = xplot[i];
  float y = yplot[i];
  bool plotted = true;

  if (useImage) {
    //use either stations[i]->image or imageNormal/imageSelected
    if (!stations[i]->image.empty() && stations[i]->image2.empty()) {
      if (stations[i]->image == "wind") {
        h = (stations[i]->isSelected ? 40 : 30)
            * getStaticPlot()->getPhysToMapScaleY();
        w = 30 * getStaticPlot()->getPhysToMapScaleX();
        plotWind(gl, i, x, y);
      } else {
        h = ig.height_(stations[i]->image) * getStaticPlot()->getPhysToMapScaleY();
        w = ig.width_(stations[i]->image) * getStaticPlot()->getPhysToMapScaleX();
        if (!ig.plotImage(gl, getStaticPlot(), stations[i]->image, x, y, true, stations[i]->scale,
            stations[i]->alpha))
          plotted = false;
      }
      if (stations[i]->isSelected && stations[i]->image != "wind")
        gl->Color3ub(255, 0, 0); //red
      glPlot(gl, Station::noStatus, x, y, w, h);
    } else if (!stations[i]->image.empty() && !stations[i]->image2.empty()) {
      float h1 = ig.height_(stations[i]->image);
      float h2 = ig.height_(stations[i]->image2);
      h = std::max(h1, h2) * getStaticPlot()->getPhysToMapScaleY();
      float w1 = ig.width_(stations[i]->image);
      float w2 = ig.width_(stations[i]->image2);
      w = 2 * std::max(w1, w2) * getStaticPlot()->getPhysToMapScaleX();
      gl->Color3ub(128, 128, 128); //grey
      glPlot(gl, Station::noStatus, x, y, w, h);
      if (!ig.plotImage(gl, getStaticPlot(), stations[i]->image, x - w1 / 2, y, true, stations[i]->scale,
          stations[i]->alpha))
        plotted = false;
      if (!ig.plotImage(gl, getStaticPlot(), stations[i]->image2, x + w2 / 2, y, true, stations[i]->scale,
          stations[i]->alpha))
        plotted = false;
      if (stations[i]->isSelected)
        gl->Color3ub(255, 0, 0); //red
        glPlot(gl, Station::noStatus, x, y, w, h);
    } else if (!stations[i]->isSelected && !imageNormal.empty()) {
      //otherwise plot images for selected/normal stations
      if (!ig.plotImage(gl, getStaticPlot(), imageNormal, x, y, true, stations[i]->scale, stations[i]->alpha))
        plotted = false;
      h = ig.height_(imageNormal) * getStaticPlot()->getPhysToMapScaleY();
    } else if (stations[i]->isSelected && !imageSelected.empty()) {
      if (!ig.plotImage(gl, getStaticPlot(), imageSelected, x, y, true, stations[i]->scale, stations[i]->alpha))
        plotted = false;
      h = ig.height_(imageSelected) * getStaticPlot()->getPhysToMapScaleY();
    } else {
      //if no image plot crosses and circles for selected/normal stations
      //METLIBS_LOG_DEBUG("useImage=false");
      glPlot(gl, Station::failed, x, y, w, h, stations[i]->isSelected);
    }

    //if something went wrong,
    //plot crosses and circles for selected/normal stations
    if (!plotted) {
      glPlot(gl, Station::failed, x, y, w, h, stations[i]->isSelected);
      plotted = true;
    }
  } else if (stations[i]->status != Station::noStatus) {
    glPlot(gl, stations[i]->status, x, y, w, h, stations[i]->isSelected);
  }

  if (useStationNameNormal && !stations[i]->isSelected) {
    float cw, ch;
    gl->Color3ub(0, 0, 0); //black
    gl->setFont("BITMAPFONT", "normal", 10);
    gl->getTextSize(stations[i]->name, cw, ch);
    gl->drawText(stations[i]->name, x - cw / 2, y + h / 2, 0.0);
  } else if (useStationNameSelected && stations[i]->isSelected) {
    float cw, ch;
    gl->setFont("BITMAPFONT", "normal", 10);
    gl->getTextSize(stations[i]->name, cw, ch);
    gl->Color3ub(255, 255, 255); //white
    glPlot(gl, Station::noStatus, x, y + h / 2 + ch * 0.1, cw * 0.6, ch * 1.1);
    gl->Color3ub(0, 0, 0); //black
    gl->drawText(stations[i]->name, x - cw / 2, y + h / 2 + ch * 0.35,
        0.0);
  }

  if (showText || stations[i]->isSelected) {
    int nt = stations[i]->vsText.size();
    for (int it = 0; it < nt; it++) {
      float cw, ch;
      gl->setColour(textColour);
      std::string text = stations[i]->vsText[it].text;
      gl->setFont("BITMAPFONT", textStyle, textSize);
      gl->getTextSize(text, cw, ch);
      if (stations[i]->vsText[it].hAlign == align_center)
        gl->drawText(text, x - cw / 2, y - ch / 4, 0.0);
      else if (stations[i]->vsText[it].hAlign == align_top)
        gl->drawText(text, x - cw / 2, y + h / 2, 0.0);
      else if (stations[i]->vsText[it].hAlign == align_bottom) {
        if (stations[i]->isSelected)
          gl->Color3ub(255, 255, 255); //white
          glPlot(gl, Station::noStatus, x, y - h / 1.9 - ch * 1.0, cw * 0.5 + 0.2 * w,
              ch);
        gl->setColour(textColour);
        gl->drawText(text, x - cw / 2, y - h / 1.9 - ch * 0.7, 0.0);
      }
    }
  }
}

void StationPlot::hide()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  visible = false;
  unselect();
}

void StationPlot::show()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  visible = true;
  unselect();
  changeProjection();
}

void StationPlot::unselect()
{
  int n = stations.size();
  for (int i = 0; i < n; i++)
    stations[i]->isSelected = false;
  index = -1;
}

bool StationPlot::isVisible()
{
  return visible;
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
  int npos = stations.size();
  xplot.clear();
  yplot.clear();
  for (int i = 0; i < npos; i++) {
    xplot.push_back(stations[i]->lon);
    yplot.push_back(stations[i]->lat);
  }
  changeProjection();
}

bool StationPlot::changeProjection()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE("Change projection to: "<< area << " wind might not be rotated");
#endif

  int npos = xplot.size();
  if (npos == 0) {
    return false;
  }

  float *xpos = new float[npos];
  float *ypos = new float[npos];
  for (int i = 0; i < npos; i++) {
    xpos[i] = stations[i]->lon;
    ypos[i] = stations[i]->lat;
  }

  if (!getStaticPlot()->GeoToMap(npos, xpos, ypos)) {
    METLIBS_LOG_ERROR("changeProjection: getPoints error");
    delete[] xpos;
    delete[] ypos;
    return false;
  }
  for (int i = 0; i < npos; i++) {
    xplot[i] = xpos[i];
    yplot[i] = ypos[i];
  }

  float *u = new float[npos];
  float *v = new float[npos];

  for (int i = 0; i < npos; i++) {
    u[i] = 0;
    v[i] = 10;
  }

  for (int i = 0; i < npos; i++) {
    if (stations[i]->image == "wind") {
      int angle = (int) (atan2f(u[i], v[i]) * RAD_TO_DEG);
      int dd = stations[i]->north + angle;
      if (dd < 1)
        dd += 360;
      if (dd > 360)
        dd -= 360;
      stations[i]->north = dd;;
      dd = stations[i]->dd + angle;
      if (dd < 1)
        dd += 360;
      if (dd > 360)
        dd -= 360;
      stations[i]->dd = dd;
    }
  }

  delete[] xpos;
  delete[] ypos;
  delete[] u;
  delete[] v;

  return true;
}

vector<Station*> StationPlot::getStations() const
{
  return stations;
}

static inline float square(float x)
{ return x*x; }

Station* StationPlot::stationAt(int x, int y)
{
  vector<Station*> found = stationsAt(x, y);

  if (found.size() > 0) {
    const XY pos = getStaticPlot()->PhysToMap(XY(x, y));

    float min_r = square(10.0f * getStaticPlot()->getPhysToMapScaleX());
    int min_i = 0;

    // Find the closest station to the point within a given radius.
    for (unsigned int i = 0; i < found.size(); ++i) {
      float sx = found[i]->lon, sy = found[i]->lat;
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

  return 0;
}

vector<Station*> StationPlot::stationsAt(int x, int y, float radius, bool useAllStations)
{
  const XY pos = getStaticPlot()->PhysToMap(XY(x, y));

  float min_r = square(radius * getStaticPlot()->getPhysToMapScaleX());

  vector<Station*> within;

  float gx = pos.x(), gy = pos.y();
  if (getStaticPlot()->MapToGeo(1, &gx, &gy)) {
    vector<Station*> found;
    if ( useAllStations ) {
      found = stations;
    } else {
      found = stationAreas[0].findStations(gy, gx);
    }

    for (unsigned int i = 0; i < found.size(); ++i) {
      if (found[i]->isVisible) {
        float sx = found[i]->lon, sy = found[i]->lat;
        if (getStaticPlot()->GeoToMap(1, &sx, &sy)) {
          float r = square(pos.x() - sx) + square(pos.y() - sy);
          if (r < min_r) {
            within.push_back(found[i]);
          }
        }
      }
    }
  }

  return within;
}

vector<std::string> StationPlot::findStation(int x, int y, bool add)
{
  vector<std::string> stationstring;

  if (!visible || !isEnabled())
    return stationstring;

  Station* found = stationAt(x, y);

  if (found && found != stations[index]) {
    add = found->isSelected || add;
    setSelectedStation(found->name, add);

    for (unsigned int i = 0; i < xplot.size(); i++) {
      if (stations[i]->isSelected)
        stationstring.push_back(stations[i]->name);
    }
  }

  return stationstring;
}

vector<std::string> StationPlot::findStations(int x, int y)
{
  vector<std::string> stationstring;

  if (!visible || !isEnabled())
    return stationstring;

  vector<Station*> found = stationsAt(x, y, 10.0f, true);

  for (unsigned int i = 0; i < found.size(); i++) {
    stationstring.push_back(found[i]->name);
  }

  return stationstring;
}

float StationPlot::getImageScale(int i)
{
  return stations[i]->scale;
}

vector<Station*> StationPlot::getSelectedStations() const
{
  vector<Station*> selected;
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

    //edit
    if (stations[i]->edit)
      editIndex = i;

    return i;
  }
  return -1;
}

void StationPlot::getStationPlotAnnotation(string &str, Colour &col)
{
  if (visible) {
    str = annotation;
    Colour c("red");
    col = c;
  } else {
    str.erase();
  }
}

void StationPlot::setStationPlotAnnotation(const string &str)
{
  annotation = str;
}

void StationPlot::setName(std::string nm)
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

void StationPlot::setImageScale(float new_scale)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  int n = stations.size();
  for (int i = 0; i < n; i++) {
    stations[i]->scale = new_scale;
  }
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

void StationPlot::setEditStations(const vector<string>& st)
{
  int m = st.size();

  int n = stations.size();
  for (int i = 0; i < n; i++) {
    int j = 0;
    while (j < m && st[j] != stations[i]->name)
      j++;
    if (j < m) {
      stations[i]->edit = true;
    } else {
      stations[i]->edit = false;
    }
  }

  editIndex = 0;

  if (!m) { //no edit stations
    editIndex = -1;
    return;
  }
}

bool StationPlot::getEditStation(int step, std::string& nname, int& iid,
    vector<std::string>& sstations)
{
  METLIBS_LOG_SCOPE();

  if (editIndex < 0)
    return false;

  bool add = (nname == "add");
  nname = name;
  iid = id;

  int n = stations.size();

  if (step == 0) {
    for (int i = 0; i < n; i++)
      if (stations[i]->isSelected && stations[i]->edit) {
        sstations.push_back(stations[i]->name);
      }
    return stations.size();
  }

  int i;
  if (step > 0) {
    i = editIndex + 1;
    while (i < n && (!stations[i]->edit))
      i++;
    if (i == n) {
      i = 0;
      while (i < n && (!stations[i]->edit))
        i++;
    }
  } else {
    i = editIndex - 1;
    while (i > -1 && !stations[i]->edit)
      i--;
    if (i == -1) {
      i = n - 1;
      while (i > -1 && !stations[i]->edit)
        i--;
    }
  }

  if (i < n && i > -1) {
    editIndex = i;
    setSelectedStation(i, add);

    //if stations[i] isn't on the map, pan the map
    const Rectangle& r = getStaticPlot()->getMapArea().R();
    if (!r.isinside(xplot[i], yplot[i])) {
      Rectangle request(r);
      request.putinside(xplot[i], yplot[i]);
      PlotModule::instance()->setMapAreaFromMap(request);
    }

    for (int i = 0; i < n; i++)
      if (stations[i]->isSelected && stations[i]->edit) {
        sstations.push_back(stations[i]->name);
      }
    return true;
  }

  return false;
}

bool StationPlot::stationCommand(const string& command,
    const vector<string>& data, const string& misc)
{
  METLIBS_LOG_SCOPE();
  if (command == "changeImageandText") {
    vector<string> description;
    boost::algorithm::split(description, misc, boost::algorithm::is_any_of(":"));
    int ndesc = description.size();
    map<std::string, int> datamap;
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
      vector<string> token;
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

  } else if (command == "setEditStations") {
    setEditStations(data);
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
    vector<string> token;
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

    vector<string> description;
    boost::algorithm::split(description, misc, boost::algorithm::is_any_of(":"));
    int ndesc = description.size();
    map<std::string, int> datamap;
    for (int i = 0; i < ndesc; i++)
      datamap[description[i]] = i;
    if (!datamap.count("showtext")) {
      METLIBS_LOG_ERROR(" StationPlot::stationCommand: description must contain showtext");
      return false;
    }
    vector<string> token;
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

bool StationPlot::stationCommand(const string& command)
{
  if (command == "show") {
    show();
  } else if (command == "hide") {
    hide();
  } else if (command == "unselect") {
    unselect();
  } else if (command == "showPositionName" ) {
    setUseStationName(true, true);
  } else {
    return false;
  }
  return true;
}

std::string StationPlot::stationRequest(const std::string& command)
{
  std::ostringstream ost;

  if (command == "selected") {
    int n = stations.size();
    for (int i = 0; i < n; i++)
      if (stations[i]->isSelected)
        ost << ":" << stations[i]->name.c_str();
    ost << ":" << name.c_str() << ":" << id;
  }

  return ost.str();
}

void StationPlot::glPlot(DiGLPainter* gl, Station::Status tp, float x, float y, float w, float h, bool selected)
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
    gl->Color4ub(100, 100, 100, 50);
    gl->Enable(DiGLPainter::gl_BLEND);
    gl->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);
    gl->fillRect(x - r, y - r, x + r, y + r);
    gl->Disable(DiGLPainter::gl_BLEND);
    break;
  case Station::failed:
    linewidth = 4;
    gl->LineWidth(linewidth);
    r = linewidth * scale;
    h = 1.5 * r;
    //plot crosses
    gl->Color3ub(255, 0, 0); //red
    gl->drawCross(x, y, r, true);
    break;
  case Station::underRepair:
    linewidth = 4;
    gl->LineWidth(linewidth);
    r = linewidth * scale;
    h = 1.5 * r;
    //plot crosses
    gl->Color3ub(255, 255, 0); //yellow
    gl->drawCross(x, y, r, true);
    break;
  case Station::working:
    linewidth = 4;
    gl->LineWidth(linewidth);
    r = linewidth * scale;
    h = 1.5 * r;
    //plot crosses
    gl->Color3ub(0, 255, 0); //green
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
    gl->Color3ub(255, 255, 0);
    gl->drawCircle(x, y, radius);
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

  gl->PushMatrix();
  gl->Translatef(x, y, 0.0);
  gl->Scalef(radius, radius, 0.0);

  if (stations[ii]->isSelected) {
    //compass
    int linewidth = 1;
    gl->LineWidth(linewidth);
    gl->Color3ub(0, 0, 0);
    gl->drawCircle(0, 0, 1);

    gl->PushMatrix();
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
    gl->PopMatrix();

  }

  if (ff < 1) {
    gl->Rotatef(45, 0.0, 0.0, 1.0);
  } else {
    gl->Rotatef(-1 * (dd + 180), 0.0, 0.0, 1.0);
  }

  gl->Color3ub(0, 0, 0); //black
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
      gl->Color4f(colour.fR(), colour.fG(), colour.fB(), 0.3);
      gl->Vertex2f(0.5, -1.0);
      gl->Vertex2f(-0.5, -1.0);
      gl->setColour(colour);
      gl->Vertex2f(-0.5, -0.3);
      gl->End();

      gl->ShadeModel(DiGLPainter::gl_FLAT);
      gl->Color4f(1.0, 1.0, 1.0, 1.0);
    }
  }

  gl->PopMatrix();

  if (ff > 0 && !classic) {
    gl->setFont("BITMAPFONT", "normal", 10);
    float sW, sH;
    ostringstream ost;
    ost << ff;
    gl->getTextSize(ost.str(), sW, sH);
    float sx = x - 0.45 * sW;
    float sy = y - 0.35 * sH;
    gl->Color4f(1.0, 1.0, 1.0, 1.0); //white
    gl->drawText(ost.str(), sx, sy);
  }

  if (stations[ii]->isSelected) {
    //wind direction
    gl->setFont("BITMAPFONT", "normal", 10);
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
    gl->Color3ub(255, 255, 255); //white
    glPlot(gl, Station::noStatus, x, y - 2.5 * sH, sW * 0.6, sH * 1.1);
    gl->Color4f(0.0, 0.0, 0.0, 1.0); //black
    gl->drawText(ddString[dd], sx, sy);
  }

  gl->Disable(DiGLPainter::gl_BLEND);
}

StationArea::StationArea(float minLat, float maxLat, float minLon, float maxLon) :
  minLat(minLat), maxLat(maxLat), minLon(minLon), maxLon(maxLon)
{
}

vector<Station*> StationArea::findStations(float lat, float lon) const
{
  METLIBS_LOG_SCOPE();

  for (unsigned int i = 0; i < areas.size(); ++i) {
    if (lat >= areas[i].minLat && lat < areas[i].maxLat && lon >= areas[i].minLon && lon < areas[i].maxLon)
      return areas[i].findStations(lat, lon);
  }
  return stations;
}

Station* StationArea::findStation(float lat, float lon) const
{
  vector<Station*> found = findStations(lat, lon);
  if (found.size() > 0)
    return *(found.begin());
  else
    return 0;
}

void StationArea::addStation(Station* station)
{
  if (areas.size() == 0) {
    stations.push_back(station);

    // ### TODO: Handle the case where there are more than 10 stations at the same location.

    if (stations.size() > 10) {
      // If there are more than 10 stations in the area, split up the area and
      // move each of the stations into the appropriate subarea.
      StationArea topLeft((minLat + maxLat)/2, maxLat, minLon, (minLon + maxLon)/2);
      areas.push_back(topLeft);

      StationArea topRight((minLat + maxLat)/2, maxLat, (minLon + maxLon)/2, maxLon);
      areas.push_back(topRight);

      StationArea bottomLeft(minLat, (minLat + maxLat)/2, minLon, (minLon + maxLon)/2);
      areas.push_back(bottomLeft);

      StationArea bottomRight(minLat, (minLat + maxLat)/2, (minLon + maxLon)/2, maxLon);
      areas.push_back(bottomRight);

      // Move all the stations into the subareas.
      for (unsigned int i = 0; i < stations.size(); ++i) {
        for (unsigned int j = 0; j < areas.size(); ++j) {
          // If the station fits into the subarea, add it to it and ignore the other subareas.
          if (stations[i]->lat >= areas[j].minLat
              && stations[i]->lat < areas[j].maxLat
              && stations[i]->lon >= areas[j].minLon
              && stations[i]->lon < areas[j].maxLon)
          {
            areas[j].stations.push_back(stations[i]);
            break;
          }
        }
      }

      // Clear the vector of stations in this area.
      stations.clear();
    }
  } else {
    // Find the appropriate subarea to store the station in.
    for (unsigned int i = 0 ; i < areas.size(); ++i) {
      if (station->lat >= areas[i].minLat
          && station->lat < areas[i].maxLat
          && station->lon >= areas[i].minLon
          && station->lon < areas[i].maxLon)
      {
        areas[i].addStation(station);
      }
    }
  }
}
