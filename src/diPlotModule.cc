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

#include "diPlotModule.h"

#include "diAreaObjectsCluster.h"
#include "diEditManager.h"
#include "diFieldPlotCluster.h"
#include "diFieldPlotManager.h"
#include "diKVListPlotCommand.h"
#include "diLocationPlot.h"
#include "diLocationPlotCluster.h"
#include "diManager.h"
#include "diMapAreaSetup.h"
#include "diMapPlotCluster.h"
#include "diMeasurementsPlot.h"
#include "diObjectManager.h"
#include "diObjectPlotCluster.h"
#include "diObsManager.h"
#include "diObsPlotCluster.h"
#include "diSatManagerBase.h"
#include "diSatPlotCluster.h"
#include "diStaticPlot.h"
#include "diStationManager.h"
#include "diStationPlot.h"
#include "diStationPlotCluster.h"
#include "diTrajectoryGenerator.h"
#include "diTrajectoryPlot.h"
#include "diTrajectoryPlotCluster.h"
#include "diUtilities.h"

#include "util/geo_util.h"
#include "util/misc_util.h"
#include "util/nearest_element.h"
#include "util/string_util.h"
#include "util/was_enabled.h"

#include <puDatatypes/miCoordinates.h>
#include <puTools/miStringFunctions.h>

#include <boost/range/adaptor/map.hpp>

#include <memory>

//#define DEBUGPRINT
//#define DEBUGREDRAW
#define MILOGGER_CATEGORY "diana.PlotModule"
#include <miLogger/miLogging.h>

using namespace miutil;

namespace {

// line width used for drawing rubberband
const float RUBBER_LINEWIDTH = 2;

// minimum rubberband size for drawing (in pixels)
const float RUBBER_LIMIT = 3 * RUBBER_LINEWIDTH;

double GreatCircleDistance(double lat1, double lat2, double lon1, double lon2)
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
  mapDefinedByUser = false;
  mapDefinedByData = false;
  mapDefinedByView = false;
}

PlotModule::~PlotModule()
{
  cleanup();
  self = nullptr;
}

void PlotModule::setCanvas(DiCanvas* canvas)
{
  METLIBS_LOG_SCOPE();
  if (mCanvas == canvas)
    return;

  // TODO set for all existing plots, and for new plots
  mCanvas = canvas;

  for (PlotCluster* pc : clusters())
    pc->setCanvas(canvas);

  for (Manager* m : boost::adaptors::values(managers))
    m->setCanvas(canvas);

  updateCanvasSize();

  setAnnotations(); // physical size change
}

void PlotModule::processInput(const PlotCommand_cpv& vpi)
{
  METLIBS_LOG_SCOPE();

  // split up input into separate products
  std::set<std::string> ordered;
  ordered.insert("AREA");
  ordered.insert(mapplots_->plotCommandKey());
  ordered.insert(fieldplots()->plotCommandKey());
  ordered.insert(obsplots()->plotCommandKey());
  ordered.insert(satplots()->plotCommandKey());
  ordered.insert(stationplots_->plotCommandKey());
  ordered.insert(objectplots_->plotCommandKey());
  ordered.insert(trajectoryplots_->plotCommandKey());
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
  mapplots_->processInput(ordered_pi[mapplots_->plotCommandKey()]);
  fieldplots_->processInput(ordered_pi[fieldplots()->plotCommandKey()]);
  obsplots_->processInput(ordered_pi[obsplots()->plotCommandKey()]);
  satplots_->processInput(ordered_pi[satplots()->plotCommandKey()]);
  stationplots_->processInput(ordered_pi[stationplots_->plotCommandKey()]);
  objectplots_->processInput(ordered_pi[objectplots_->plotCommandKey()]);
  trajectoryplots_->processInput(ordered_pi[trajectoryplots_->plotCommandKey()]);
  prepareAnnotation(ordered_pi["LABEL"]);
  editm->prepareEditFields(ordered_pi["EDITFIELD"]);

  // Send the commands to the other managers.
  for (auto& it : managers) {
    Manager* manager = it.second;
    manager->processInput(manager_pi[it.first]);
  }

  if (!mapplots_->getBackColour().empty())
    staticPlot_->setBgColour(mapplots_->getBackColour());
  staticPlot_->setVerticalLevel(fieldplots_->getVerticalLevel());  // vertical level for observations "as field"

  const Area mapAreaBefore = staticPlot_->getMapArea();
  defineMapArea();
  if (mapAreaBefore == staticPlot_->getMapArea())
    notifyChangeProjection(); // new plots must receive a changeProjection call

  // changeTime()?
}

void PlotModule::prepareArea(const PlotCommand_cpv& inp)
{
  METLIBS_LOG_SCOPE();

  if (!inp.size())
    return;
  if (inp.size() > 1)
    METLIBS_LOG_DEBUG("More AREA definitions, using: " <<inp[0]->toString());
  KVListPlotCommand_cp cmd = std::dynamic_pointer_cast<const KVListPlotCommand>(inp[0]);
  if (!cmd)
    return;

  const std::string key_name=  "name";
  const std::string key_proj=  "proj4string";
  const std::string key_rectangle=  "rectangle";
  const std::string key_rect = "rect";

  Area requestedarea = staticPlot_->getRequestedarea();
  Projection proj;
  Rectangle rect;

  for (const miutil::KeyValue& kv : cmd->all()) {
    if (!kv.value().empty()) {
      if (kv.key() == key_name) {
        if (!MapAreaSetup::instance()->getMapAreaByName(kv.value(), requestedarea)) {
          METLIBS_LOG_WARN("Unknown AREA definition '"<< kv.value() << "'");
        }

      } else if (kv.key() == key_proj) {
        if (proj.setFromString(kv.value())) {
          requestedarea.setP(proj);
        } else {
          METLIBS_LOG_WARN("Unknown proj definition '" << kv.value());
        }
      } else if (kv.key() == key_rectangle || kv.key() == key_rect) {
        if (rect.setRectangle(kv.value())) {
          if (kv.key() == key_rectangle && requestedarea.P().isDegree()) {
            // backward compatibility: when using proj4, rectangles were in radians; keep it like that for now
            diutil::convertRectToDegrees(rect);
          }
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

void PlotModule::prepareAnnotation(const PlotCommand_cpv& inp)
{
  METLIBS_LOG_SCOPE();

  // for now -- erase all annotationplots
  diutil::delete_all_and_clear(vap);
  annotationCommands = inp;
}

std::vector<PlotElement> PlotModule::getPlotElements()
{
  //  METLIBS_LOG_SCOPE();
  std::vector<PlotElement> pel;

  for (PlotCluster* pc : clusters())
    pc->addPlotElements(pel);

  for (Manager* m : boost::adaptors::values(managers))
    diutil::insert_all(pel, m->getPlotElements());

  return pel;
}

void PlotModule::enablePlotElement(const PlotElement& pe)
{
  bool change = false;
  for (PlotCluster* pc : clusters()) {
    if (pc->enablePlotElement(pe)) {
      change = true;
      break;
    }
  }
  if (!change) {
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
    setAnnotations(); // plot enabled/disabled
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
  std::vector<AnnotationPlot::Annotation> annotations;
  AnnotationPlot::Annotation ann;

  const plottimes_t fieldAnalysisTimes = fieldplots_->fieldAnalysisTimes();

  for (PlotCluster* pc : clusters())
    pc->addAnnotations(annotations);

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
    std::vector<std::vector<std::string> > vvstr = ap->getAnnotationStrings();
    for (std::vector<std::string>& as : vvstr) {
      for (PlotCluster* pc : clusters())
        pc->getDataAnnotations(as);

      editm->getDataAnnotations(as);
    }
    ap->setAnnotationStrings(vvstr);
  }

  for (PlotCluster* pc : clusters())
    diutil::insert_all(vap, pc->getExtraAnnotations());
}

void PlotModule::changeTime(const miutil::miTime& mapTime)
{
  METLIBS_LOG_SCOPE();
  staticPlot_->setTime(mapTime);

  for (PlotCluster* pc : clusters())
    pc->changeTime(mapTime);

  for (Manager* m : boost::adaptors::values(managers)) {
    if (m->isEnabled())
      m->changeTime(mapTime);
  }

  defineMapArea();
  setAnnotations(); // time / data change
}

PlotStatus PlotModule::getStatus()
{
  PlotStatus pcs;
  for (PlotCluster* pc : clusters())
    pcs.add(pc->getStatus());

  // FIXME vMeasurementsPlot

  for (Manager* m : boost::adaptors::values(managers)) {
    if (m->isEnabled())
      pcs.add(m->getStatus());
  }

  return pcs;
}

bool PlotModule::hasData()
{
  return (getStatus().get(P_OK_DATA) > 0);
}

bool PlotModule::hasError()
{
  return (getStatus().get(P_ERROR) > 0);
}

bool PlotModule::defineMapAreaFromData(Area& newMapArea, bool& allowKeepCurrentArea)
{
  // TODO this is common for all grid-based plots, in strange order

  if (satplots_->getSatArea(newMapArea)) {
    // set area equal to first EXISTING sat-area
    allowKeepCurrentArea = true;
    return true;
  }

  if (editm->getFieldArea(newMapArea)) {
    // set area equal to editfield-area
    allowKeepCurrentArea = false;
    return true;
  }

  if (fieldplots_->getRealFieldArea(newMapArea)) {
    allowKeepCurrentArea = true;
    return true;
  }
  return false;
}

void PlotModule::defineMapArea()
{
  bool mapdefined = false;
  Area newMapArea;

  if (mapDefinedByUser) { // area != "modell/sat-omr."

    newMapArea = staticPlot_->getRequestedarea();
    if (keepcurrentarea)
      newMapArea = staticPlot_->findBestMatch(newMapArea);
    mapdefined = true;

  } else if (keepcurrentarea && previousrequestedarea != staticPlot_->getRequestedarea()) {
    // change from specified area to model/sat area
    mapDefinedByData = false;
  }

  if (keepcurrentarea && mapDefinedByData)
    mapdefined = true;

  if (!mapdefined) {
    bool allowKeepCurrentArea = true;
    if (defineMapAreaFromData(newMapArea, allowKeepCurrentArea)) {
      if (allowKeepCurrentArea && keepcurrentarea)
        newMapArea = staticPlot_->findBestMatch(newMapArea);
      mapdefined = mapDefinedByData = true;
    }
  }

  if (keepcurrentarea && mapDefinedByView)
    mapdefined = true;

  if (!mapdefined) {
    newMapArea.setP(Projection("+proj=ob_tran +o_proj=longlat +lon_0=0 +o_lat_p=25 +x_0=0.811578 +y_0=0.637045 +ellps=WGS84 +towgs84=0,0,0 +no_defs"));
    newMapArea.setR(Rectangle(0, 0, 93.5, 75.5));
    mapDefinedByView = true;
  }

  setMapArea(newMapArea);

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

  if (under)
    plotUnder(gl);

  if (over)
    plotOver(gl);
}

void PlotModule::plotInit(DiGLPainter* gl)
{
  // set correct worldcoordinates
  gl->LoadIdentity();
  const Rectangle& plotr = staticPlot_->getPlotSize();
  gl->Ortho(plotr.x1, plotr.x2, plotr.y1, plotr.y2, -1, 1);

  gl->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);

  gl->ClearStencil(0);
  const Colour& cback = staticPlot_->getBackgroundColour();
  gl->ClearColor(cback.fR(), cback.fG(), cback.fB(), cback.fA());
  gl->Clear(DiGLPainter::gl_COLOR_BUFFER_BIT | DiGLPainter::gl_DEPTH_BUFFER_BIT | DiGLPainter::gl_STENCIL_BUFFER_BIT);
}

void PlotModule::plotUnder(DiGLPainter* gl)
{
#ifdef DEBUGREDRAW
  METLIBS_LOG_SCOPE();
#endif

  plotInit(gl);

  const PlotOrder pos[] = {PO_BACKGROUND, PO_SHADE_BACKGROUND, PO_SHADE, PO_LINES_BACKGROUND, PO_LINES};
  for (const PlotOrder po : pos)
    plotClustersAndManagers(gl, po);
}

// plot overlay ---------------------------------------
void PlotModule::plotOver(DiGLPainter* gl)
{
#ifdef DEBUGREDRAW
  METLIBS_LOG_SCOPE();
#endif

  const PlotOrder pos[] = {PO_OVERLAY, PO_OVERLAY_TOP};
  for (const PlotOrder po : pos)
    plotClustersAndManagers(gl, po);

  // draw frame if map has a border
  if (staticPlot_->getMapBorder() > 0) {
    gl->ShadeModel(DiGLPainter::gl_FLAT);
    gl->setLineStyle(Colour(0, 0, 0), 1.0);
    const Rectangle mr = diutil::adjustedRectangle(staticPlot_->getMapSize(), -0.0001, -0.0001);
    gl->drawRect(false, mr.x1, mr.y1, mr.x2, mr.y2);
  }

  // plot rubberbox
  if (dorubberband && std::abs(rubberband.x2 - rubberband.x1) > RUBBER_LIMIT && std::abs(rubberband.y2 - rubberband.y1) > RUBBER_LIMIT) {
    const XY pold = staticPlot_->PhysToMap(XY(rubberband.x1, rubberband.y1));
    const XY pnew = staticPlot_->PhysToMap(XY(rubberband.x2, rubberband.y2));

    gl->setLineStyle(staticPlot_->getBackContrastColour(), RUBBER_LINEWIDTH);
    gl->drawRect(false, pold.x(), pold.y(), pnew.x(), pnew.y());
  }
}

std::vector<PlotCluster*> PlotModule::clusters() const
{
  std::vector<PlotCluster*> pcs = {
      // clang-format off
      mapplots_.get(),
      fieldplots_.get(),
      obsplots_.get(),
      satplots_.get(),
      stationplots_.get(),
      objectplots_.get(),
      trajectoryplots_.get(),
      areaobjects_.get(),
      locationplots_.get()
      // clang-format on
  };
  pcs.erase(std::remove(pcs.begin(), pcs.end(), nullptr), pcs.end());
  return pcs;
}

void PlotModule::plotClustersAndManagers(DiGLPainter* gl, PlotOrder po)
{
  for (PlotCluster* pc : clusters())
    pc->plot(gl, po);

  for (MeasurementsPlot* mp : vMeasurementsPlot)
    mp->plot(gl, po);

  editm->plot(gl, po);

  for (Manager* m : boost::adaptors::values(managers)) {
    if (m->isEnabled())
      m->plot(gl, po);
  }

  const bool inEdit = editm->isInEdit();
  if (showanno && ((po == PO_LINES && !inEdit) || (po == PO_OVERLAY && inEdit))) {
    for (AnnotationPlot* ap : vap)
      ap->plot(gl, po);
  }
  if (inEdit && po == PO_OVERLAY) {
    for (AnnotationPlot* ap : editVap)
      ap->plot(gl, po);
  }
}

const std::vector<AnnotationPlot*>& PlotModule::getAnnotations()
{
  return vap;
}

std::vector<Rectangle> PlotModule::plotAnnotations(DiGLPainter* gl)
{
  plotInit(gl);

  std::vector<Rectangle> rectangles;
  rectangles.reserve(vap.size());
  for (AnnotationPlot* ap : vap) {
    ap->plot(gl, PO_LINES);
    rectangles.push_back(ap->getBoundingBox());
  }
  return rectangles;
}

void PlotModule::setPhysSize(int w, int h)
{
  if (staticPlot_->setPhysSize(w, h))
    notifyChangeProjection();
}

void PlotModule::cleanup()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  for (PlotCluster* pc : clusters())
    pc->cleanup();

  diutil::delete_all_and_clear(vMeasurementsPlot);
  diutil::delete_all_and_clear(vap);

  annotationCommands.clear();
}

void PlotModule::setPanning(bool pan)
{
  staticPlot_->setPanning(pan);
}

bool PlotModule::isPanning() const
{
  return staticPlot_->isPanning();
}

void PlotModule::updateCanvasSize()
{
  if (!mCanvas)
    return;
  StaticPlot* sp = getStaticPlot();
  mCanvas->setVpGlSize(sp->getPhysWidth(), sp->getPhysHeight(), sp->getPlotSize().width(), sp->getPlotSize().height());
}

void PlotModule::notifyChangeProjection()
{
  updateCanvasSize(); // TODO canvas size update is too frequent

  const Area& ma = staticPlot_->getMapArea();
  const Rectangle& ps = staticPlot_->getPlotSize();
  const diutil::PointI& wh = staticPlot_->getPhysSize();

  for (PlotCluster* pc : clusters())
    pc->changeProjection(ma, ps, wh);

  for (MeasurementsPlot* mp : vMeasurementsPlot)
    mp->changeProjection(ma, ps, wh);

  // editm

  for (Manager* m : boost::adaptors::values(managers))
    m->changeProjection(ma, ps, wh);

  setAnnotations(); // projection change
}

void PlotModule::setMapArea(const Area& area)
{
  if (staticPlot_->setMapArea(area))
    notifyChangeProjection();
}

void PlotModule::setMapAreaFromMap(const Rectangle& rectangle)
{
  const Area ma(staticPlot_->getMapProjection(), rectangle);
  setMapArea(ma);
}

void PlotModule::setMapAreaFromPhys(const Rectangle& phys)
{
  if (staticPlot_->hasPhysSize())
    setMapAreaFromMap(makeRectangle(staticPlot_->PhysToMap(XY(phys.x1, phys.y1)), staticPlot_->PhysToMap(XY(phys.x2, phys.y2))));
}

bool PlotModule::PhysToGeo(float xphys, float yphys, float& lat, float& lon)
{
  return staticPlot_->PhysToGeo(xphys, yphys, lat, lon);
}

bool PlotModule::GeoToPhys(float lat, float lon, float& xphys, float& yphys)
{
  return staticPlot_->GeoToPhys(lat, lon, xphys, yphys);
}

void PlotModule::PhysToMap(float xphys, float yphys, float& xmap, float& ymap)
{
  staticPlot_->PhysToMap(xphys, yphys, xmap, ymap);
}

/// return field grid x,y from map x,y if field defined and map proj = field proj
bool PlotModule::MapToGrid(float xmap, float ymap, float& gridx, float& gridy)
{
  // TODO this is common for all grid-based plots, in strange order; see also defineMapAreaFromData

  if (satplots_->MapToGrid(staticPlot_->getMapProjection(), xmap, ymap, gridx, gridy))
    return true;

  if (fieldplots_->MapToGrid(staticPlot_->getMapProjection(), xmap, ymap, gridx, gridy))
    return true;

  return false;
}

double PlotModule::getWindowDistances(float x1, float y1, float x2, float y2, bool horizontal)
{
  float flat3, flat4, flon3, flon4;
  PhysToGeo(x1, y2, flat3, flon3);
  if (horizontal) {
    PhysToGeo(x2, y2, flat4, flon4);
  } else {
    PhysToGeo(x1, y1, flat4, flon4);
  }
  return GreatCircleDistance(flat3, flat4, flon3, flon4);
}

double PlotModule::getEntireWindowDistances(const bool horizontal)
{
  const diutil::PointI& ps = getPhysSize();
  return getWindowDistances(ps.x(), 0, 0, ps.y(), horizontal);
}

double PlotModule::getWindowDistances(float x, float y, bool horizontal)
{
  if (!dorubberband)
    return getEntireWindowDistances(horizontal);
  else
    return getWindowDistances(rubberband.x1, rubberband.y1, x, y, horizontal);
}

double PlotModule::getMarkedArea(float x, float y)
{
  if (!dorubberband)
    return 0;
  else
    return getWindowArea(rubberband.x1, rubberband.y1, x, y);
}

double PlotModule::getWindowArea()
{
  const diutil::PointI& ps = getPhysSize();
  return getWindowArea(ps.x(), 0, 0, ps.y());
}

double PlotModule::getWindowArea(int x1, int y1, int x2, int y2)
{
  float flat1, flat2, flat3, flat4, flon1, flon2, flon3, flon4;
  PhysToGeo(x1, y1, flat1, flon1);
  PhysToGeo(x2, y1, flat2, flon2);
  PhysToGeo(x1, y2, flat3, flon3);
  PhysToGeo(x2, y2, flat4, flon4);
  return getArea(flat1, flat2, flat3, flat4, flon1, flon2, flon3, flon4);
}

// static
double PlotModule::calculateArea(double hLSide, double hUSide, double vLSide, double vRSide, double diag)
{
  /*Calculates the area as two triangles
   * Each triangle is calculated with Herons formula*/
  // Returns the calculated area as m2
  double p1 = ((hLSide + vLSide + diag)/2);
  double p2 = ((hUSide + vRSide + diag)/2);
  double nonsqrt1 = p1 * (p1 - hLSide) * (p1 - vLSide) * (p1 - diag);
  double nonsqrt2 = p2 * (p1 - hUSide) * (p1 - vRSide) * (p1 - diag);
  return sqrt(nonsqrt1) + sqrt(nonsqrt2);
}

// static
double PlotModule::getArea(float flat1, float flat2, float flat3, float flat4, float flon1, float flon2, float flon3, float flon4)
{
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
void PlotModule::setManagers(FieldPlotManager* fpm, ObsManager* om, SatManagerBase* sm, StationManager* stm, ObjectManager* obm, EditManager* edm)
{
  METLIBS_LOG_SCOPE();
  fieldplotm = fpm;
  obsm = om;
  satm = sm;
  stam = stm;
  objm = obm;
  editm = edm;

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
  mapplots_.reset(new MapPlotCluster());
  trajectoryplots_.reset(new TrajectoryPlotCluster());
  fieldplots_.reset(new FieldPlotCluster(fieldplotm));
  satplots_.reset(new SatPlotCluster(satm));
  stationplots_.reset(new StationPlotCluster(stam));
  objectplots_.reset(new ObjectPlotCluster(objm));
  locationplots_.reset(new LocationPlotCluster());
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
typedef std::map<std::string, plottimes_t> times_t;
void insertTimes(times_t& times, const std::string& plotkey, const plottimes_t& plottimes)
{
  if (!plottimes.empty())
    times.insert(std::make_pair(plotkey, plottimes));
}
} // namespace

void PlotModule::getPlotTimes(std::map<std::string, plottimes_t>& times)
{
  METLIBS_LOG_TIME();
  times.clear();

  insertTimes(times, "fields", fieldplots_->getTimes());
  insertTimes(times, "satellites", satplots_->getTimes());
  insertTimes(times, "observations", obsplots_->getTimes());
  insertTimes(times, "objects", objectplots_->getTimes());
  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it) {
    insertTimes(times, it->first, it->second->getTimes());
  }
}

void PlotModule::getCapabilitiesTime(plottimes_t& ctimes, const PlotCommand_cpv& pinfos, bool time_union)
{
  bool first_time = true;
  for (PlotCommand_cp pc : pinfos) {
    int timediff = -1;
    plottimes_t ntimes;

    const std::string& type = pc->commandKey();
    if (type == "FIELD")
      fieldplotm->getCapabilitiesTime(ntimes, timediff, pc);
    else if (type == "SAT")
      satm->getCapabilitiesTime(ntimes, timediff, pc);
    else if (type == "OBS")
      obsm->getCapabilitiesTime(ntimes, timediff, pc);
    else if (type == "OBJECTS")
      objm->getCapabilitiesTime(ntimes, timediff, pc);
    else
      continue;

    if (time_union || first_time) { // union or first el. of intersection
      ctimes.insert(ntimes.begin(), ntimes.end());

    } else { // intersection

      // no common times, no need to look for more times
      if (ntimes.empty())
        break;

      plottimes_t tmptimes;
      for (const miutil::miTime& p : ctimes) {
        // NOTE possible improvement: remember iterator to previous nearest time
        plottimes_t::const_iterator it = diutil::nearest_element(ntimes, p, miTime::minDiff);
        if (it != ntimes.end() && abs(miTime::minDiff(p, *it)) <= timediff)
          tmptimes.insert(p); // time ok
      }
      ctimes = tmptimes;
    }

    first_time = false;
  }
}

void PlotModule::updateObs()
{
  // Update ObsPlots if data files have changed
  obsplots_->update(true, staticPlot_->getTime());

  // get annotations from all plots
  setAnnotations(); // obs data changed (data count included in annotation)
}

AreaObjectsCluster* PlotModule::areaobjects()
{
  if (!areaobjects_.get()) {
    areaobjects_.reset(new AreaObjectsCluster());
    areaobjects_->changeProjection(staticPlot_->getMapArea(), staticPlot_->getPlotSize(), staticPlot_->getPhysSize());
    areaobjects_->changeTime(staticPlot_->getTime());
  }
  return areaobjects_.get();
}

void PlotModule::putLocation(const LocationData& locationdata)
{
  locationplots_->putLocation(locationdata);
  setAnnotations();
}

void PlotModule::deleteLocation(const std::string& name)
{
  if (locationplots_->deleteLocation(name))
    setAnnotations();
}

void PlotModule::setSelectedLocation(const std::string& name, const std::string& elementname)
{
  locationplots_->setSelectedLocation(name, elementname);
}

std::string PlotModule::findLocation(int x, int y, const std::string& name)
{
  return locationplots_->findLocation(x, y, name);
}

//****************************************************

void PlotModule::trajPos(const std::vector<std::string>& vstr)
{
  if (trajectoryplots_->trajPos(vstr))
    setAnnotations();
}

void PlotModule::measurementsPos(const std::vector<std::string>& vstr)
{
  //if vstr starts with "quit", delete all MeasurementsPlot objects
  for (const std::string& s : vstr) {
    if (diutil::startswith(s, "quit")) {
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
  for (AnnotationPlot* eap : editVap)
    if (eap->markAnnotationPlot(x, y))
      marked = true;
  return marked;
}

std::string PlotModule::getMarkedAnnotation()
{
  std::string annotext;
  for (AnnotationPlot* eap : editVap) {
    std::string text = eap->getMarkedAnnotation();
    if (!text.empty())
      annotext = text;
  }
  return annotext;
}

void PlotModule::changeMarkedAnnotation(std::string text, int cursor, int sel1,
    int sel2)
{
  for (AnnotationPlot* eap : editVap)
    eap->changeMarkedAnnotation(text, cursor, sel1, sel2);
}

void PlotModule::DeleteMarkedAnnotation()
{
  for (AnnotationPlot* eap : editVap)
    eap->DeleteMarkedAnnotation();
}

void PlotModule::startEditAnnotation()
{
  for (AnnotationPlot* eap : editVap)
    eap->startEditAnnotation();
}

void PlotModule::stopEditAnnotation()
{
  for (AnnotationPlot* eap : editVap)
    eap->stopEditAnnotation();
}

void PlotModule::editNextAnnoElement()
{
  for (AnnotationPlot* eap : editVap)
    eap->editNextAnnoElement();
}

void PlotModule::editLastAnnoElement()
{
  for (AnnotationPlot* eap : editVap)
    eap->editLastAnnoElement();
}

PlotCommand_cpv PlotModule::writeAnnotations(const std::string& prodname)
{
  PlotCommand_cpv annoCommands;
  for (AnnotationPlot* eap : editVap) {
    if (PlotCommand_cp pc = eap->writeAnnotation(prodname))
      annoCommands.push_back(pc);
  }
  return annoCommands;
}

void PlotModule::updateEditLabels(const PlotCommand_cpv& productLabelCommands,
    const std::string& productName, bool newProduct)
{
  METLIBS_LOG_SCOPE();
  std::vector<AnnotationPlot*> oldVap; //display object labels
  //read the old labels...

  for (PlotCommand_cp pc : objm->getEditObjects().getObjectLabels())
    oldVap.push_back(new AnnotationPlot(pc));

  for (PlotCommand_cp pc : productLabelCommands) {
    AnnotationPlot* ap = new AnnotationPlot(pc);
    ap->setProductName(productName);

    std::vector< std::vector<std::string> > vvstr = ap->getAnnotationStrings();
    for (std::vector<std::string>& vstr : vvstr)
      fieldplots_->getDataAnnotations(vstr);
    ap->setAnnotationStrings(vvstr);

    // here we compare the labels, take input from oldVap
    for (AnnotationPlot* oap : oldVap)
      ap->updateInputLabels(oap, newProduct);

    editVap.push_back(ap);
  }

  diutil::delete_all_and_clear(oldVap);
}

void PlotModule::deleteAllEditAnnotations()
{
  diutil::delete_all_and_clear(editVap);
}

bool PlotModule::startTrajectoryComputation()
{
  METLIBS_LOG_SCOPE();
  if (!trajectoryplots_->hasTrajectories())
    return false;

  TrajectoryPlot* tp = trajectoryplots_->getPlot(0);
  const FieldPlot* fp = fieldplots()->findTrajectoryPlot(miutil::to_lower(tp->getFieldName()));
  if (!fp)
    return false;

  TrajectoryGenerator tg(fieldplotm, fp, staticPlot_->getTime());
  tg.setIterationCount(tp->getIterationCount());
  tg.setTimeStep(tp->getTimeStep());
  const TrajectoryGenerator::LonLat_v& pos = tp->getStartPositions();
  for (TrajectoryGenerator::LonLat_v::const_iterator itP = pos.begin(); itP != pos.end(); ++itP)
    tg.addPosition(*itP);

  const TrajectoryData_v trajectories = tg.compute();
  tp->setTrajectoryData(trajectories);

  return true;
}

void PlotModule::stopTrajectoryComputation()
{
}

// write trajectory positions to file
bool PlotModule::printTrajectoryPositions(const std::string& filename)
{
  if (trajectoryplots_->hasTrajectories())
    return trajectoryplots_->getPlot(0)->printTrajectoryPositions(filename);

  return false;
}

const Area& PlotModule::getMapArea()
{
  return staticPlot_->getMapArea();
}

const Rectangle& PlotModule::getPlotSize()
{
  return staticPlot_->getPlotSize();
}

const diutil::PointI& PlotModule::getPhysSize() const
{
  return staticPlot_->getPhysSize();
}
