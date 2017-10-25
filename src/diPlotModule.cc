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

#include "diPlotModule.h"

#include "diAreaObjectsCluster.h"
#include "diObsManager.h"
#include "diObsPlotCluster.h"
#include "diSatManager.h"
#include "diStationManager.h"
#include "diObjectManager.h"
#include "diEditManager.h"
#include "diFieldPlotCluster.h"
#include "diFieldPlotManager.h"
#include "diKVListPlotCommand.h"
#include "diLocationPlot.h"
#include "diManager.h"
#include "diMapManager.h"
#include "diMapPlot.h"
#include "diMeasurementsPlot.h"
#include "diStationPlot.h"
#include "diTrajectoryGenerator.h"
#include "diTrajectoryPlot.h"
#include "diUtilities.h"
#include "diWeatherArea.h"

#include "util/string_util.h"
#include "util/was_enabled.h"

#include "diField/diFieldManager.h"
#include <puDatatypes/miCoordinates.h>
#include <puTools/miStringFunctions.h>

#include <QMouseEvent>

#include <boost/range/adaptor/map.hpp>

#include <memory>

//#define DEBUGPRINT
//#define DEBUGREDRAW
#define MILOGGER_CATEGORY "diana.PlotModule"
#include <miLogger/miLogging.h>

using namespace miutil;
using namespace std;

namespace {

float GreatCircleDistance(float lat1, float lat2, float lon1, float lon2)
{
  return LonLat::fromDegrees(lon1, lat1).distanceTo(LonLat::fromDegrees(lon2, lat2));
}

} // anonymous namespace

PlotModule *PlotModule::self = 0;

PlotModule::PlotModule()
  : showanno(true)
  , staticPlot_(new StaticPlot())
  , mCanvas(0)
  , dorubberband(false)
  , keepcurrentarea(true)
{
  self = this;
  oldx = newx = oldy = newy = startx = starty = 0;
  mapDefinedByUser = false;
  mapDefinedByData = false;
  mapDefinedByView = false;

  // used to detect map area changes
  areaIndex = -1;
  areaSaved = false;
}

PlotModule::~PlotModule()
{
  cleanup();
}

void PlotModule::setCanvas(DiCanvas* canvas)
{
  METLIBS_LOG_SCOPE();
  // TODO set for all existing plots, and for new plots
  mCanvas = canvas;
  for (size_t i = 0; i < vmp.size(); i++)
    vmp[i]->setCanvas(canvas);
  obsplots_->setCanvas(mCanvas);
  fieldplots_->setCanvas(mCanvas);
  for (Manager* m : boost::adaptors::values(managers))
    m->setCanvas(canvas);
}

void PlotModule::preparePlots(const PlotCommand_cpv& vpi)
{
  METLIBS_LOG_SCOPE();
  // reset flags
  mapDefinedByUser = false;

  // split up input into separate products
  std::set<std::string> ordered;
  ordered.insert(fieldplots()->plotCommandKey());
  ordered.insert(obsplots()->plotCommandKey());
  ordered.insert("MAP");
  ordered.insert("AREA");
  ordered.insert("SAT");
  ordered.insert("STATION");
  ordered.insert("OBJECTS");
  ordered.insert("TRAJECTORY");
  ordered.insert("LABEL");
  ordered.insert("EDITFIELD");

  typedef std::map<std::string, PlotCommand_cpv> manager_pi_t;
  manager_pi_t manager_pi;
  manager_pi_t ordered_pi;

  // merge PlotInfo's for same type
  for (PlotCommand_cp pc : vpi) {
    const std::string& type = pc->commandKey();
    METLIBS_LOG_INFO(pc->toString());
    if (ordered.find(type) != ordered.end())
      ordered_pi[type].push_back(pc);
    else if (managers.find(type) != managers.end())
      manager_pi[type].push_back(pc);
    else
      METLIBS_LOG_WARN("unknown type '" << type << "'");
  }

  // call prepare methods
  prepareArea(ordered_pi["AREA"]);
  prepareMap(ordered_pi["MAP"]);
  fieldplots_->prepare(ordered_pi[fieldplots()->plotCommandKey()]);
  obsplots_->prepare(ordered_pi[obsplots()->plotCommandKey()]);
  satm->prepareSat(ordered_pi["SAT"]);
  prepareStations(ordered_pi["STATION"]);
  objm->prepareObjects(ordered_pi["OBJECTS"]);
  prepareTrajectory(ordered_pi["TRAJECTORY"]);
  prepareAnnotation(ordered_pi["LABEL"]);
  editm->prepareEditFields(ordered_pi["EDITFIELD"]);

  // Send the commands to the other managers.
  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it) {
    Manager *manager = it->second;

    // send any plot commands to the relevant manager.
    const manager_pi_t::const_iterator itp = manager_pi.find(it->first);
    if (itp != manager_pi.end())
      manager->processInput(itp->second);
    else
      manager->processInput(PlotCommand_cpv());
  }
}

void PlotModule::prepareArea(const PlotCommand_cpv& inp)
{
  METLIBS_LOG_SCOPE();

  MapManager mapm;

  if (!inp.size())
    return;
  if (inp.size() > 1)
    METLIBS_LOG_DEBUG("More AREA definitions, using: " <<inp[0]->toString());

  const std::string key_name=  "name";
  const std::string key_proj=  "proj4string";
  const std::string key_rectangle=  "rectangle";

  Area requestedarea = staticPlot_->getRequestedarea();
  Projection proj;
  Rectangle rect;

  KVListPlotCommand_cp cmd = std::dynamic_pointer_cast<const KVListPlotCommand>(inp[0]);
  if (!cmd)
    return;

  for (const miutil::KeyValue& kv : cmd->all()) {
    if (!kv.value().empty()) {
      if (kv.key() == key_name) {
        if (!mapm.getMapAreaByName(kv.value(), requestedarea)) {
          METLIBS_LOG_WARN("Unknown AREA definition '"<< kv.value() << "'");
        }

      } else if (kv.key() == key_proj) {
        if (proj.set_proj_definition(kv.value())) {
          requestedarea.setP(proj);
        } else {
          METLIBS_LOG_WARN("Unknown proj definition '" << kv.value());
        }
      } else if (kv.key() == key_rectangle) {
        if (rect.setRectangle(kv.value())) {
          requestedarea.setR(rect);
        } else {
          METLIBS_LOG_WARN("Unknown rectangle definition '"<< kv.value() << "'");
        }
      }
    }
  }
  // check area
  mapDefinedByUser = requestedarea.P().isDefined();
  staticPlot_->setRequestedarea(requestedarea);
}

void PlotModule::prepareMap(const PlotCommand_cpv& inp)
{
  METLIBS_LOG_SCOPE();

  // init inuse array
  const size_t nm = vmp.size();
  vector<bool> inuse(nm, false);

  std::vector<MapPlot*> new_vmp; // new vector of map plots

  for (PlotCommand_cp pc : inp) { // loop through all plotinfo's
    bool isok = false;
    for (size_t j = 0; j < nm; j++) {
      if (!inuse[j]) { // not already taken
        if (vmp[j]->prepare(pc, true)) {
          inuse[j] = true;
          isok = true;
          new_vmp.push_back(vmp[j]);
          break;
        }
      }
    }
    if (isok)
      continue;

    // make new mapPlot object and push it on the list
    std::unique_ptr<MapPlot> mp(new MapPlot());
    if (mp->prepare(pc, false)) {
      mp->setCanvas(mCanvas);
      new_vmp.push_back(mp.release());
    }
  } // end plotinfo loop

  // delete unwanted mapplots
  for (size_t i = 0; i < nm; i++) {
    if (!inuse[i])
      delete vmp[i];
  }

  std::swap(vmp, new_vmp);
}

void PlotModule::prepareStations(const PlotCommand_cpv& inp)
{
  METLIBS_LOG_SCOPE();

  if (!stam->init(inp)) {
    METLIBS_LOG_DEBUG("init returned false");
  }
}

void PlotModule::prepareAnnotation(const PlotCommand_cpv& inp)
{
  METLIBS_LOG_SCOPE();

  // for now -- erase all annotationplots
  diutil::delete_all_and_clear(vap);
  annotationCommands = inp;
}

void PlotModule::prepareTrajectory(const PlotCommand_cpv& inp)
{
  METLIBS_LOG_SCOPE();
  // vtp.push_back(new TrajectoryPlot());
}

vector<PlotElement> PlotModule::getPlotElements()
{
  //  METLIBS_LOG_SCOPE();
  std::vector<PlotElement> pel;

  fieldplots_->addPlotElements(pel);
  obsplots_->addPlotElements(pel);
  satm->addPlotElements(pel);
  objm->addPlotElements(pel);

  // get trajectory names
  for (size_t j = 0; j < vtp.size(); j++) {
    std::string str = vtp[j]->getPlotName();
    if (not str.empty()) {
      str += "# " + miutil::from_number(int(j));
      bool enabled = vtp[j]->isEnabled();
      pel.push_back(PlotElement("TRAJECTORY", str, "TRAJECTORY", enabled));
    }
  }

  // get stationPlot names; vector is built on each call to stam->plots()
  const std::vector<StationPlot*>& stam_plots = stam->plots();
  for (size_t j = 0; j < stam_plots.size(); j++) {
    StationPlot* sp = stam_plots[j];
    if (!sp->isVisible())
      continue;
    std::string str = sp->getPlotName();
    if (not str.empty()) {
      str += "# " + miutil::from_number(int(j));
      bool enabled = sp->isEnabled();
      std::string icon = sp->getIcon();
      if (icon.empty())
        icon = "STATION";
      std::string ptype = "STATION";
      pel.push_back(PlotElement(ptype, str, icon, enabled));
    }
  }

  if (areaobjects_.get())
    areaobjects_->addPlotElements(pel);

  // get locationPlot annotations
  for (size_t j = 0; j < locationPlots.size(); j++) {
    std::string str = locationPlots[j]->getPlotName();
    if (not str.empty()) {
      str += "# " + miutil::from_number(int(j));
      bool enabled = locationPlots[j]->isEnabled();
      pel.push_back(PlotElement("LOCATION", str, "LOCATION", enabled));
    }
  }

  for (Manager* m : boost::adaptors::values(managers)) {
    const std::vector<PlotElement> pe = m->getPlotElements();
    pel.insert(pel.end(), pe.begin(), pe.end());
  }

  return pel;
}

void PlotModule::enablePlotElement(const PlotElement& pe)
{
  Plot* plot = 0;
  if (pe.type == "TRAJECTORY") {
    for (unsigned int i = 0; i < vtp.size(); i++) {
      std::string str = vtp[i]->getPlotName() + "# " + miutil::from_number(int(i));
      if (str == pe.str) {
        plot = vtp[i];
        break;
      }
    }
  } else if (pe.type == "STATION") {
    const std::vector<StationPlot*>& stam_plots = stam->plots();
    for (size_t i = 0; i < stam_plots.size(); i++) {
      StationPlot* sp = stam_plots[i];
      std::string str = sp->getPlotName() + "# " + miutil::from_number(int(i));
      if (str == pe.str) {
        plot = sp;
        break;
      }
    }
  } else if (pe.type == "LOCATION") {
    for (unsigned int i = 0; i < locationPlots.size(); i++) {
      std::string str = locationPlots[i]->getPlotName() + "# " + miutil::from_number(int(i));
      if (str == pe.str) {
        plot = locationPlots[i];
        break;
      }
    }
  }

  bool change = false;
  if (plot) {
    if (plot->isEnabled() != pe.enabled) {
      plot->setEnabled(pe.enabled);
      change = true;
    }
  } else if (pe.type == fieldplots_->keyPlotElement()) {
    change = fieldplots_->enablePlotElement(pe);
  } else if (pe.type == obsplots_->keyPlotElement()) {
    change = obsplots_->enablePlotElement(pe);
  } else if (pe.type == "RASTER") {
    change = satm->enablePlotElement(pe);
  } else if (pe.type == "OBJECTS") {
    change = objm->enablePlotElement(pe);
  } else if (pe.type == "AREAOBJECTS") {
    if (areaobjects_.get())
      change = areaobjects_->enablePlotElement(pe);
  } else {
    const QString qtype = QString::fromStdString(pe.type);
    for (Manager* m : boost::adaptors::values(managers)) {
      if (qtype == m->plotElementTag()) {
        change = m->enablePlotElement(pe);
        break;
      }
    }
  }

  // get annotations from all plots
  if (change)
    setAnnotations();
}

void PlotModule::setAnnotations()
{
  METLIBS_LOG_SCOPE();

  diutil::delete_all_and_clear(vap);

  for (PlotCommand_cp pc : annotationCommands) {
    std::unique_ptr<AnnotationPlot> ap(new AnnotationPlot());
    // Dont add an invalid object to vector
    if (ap->prepare(pc))
      vap.push_back(ap.release());
  }

  //Annotations from setup, qmenu, etc.

  // set annotation-data
  vector<AnnotationPlot::Annotation> annotations;
  AnnotationPlot::Annotation ann;

  const std::vector<miutil::miTime> fieldAnalysisTimes = fieldplots_->fieldAnalysisTimes();

  fieldplots_->addAnnotations(annotations);

  satm->addSatAnnotations(annotations);

  { // get obj annotations
    objm->getObjAnnotation(ann.str, ann.col);
    if (!ann.str.empty())
      annotations.push_back(ann);
  }

  obsplots_->addAnnotations(annotations);

  // get trajectory annotations
  for (TrajectoryPlot* tp : vtp) {
    if (!tp->isEnabled())
      continue;
    tp->getAnnotation(ann.str, ann.col);
    // empty string if data plot is off
    if (!ann.str.empty())
      annotations.push_back(ann);
  }

  // get stationPlot annotations
  for (StationPlot* sp : stam->plots()) {
    if (!sp->isEnabled())
      continue;
    sp->getAnnotation(ann.str, ann.col);
    annotations.push_back(ann);
  }

  // get locationPlot annotations
  for (LocationPlot* lp : locationPlots) {
    if (!lp->isEnabled())
      continue;
    lp->getAnnotation(ann.str, ann.col);
    annotations.push_back(ann);
  }

  // Miscellaneous managers
  ann.col = Colour(0, 0, 0);
  for (Manager* m : boost::adaptors::values(managers)) {
    // Obtain the annotations for enabled managers.
    if (m->isEnabled()) {
      for (const std::string& a : m->getAnnotations()) {
        ann.str = a;
        annotations.push_back(ann);
      }
    }
  }

  for (AnnotationPlot* ap : vap) {
    ap->setData(annotations, fieldAnalysisTimes);
    ap->setfillcolour(staticPlot_->getBackgroundColour());
  }

  //annotations from data

  //get field and sat annotations
  for (AnnotationPlot* ap : vap) {
    vector<vector<string> > vvstr = ap->getAnnotationStrings();
    for (vector<string>& as : vvstr) {
      fieldplots_->getDataAnnotations(as);
      obsplots_->getDataAnnotations(as);
      satm->getDataAnnotations(as);
      editm->getDataAnnotations(as);
      objm->getDataAnnotations(as);
    }
    ap->setAnnotationStrings(vvstr);
  }

  //get obs annotations
  obsplots_->getExtraAnnotations(vap);

  //objects
  PlotCommand_cpv objLabels = objm->getObjectLabels();
  for (PlotCommand_cp pc : objLabels) {
    AnnotationPlot* ap = new AnnotationPlot(pc);
    vap.push_back(ap);
  }
}

bool PlotModule::updatePlots()
{
  METLIBS_LOG_SCOPE();

  const miTime& t = staticPlot_->getTime();

  bool nodata = vmp.empty(); // false when data are found


  if (fieldplots_->update()) {
    nodata = false;
    // level for vertical level observations "as field"
    staticPlot_->setVerticalLevel(fieldplots_->getVerticalLevel());
  }

  // prepare data for satellite plots
  if (satm->setData())
    nodata = false;
  else
    METLIBS_LOG_DEBUG("SatManager returned false from setData");

  // set maparea from map spec., sat or fields

  defineMapArea();

  if (obsplots_->update(false, t))
    nodata = false;

  // prepare met-objects
  if (objm->prepareObjects(t, staticPlot_->getMapArea()))
    nodata = false;

  // prepare item stored in miscellaneous managers
  for (Manager* m : boost::adaptors::values(managers)) {
    // If the preparation fails then return false to indicate an error.
    if (m->isEnabled() && !m->prepare(t))
      nodata = false;
  }

  // prepare editobjects (projection etc.)
  objm->changeProjection(staticPlot_->getMapArea());

  // this is called in plotUnder:
  // vareaobjects[i].changeProjection(staticPlot_->getMapArea());

  const std::vector<StationPlot*>& stam_plots = stam->plots();
  for (size_t i = 0; i < stam_plots.size(); i++) {
    stam_plots[i]->changeProjection();
    nodata = false;
  }

  // Prepare/compute trajectories - change projection
  if (vtp.size() > 0) {
    vtp[0]->prepare();
    nodata = false;
  }

  // Prepare measurement positions - change projection
  if (vMeasurementsPlot.size() > 0) {
    vMeasurementsPlot[0]->changeProjection();
  }

  setAnnotations();

  PlotAreaSetup();

  // Update drawing items - this needs to be after the PlotAreaSetup call
  // because we need to reproject the items to screen coordinates.
  callManagersChangeProjection();

  return !nodata;
}

void PlotModule::defineMapArea()
{
  bool mapdefined = false;
  Area newMapArea;

  if (mapDefinedByUser) {     // area != "modell/sat-omr."

    if (!keepcurrentarea) { // show def. area
      newMapArea = staticPlot_->getRequestedarea();
    } else if( getMapArea().P() != staticPlot_->getRequestedarea().P() // or user just selected new area
        || previousrequestedarea.R() != staticPlot_->getRequestedarea().R())
    {
      newMapArea = staticPlot_->findBestMatch(staticPlot_->getRequestedarea());
    }
    mapdefined = true;

  } else if (keepcurrentarea && previousrequestedarea != staticPlot_->getRequestedarea()) {
    // change from specified area to model/sat area
    mapDefinedByData = false;
  }

  if (keepcurrentarea && mapDefinedByData)
    mapdefined = true;

  if (!mapdefined) {
    if (satm->getSatArea(newMapArea)) {
      // set area equal to first EXISTING sat-area
      if (keepcurrentarea)
        newMapArea = staticPlot_->findBestMatch(newMapArea);
      mapdefined = mapDefinedByData = true;
    }
  }

  if (!mapdefined && editm->isInEdit()) {
    // set area equal to editfield-area
    if (editm->getFieldArea(newMapArea)) {
      mapdefined = mapDefinedByData = true;
    }
  }

  if (!mapdefined) {
    if (fieldplots_->getRealFieldArea(newMapArea)) {
      if (keepcurrentarea)
        newMapArea = staticPlot_->findBestMatch(newMapArea);
      mapdefined = mapDefinedByData = true;
    }
  }

  if (keepcurrentarea && mapDefinedByView)
    mapdefined = true;

  if (!mapdefined) {
    newMapArea.setDefault();
    mapdefined = mapDefinedByView = true;
  }

  staticPlot_->setMapArea(newMapArea);

  previousrequestedarea = staticPlot_->getRequestedarea();
}

// -------------------------------------------------------------------------
// Master plot-routine
// The under/over toggles are used for speedy editing/drawing
// under: plot underlay part of image (static fields, sat.pict., obs. etc.)
// over:  plot overlay part (editfield, objects etc.)
//--------------------------------------------------------------------------
void PlotModule::plot(DiGLPainter* gl, bool under, bool over)
{
#if defined(DEBUGPRINT) || defined(DEBUGREDRAW)
  METLIBS_LOG_SCOPE(LOGVAL(under) << LOGVAL(over));
#endif

  //if plotarea has changed, calculate great circle distance...
  if (staticPlot_->getDirty())
    staticPlot_->updateGcd(gl);

  if (under)
    plotUnder(gl);

  if (over)
    plotOver(gl);

  staticPlot_->setDirty(false);
}

void PlotModule::plotUnder(DiGLPainter* gl)
{
#ifdef DEBUGREDRAW
  METLIBS_LOG_SCOPE();
#endif

  const Rectangle& plotr = staticPlot_->getPlotSize();

  const Colour& cback = staticPlot_->getBackgroundColour();

  // set correct worldcoordinates
  gl->LoadIdentity();
  gl->Ortho(plotr.x1, plotr.x2, plotr.y1, plotr.y2, -1, 1);

  gl->Enable(DiGLPainter::gl_BLEND);
  gl->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);

  // Set the default stencil buffer value.
  gl->ClearStencil(0);

  gl->ClearColor(cback.fR(), cback.fG(), cback.fB(), cback.fA());
  gl->Clear(DiGLPainter::gl_COLOR_BUFFER_BIT | DiGLPainter::gl_DEPTH_BUFFER_BIT | DiGLPainter::gl_STENCIL_BUFFER_BIT);

  // draw background (for hardcopy)
  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
  gl->Color4f(cback.fR(), cback.fG(), cback.fB(), cback.fA());
  const float d = 0;
  gl->Rectf(plotr.x1 + d, plotr.y1 + d, plotr.x2 - d, plotr.y2 - d);
  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_LINE);
  gl->Disable(DiGLPainter::gl_BLEND);

  for (Manager* m : boost::adaptors::values(managers)) {
    if (m->isEnabled())
      m->changeProjection(staticPlot_->getMapArea());
  }

  // plot map-elements for lowest zorder
  for (size_t i = 0; i < vmp.size(); i++)
    vmp[i]->plot(gl, Plot::BACKGROUND);

  // plot other objects, including drawing items
  for (Manager* m : boost::adaptors::values(managers)) {
    if (m->isEnabled())
      m->plot(gl, Plot::BACKGROUND);
  }

  // plot satellite images
  satm->plot(gl, Plot::SHADE_BACKGROUND);

  // mark undefined areas/values in field (before map)
  fieldplots_->plot(gl, Plot::SHADE_BACKGROUND);

  // plot other objects, including drawing items
  for (Manager* m : boost::adaptors::values(managers)) {
    if (m->isEnabled())
      m->plot(gl, Plot::SHADE_BACKGROUND);
  }

  // plot fields (shaded fields etc. before map)
  fieldplots_->plot(gl, Plot::SHADE);

  // plot other objects, including drawing items
  for (Manager* m : boost::adaptors::values(managers)) {
    if (m->isEnabled())
      m->plot(gl, Plot::SHADE);
  }

  // plot map-elements for auto zorder
  for (size_t i = 0; i < vmp.size(); i++)
    vmp[i]->plot(gl, Plot::LINES_BACKGROUND);

  // plot other objects, including drawing items
  for (Manager* m : boost::adaptors::values(managers)) {
    if (m->isEnabled())
      m->plot(gl, Plot::LINES_BACKGROUND);
  }

  // plot locationPlots (vcross,...)
  for (size_t i = 0; i < locationPlots.size(); i++)
    locationPlots[i]->plot(gl, Plot::LINES);

  // plot fields (isolines, vectors etc. after map)
  fieldplots_->plot(gl, Plot::LINES);

  // next line also calls objects.changeProjection
  objm->plotObjects(gl, Plot::LINES);

  if (areaobjects_.get())
    areaobjects_->plot(gl, Plot::LINES);

  // plot station plots
  const std::vector<StationPlot*>& stam_plots = stam->plots();
  for (size_t j = 0; j < stam_plots.size(); j++)
    stam_plots[j]->plot(gl, Plot::LINES);

  // plot inactive edit fields/objects under observations
  editm->plot(gl, Plot::LINES);

  // plot other objects, including drawing items
  for (Manager* m : boost::adaptors::values(managers)) {
    if (m->isEnabled())
      m->plot(gl, Plot::LINES);
  }

  obsplots_->plot(gl, Plot::LINES);

  //plot trajectories
  for (size_t i = 0; i < vtp.size(); i++)
    vtp[i]->plot(gl, Plot::LINES);

  for (size_t i = 0; i < vMeasurementsPlot.size(); i++)
    vMeasurementsPlot[i]->plot(gl, Plot::LINES);

  if (showanno && !editm->isInEdit()) {
    // plot Annotations
    for (size_t i = 0; i < vap.size(); i++)
      vap[i]->plot(gl, Plot::LINES);
  }
}

// plot overlay ---------------------------------------
void PlotModule::plotOver(DiGLPainter* gl)
{
#ifdef DEBUGREDRAW
  METLIBS_LOG_SCOPE();
#endif

  // plot active draw- and editobjects here
  editm->plot(gl, Plot::OVERLAY);

  obsplots_->plot(gl, Plot::OVERLAY);

  if (editm->isInEdit()) {
    // Annotations
    if (showanno) {
      for (size_t i = 0; i < vap.size(); i++)
        vap[i]->plot(gl, Plot::OVERLAY);
    }
    for (size_t i = 0; i < editVap.size(); i++)
      editVap[i]->plot(gl, Plot::OVERLAY);

  } // if editm->isInEdit()

  for (Manager* m : boost::adaptors::values(managers)) {
    if (m->isEnabled()) {
      m->changeProjection(staticPlot_->getMapArea());
      m->plot(gl, Plot::OVERLAY);
    }
  }

  // plot map-elements for highest zorder
  for (size_t i = 0; i < vmp.size(); i++)
    vmp[i]->plot(gl, Plot::OVERLAY);

  // frame (not needed if maprect==fullrect)
  if (staticPlot_->getMapSize() != staticPlot_->getPlotSize()) {
    gl->ShadeModel(DiGLPainter::gl_FLAT);
    gl->setLineStyle(Colour(0, 0, 0), 1.0);
    const Rectangle mr = diutil::adjustedRectangle(staticPlot_->getMapSize(), -0.0001, -0.0001);
    gl->drawRect(false, mr.x1, mr.y1, mr.x2, mr.y2);
  }

  // plot rubberbox
  if (dorubberband) {
#ifdef DEBUGREDRAW
    METLIBS_LOG_DEBUG("PlotModule::plot rubberband oldx,oldy,newx,newy: "
        <<oldx<<" "<<oldy<<" "<<newx<<" "<<newy);
#endif
    const XY pold = staticPlot_->PhysToMap(XY(oldx, oldy));
    const XY pnew = staticPlot_->PhysToMap(XY(newx, newy));

    gl->setLineStyle(staticPlot_->getBackContrastColour(), 2);
    gl->drawRect(false, pold.x(), pold.y(), pnew.x(), pnew.y());
  }
}

const vector<AnnotationPlot*>& PlotModule::getAnnotations()
{
  return vap;
}

vector<Rectangle> PlotModule::plotAnnotations(DiGLPainter* gl)
{
  staticPlot_->updateGcd(gl); // FIXME add mCanvas to staticPlot_ and drop this

  // set correct worldcoordinates
  gl->LoadIdentity();
  const Rectangle& plotr = staticPlot_->getPlotSize();
  gl->Ortho(plotr.x1, plotr.x2, plotr.y1, plotr.y2, -1, 1);

  const Colour& cback = staticPlot_->getBackgroundColour();
  gl->ClearColor(cback.fR(), cback.fG(), cback.fB(), cback.fA());
  gl->Clear(DiGLPainter::gl_COLOR_BUFFER_BIT | DiGLPainter::gl_DEPTH_BUFFER_BIT | DiGLPainter::gl_STENCIL_BUFFER_BIT);

  vector<Rectangle> rectangles;

  unsigned int n = vap.size();
  for (unsigned int i = 0; i < n; i++) {
    //	METLIBS_LOG_DEBUG("i:"<<i);
    vap[i]->plot(gl, Plot::LINES);
    rectangles.push_back(vap[i]->getBoundingBox());
  }

  return rectangles;
}

void PlotModule::PlotAreaSetup()
{
  METLIBS_LOG_SCOPE();

  if (!staticPlot_->hasPhysSize())
    return;

  const Area& ma = staticPlot_->getMapArea();
  const Rectangle& mapr = ma.R();

  const float waspr = staticPlot_->getPhysWidth() / float(staticPlot_->getPhysHeight());
  const Rectangle mr = diutil::fixedAspectRatio(mapr, waspr, true);

  // update full plot area -- add border
  const float border = 0.0;
  const Rectangle fr = diutil::adjustedRectangle(mr, border, border);

  staticPlot_->setMapPlotSize(mr, fr);
}

void PlotModule::setPlotWindow(int w, int h)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE(LOGVAL(w) << LOGVAL(h));
#endif

  staticPlot_->setPhysSize(w, h);

  PlotAreaSetup();
}

void PlotModule::cleanup()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  diutil::delete_all_and_clear(vmp);

  fieldplots_->cleanup();

  satm->clear();

  stam->cleanup();

  obsplots_->cleanup();

  objm->clearObjects();

  diutil::delete_all_and_clear(vtp);

  diutil::delete_all_and_clear(vMeasurementsPlot);

  annotationCommands.clear();

  diutil::delete_all_and_clear(vap);
}

Rectangle PlotModule::getPhysRectangle() const
{
  const float pw = staticPlot_->getPhysWidth(), ph = staticPlot_->getPhysHeight();
  return Rectangle(0, 0, pw, ph);
}

void PlotModule::callManagersChangeProjection()
{
  for (Manager* m : boost::adaptors::values(managers))
    m->changeProjection(staticPlot_->getMapArea());
}

void PlotModule::setMapArea(const Area& area)
{
  const bool projChanged = (staticPlot_->getMapArea().P() != area.P());

  staticPlot_->setMapArea(area);
  PlotAreaSetup();

  if (projChanged) {
    updatePlots();
  } else {
    callManagersChangeProjection();
  }
}

void PlotModule::setMapAreaFromMap(const Rectangle& rectangle)
{
  const Area a(staticPlot_->getMapArea().P(), rectangle);

  staticPlot_->setMapArea(a);
  PlotAreaSetup();

  updatePlots();
}

void PlotModule::setMapAreaFromPhys(const Rectangle& phys)
{
  if (!staticPlot_->hasPhysSize())
    return;

  const Rectangle newr = makeRectangle(staticPlot_->PhysToMap(XY(phys.x1, phys.y1)),
      staticPlot_->PhysToMap(XY(phys.x2, phys.y2)));
  const Area ma(staticPlot_->getMapArea().P(), newr);

  setMapArea(ma);
}

bool PlotModule::PhysToGeo(const float x, const float y, float& lat, float& lon)
{
  return staticPlot_->PhysToGeo(x, y, lat, lon);
}

bool PlotModule::GeoToPhys(const float lat, const float lon, float& x, float& y)
{
  return staticPlot_->GeoToPhys(lat, lon, x, y);
}

void PlotModule::PhysToMap(const float x, const float y, float& xmap, float& ymap)
{
  staticPlot_->PhysToMap(x, y, xmap, ymap);
}

/// return field grid x,y from map x,y if field defined and map proj = field proj
bool PlotModule::MapToGrid(const float xmap, const float ymap,
    float& gridx, float& gridy)
{
  Area a;
  if (satm->getSatArea(a) and staticPlot_->getMapArea().P() == a.P()) {
    float rx=0, ry=0;
    if (satm->getGridResolution(rx, ry)) {
      gridx = xmap/rx;
      gridy = ymap/ry;
      return true;
    }
  }

  if (fieldplots_->MapToGrid(staticPlot_->getMapArea().P(), xmap, ymap, gridx, gridy))
    return true;

  return false;
}

double PlotModule::getEntireWindowDistances(const bool horizontal){
  int x1,  y2, x2 = 0, y1 = 0;
  getPlotWindow(x1, y2);

  float flat1, flat3, flat4, flon1, flon3, flon4;
  PhysToGeo(x1, y1, flat1, flon1);
  PhysToGeo(x1, y2, flat3, flon3);
  PhysToGeo(x2, y2, flat4, flon4);
  double dist;
  if(horizontal){
    dist = GreatCircleDistance(flat3,flat4,flon3 ,flon4);
  } else {
    dist = GreatCircleDistance(flat1,flat3,flon1 ,flon3);
  }
  return dist;
}

double PlotModule::getWindowDistances(const float& x, const float& y, const bool horizontal){

  if ( !dorubberband )
    return getEntireWindowDistances(horizontal);

  float flat1, flat3, flat4, flon1, flon3, flon4;
  PhysToGeo(startx, starty, flat1, flon1);
  PhysToGeo(startx, y, flat3, flon3);
  PhysToGeo(x, y, flat4, flon4);
  double dist;
  if(horizontal){
    dist = GreatCircleDistance(flat3,flat4,flon3 ,flon4);
  } else {
    dist = GreatCircleDistance(flat1,flat3,flon1 ,flon3);
  }
  return dist;
}

double PlotModule::getMarkedArea(const float& x, const float& y){

  if ( !dorubberband) return 0.;

  float flat1, flat2, flat3, flat4, flon1, flon2, flon3, flon4;
  PhysToGeo(startx, starty, flat1, flon1);
  PhysToGeo(x, starty, flat2, flon2);
  PhysToGeo(startx, y, flat3, flon3);
  PhysToGeo(x, y, flat4, flon4);
  return getArea(flat1, flat2, flat3, flat4, flon1, flon2, flon3, flon4);
}

double PlotModule::getWindowArea(){
  int x1,  y2, x2 = 0, y1 = 0;
  getPlotWindow(x1, y2);
  float flat1, flat2, flat3, flat4, flon1, flon2, flon3, flon4;
  PhysToGeo(x1, y1, flat1, flon1);
  PhysToGeo(x2, y1, flat2, flon2);
  PhysToGeo(x1, y2, flat3, flon3);
  PhysToGeo(x2, y2, flat4, flon4);
  return getArea(flat1, flat2, flat3, flat4, flon1, flon2, flon3, flon4);
}

double PlotModule::calculateArea(double hLSide, double hUSide, double vLSide, double vRSide, double diag){
  /*Calculates the area as two triangles
   * Each triangle is calculated with Herons formula*/
  // Returns the calculated area as m2
  double calcArea = 0;
  double p1 = ((hLSide + vLSide + diag)/2);
  double p2 = ((hUSide + vRSide + diag)/2);
  double nonsqrt1, nonsqrt2;
  nonsqrt1 = p1*(p1 - hLSide)*(p1 - vLSide)*(p1 - diag);
  nonsqrt2 = p2*(p1 - hUSide)*(p1 - vRSide)*(p1 - diag);
  calcArea = sqrt(nonsqrt1)+sqrt(nonsqrt2);
  return calcArea;
}

double PlotModule::getArea(const float& flat1,
    const float& flat2,
    const float& flat3,
    const float& flat4,
    const float& flon1,
    const float& flon2,
    const float& flon3,
    const float& flon4){
  //Calculate distance vertical left side with earth radius in mind
  double vLSide = GreatCircleDistance(flat1,flat3,flon1,flon3);
  //Calculate distance horizontal lower side with earth radius in mind
  double hLSide = GreatCircleDistance(flat3,flat4,flon3,flon4);
  //Calculate distance vertical right side with earth radius in mind
  double vRSide = GreatCircleDistance(flat2,flat4,flon2,flon4);
  //Calculate distance horizontal upper side with earth radius in mind
  double hUSide = GreatCircleDistance(flat1,flat2,flon1,flon2);
  //Calculate distance diagonal with earth radius in mind
  double diagonal = GreatCircleDistance(flat1,flat4,flon1,flon4);
  //Calculate area as addition of two triangles
  return calculateArea(hLSide, hUSide, vLSide, vRSide, diagonal);
}


// set managers
void PlotModule::setManagers(FieldManager* fieldm, FieldPlotManager* fpm,
    ObsManager* om, SatManager* sm, StationManager* stm, ObjectManager* obm, EditManager* edm)
{
  METLIBS_LOG_SCOPE();
  fieldplotm = fpm;
  obsm = om;
  satm = sm;
  stam = stm;
  objm = obm;
  editm = edm;

  if (!fieldm)
    METLIBS_LOG_ERROR("fieldmanager==0");
  if (!fieldplotm)
    METLIBS_LOG_ERROR("fieldplotmanager==0");
  if (!obsm)
    METLIBS_LOG_ERROR("obsmanager==0");
  if (!satm)
    METLIBS_LOG_ERROR("satmanager==0");
  if (!stam)
    METLIBS_LOG_ERROR("stationmanager==0");
  if (!objm)
    METLIBS_LOG_ERROR("objectmanager==0");
  if (!editm)
    METLIBS_LOG_ERROR("editmanager==0");

  obsplots_.reset(new ObsPlotCluster(obsm, editm));
  fieldplots_.reset(new FieldPlotCluster(fieldm, fieldplotm));
}

Manager *PlotModule::getManager(const std::string &name)
{
  managers_t::iterator it = managers.find(name);
  if (it == managers.end())
    return 0;
  else
    return it->second;
}

// return current plottime
const miutil::miTime& PlotModule::getPlotTime() const
{
  return staticPlot_->getTime();
}

namespace {
typedef std::vector<miutil::miTime> plottimes_t;
typedef std::map<std::string, plottimes_t> times_t;
void insertTimes(times_t& times, const std::string& plotkey, const plottimes_t& plottimes)
{
  if (!plottimes.empty())
    times.insert(std::make_pair(plotkey, plottimes));
}
} // namespace

void PlotModule::getPlotTimes(map<string,vector<miutil::miTime> >& times)
{
  times.clear();

  insertTimes(times, "products", editm->getTimes());
  insertTimes(times, "fields", fieldplots_->getTimes());
  insertTimes(times, "satellites", satm->getSatTimes());
  insertTimes(times, "observations", obsplots_->getTimes());
  insertTimes(times, "objects", objm->getTimes());
  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it) {
    insertTimes(times, it->first, it->second->getTimes());
  }
}

//returns union or intersection of plot times from all pinfos
void PlotModule::getCapabilitiesTime(set<miTime>& okTimes, const PlotCommand_cpv& pinfos, bool allTimes)
{
  vector<miTime> normalTimes;
  int timediff = -1;
  bool normalTimesFound = false;
  bool moreTimes = true;
  for (PlotCommand_cp pc : pinfos) {
    const std::string& type = pc->commandKey();
    if (type == "FIELD")
      fieldplotm->getCapabilitiesTime(normalTimes, timediff, pc);
    else if (type == "SAT")
      satm->getCapabilitiesTime(normalTimes, timediff, pc);
    else if (type == "OBS")
      obsm->getCapabilitiesTime(normalTimes, timediff, pc);
    else if (type == "OBJECTS")
      objm->getCapabilitiesTime(normalTimes, timediff, pc);

    if (moreTimes) { //insert okTimes

      if ((!normalTimesFound && normalTimes.size()))
        normalTimesFound = true;

      //if intersection and no common times, no need to look for more times
      if ((!allTimes && normalTimesFound && !normalTimes.size()))
        moreTimes = false;

      if (allTimes || okTimes.empty()) { // union or first el. of intersection
        okTimes.insert(normalTimes.begin(), normalTimes.end());

      } else { //intersection

        set<miTime> tmptimes;
        for (set<miTime>::const_iterator p = okTimes.begin(); p != okTimes.end(); p++) {
          vector<miTime>::const_iterator itn = normalTimes.begin();
          while (itn != normalTimes.end() and abs(miTime::minDiff(*p, *itn)) > timediff)
            ++itn;
          if (itn != normalTimes.end())
            tmptimes.insert(*p); //time ok
        }
        okTimes = tmptimes;
      }
    } // if neither normalTimes nor constatTime, product is ignored
    normalTimes.clear();
  }
}

// set plottime
void PlotModule::setPlotTime(const miTime& t)
{
  staticPlot_->setTime(t);
}

void PlotModule::updateObs()
{
  // Update ObsPlots if data files have changed
  obsplots_->update(true, staticPlot_->getTime());

  // get annotations from all plots
  setAnnotations();
}

AreaObjectsCluster* PlotModule::areaobjects()
{
  if (!areaobjects_.get()) {
    areaobjects_.reset(new AreaObjectsCluster(this));
  }
  return areaobjects_.get();
}


//********** plotting and selecting locationPlots on the map *************

void PlotModule::putLocation(const LocationData& locationdata)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  bool found = false;
  for (size_t i = 0; i < locationPlots.size(); i++) {
    if (locationdata.name == locationPlots[i]->getName()) {
      bool visible = locationPlots[i]->isVisible();
      delete locationPlots[i];
      locationPlots[i] = new LocationPlot();
      locationPlots[i]->setData(locationdata);
      if (!visible)
        locationPlots[i]->hide();
      found = true;
    }
  }
  if (!found) {
    LocationPlot* lp = new LocationPlot();
    lp->setData(locationdata);
    locationPlots.push_back(lp);
  }
  setAnnotations();
}

void PlotModule::updateLocation(const LocationData& locationdata)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  const std::string& name = locationdata.name;
  for (size_t i = 0; i < locationPlots.size(); i++) {
    if (name == locationPlots[i]->getName()) {
      locationPlots[i]->updateOptions(locationdata);
    }
  }
}

void PlotModule::deleteLocation(const std::string& name)
{
  vector<LocationPlot*>::iterator p = locationPlots.begin();
  vector<LocationPlot*>::iterator pend = locationPlots.end();

  while (p != pend && name != (*p)->getName())
    p++;
  if (p != pend) {
    delete (*p);
    locationPlots.erase(p);
    setAnnotations();
  }
}

void PlotModule::setSelectedLocation(const std::string& name,
    const std::string& elementname)
{
  int n = locationPlots.size();
  for (int i = 0; i < n; i++) {
    if (name == locationPlots[i]->getName())
      locationPlots[i]->setSelected(elementname);
  }
}

std::string PlotModule::findLocation(int x, int y, const std::string& name)
{

  int n = locationPlots.size();
  for (int i = 0; i < n; i++) {
    if (name == locationPlots[i]->getName())
      return locationPlots[i]->find(x, y);
  }
  return std::string();
}

//****************************************************

void PlotModule::trajPos(const vector<std::string>& vstr)
{
  const int n = vtp.size();

  //if vstr starts with "quit", delete all trajectoryPlot objects
  for (size_t j = 0; j < vstr.size(); j++) {
    if (vstr[j].substr(0, 4) == "quit") {
      diutil::delete_all_and_clear(vtp);
      setAnnotations(); // will remove tarjectoryPlot annotation
      return;
    }
  }

  //if no trajectoryPlot object, make one
  if (n == 0)
    vtp.push_back(new TrajectoryPlot());

  //there are never more than one trajectoryPlot object (so far..)
  int action = vtp[0]->trajPos(vstr);

  if (action == 1) {
    // trajectories cleared, reset annotations
    setAnnotations(); // will remove tarjectoryPlot annotation
  }
}

void PlotModule::measurementsPos(const vector<std::string>& vstr)
{
  //if vstr starts with "quit", delete all MeasurementsPlot objects
  for (size_t j = 0; j < vstr.size(); j++) {
    if (diutil::startswith(vstr[j], "quit")) {
      diutil::delete_all_and_clear(vMeasurementsPlot);
      return;
    }
  }

  //if no MeasurementsPlot object, make one
  if (vMeasurementsPlot.empty())
    vMeasurementsPlot.push_back(new MeasurementsPlot());

  //there are never more than one MeasurementsPlot object (so far..)
  vMeasurementsPlot[0]->measurementsPos(vstr);
}

bool PlotModule::markAnnotationPlot(int x, int y)
{
  bool marked = false;
  for (size_t j = 0; j < editVap.size(); j++)
    if (editVap[j]->markAnnotationPlot(x, y))
      marked = true;
  return marked;
}

std::string PlotModule::getMarkedAnnotation()
{
  std::string annotext;
  for (size_t j = 0; j < editVap.size(); j++) {
    std::string text = editVap[j]->getMarkedAnnotation();
    if (!text.empty())
      annotext = text;
  }
  return annotext;
}

void PlotModule::changeMarkedAnnotation(std::string text, int cursor, int sel1,
    int sel2)
{
  for (size_t j = 0; j < editVap.size(); j++)
    editVap[j]->changeMarkedAnnotation(text, cursor, sel1, sel2);
}

void PlotModule::DeleteMarkedAnnotation()
{
  for (size_t j = 0; j < editVap.size(); j++)
    editVap[j]->DeleteMarkedAnnotation();
}

void PlotModule::startEditAnnotation()
{
  for (size_t j = 0; j < editVap.size(); j++)
    editVap[j]->startEditAnnotation();
}

void PlotModule::stopEditAnnotation()
{
  for (size_t j = 0; j < editVap.size(); j++)
    editVap[j]->stopEditAnnotation();
}

void PlotModule::editNextAnnoElement()
{
  for (size_t j = 0; j < editVap.size(); j++)
    editVap[j]->editNextAnnoElement();
}

void PlotModule::editLastAnnoElement()
{
  for (size_t j = 0; j < editVap.size(); j++)
    editVap[j]->editLastAnnoElement();
}

PlotCommand_cpv PlotModule::writeAnnotations(const string& prodname)
{
  PlotCommand_cpv annoCommands;
  for (AnnotationPlot* ap : editVap) {
    if (PlotCommand_cp pc = ap->writeAnnotation(prodname))
      annoCommands.push_back(pc);
  }
  return annoCommands;
}

void PlotModule::updateEditLabels(const PlotCommand_cpv& productLabelCommands,
    const std::string& productName, bool newProduct)
{
  METLIBS_LOG_SCOPE();
  vector<AnnotationPlot*> oldVap; //display object labels
  //read the old labels...

  const PlotCommand_cpv& objLabels = objm->getEditObjects().getObjectLabels();
  for (PlotCommand_cp pc : objLabels)
    oldVap.push_back(new AnnotationPlot(pc));

  for (PlotCommand_cp pc : productLabelCommands) {
    AnnotationPlot* ap = new AnnotationPlot(pc);
    ap->setProductName(productName);

    std::vector< std::vector<std::string> > vvstr = ap->getAnnotationStrings();
    for (std::vector<std::string>& vstr : vvstr)
      fieldplots_->getDataAnnotations(vstr);
    ap->setAnnotationStrings(vvstr);

    // here we compare the labels, take input from oldVap
    for (size_t i = 0; i < oldVap.size(); i++)
      ap->updateInputLabels(oldVap[i], newProduct);

    editVap.push_back(ap);
  }

  diutil::delete_all_and_clear(oldVap);
}

void PlotModule::deleteAllEditAnnotations()
{
  diutil::delete_all_and_clear(editVap);
}

void PlotModule::setObjAuto(bool autoF)
{
  objm->setObjAuto(autoF);
}

void PlotModule::areaInsert(bool newArea)
{
  if (newArea && areaSaved) {
    areaSaved = false;
    return;
  }
  if (!newArea) {
    if (areaSaved)
      return;
    else
      areaSaved = true;
  }

  if(areaIndex>-1){
    areaQ.erase(areaQ.begin() + areaIndex + 1, areaQ.end());
  }
  if (areaQ.size() > 20)
    areaQ.pop_front();
  else
    areaIndex++;

  areaQ.push_back(staticPlot_->getMapArea());
}

void PlotModule::changeArea(ChangeAreaCommand ca)
{
  if (ca == CA_DEFINE_MYAREA) {
    myArea = staticPlot_->getMapArea();
    return;
  }

  Area a;
  MapManager mapm;

  if (ca == CA_HISTORY_PREVIOUS || ca == CA_HISTORY_NEXT) {
    areaInsert(false);
    if (ca == CA_HISTORY_PREVIOUS) {
      if (areaIndex < 1)
        return;
      areaIndex--;
    } else { // go to next area
      if (areaIndex + 2 > int(areaQ.size()))
        return;
      areaIndex++;
    }
    a = areaQ[areaIndex];
  } else {
    areaInsert(true);
    if (ca == CA_RECALL_MYAREA) {
      a = myArea;
    } else if (ca == CA_RECALL_F5) { //get predefined areas
      mapm.getMapAreaByFkey("F5", a);
    } else if (ca == CA_RECALL_F6) {
      mapm.getMapAreaByFkey("F6", a);
    } else if (ca == CA_RECALL_F7) {
      mapm.getMapAreaByFkey("F7", a);
    } else if (ca == CA_RECALL_F8) {
      mapm.getMapAreaByFkey("F8", a);
    }
  }

  setMapArea(a);
}

void PlotModule::zoomOut()
{
  const float scale = 0.15;
  const float dx = scale * staticPlot_->getPhysWidth(),
      dy = scale * staticPlot_->getPhysHeight();

  areaInsert(true);
  setMapAreaFromPhys(diutil::adjustedRectangle(getPhysRectangle(), dx, dy));
}

// keyboard/mouse events
void PlotModule::sendMouseEvent(QMouseEvent* me, EventResult& res)
{
  newx = me->x();
  newy = me->y();

  // ** mousepress
  if (me->type() == QEvent::MouseButtonPress) {
    oldx = me->x();
    oldy = me->y();

    if (me->button() == Qt::LeftButton) {
      dorubberband = true;
      res.enable_background_buffer = true;
      res.update_background_buffer = false;
      res.repaint = true;
      startx = me->x();
      starty = me->y();
      return;

    } else if (me->button() == Qt::MidButton) {
      areaInsert(true);
      staticPlot_->panPlot(true);
      res.newcursor = paint_move_cursor;
      return;
    }

    else if (me->button() == Qt::RightButton) {
      res.action = rightclick;
    }

    return;
  }
  // ** mousemove
  else if (me->type() == QEvent::MouseMove) {

    res.action = browsing;

    if (dorubberband) {
      res.action = quick_browsing;
      res.enable_background_buffer = true;
      res.update_background_buffer = false;
      res.repaint = true;
      return;

    } else if (staticPlot_->isPanning()) {
      const float dx = oldx - me->x(), dy = oldy - me->y();
      setMapAreaFromPhys(diutil::translatedRectangle(getPhysRectangle(), dx, dy));
      oldx = me->x();
      oldy = me->y();

      res.action = quick_browsing;
      res.enable_background_buffer = true;
      res.update_background_buffer = true;
      res.repaint = true;
      res.newcursor = paint_move_cursor;
      return;
    }

  }
  // ** mouserelease
  else if (me->type() == QEvent::MouseButtonRelease) {

    bool plotnew = false;

    res.enable_background_buffer = false;
    res.update_background_buffer = false;

    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    // minimum rubberband size for zooming (in pixels)
    const float rubberlimit = 15.;

    if (me->button() == Qt::RightButton) { // zoom out

      //end of popup
      //res.action= rightclick;

    } else if (me->button() == Qt::LeftButton) {

      x1 = oldx;
      y1 = oldy;
      x2 = me->x();
      y2 = me->y();

      if (oldx > x2) {
        x1 = x2;
        x2 = oldx;
      }
      if (oldy > y2) {
        y1 = y2;
        y2 = oldy;
      }
      if (fabsf(x2 - x1) > rubberlimit && fabsf(y2 - y1) > rubberlimit) {
        if (dorubberband)
          plotnew = true;
      } else {
        res.action = pointclick;
      }

      dorubberband = false;
      startx = starty = 0;

    } else if (me->button() == Qt::MidButton) {
      staticPlot_->panPlot(false);
      res.enable_background_buffer = false;
      res.update_background_buffer = false;
      res.repaint = true;
      return;
    }
    if (plotnew) {
      //define new plotarea, first save the old one
      areaInsert(true);
      setMapAreaFromPhys(Rectangle(x1, y1, x2, y2));

      res.enable_background_buffer = false;
      res.update_background_buffer = true;
      res.repaint = true;
    }

    return;
  }
  // ** mousedoubleclick
  else if (me->type() == QEvent::MouseButtonDblClick) {
    res.action = doubleclick;
  }
}

void PlotModule::areaNavigation(PlotModule::AreaNavigationCommand anav, EventResult& res)
{
  static int arrowKeyDirection = 1;

  float dx = 0, dy = 0;
  float zoom = 0.;

  if (anav == ANAV_HOME) {
    keepcurrentarea = false;
    updatePlots();
    keepcurrentarea = true;
  } else if (anav == ANAV_TOGGLE_DIRECTION) {
    arrowKeyDirection *= -1;
    return;
  } else if (anav == ANAV_PAN_LEFT)
    dx = -staticPlot_->getPhysWidth() / 8.0f;
  else if (anav == ANAV_PAN_RIGHT)
    dx = staticPlot_->getPhysWidth() / 8.0f;
  else if (anav == ANAV_PAN_DOWN)
    dy = -staticPlot_->getPhysHeight() / 8.0f;
  else if (anav == ANAV_PAN_UP)
    dy = staticPlot_->getPhysHeight() / 8.0f;
  else if (anav == ANAV_ZOOM_OUT)
    zoom = 1.3;
  else if (anav == ANAV_ZOOM_IN)
    zoom = 1. / 1.3;

  if (zoom > 0. || dx != 0 || dy != 0) {
    Rectangle r;
    if (dx != 0 || dy != 0) {
      dx *= arrowKeyDirection;
      dy *= arrowKeyDirection;
      r = diutil::translatedRectangle(getPhysRectangle(), dx, dy);
    } else {
      dx = staticPlot_->getPhysWidth()*(zoom-1);
      dy = staticPlot_->getPhysHeight()*(zoom-1);
      r = diutil::adjustedRectangle(getPhysRectangle(), dx, dy);
    }
    //define new plotarea, first save the old one
    areaInsert(true);
    setMapAreaFromPhys(r);
  }

  res.enable_background_buffer = false;
  res.repaint = true;
}

bool PlotModule::startTrajectoryComputation()
{
  METLIBS_LOG_SCOPE();
  if (vtp.size() < 1)
    return false;

  const FieldPlot* fp = fieldplots()->findTrajectoryPlot(miutil::to_lower(vtp[0]->getFieldName()));
  if (!fp)
    return false;

  TrajectoryGenerator tg(fieldplotm, fp, staticPlot_->getTime());
  tg.setIterationCount(vtp[0]->getIterationCount());
  tg.setTimeStep(vtp[0]->getTimeStep());
  const TrajectoryGenerator::LonLat_v& pos = vtp[0]->getStartPositions();
  for (TrajectoryGenerator::LonLat_v::const_iterator itP = pos.begin(); itP != pos.end(); ++itP)
    tg.addPosition(*itP);

  const TrajectoryData_v trajectories = tg.compute();
  vtp[0]->setTrajectoryData(trajectories);

  return true;
}

void PlotModule::stopTrajectoryComputation()
{
}

// write trajectory positions to file
bool PlotModule::printTrajectoryPositions(const std::string& filename)
{
  if (vtp.size() > 0)
    return vtp[0]->printTrajectoryPositions(filename);

  return false;
}

/********************* reading and writing log file *******************/

vector<std::string> PlotModule::writeLog()
{
  //put last area in areaQ
  areaInsert(true);

  vector<std::string> vstr;

  //Write self-defined area (F2)
  std::string aa = "name=F2 " + myArea.getAreaString();
  vstr.push_back(aa);

  //Write all araes in list (areaQ)
  for (size_t i = 0; i < areaQ.size(); i++) {
    aa = "name=" + miutil::from_number(int(i)) + " " + areaQ[i].getAreaString();
    vstr.push_back(aa);
  }

  return vstr;
}

void PlotModule::readLog(const vector<std::string>& vstr,
    const std::string& thisVersion, const std::string& logVersion)
{
  areaQ.clear();
  Area area;
  for (size_t i = 0; i < vstr.size(); i++) {

    if(!area.setAreaFromString(vstr[i])) {
      continue;
    }
    if (area.Name() == "F2") {
      myArea = area;
    } else {
      areaQ.push_back(area);
    }
  }

  areaIndex = areaQ.size() - 1;
}

void PlotModule::getPlotWindow(int &width, int &height)
{
  width = staticPlot_->getPhysWidth();
  height = staticPlot_->getPhysHeight();
}
