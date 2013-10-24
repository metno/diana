/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#include "diVcrossManager.h"

#include "diController.h"
#include "diPlotOptions.h"
#include "diPrintOptions.h"
#include "diLocationPlot.h"
#include "diVcrossFile.h"
#include "diVcrossField.h"
#include "diVcrossOptions.h"
#include "diVcrossPlot.h"
#include "diVcrossSetup.h"
#include "diVcrossUtil.h"

#include <puTools/miSetupParser.h>

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm.hpp>

#define MILOGGER_CATEGORY "diana.VcrossManager"
#include <miLogger/miLogging.h>

namespace /* anonymous */ {

const std::string FILETYPE_VCFILE = "standard";
const std::string FILETYPE_GRIB   = "GribFile";

const std::string EMPTY_STRING;

} // namespace anonymous


VcrossManager::VcrossManager(Controller *co)
  : mOptions(new VcrossOptions())
  , mSetup(new VcrossSetup())
  , mPlot(new VcrossPlot(mOptions.get()))
  , fieldm(co->getFieldManager())
  , dataChange(true)
  , plotCrossection(-1)
  , timeGraphPos(-1)
  , plotTime(-1)
  , hardcopy(false)
{
  METLIBS_LOG_SCOPE();
  parseSetup();
}

VcrossManager::~VcrossManager()
{
  METLIBS_LOG_SCOPE();
  cleanup();
}

void VcrossManager::parseSetup()
{
  mSetup->parseSetup();
}

void VcrossManager::readOptions(const std::vector<std::string>& vstr)
{
  return mOptions->readOptions(vstr);
}

void VcrossManager::cleanup()
{
  METLIBS_LOG_SCOPE();

  plotCrossection = -1;
  plotTime = -1;
  timeGraphPos = -1;

  selected.clear();
  selectedLabel.clear();

  BOOST_FOREACH(VcrossFile*& f, boost::adaptors::values(vcfiles)) {
    delete f;
  }
  vcfiles.clear();

  BOOST_FOREACH(VcrossField*& f, boost::adaptors::values(vcfields)) {
    delete f;
  }
  vcfields.clear();
}

void VcrossManager::cleanupDynamicCrossSections()
{
  METLIBS_LOG_SCOPE();
  BOOST_FOREACH(VcrossField* f, boost::adaptors::values(vcfields)) {
    if (f)
      f->cleanup();
  }
}

void VcrossManager::setCrossection(const std::string& crossection)
{
  METLIBS_LOG_SCOPE();

  strings_t::const_iterator it = std::find(nameList.begin(), nameList.end(), crossection);
  if (it == nameList.end()) {
    METLIBS_LOG_WARN("crossection '" << crossection << "' not found");
    return;
  }
  
  plotCrossection = (it - nameList.begin());
  dataChange = true;
}

bool VcrossManager::setCrossection(float lat, float lon)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(lat) << LOGVAL(lon));

  if (vcfields.empty())
    return false;

  std::set<std::string> csnames(nameList.begin(), nameList.end());
  BOOST_FOREACH(VcrossField* f, boost::adaptors::values(vcfields)) {
    const std::string n = f->setLatLon(lat, lon);
    if (not n.empty())
      csnames.insert(n);
  }
  VcrossUtil::from_set(nameList, csnames);
  dataChange = true;
  return true;
}

void VcrossManager::setTime(const miutil::miTime& time)
{
  METLIBS_LOG_SCOPE();

  times_t::const_iterator it = std::find(timeList.begin(), timeList.end(), time);
  if (it == timeList.end()) {
    METLIBS_LOG_WARN("time " << time << " not found");
    return;
  }
  
  plotTime = (it - timeList.begin());
  dataChange = true;
}


std::string VcrossManager::setCrossection(int step)
{
  METLIBS_LOG_SCOPE();

  if (nameList.empty())
    return "";

  dataChange = VcrossUtil::step_index(plotCrossection, step, nameList.size());
  return currentCSName();
}

const std::string& VcrossManager::currentCSName() const
{
  if (plotCrossection >= 0 and plotCrossection < (int)nameList.size())
    return nameList.at(plotCrossection);
  else
    return EMPTY_STRING;
}

VcrossManager::vctime_t VcrossManager::setTime(int step)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(step));

  if (timeList.empty())
    return miutil::miTime::nowTime();

  dataChange = VcrossUtil::step_index(plotTime, step, timeList.size());
  return currentTime();
}

VcrossManager::vctime_t VcrossManager::currentTime() const
{
  if (plotTime >= 0 and plotTime < (int)timeList.size())
    return timeList.at(plotTime);
  else
    return vctime_t::nowTime();
}

void VcrossManager::startHardcopy(const printOptions& po)
{
  if (hardcopy && hardcopystarted) {
    // if hardcopy in progress and same filename: make new page
    if (po.fname == printoptions.fname){
      mPlot->nextPSpage();
      return;
    }
    // different filename: end current output and start a new
    mPlot->endPSoutput();
  }
  hardcopy = true;
  printoptions = po;
  hardcopystarted = false;
}

void VcrossManager::endHardcopy()
{
  if (hardcopy)
    mPlot->endPSoutput();
  hardcopy = false;
}

void VcrossManager::setPlotWindow(int w, int h)
{
  mPlot->viewSetWindow(w, h);
}

void VcrossManager::movePart(int pxmove, int pymove)
{
  mPlot->viewPan(pxmove, pymove);
}

void VcrossManager::decreasePart(int px1, int py1, int px2, int py2)
{
  mPlot->viewZoomIn(px1, py1, px2, py2);
}

void VcrossManager::increasePart()
{
  mPlot->viewZoomOut();
}

void VcrossManager::standardPart()
{
  mPlot->viewStandard();
}

// ------------------------------------------------------------------------

void VcrossManager::getPlotSize(float& x1, float& y1, float& x2, float& y2, Colour& rubberbandColour)
{
  return mPlot->getPlotSize(x1, y1, x2, y2, rubberbandColour);
}

// ------------------------------------------------------------------------

bool VcrossManager::plot()
{
  METLIBS_LOG_SCOPE();

  try {
  if (dataChange) {
    preparePlot();
    dataChange= false;
  }

  if (hardcopy && !hardcopystarted) {
    mPlot->startPSoutput(printoptions);
    hardcopystarted= true;
  }

  mPlot->plot();
  return true;
  } catch (std::exception& e) {
    METLIBS_LOG_ERROR(e.what());
  } catch (...) {
    METLIBS_LOG_ERROR("unknown exception");
  }
  return false;
}

// ------------------------------------------------------------------------

void VcrossManager::preparePlot()
{
  METLIBS_LOG_SCOPE();

  const std::string& cs = currentCSName();
  if (cs.empty()) {
    METLIBS_LOG_WARN("no crossection chosen");
    return;
  }
  
  mPlot->clear();
  if (selected.empty())
    return;

  VcrossData::Cut::lonlat_t csll;
  BOOST_FOREACH(VcrossSelected& select, selected) {
    VcrossSource* vcs = getVcrossSource(select.model);
    const VcrossSource::crossections_t& css = vcs->getCrossections();

    BOOST_FOREACH(const VcrossData::Cut& c, css) {
      if (c.name == cs) {
        csll = c.lonlat;
        break;
      }
    }
    if (not csll.empty())
      break;
  }
  if (csll.empty())
    return;

  mPlot->setHorizontalCross(cs, csll);
  if (timeGraphPos >= 0)
    mPlot->setHorizontalTime(timeList, timeGraphPos);

  const VcrossData::Parameters_t csPar = calculateCSParameters(csll);

  typedef std::map<std::string, std::set<std::string> > params4model_t;
  params4model_t params4model;

  BOOST_FOREACH(VcrossSelected& select, selected) {
    METLIBS_LOG_DEBUG(LOGVAL(select.model));
    const std::set<std::string>& parameterNames = mSetup->getPlotParameterNames(select.field);
    BOOST_FOREACH(const std::string& pn, parameterNames) {
      const std::string pid = mSetup->findParameterId(select.model, pn);
      METLIBS_LOG_DEBUG(LOGVAL(pn) << LOGVAL(pid));
      params4model[select.model].insert(pid);
    }
  }

  typedef std::map<std::string, VcrossData*> data4model_t;
  data4model_t data4model;

  const miutil::miTime ct = currentTime();

  BOOST_FOREACH(const params4model_t::value_type& mp, params4model) {
    VcrossSource* vcs = getVcrossSource(mp.first);
    if (not vcs) {
      METLIBS_LOG_ERROR("missing source for selected model '" << mp.first << "'");
      continue;
    }

    VcrossData* data = 0;
    if (timeGraphPos < 0) {
      // t = ct.addHour(select.hourOffset);
      data = vcs->getCrossData(cs, mp.second, ct);
    } else {
      data = vcs->getTimeData(cs, mp.second, timeGraphPos);
    }
    if (not data) {
      METLIBS_LOG_WARN("no data for model '" << mp.first << "'");
      continue;
    }

    // translate from param ids to param names
    VcrossData::Parameters_t parametersByName;
    BOOST_FOREACH(const VcrossData::Parameters_t::value_type& p, data->parameters) {
      parametersByName.insert(std::make_pair(mSetup->findParameterName(mp.first, p.first), p.second));
    }
    data->parameters = parametersByName;
    data4model[mp.first] = data;
  }

  bool haveZaxis = false;
  VcrossData::ZAxis::Quantity zQuantity = VcrossData::ZAxis::PRESSURE;
  BOOST_FOREACH(VcrossSelected& select, selected) {
    VcrossData* data = data4model[select.model];
    if (not data)
      continue;

    const std::vector<std::string>& arguments = mSetup->getPlotArguments(select.field);
    if (not addComputedValues(data, arguments, csPar))
      continue;

    bool goodZaxis = true;
    const std::string& arg0_name = arguments.front();
    const VcrossData::ParameterData& arg0 = data->parameters[arg0_name];
    if (not arg0.values) {
      METLIBS_LOG_WARN("no values for argument0 '" << arg0_name << "'");
      continue;
    }

    VcrossData::ZAxisPtr zax = arg0.zAxis;
    METLIBS_LOG_DEBUG(LOGVAL(arg0_name) << (zax ? " have zax" : " no zax") << LOGVAL(arg0.unit));
    
    for (size_t a=1; a<arguments.size(); ++a) {
      const VcrossData::ParameterData& arg = data->parameters[arguments[a]];
      if (not arg.values) {
        METLIBS_LOG_WARN("no values for argument '" << arguments[a] << "'");
        goodZaxis = false;
      }

      VcrossData::ZAxisPtr zax1 = arg.zAxis;
      METLIBS_LOG_DEBUG(LOGVAL(arguments[a]) << (zax1 ? " have zax1" : " no zax1"));
      goodZaxis &= (zax == zax1);
    }
    if (not goodZaxis) {
      METLIBS_LOG_WARN("bad zaxis for plot '" << select.field << "'");
      continue;
    }

    if (not zax) {
      METLIBS_LOG_DEBUG("1D data");
      // create "z axis" containing the parameter values, possibly converted to HEIGHT/PRESSURE 
      VcrossData::ZAxisPtr zaxc = boost::make_shared<VcrossData::ZAxis>();
      zaxc->quantity = zQuantity;
      zaxc->name = arg0_name + ":1d";
      zaxc->mPoints = arg0.mPoints;
      zaxc->mLevels = 1;
      zaxc->alloc();
      for (int p=0; p<zaxc->mPoints; ++p) {
        const float a = arg0.value(0, p);
        float c = a;
        if (zQuantity == VcrossData::ZAxis::PRESSURE and arg0.unit == "m")
          c = VcrossUtil::pressureFromHeight(a);
        else if (zQuantity == VcrossData::ZAxis::HEIGHT and arg0.unit == "hPa")
          c = VcrossUtil::heightFromPressure(a);
        else if (zQuantity == VcrossData::ZAxis::HEIGHT and arg0.unit == "Pa")
          c = VcrossUtil::heightFromPressure(a/100);
        else if (zQuantity == VcrossData::ZAxis::PRESSURE and arg0.unit == "Pa")
          c = a/100;
        zaxc->setValue(0, p, c);
      }
      zax = zaxc;
    } else if (haveZaxis and zax->quantity != zQuantity) {
      METLIBS_LOG_DEBUG("2D data, convert from " << zax->quantity << " to " << zQuantity);
      // convert z axis to zQuantity (HEIGHT/PRESSURE)
      VcrossData::ZAxisPtr zaxc = boost::make_shared<VcrossData::ZAxis>();
      zaxc->quantity = zQuantity;
      zaxc->name = zax->name + ":converted";
      zaxc->mPoints = zax->mPoints;
      zaxc->mLevels = zax->mLevels;
      METLIBS_LOG_DEBUG(LOGVAL(zax->mPoints) << LOGVAL(zax->mLevels));
      zaxc->alloc();
      for (int l=0; l<zaxc->mLevels; ++l) {
        for (int p=0; p<zaxc->mPoints; ++p) {
          const float z = zax->value(l, p);
          float c = z;
          if (zQuantity == VcrossData::ZAxis::PRESSURE and zax->quantity == VcrossData::ZAxis::HEIGHT)
            c = VcrossUtil::pressureFromHeight(z);
          else if (zQuantity == VcrossData::ZAxis::HEIGHT and zax->quantity == VcrossData::ZAxis::PRESSURE)
            c = VcrossUtil::heightFromPressure(z);
          zaxc->setValue(l, p, c);
        }
      }
      zax = zaxc;
    }
    METLIBS_LOG_DEBUG(LOGVAL(not not zax) << LOGVAL(haveZaxis));

    if (zax and not haveZaxis) {
      zQuantity = zax->quantity;
      haveZaxis = true;
    }

    // add plots
    VcrossData::values_t p0, p1;
    if (arguments.size() >= 1)
      p0 = data->parameters[arguments[0]].values;
    if (arguments.size() >= 2)
      p1 = data->parameters[arguments[1]].values;
    mPlot->addPlot(select.model, select.field, mSetup->getPlotType(select.field), p0, p1, zax, select.plotOptions);
  }

  METLIBS_LOG_DEBUG(LOGVAL(zQuantity) << LOGVAL(haveZaxis));
  if (haveZaxis) {
    mPlot->setVerticalAxis(zQuantity);
    mPlot->prepare();
  }
}

// ------------------------------------------------------------------------

VcrossData::Parameters_t VcrossManager::calculateCSParameters(const VcrossData::Cut::lonlat_t& cut)
{
  METLIBS_LOG_SCOPE();
  VcrossData::ParameterData pStep, pBearing, pCoriolis;
  pStep    .mPoints = cut.size();
  pBearing .mPoints = cut.size();
  pCoriolis.mPoints = cut.size();
  pStep    .mLevels = 1;
  pBearing .mLevels = 1;
  pCoriolis.mLevels = 1;
  pStep    .alloc();
  pBearing .alloc();
  pCoriolis.alloc();

  if (not cut.empty()) {
    pStep    .setValue(0, 0, 0);
    pCoriolis.setValue(0, 0, VcrossUtil::coriolisFactor(cut.front().lat()));
    if (cut.size() == 1) {
      pBearing.setValue(0, 0, 0);
    } else {
      const LonLat &p0 = cut.at(0), &p1 = cut.at(1);
      pBearing.setValue(0, 0, p0.bearingTo(p1));
    }
    for (size_t i=1; i<cut.size(); ++i) {
      const LonLat &p0 = cut.at(i-1), &p1 = cut.at(i);
      pStep    .setValue(0, i, p0.distanceTo(p1));
      pBearing .setValue(0, i, p0.bearingTo(p1));
      pCoriolis.setValue(0, i, VcrossUtil::coriolisFactor(p1.lat()));
    }
  }

  VcrossData::Parameters_t csPar;
  csPar.insert(std::make_pair("__STEP",     pStep));
  csPar.insert(std::make_pair("__BEARING",  pBearing));
  csPar.insert(std::make_pair("__CORIOLIS", pCoriolis));
  return csPar;
}

// ------------------------------------------------------------------------

bool VcrossManager::addComputedValues(VcrossData* data, const std::vector<std::string>& plotArguments,
    const VcrossData::Parameters_t& csPar)
{
  METLIBS_LOG_SCOPE();
  std::set<std::string> missingArgs;
  BOOST_FOREACH(const std::string& a, plotArguments) {
    if (data->parameters.find(a) == data->parameters.end())
      missingArgs.insert(a);
  }

  while (not missingArgs.empty()) {
    bool calculated1 = false;
    BOOST_FOREACH(const std::string& a, missingArgs) {
      if (mSetup->isFunction(a)) {
        // if function args present
        const std::vector<std::string>& fargs = mSetup->getFunctionArguments(a);
        std::vector<VcrossData::ParameterData> fargs_data;
        bool allFunctionAgrsPresent = true;
        BOOST_FOREACH(const std::string& a, fargs) {
          const VcrossData::Parameters_t::const_iterator it = data->parameters.find(a);
          if (it == data->parameters.end()) {
            allFunctionAgrsPresent = false;
            break;
          } else {
            fargs_data.push_back(it->second);
          }
        }
        if (allFunctionAgrsPresent) {
          const VcrossData::ParameterData fd = VcrossComputer::compute(csPar, mSetup->getFunctionType(a), fargs_data);
          if (not fd.values) {
            METLIBS_LOG_INFO("failed to compute values for '" << a << "'");
            return false;
          }
          if (not fd.zAxis)
            METLIBS_LOG_INFO("no z axis for '" << a << "'");
          data->parameters.insert(std::make_pair(a, fd));
          missingArgs.erase(a);
          calculated1 = true;
        }
      }
    }
    if (not calculated1)
      return false;
  }
  return true;
}

// ------------------------------------------------------------------------

std::vector<std::string> VcrossManager::getAllModels()
{
  METLIBS_LOG_SCOPE();
  return mSetup->getAllModelNames();
}

// ------------------------------------------------------------------------

std::map<std::string,std::string> VcrossManager::getAllFieldOptions()
{
  METLIBS_LOG_SCOPE();
  return mSetup->getAllPlotOptions();
}

// ------------------------------------------------------------------------

std::vector<std::string> VcrossManager::getFieldNames(const std::string& model)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(model));

  // make sure that we searched for a source for this model
  getVcrossSource(model);

  const filecontents_t::const_iterator fc_it = vcfilecontents.find(model);
  if (fc_it == vcfilecontents.end()) {
    METLIBS_LOG_WARN("no filecontents for model '" << model << "'");
    return std::vector<std::string>();
  }
  return fc_it->second.plotNames;
}

// ------------------------------------------------------------------------

VcrossField* VcrossManager::getVcrossField(const std::string& modelname)
{
  const vcfields_t::iterator it = vcfields.find(modelname);
  if (it != vcfields.end())
    return it->second;

  std::auto_ptr<VcrossField> vcf(new VcrossField(modelname, fieldm));
  if (not vcf->update())
    return 0;

  vcfields.insert(std::make_pair(modelname, vcf.get()));
  return vcf.release();
}

// ------------------------------------------------------------------------

VcrossFile* VcrossManager::getVcrossFile(const std::string& modelname)
{
  const vcfiles_t::iterator it = vcfiles.find(modelname);
  if (it != vcfiles.end())
    return it->second;

  const std::string& filename = mSetup->getModelFileName(modelname);
  std::auto_ptr<VcrossFile> vcf(new VcrossFile(filename, modelname));
  if (not vcf->update())
    vcf.reset(0);

  vcfiles.insert(std::make_pair(modelname, vcf.get()));
  return vcf.release();
}

// ------------------------------------------------------------------------

VcrossSource* VcrossManager::getVcrossSource(const std::string& modelname)
{
  METLIBS_LOG_SCOPE();

  const VcrossSetup::FileType ft = mSetup->getModelFileType(modelname);
  if (ft == VcrossSetup::VCUNKNOWN) {
    METLIBS_LOG_WARN("model '" << modelname << "' unknown");
    return 0;
  }

  VcrossSource* vcs = 0;
  if (ft == VcrossSetup::VCFILE) {
    vcs = getVcrossFile(modelname);
  } else if (ft == VcrossSetup::VCFIELD) {
    vcs = getVcrossField(modelname);
  }
  if (vcs) {
    if (vcfilecontents.find(modelname) == vcfilecontents.end()) {
      const std::vector<std::string> par_v = vcs->getParameterIds();
      const std::set<std::string> par_s(par_v.begin(), par_v.end());
      vcfilecontents.insert(std::make_pair(modelname, mSetup->makeContents(modelname, par_s)));
    }
  }
  return vcs;
}

// ------------------------------------------------------------------------

void VcrossManager::getCrossections(LocationData& locationdata)
{
  METLIBS_LOG_SCOPE();

  fillLocationData(locationdata);

  if (not nameList.empty()) {
    if (plotCrossection < 0 or plotCrossection >= (int)nameList.size())
      plotCrossection = 0;
  } else {
    plotCrossection = -1;
  }
}

// ------------------------------------------------------------------------

void VcrossManager::fillLocationData(LocationData& ld)
{
  METLIBS_LOG_SCOPE();

  Projection pgeo;
  pgeo.setGeographic();
  const Rectangle rgeo(0, 0, 90, 360);
  const Area geoArea(pgeo, rgeo);

  std::ostringstream annot;
  annot << "Vertikalsnitt";
  
  BOOST_FOREACH(VcrossSelected& select, selected) {
    METLIBS_LOG_DEBUG(LOGVAL(select.model));
    annot << ' ' << select.model;

    VcrossSource* vcs = getVcrossSource(select.model);
    if (not vcs)
      continue;

    const VcrossSource::crossections_t& cs = vcs->getCrossections();

    BOOST_FOREACH(const VcrossData::Cut& c, cs) {
      LocationElement el;
      el.name = c.name;
      BOOST_FOREACH(const LonLat& ll, c.lonlat) {
        el.xpos.push_back(ll.lonDeg());
        el.ypos.push_back(ll.latDeg());
      }
      ld.elements.push_back(el);
    }
  }

  ld.name =              "vcross";
  ld.locationType =      location_line;
  ld.area =              geoArea;
  ld.annotation =        annot.str();
  ld.colour =            mOptions->vcOnMapColour;
  ld.linetype =          mOptions->vcOnMapLinetype;
  ld.linewidth =         mOptions->vcOnMapLinewidth;
  ld.colourSelected =    mOptions->vcSelectedOnMapColour;
  ld.linetypeSelected =  mOptions->vcSelectedOnMapLinetype;
  ld.linewidthSelected = mOptions->vcSelectedOnMapLinewidth;
}

// ------------------------------------------------------------------------

void VcrossManager::mainWindowTimeChanged(const miutil::miTime& mwTime)
{
  METLIBS_LOG_SCOPE();
  setTimeToBestMatch(mwTime);
}

// ------------------------------------------------------------------------

void VcrossManager::setTimeToBestMatch(const vctime_t& time)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(time));
  
  if (timeList.empty()) {
    plotTime = -1;
    return;
  }

  int bestTime = 0;
  int bestDiff = std::abs(miutil::miTime::minDiff(timeList.front(), time));
  for (size_t i=1; i<timeList.size(); i++) {
    const int diff = std::abs(miutil::miTime::minDiff(timeList[i], time));
    if (diff < bestDiff) {
      bestDiff = diff;
      bestTime = i;
    }
  }
  plotTime = bestTime;
  dataChange = true;
}

// ------------------------------------------------------------------------

bool VcrossManager::setSelection(const std::vector<std::string>& vstr)
{
  METLIBS_LOG_SCOPE();

  // save plotStrings
  plotStrings = vstr;

  selected.clear();
  selectedLabel.clear();

  BOOST_FOREACH(const std::string& sel, vstr) {
    METLIBS_LOG_DEBUG(LOGVAL(sel));

    const std::vector<std::string> vs1 = miutil::split(sel, 0, " ");
    if (vs1.size()>1 && miutil::to_upper(vs1[1])=="LABEL") {
      selectedLabel.push_back(sel);
      continue;
    }
    
    VcrossSelected select;
    select.hourOffset = 0;
    select.plotShaded = false;
    std::string options;
    BOOST_FOREACH(const std::string& kvpair, vs1) {
      const std::vector<std::string> kv = miutil::split(kvpair, 0, "=");
      if (kv.size() == 2) {
        const std::string key = miutil::to_lower(kv[0]);
        const std::string& value = kv[1];
        if (key=="model") {
          select.model = value;
        } else if (key=="field") {
          select.field = value;
        } else if (key=="hour.offset") {
          select.hourOffset = miutil::to_int(value);
        } else {
          options += (kvpair + " ");
          if (key=="palettecolours") {
            if (value != "off")
              select.plotShaded = true;
          }
        }
      }
    }
    if ((not select.model.empty()) and (not select.field.empty()) and getVcrossSource(select.model)) {
      // there may be options not handled in dialog
      // or uncomplete batch input
      const std::string& defaultOptions = mSetup->getPlotOptions(select.field);
      if (not defaultOptions.empty())
        PlotOptions::parsePlotOption(defaultOptions, select.plotOptions);
      PlotOptions::parsePlotOption(options, select.plotOptions);
      METLIBS_LOG_DEBUG("added '" << select.model << "'" << LOGVAL(defaultOptions) << LOGVAL(options) << LOGVAL(select.plotOptions.density));
      selected.push_back(select);
    }
  }

  dataChange = true;
  return setModels();
}

// ------------------------------------------------------------------------

bool VcrossManager::setModels()
{
  METLIBS_LOG_SCOPE();

  const vctime_t timeBefore = currentTime(); // remember current time, search for it later
  const std::string csBefore = currentCSName(); // remember current cs' name, search for it later

  std::set<std::string> csnames;
  std::set<miutil::miTime> times;
  std::set<std::string> usedModels;

  BOOST_FOREACH(VcrossSelected& select, selected) {
    METLIBS_LOG_DEBUG(LOGVAL(select.model));
    VcrossSource* vcs = getVcrossSource(select.model);

    const VcrossSource::crossections_t& cs = vcs->getCrossections();
    BOOST_FOREACH(const VcrossData::Cut& c, cs) {
      csnames.insert(c.name);
    }

    const VcrossSource::times_t stimes = vcs->getTimes();
    VcrossUtil::set_insert(times, stimes);
    usedModels.insert(select.model);
  }
  VcrossUtil::from_set(nameList, csnames);
  VcrossUtil::from_set(timeList, times);

  if (usedModels.empty() or nameList.empty() or timeList.empty()) {
    METLIBS_LOG_DEBUG("no times or crossections");
    plotCrossection = -1;
    plotTime = -1;
    return not (nameList.empty() and timeList.empty());
  }

  setTimeToBestMatch(timeBefore);

  bool modelChange = false;
  if (plotCrossection < 0 // no previous cs
      or csnames.find(csBefore) == csnames.end()) // current cs' name no longer known
  {
    plotCrossection = 0;
    modelChange = true;
  }
  return modelChange;
}

// ------------------------------------------------------------------------

bool VcrossManager::timeGraphOK()
{
  METLIBS_LOG_SCOPE();
  return (not selected.empty());
}

// ------------------------------------------------------------------------

void VcrossManager::disableTimeGraph()
{
  METLIBS_LOG_SCOPE();
  timeGraphPos = -1;
  dataChange = true;
}

// ------------------------------------------------------------------------

void VcrossManager::setTimeGraphPos(int plotx, int ploty)
{
  METLIBS_LOG_SCOPE();

  if (selected.empty())
    return;
  
  // convert plotx to lonlat
  timeGraphPos = mPlot->getNearestPos(plotx);
  dataChange = true;
#if 0
  METLIBS_LOG_SCOPE();
  int n = 0;

  if (nxs >= 0) {
    float sx = (xWindowmax - xWindowmin) / float(plotw);
    float x = xWindowmin + float(px) * sx;
    float d2 = fieldUndef;

    for (int i = 0; i < nPoint; i++) {
      float dx = cdata1d[nxs][i] - x;
      if (d2 > dx * dx) {
        d2 = dx * dx;
        n = i;
      }
    }
  }

  return n;
#endif
}

// ------------------------------------------------------------------------

void VcrossManager::setTimeGraphPos(int incr)
{
  METLIBS_LOG_SCOPE();
  dataChange = VcrossUtil::step_index(timeGraphPos, incr, timeList.size());
}

// ------------------------------------------------------------------------

void VcrossManager::parseQuickMenuStrings(const std::vector<std::string>& vstr)
{
  std::vector<std::string> vcross_data, vcross_options;
  std::string crossection;

  BOOST_FOREACH(std::string line, vstr) { // copy because it has to be trimmed
    miutil::trim(line);
    if (line.empty())
      continue;

    const std::string upline = miutil::to_upper(line);

    if (miutil::contains(upline, "CROSSECTION=")) {
      const std::vector<std::string> vs = miutil::split(line, "=");
      crossection = vs[1];
      if (miutil::contains(crossection, "\""))
        miutil::remove(crossection, '\"');
    } else if (miutil::contains(upline, "VCROSS ")) {
      vcross_data.push_back(line);
    } else {
      // assume plot-options
      vcross_options.push_back(line);
    }
  }

  readOptions(vcross_options);
  setSelection(vcross_data);
  setCrossection(crossection);
}

// ------------------------------------------------------------------------

std::vector<std::string> VcrossManager::getQuickMenuStrings()
{
  std::vector<std::string> vstr;

  const std::vector<std::string> vstrOpt = getOptions()->writeOptions();
  const std::vector<std::string> vstrPlot = plotStrings;
  const std::string crossection = "CROSSECTION=" + getCrossection();

  vstr.push_back("VCROSS");
  vstr.insert(vstr.end(), vstrOpt.begin(),  vstrOpt.end());
  vstr.insert(vstr.end(), vstrPlot.begin(), vstrPlot.end());
  vstr.push_back(crossection);

  return vstr;
}

// ------------------------------------------------------------------------

std::vector<std::string> VcrossManager::writeLog()
{
  return mOptions->writeOptions();
}

// ------------------------------------------------------------------------

void VcrossManager::readLog(const std::vector<std::string>& vstr,
    const std::string& thisVersion, const std::string& logVersion)
{
  mOptions->readOptions(vstr);
}
