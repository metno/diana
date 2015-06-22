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

#include "diObsManager.h"
#include "diObsPlot.h"
#include "diSatManager.h"
#include "diStationManager.h"
#include "diObjectManager.h"
#include "diEditManager.h"
#include "diFieldPlot.h"
#include "diFieldPlotManager.h"
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

#include <diField/diFieldManager.h>
#include <puDatatypes/miCoordinates.h>
#include <puTools/miStringFunctions.h>

#include <QMouseEvent>

//#define DEBUGPRINT
//#define DEBUGREDRAW
#define MILOGGER_CATEGORY "diana.PlotModule"
#include <miLogger/miLogging.h>

using namespace miutil;
using namespace std;

namespace diutil {

void was_enabled::save(const Plot* plot, const std::string& key)
{
  key_enabled[key] = plot->isEnabled();
}

void was_enabled::restore(Plot* plot, const std::string& key) const
{
  key_enabled_t::const_iterator it = key_enabled.find(key);
  if (it != key_enabled.end())
    plot->setEnabled(it->second);
};

} // namespace diutil

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
  , hardcopy(false)
  , dorubberband(false)
  , keepcurrentarea(true)
  , obsnr(0)
{
  self = this;
  oldx = newx = oldy = newy = startx = starty = 0;
  mapDefinedByUser = false;
  mapDefinedByData = false;
  mapDefinedByView = false;

  // used to detect map area changes
  Projection p;
  Rectangle r(0., 0., 0., 0.);
  previousrequestedarea = Area(p, r);
  requestedarea = Area(p, r);
  staticPlot_->setRequestedarea(requestedarea);
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
  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it)
    it->second->setCanvas(canvas);
}

void PlotModule::preparePlots(const vector<string>& vpi)
{
  METLIBS_LOG_SCOPE();
  // reset flags
  mapDefinedByUser = false;

  // split up input into separate products
  vector<string> fieldpi, obspi, areapi, mappi, satpi, statpi, objectpi, trajectorypi,
  labelpi, editfieldpi;

  typedef std::map<std::string, std::vector<std::string> > manager_pi_t;
  manager_pi_t manager_pi;

  // merge PlotInfo's for same type
  for (size_t i = 0; i < vpi.size(); i++) {
    vector<string> tokens = miutil::split(vpi[i], 1);
    if (!tokens.empty()) {
      std::string type = miutil::to_upper(tokens[0]);
      METLIBS_LOG_INFO(vpi[i]);
      if (type == "FIELD")
        fieldpi.push_back(vpi[i]);
      else if (type == "OBS")
        obspi.push_back(vpi[i]);
      else if (type == "MAP")
        mappi.push_back(vpi[i]);
      else if (type == "AREA")
        areapi.push_back(vpi[i]);
      else if (type == "SAT")
        satpi.push_back(vpi[i]);
      else if (type == "STATION")
        statpi.push_back(vpi[i]);
      else if (type == "OBJECTS")
        objectpi.push_back(vpi[i]);
      else if (type == "TRAJECTORY")
        trajectorypi.push_back(vpi[i]);
      else if (type == "LABEL")
        labelpi.push_back(vpi[i]);
      else if (type == "EDITFIELD")
        editfieldpi.push_back(vpi[i]);
      else if (managers.find(type) != managers.end())
        manager_pi[type].push_back(vpi[i]);
      else
        METLIBS_LOG_WARN("unknown type '" << type << "'");
    }
  }

  // call prepare methods
  prepareArea(areapi);
  prepareMap(mappi);
  prepareFields(fieldpi);
  prepareObs(obspi);
  satm->prepareSat(satpi);
  prepareStations(statpi);
  objm->prepareObjects(objectpi);
  prepareTrajectory(trajectorypi);
  prepareAnnotation(labelpi);

  if (editm->isInEdit() and not editfieldpi.empty()) {
    std::string plotName;
    vector<FieldRequest> vfieldrequest;
    fieldplotm->parsePin(editfieldpi[0],vfieldrequest,plotName);
    editm->prepareEditFields(plotName,editfieldpi);
  }

  // Send the commands to the other managers.
  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it) {
    Manager *manager = it->second;

    // send any plot commands to the relevant manager.
    const manager_pi_t::const_iterator itp = manager_pi.find(it->first);
    if (itp != manager_pi.end())
      manager->processInput(itp->second);
    else
      manager->processInput(vector<std::string>());
  }
}

void PlotModule::prepareArea(const vector<string>& inp)
{
  METLIBS_LOG_SCOPE();

  MapManager mapm;

  if (!inp.size())
    return;
  if (inp.size() > 1)
    METLIBS_LOG_DEBUG("More AREA definitions, using: " <<inp[0]);

  const std::string key_name=  "name";
  const std::string key_proj=  "proj4string";
  const std::string key_rectangle=  "rectangle";

  Projection proj;
  Rectangle rect;

  const vector<std::string> tokens= miutil::split_protected(inp[0], '"','"'," ",true);
  for (size_t i=0; i<tokens.size(); i++){
    const vector<std::string> stokens= miutil::split(tokens[i], 1, "=");
    if (stokens.size() > 1) {
      const std::string key= miutil::to_lower(stokens[0]);

      if (key==key_name) {
        if ( !mapm.getMapAreaByName(stokens[1], requestedarea) ) {
          METLIBS_LOG_WARN("Unknown AREA definition: "<< inp[0]);
        }

      } else if (key==key_proj){
        if ( proj.set_proj_definition(stokens[1]) ) {
          requestedarea.setP(proj);
        } else {
          METLIBS_LOG_WARN("Unknown proj definition: "<< stokens[1]);
        }
      } else if (key==key_rectangle){
        if ( rect.setRectangle(stokens[1],false) ) {
          requestedarea.setR(rect);
        } else {
          METLIBS_LOG_WARN("Unknown rectangle definition: "<< stokens[1]);
        }
      }
    }
  }
  // check area
  mapDefinedByUser = requestedarea.P().isDefined();
  staticPlot_->setRequestedarea(requestedarea);
}

void PlotModule::prepareMap(const vector<string>& inp)
{
  METLIBS_LOG_SCOPE();

  // init inuse array
  const size_t nm = vmp.size();
  vector<bool> inuse(nm, false);


  std::vector<MapPlot*> new_vmp; // new vector of map plots

  for (size_t k = 0; k < inp.size(); k++) { // loop through all plotinfo's
    bool isok = false;
    for (size_t j = 0; j < nm; j++) {
      if (!inuse[j]) { // not already taken
        if (vmp[j]->prepare(inp[k], true)) {
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
    MapPlot *mp = new MapPlot();
    if (!mp->prepare(inp[k], false)) {
      delete mp;
    } else {
      mp->setCanvas(mCanvas);
      new_vmp.push_back(mp);
    }
  } // end plotinfo loop

  // delete unwanted mapplots
  for (size_t i = 0; i < nm; i++) {
    if (!inuse[i])
      delete vmp[i];
  }
  vmp = new_vmp;

}

void PlotModule::prepareFields(const vector<string>& inp)
{
  METLIBS_LOG_SCOPE();

  diutil::was_enabled plotenabled;

  // for now -- erase all fieldplots
  for (unsigned int i = 0; i < vfp.size(); i++) {
    FieldPlot* fp = vfp[i];
    plotenabled.save(fp, fp->getPlotInfo("model,plot,parameter,reftime"));
    // free old fields
    freeFields(fp);
    delete fp;
  }
  vfp.clear();

  // NOTE: If we use the fieldCache, we must clear it here
  // to avoid memory consumption!
  if (inp.empty()) {
    // No fields will be used any more...
    fieldm->flushCache();
    return;
  }

  for (size_t i=0; i < inp.size(); i++) {
    FieldPlot *fp = new FieldPlot();

    std::string plotName;
    vector<FieldRequest> vfieldrequest;
    std::string inpstr = inp[i];
    fieldplotm->parsePin(inpstr,vfieldrequest,plotName);
    if (!fp->prepare(plotName, inp[i])) {
      delete fp;
    } else {
      plotenabled.restore(fp, fp->getPlotInfo("model,plot,parameter,reftime"));
      vfp.push_back(fp);
      fp->setCanvas(mCanvas);
    }
  }
}

void PlotModule::prepareObs(const vector<string>& inp)
{
  METLIBS_LOG_SCOPE();

  diutil::was_enabled plotenabled;
  for (unsigned int i = 0; i < vop.size(); i++)
    plotenabled.save(vop[i], vop[i]->getPlotInfo(3));

  // for now -- erase all obsplots etc..
  //first log stations plotted
  for (size_t i = 0; i < vop.size(); i++)
    vop[i]->logStations();
  vop.clear();
  for (size_t i = 0; i < vobsTimes.size(); i++)
    diutil::delete_all_and_clear(vobsTimes[i].vobsOneTime);
  vobsTimes.clear();

  for (size_t i = 0; i < inp.size(); i++) {
    ObsPlot *op = obsm->createObsPlot(inp[i]);
    if (op) {
      plotenabled.restore(op, op->getPlotInfo(3));
      op->setCanvas(mCanvas);

      if (vobsTimes.empty()) {
        obsOneTime ot;
        vobsTimes.push_back(ot);
      }

      vobsTimes[0].vobsOneTime.push_back(op);
      // vobsTimes[0].vobsOneTime[n] = vop[i];//forsiktig!!!!
      //     FIXME alexanderb vop[i] may be undefined; assuming that vop[i] == op anyhow, therefore commented out

      vop.push_back(op);
    }
  }
  obsnr = 0;
}

void PlotModule::prepareStations(const vector<string>& inp)
{
  METLIBS_LOG_SCOPE();

  if (!stam->init(inp)) {
    METLIBS_LOG_DEBUG("init returned false");
  }
}

void PlotModule::prepareAnnotation(const vector<string>& inp)
{
  METLIBS_LOG_SCOPE();

  // for now -- erase all annotationplots
  diutil::delete_all_and_clear(vap);
  annotationStrings = inp;
}

void PlotModule::prepareTrajectory(const vector<string>& inp)
{
  METLIBS_LOG_SCOPE();
  // vtp.push_back(new TrajectoryPlot());
}

vector<PlotElement> PlotModule::getPlotElements()
{
  //  METLIBS_LOG_SCOPE();
  std::vector<PlotElement> pel;

  // get field names
  for (size_t j = 0; j < vfp.size(); j++) {
    std::string str = vfp[j]->getPlotName() + "# " + miutil::from_number(int(j));
    bool enabled = vfp[j]->isEnabled();
    pel.push_back(PlotElement("FIELD", str, "FIELD", enabled));
  }

  // get obs names
  for (size_t j = 0; j < vop.size(); j++) {
    std::string str = vop[j]->getPlotName() + "# " + miutil::from_number(int(j));
    bool enabled = vop[j]->isEnabled();
    pel.push_back(PlotElement("OBS", str, "OBS", enabled));
  }

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
  const std::vector<StationPlot*> stam_plots(stam->plots());
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

  // get area objects names
  for (size_t j = 0; j < vareaobjects.size(); j++) {
    std::string str = vareaobjects[j].getName();
    if (not str.empty()) {
      str += "# " + miutil::from_number(int(j));
      bool enabled = vareaobjects[j].isEnabled();
      std::string icon = vareaobjects[j].getIcon();
      pel.push_back(PlotElement("AREAOBJECTS", str, icon, enabled));
    }
  }

  // get locationPlot annotations
  for (size_t j = 0; j < locationPlots.size(); j++) {
    std::string str = locationPlots[j]->getPlotName();
    if (not str.empty()) {
      str += "# " + miutil::from_number(int(j));
      bool enabled = locationPlots[j]->isEnabled();
      pel.push_back(PlotElement("LOCATION", str, "LOCATION", enabled));
    }
  }

  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it) {
    const std::vector<PlotElement> pe = it->second->getPlotElements();
    pel.insert(pel.end(), pe.begin(), pe.end());
  }

  return pel;
}

void PlotModule::enablePlotElement(const PlotElement& pe)
{
  if (pe.type == "FIELD") {
    for (unsigned int i = 0; i < vfp.size(); i++) {
      std::string str = vfp[i]->getPlotName() + "# " + miutil::from_number(int(i));
      if (str == pe.str) {
        vfp[i]->setEnabled(pe.enabled);
        break;
      }
    }
  } else if (pe.type == "RASTER") {
    satm->enablePlotElement(pe);
  } else if (pe.type == "OBS") {
    for (unsigned int i = 0; i < vop.size(); i++) {
      std::string str = vop[i]->getPlotName() + "# " + miutil::from_number(int(i));
      if (str == pe.str) {
        vop[i]->setEnabled(pe.enabled);
        break;
      }
    }
  } else if (pe.type == "OBJECTS") {
    objm->enablePlotElement(pe);
  } else if (pe.type == "TRAJECTORY") {
    for (unsigned int i = 0; i < vtp.size(); i++) {
      std::string str = vtp[i]->getPlotName() + "# " + miutil::from_number(int(i));
      if (str == pe.str) {
        vtp[i]->setEnabled(pe.enabled);
        break;
      }
    }
  } else if (pe.type == "STATION") {
    const std::vector<StationPlot*> stam_plots(stam->plots());
    for (size_t i = 0; i < stam_plots.size(); i++) {
      StationPlot* sp = stam_plots[i];
      std::string str = sp->getPlotName() + "# " + miutil::from_number(int(i));
      if (str == pe.str) {
        sp->setEnabled(pe.enabled);
        break;
      }
    }
  } else if (pe.type == "AREAOBJECTS") {
    for (size_t i = 0; i < vareaobjects.size(); i++) {
      std::string str = vareaobjects[i].getName();
      if (not str.empty()) {
        str += "# " + miutil::from_number(int(i));
        if (str == pe.str) {
          vareaobjects[i].enable(pe.enabled);
          break;
        }
      }
    }
  } else if (pe.type == "LOCATION") {
    for (unsigned int i = 0; i < locationPlots.size(); i++) {
      std::string str = locationPlots[i]->getPlotName() + "# " + miutil::from_number(int(i));
      if (str == pe.str) {
        locationPlots[i]->setEnabled(pe.enabled);
        break;
      }
    }
  } else {
    // unknown
    return;
  }

  // get annotations from all plots
  setAnnotations();
}

void PlotModule::setAnnotations()
{
  METLIBS_LOG_SCOPE();

  diutil::delete_all_and_clear(vap);

  for (size_t i = 0; i < annotationStrings.size(); i++) {
    AnnotationPlot* ap= new AnnotationPlot();
    // Dont add an invalid object to vector
    if (!ap->prepare(annotationStrings[i]))
      delete ap;
    else
      vap.push_back(ap);
  }

  //Annotations from setup, qmenu, etc.

  // set annotation-data
  Colour col;
  std::string str;
  vector<AnnotationPlot::Annotation> annotations;
  AnnotationPlot::Annotation ann;

  vector<miTime> fieldAnalysisTime;

  // get field annotations
  for (size_t j = 0; j < vfp.size(); j++) {
    fieldAnalysisTime.push_back(vfp[j]->getAnalysisTime());

    if (!vfp[j]->isEnabled())
      continue;
    vfp[j]->getFieldAnnotation(str, col);
    ann.str = str;
    ann.col = col;
    annotations.push_back(ann);
  }

  satm->addSatAnnotations(annotations);

  { // get obj annotations
    objm->getObjAnnotation(ann.str, ann.col);
    if (not ann.str.empty())
      annotations.push_back(ann);
  }
  // get obs annotations
  for (size_t j = 0; j < vop.size(); j++) {
    if (!vop[j]->isEnabled())
      continue;
    vop[j]->getObsAnnotation(str, col);
    ann.str = str;
    ann.col = col;
    annotations.push_back(ann);
  }

  // get trajectory annotations
  for (size_t j = 0; j < vtp.size(); j++) {
    if (!vtp[j]->isEnabled())
      continue;
    vtp[j]->getTrajectoryAnnotation(str, col);
    // empty string if data plot is off
    if (not str.empty()) {
      ann.str = str;
      ann.col = col;
      annotations.push_back(ann);
    }
  }

  // get stationPlot annotations
  const std::vector<StationPlot*> stam_plots(stam->plots());
  for (size_t j = 0; j < stam_plots.size(); j++) {
    StationPlot* sp = stam_plots[j];
    if (!sp->isEnabled())
      continue;
    sp->getStationPlotAnnotation(str, col);
    ann.str = str;
    ann.col = col;
    annotations.push_back(ann);
  }

  // get locationPlot annotations
  for (size_t j = 0; j < locationPlots.size(); j++) {
    if (!locationPlots[j]->isEnabled())
      continue;
    locationPlots[j]->getAnnotation(str, col);
    ann.str = str;
    ann.col = col;
    annotations.push_back(ann);
  }

  // Miscellaneous managers
  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it) {
    // Obtain the annotations for enabled managers.
    if (it->second->isEnabled()) {
      vector<string> plotAnnotations = it->second->getAnnotations();
      vector<string>::const_iterator iti;
      for (iti = plotAnnotations.begin(); iti != plotAnnotations.end(); ++iti) {
        ann.str = *iti;
        ann.col = Colour(0, 0, 0);
        annotations.push_back(ann);
      }
    }
  }

  for (size_t i = 0; i < vap.size(); i++) {
    vap[i]->setData(annotations, fieldAnalysisTime);
    vap[i]->setfillcolour(staticPlot_->getBackgroundColour());
  }

  //annotations from data

  //get field and sat annotations
  for (size_t i = 0; i < vap.size(); i++) {
    vector<vector<string> > vvstr = vap[i]->getAnnotationStrings();
    for (size_t k = 0; k < vvstr.size(); k++) {
      for (size_t j = 0; j < vfp.size(); j++) {
        vfp[j]->getDataAnnotations(vvstr[k]);
      }
      for (size_t j = 0; j < vop.size(); j++) {
        vop[j]->getDataAnnotations(vvstr[k]);
      }
      satm->getSatAnnotations(vvstr[k]);
      editm->getAnnotations(vvstr[k]);
      objm->getObjectsAnnotations(vvstr[k]);
    }
    vap[i]->setAnnotationStrings(vvstr);
  }

  //get obs annotations
  for (size_t i = 0; i < vop.size(); i++) {
    if (!vop[i]->isEnabled())
      continue;
    vector<std::string> obsinfo = vop[i]->getObsExtraAnnotations();
    for (size_t j = 0; j < obsinfo.size(); j++) {
      AnnotationPlot* ap = new AnnotationPlot(obsinfo[j]);
      vap.push_back(ap);
    }
  }

  //objects
  vector<std::string> objLabelstring = objm->getObjectLabels();
  for (size_t i = 0; i < objLabelstring.size(); i++) {
    AnnotationPlot* ap = new AnnotationPlot(objLabelstring[i]);
    vap.push_back(ap);
  }
}

bool PlotModule::updatePlots()
{
  METLIBS_LOG_SCOPE();

  const miTime& t = staticPlot_->getTime();

  bool nodata = vmp.empty(); // false when data are found

  // prepare data for field plots
  bool haveFieldData = false;
  for (size_t i = 0; i < vfp.size(); i++) {
    std::string pin;
    if (vfp[i]->updateNeeded(pin)) {
      std::vector<Field*> fv;
      if (fieldplotm->makeFields(pin, t, fv))
        haveFieldData = true;
      freeFields(vfp[i]);
      vfp[i]->setData(fv, t);
    }
  }
  if (haveFieldData) {
    nodata = false;
    // level for vertical level observations "as field"
    staticPlot_->setVerticalLevel(vfp.back()->getLevel());
  }

  // prepare data for satellite plots
  if (satm->setData())
    nodata = false;
  else
    METLIBS_LOG_DEBUG("SatManager returned false from setData");

  // set maparea from map spec., sat or fields

  defineMapArea();

  // prepare data for observation plots
  obsnr = 0;
  for (size_t i = 0; i < vop.size(); i++) {
    vop[i]->logStations();
    vop[i] = vobsTimes[0].vobsOneTime[i];
  }
  for (; vobsTimes.size() > 1; vobsTimes.pop_back())
    diutil::delete_all_and_clear(vobsTimes.back().vobsOneTime);
  for (size_t i = 0; i < vop.size(); i++) {
    if (!obsm->prepare(vop[i], t)) {
      METLIBS_LOG_DEBUG("ObsManager returned false from prepare");
    } else {
      nodata = false;
    }
  }

  //update list of positions ( used in "PPPP-mslp")
  obsm->updateObsPositions(vop);

  // prepare met-objects
  if (objm->prepareObjects(t, staticPlot_->getMapArea()))
    nodata = false;

  // prepare item stored in miscellaneous managers
  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it) {
    // If the preparation fails then return false to indicate an error.
    if (it->second->isEnabled() && !it->second->prepare(t))
      nodata = false;
  }

  // prepare editobjects (projection etc.)
  objm->changeProjection(staticPlot_->getMapArea());

  // this is called in plotUnder:
  // vareaobjects[i].changeProjection(staticPlot_->getMapArea());

  const std::vector<StationPlot*> stam_plots(stam->plots());
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
      newMapArea = requestedarea;
    } else if( getMapArea().P() != requestedarea.P() // or user just selected new area
        || previousrequestedarea.R() != requestedarea.R())
    {
      newMapArea = staticPlot_->findBestMatch(requestedarea);
    }
    mapdefined = true;

  } else if (keepcurrentarea && previousrequestedarea != requestedarea) {
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

  if (!mapdefined && vfp.size() > 0) {
    // set area equal to first EXISTING field-area ("all timesteps"...)
    const int n = vfp.size();
    int i = 0;
    while (i < n && !vfp[i]->getRealFieldArea(newMapArea))
      i++;
    if (i < n) {
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

  previousrequestedarea = requestedarea;
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

  // plot map-elements for lowest zorder
  for (size_t i = 0; i < vmp.size(); i++)
    vmp[i]->plot(gl, Plot::BACKGROUND);

  // plot satellite images
  satm->plot(gl, Plot::SHADE_BACKGROUND);

  // mark undefined areas/values in field (before map)
  for (size_t i = 0; i < vfp.size(); i++)
    vfp[i]->plot(gl, Plot::SHADE_BACKGROUND);

  // plot fields (shaded fields etc. before map)
  for (size_t i = 0; i < vfp.size(); i++)
    vfp[i]->plot(gl, Plot::SHADE);

  // plot map-elements for auto zorder
  for (size_t i = 0; i < vmp.size(); i++)
    vmp[i]->plot(gl, Plot::LINES_BACKGROUND);

  // plot locationPlots (vcross,...)
  for (size_t i = 0; i < locationPlots.size(); i++)
    locationPlots[i]->plot(gl, Plot::LINES);

  // plot fields (isolines, vectors etc. after map)
  for (size_t i = 0; i < vfp.size(); i++) {
    vfp[i]->plot(gl, Plot::LINES);
  }

  // next line also calls objects.changeProjection
  objm->plotObjects(gl, Plot::LINES);

  for (size_t i = 0; i < vareaobjects.size(); i++) {
    vareaobjects[i].changeProjection(staticPlot_->getMapArea());
    vareaobjects[i].plot(gl, Plot::LINES);
  }

  // plot station plots
  const std::vector<StationPlot*> stam_plots(stam->plots());
  for (size_t j = 0; j < stam_plots.size(); j++)
    stam_plots[j]->plot(gl, Plot::LINES);

  // plot inactive edit fields/objects under observations
  if (editm->isInEdit()) {
    editm->plot(gl, Plot::LINES);
  }

  // plot other objects, including drawing items
  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it) {
    if (it->second->isEnabled()) {
      it->second->changeProjection(staticPlot_->getMapArea());
      it->second->plot(gl, Plot::LINES);
    }
  }

  // plot observations (if in fieldEditMode  and the option obs_mslp is true, plot observations in overlay)
  if (! (editm->isInEdit()
          && (editm->getMapMode() == fedit_mode
              || editm->getMapMode() == combine_mode)
          && obsm->obs_mslp()))
  {
    ObsPlot::clearPos();
    for (size_t i = 0; i < vop.size(); i++)
      vop[i]->plot(gl, Plot::LINES);
  }

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
  if (editm->isInEdit()) {

    editm->plot(gl, Plot::OVERLAY);

    // if PPPP-mslp, calc. values and plot observations,
    // in overlay while changing the field
    if (obsm->obs_mslp() && (editm->getMapMode() == fedit_mode || editm->getMapMode() == combine_mode)) {
      if (editm->obs_mslp(obsm->getObsPositions())) {
        obsm->calc_obs_mslp(gl, Plot::OVERLAY, vop);
      }
    }

    // Annotations
    if (showanno) {
      for (size_t i = 0; i < vap.size(); i++)
        vap[i]->plot(gl, Plot::OVERLAY);
    }
    for (size_t i = 0; i < editVap.size(); i++)
      editVap[i]->plot(gl, Plot::OVERLAY);

  } // if editm->isInEdit()

  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it) {
    if (it->second->isEnabled()) {
      it->second->changeProjection(staticPlot_->getMapArea());
      it->second->plot(gl, Plot::OVERLAY);
    }
  }

  // plot map-elements for highest zorder
  for (size_t i = 0; i < vmp.size(); i++)
    vmp[i]->plot(gl, Plot::OVERLAY);

  // frame (not needed if maprect==fullrect)
  if (hardcopy || staticPlot_->getMapSize() != staticPlot_->getPlotSize()) {
    gl->ShadeModel(DiGLPainter::gl_FLAT);
    gl->setLineStyle(Colour(0, 0, 0), 1.0);
    const Rectangle mr = diutil::adjustedRectangle(staticPlot_->getMapSize(), -0.0001, -0.0001);
    gl->drawRect(mr.x1, mr.y1, mr.x2, mr.y2);
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
    gl->drawRect(pold.x(), pold.y(), pnew.x(), pnew.y());
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

  const float waspr = staticPlot_->getPhysWidth() / staticPlot_->getPhysHeight();
  const Rectangle mr = diutil::fixedAspectRatio(mapr, waspr, true);
  staticPlot_->setMapSize(mr);

  // update full plot area -- add border
  const float border = 0.0;
  const Rectangle fr = diutil::adjustedRectangle(mr, border, border);
  staticPlot_->setPlotSize(fr);
}

void PlotModule::setPlotWindow(const int& w, const int& h)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE(LOGVAL(w) << LOGVAL(h));
#endif

  staticPlot_->setPhysSize(w, h);

  PlotAreaSetup();
}

void PlotModule::freeFields(FieldPlot* fp)
{
  fieldplotm->freeFields(fp->getFields());
  fp->clearFields();
}

void PlotModule::cleanup()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  diutil::delete_all_and_clear(vmp);

  // Field deletion at the end is done in the cache. The cache destructor is called by
  // FieldPlotManagers destructor, which comes before this destructor. Basically we try to
  // destroy something in a dead pointer here....
  for (size_t i = 0; i < vfp.size(); i++) {
    freeFields(vfp[i]);
    delete vfp[i];
  }
  vfp.clear();

  satm->clear();

  std::vector<StationPlot*> stam_plots(stam->plots());
  diutil::delete_all_and_clear(stam_plots); // FIXME this does not clear anything in StationManager

  for (; not vobsTimes.empty(); vobsTimes.pop_back())
    diutil::delete_all_and_clear(vobsTimes.back().vobsOneTime);
  vop.clear();

  objm->clearObjects();

  diutil::delete_all_and_clear(vtp);

  diutil::delete_all_and_clear(vMeasurementsPlot);

  annotationStrings.clear();

  diutil::delete_all_and_clear(vap);
}

Rectangle PlotModule::getPhysRectangle() const
{
  const float pw = staticPlot_->getPhysWidth(), ph = staticPlot_->getPhysHeight();
  return Rectangle(0, 0, pw, ph);
}

void PlotModule::callManagersChangeProjection()
{
  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it)
    it->second->changeProjection(staticPlot_->getMapArea());
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

  if (vfp.size()>0) {
    if (staticPlot_->getMapArea().P() == vfp[0]->getFieldArea().P()) {
      vector<Field*> ff = vfp[0]->getFields();
      if (ff.size()>0) {
        gridx = xmap/ff[0]->area.resolutionX;
        gridy = ymap/ff[0]->area.resolutionY;
        return true;
      }
    }
  }

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
void PlotModule::setManagers(FieldManager* fm, FieldPlotManager* fpm,
    ObsManager* om, SatManager* sm, StationManager* stm, ObjectManager* obm, EditManager* edm)
{
  fieldm = fm;
  fieldplotm = fpm;
  obsm = om;
  satm = sm;
  stam = stm;
  objm = obm;
  editm = edm;

  if (!fieldm)
    METLIBS_LOG_ERROR("PlotModule::ERROR fieldmanager==0");
  if (!fieldplotm)
    METLIBS_LOG_ERROR("PlotModule::ERROR fieldplotmanager==0");
  if (!obsm)
    METLIBS_LOG_ERROR("PlotModule::ERROR obsmanager==0");
  if (!satm)
    METLIBS_LOG_ERROR("PlotModule::ERROR satmanager==0");
  if (!stam)
    METLIBS_LOG_ERROR("PlotModule::ERROR stationmanager==0");
  if (!objm)
    METLIBS_LOG_ERROR("PlotModule::ERROR objectmanager==0");
  if (!editm)
    METLIBS_LOG_ERROR("PlotModule::ERROR editmanager==0");
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
void PlotModule::getPlotTime(std::string& s)
{
  const miTime& t = staticPlot_->getTime();

  s = t.isoTime();
}

void PlotModule::getPlotTime(miTime& t)
{
  t = staticPlot_->getTime();
}

void PlotModule::getPlotTimes(map<string,vector<miutil::miTime> >& times,
    bool updateSources)
{
  times.clear();

  { // edit product proper time
    miutil::miTime pt;;
    if (editm->getProductTime(pt))
      times["products"].push_back(pt);
  }

  vector<std::string> pinfos;
  for (size_t i = 0; i < vfp.size(); i++) {
    pinfos.push_back(vfp[i]->getPlotInfo());
    METLIBS_LOG_DEBUG("Field plotinfo:" << vfp[i]->getPlotInfo());
  }
  if (pinfos.size() > 0) {
    times["fields"] = fieldplotm->getFieldTime(pinfos, updateSources);
  }

  { std::vector<miTime> sattimes = satm->getSatTimes();
  if (not sattimes.empty())
    times["satellites"] = sattimes;
  }

  pinfos.clear();
  for (size_t i = 0; i < vop.size(); i++)
    pinfos.push_back(vop[i]->getPlotInfo());
  if (pinfos.size() > 0) {
    times["observations"] = obsm->getObsTimes(pinfos);
  }

  times["objects"] = objm->getObjectTimes();

  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it)
    times[it->first] = it->second->getTimes();
}

//returns union or intersection of plot times from all pinfos
void PlotModule::getCapabilitiesTime(set<miTime>& okTimes, const vector<std::string>& pinfos,
    bool allTimes, bool updateSources)
{
  vector<miTime> normalTimes;
  int timediff;
  bool normalTimesFound = false;
  bool moreTimes = true;
  for (size_t i = 0; i < pinfos.size(); i++) {
    vector<std::string> tokens = miutil::split(pinfos[i], 1);
    if (!tokens.empty()) {
      std::string type = miutil::to_upper(tokens[0]);
      if (type == "FIELD")
        fieldplotm->getCapabilitiesTime(normalTimes, timediff, pinfos[i], updateSources);
      else if (type == "SAT")
        satm->getCapabilitiesTime(normalTimes, timediff, pinfos[i]);
      else if (type == "OBS")
        obsm->getCapabilitiesTime(normalTimes, timediff, pinfos[i]);
      else if (type == "OBJECTS")
        objm->getCapabilitiesTime(normalTimes, timediff, pinfos[i]);
    }

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
bool PlotModule::setPlotTime(miTime& t)
{
  staticPlot_->setTime(t);
  return true;
}

void PlotModule::updateObs()
{
  // Update ObsPlots if data files have changed

  //delete vobsTimes
  for (size_t i = 0; i < vop.size(); i++)
    vop[i] = vobsTimes[0].vobsOneTime[i];

  for (; vobsTimes.size() > 1; vobsTimes.pop_back())
    diutil::delete_all_and_clear(vobsTimes.back().vobsOneTime);
  obsnr = 0;

  // if time of current vop[0] != splot.getTime() or  files have changed,
  // read files from disk
  for (size_t i = 0; i < vop.size(); i++) {
    if (vop[i]->updateObs()) {
      if (!obsm->prepare(vop[i], staticPlot_->getTime()))
        METLIBS_LOG_WARN("ObsManager returned false from prepare");
    }
  }

  //update list of positions ( used in "PPPP-mslp")
  obsm->updateObsPositions(vop);

  // get annotations from all plots
  setAnnotations();
}

bool PlotModule::findObs(int x, int y)
{
  bool found = false;

  for (size_t i = 0; i < vop.size(); i++)
    if (vop[i]->showpos_findObs(x, y))
      found = true;

  return found;
}

bool PlotModule::getObsName(int x, int y, std::string& name)
{
  for (size_t i = 0; i < vop.size(); i++)
    if (vop[i]->getObsName(x, y, name))
      return true;

  return false;
}
std::string PlotModule::getObsPopupText(int x, int y)
{
  size_t n = vop.size();
  std::string obsText = "";
 
  for (size_t i = 0; i < n; i++)
    if (vop[i]->getObsPopupText(x, y, obsText))
      return obsText;

  return obsText;
}


//areas
void PlotModule::makeAreas(std::string name, std::string areastring, int id)
{
  //   METLIBS_LOG_DEBUG("makeAreas:"<<n);
  //   METLIBS_LOG_DEBUG("name:"<<name);
  //   METLIBS_LOG_DEBUG("areastring:"<<areastring);
  //name can be name:icon
  vector<std::string> tokens = miutil::split(name, ":");
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
    it->makeAreas(name, icon, areastring, id, staticPlot_->getMapArea());
    return;
  }

  //make new dataset
  AreaObjects new_areaobjects;
  new_areaobjects.makeAreas(name, icon, areastring, id, staticPlot_->getMapArea());
  vareaobjects.push_back(new_areaobjects);
}

void PlotModule::areaCommand(const std::string& command, const std::string& dataSet,
    const std::string& data, int id)
{
  //   METLIBS_LOG_DEBUG("PlotModule::areaCommand");
  //   METLIBS_LOG_DEBUG("id=" << id);
  //   METLIBS_LOG_DEBUG("command=" << command);
  //   METLIBS_LOG_DEBUG("data="<<data);

  int n = vareaobjects.size();
  for (int i = 0; i < n && i > -1; i++) {
    if ((id == -1 || id == vareaobjects[i].getId()) && (dataSet == "all"
        || dataSet == vareaobjects[i].getName())) {
      if (command == "delete" && (data == "all" || data.empty())) {
        vareaobjects.erase(vareaobjects.begin() + i);
        i--;
        n = vareaobjects.size();
      } else {
        vareaobjects[i].areaCommand(command, data);
        //zoom to selected area
        if (command == "select" && vareaobjects[i].autoZoom()) {
          vector<std::string> token = miutil::split(data, ":");
          if (token.size() == 2 && token[1] == "on") {
            setMapAreaFromMap(vareaobjects[i].getBoundBox(token[0]));
          }
        }
      }
    }
  }
}

vector<selectArea> PlotModule::findAreas(int x, int y, bool newArea)
{
  //METLIBS_LOG_DEBUG("PlotModule::findAreas"  << x << " " << y);
  float xm = 0, ym = 0;
  PhysToMap(x, y, xm, ym);
  vector<selectArea> vsA;
  int n = vareaobjects.size();
  for (int i = 0; i < n; i++) {
    if (!vareaobjects[i].isEnabled())
      continue;
    vector<selectArea> sub_vsA;
    sub_vsA = vareaobjects[i].findAreas(xm, ym, newArea);
    vsA.insert(vsA.end(), sub_vsA.begin(), sub_vsA.end());
  }
  return vsA;
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

void PlotModule::nextObs(bool next)
{
  for (size_t i = 0; i < vop.size(); i++)
    vop[i]->nextObs(next);
}

void PlotModule::obsTime(bool forward, EventResult& res)
{
  // This function changes the observation time one hour,
  // and leaves the rest (fields, images etc.) unchanged.
  // It saves the obsPlot object in the vector vobsTimes.
  // This only works for vop[0], which is the only one used at the moment.
  // This only works in edit modus
  // vobsTimes is deleted when anything else are changed or edit modus are left

  if (vop.empty())
    return;
  if (!editm->isInEdit())
    return;

  if (forward) {
    if (obsnr > 20)
      return;
    obsnr++;
  } else { // backward
    if (obsnr == 0)
      return;
    obsnr--;
  }

  obsm->clearObsPositions();
  miTime newTime = staticPlot_->getTime();
  newTime.addHour(-1 * obsTimeStep * obsnr);

  //log old stations
  for (size_t i = 0; i < vop.size(); i++)
    vop[i]->logStations();

  //Make new obsPlot object
  if (obsnr == int(vobsTimes.size())) {

    obsOneTime ot;
    for (size_t i = 0; i < vop.size(); i++) {
      const std::string& pin = vop[i]->getInfoStr();
      ObsPlot *op = obsm->createObsPlot(pin);
      if (op) {
        if (!obsm->prepare(op, newTime))
          METLIBS_LOG_WARN("ObsManager returned false from prepare");
      }
      ot.vobsOneTime.push_back(op);
    }
    vobsTimes.push_back(ot);

  } else {
    for (size_t i = 0; i < vop.size(); i++)
      vobsTimes[obsnr].vobsOneTime[i]->readStations();
  }

  //ask last plot object which stations was plotted,
  //and tell this plot object
  for (size_t i = 0; i < vop.size(); i++) {
    vop[i] = vobsTimes[obsnr].vobsOneTime[i];
  }

  //update list of positions ( used in "PPPP-mslp")
  obsm->updateObsPositions(vop);

  std::string labelstr;
  if (obsnr != 0) {
    std::string timer = miutil::from_number(obsnr * obsTimeStep);
    labelstr = "LABEL text=\"OBS -" + timer;
    labelstr += "\" tcolour=black bcolour=red fcolour=red:150 ";
    labelstr += "polystyle=both halign=center valign=top fontsize=18";
  }
  if (vop.size() > 0) {
    vop[0]->setLabel(labelstr);
  }

  setAnnotations();
}

void PlotModule::obsStepChanged(int step)
{
  obsTimeStep = step;

  int n = vop.size();
  for (int i = 0; i < n; i++)
    vop[i] = vobsTimes[0].vobsOneTime[i];

  int m = vobsTimes.size();
  for (int i = m - 1; i > 0; i--) {
    int l = vobsTimes[i].vobsOneTime.size();
    for (int j = 0; j < l; j++) {
      delete vobsTimes[i].vobsOneTime[j];
      vobsTimes[i].vobsOneTime.pop_back();
    }
    vobsTimes[i].vobsOneTime.clear();
    vobsTimes.pop_back();
  }

  if (obsnr > 0)
    obsnr = 0;

  setAnnotations();
}

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

vector<string> PlotModule::writeAnnotations(const string& prodname)
{
  vector<std::string> annostrings;
  for (size_t j = 0; j < editVap.size(); j++) {
    string str = editVap[j]->writeAnnotation(prodname);
    if (not str.empty())
      annostrings.push_back(str);
  }
  return annostrings;
}

void PlotModule::updateEditLabels(const vector<std::string>& productLabelstrings,
    const std::string& productName, bool newProduct)
{
  METLIBS_LOG_SCOPE();
  vector<AnnotationPlot*> oldVap; //display object labels
  //read the old labels...

  vector<std::string> objLabelstrings = objm->getEditObjects().getObjectLabels();
  for (size_t i = 0; i < objLabelstrings.size(); i++) {
    AnnotationPlot* ap = new AnnotationPlot(objLabelstrings[i]);
    oldVap.push_back(ap);
  }

  for (size_t j = 0; j < productLabelstrings.size(); j++) {
    AnnotationPlot* ap = new AnnotationPlot(productLabelstrings[j]);
    ap->setProductName(productName);

    vector<vector<string> > vvstr = ap->getAnnotationStrings();
    for (size_t k = 0; k < vvstr.size(); k++) {
      for (size_t j = 0; j < vfp.size(); j++)
        vfp[j]->getDataAnnotations(vvstr[k]);
    }
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
      res.savebackground = true;
      res.background = true;
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
      res.background = false;
      res.repaint = true;
      return;

    } else if (staticPlot_->isPanning()) {
      const float dx = oldx - me->x(), dy = oldy - me->y();
      setMapAreaFromPhys(diutil::translatedRectangle(getPhysRectangle(), dx, dy));
      oldx = me->x();
      oldy = me->y();

      res.action = quick_browsing;
      res.background = true;
      res.repaint = true;
      res.newcursor = paint_move_cursor;
      return;
    }

  }
  // ** mouserelease
  else if (me->type() == QEvent::MouseButtonRelease) {

    bool plotnew = false;

    res.savebackground = false;

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
      res.repaint = true;
      res.background = true;
      return;
    }
    if (plotnew) {
      //define new plotarea, first save the old one
      areaInsert(true);
      setMapAreaFromPhys(Rectangle(x1, y1, x2, y2));

      res.repaint = true;
      res.background = true;
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
    dx = -staticPlot_->getPhysWidth() / 8;
  else if (anav == ANAV_PAN_RIGHT)
    dx = staticPlot_->getPhysWidth() / 8;
  else if (anav == ANAV_PAN_DOWN)
    dy = -staticPlot_->getPhysHeight() / 8;
  else if (anav == ANAV_PAN_UP)
    dy = staticPlot_->getPhysHeight() / 8;
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

  res.repaint = true;
  res.background = true;
}

vector<std::string> PlotModule::getFieldModels()
{
  std::set<std::string> unique;
  for (size_t i = 0; i < vfp.size(); i++) {
    const std::string& fname = vfp[i]->getModelName();
    if (not fname.empty())
      unique.insert(fname);
  }
  return std::vector<std::string>(unique.begin(), unique.end());
}

vector<std::string> PlotModule::getTrajectoryFields()
{
  vector<std::string> vstr;
  for (size_t i = 0; i < vfp.size(); i++) {
    std::string fname = vfp[i]->getTrajectoryFieldName();
    if (not fname.empty())
      vstr.push_back(fname);
  }
  return vstr;
}

const FieldPlot* PlotModule::findTrajectoryPlot(const std::string& fieldname)
{
  for (size_t i = 0; i < vfp.size(); ++i) {
    if (miutil::to_lower(vfp[i]->getTrajectoryFieldName()) == fieldname)
      return vfp[i];
  }
  return 0;
}

bool PlotModule::startTrajectoryComputation()
{
  METLIBS_LOG_SCOPE();
  if (vtp.size() < 1)
    return false;

  const FieldPlot* fp = findTrajectoryPlot(miutil::to_lower(vtp[0]->getFieldName()));
  if (!fp)
    return false;

  TrajectoryGenerator tg(fieldplotm, fp, staticPlot_->getTime());
  tg.setIterationCount(vtp[0]->getIterationCount());
  tg.setTimeStep(vtp[0]->getTimeStep());
  const TrajectoryGenerator::LonLat_v& pos = vtp[0]->getStartPositions();
  for (size_t i=0; i<pos.size(); ++i)
    tg.addPosition(pos.at(i));

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

// Miscellaneous get methods
const vector<FieldPlot*>& PlotModule::getFieldPlots() const
{
  return vfp;
}

const vector<ObsPlot*>& PlotModule::getObsPlots() const
{
  return vop;
}

void PlotModule::getPlotWindow(int &width, int &height)
{
  width = staticPlot_->getPhysWidth();
  height = staticPlot_->getPhysHeight();
}
