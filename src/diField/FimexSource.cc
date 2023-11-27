/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2014-2022 met.no

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

#include "FimexSource.h"

#include "DataReshape.h"
#include "VcrossUtil.h"

#include "../diUtilities.h"
#include "../util/charsets.h"
#include "../util/diUnitsConverter.h"
#include "../util/string_util.h"

#include <mi_fieldcalc/math_util.h>

#include <puCtools/puCglob.h>

#include <puTools/miStringBuilder.h>
#include <puTools/miDirtools.h>
#include <puTools/TimeFilter.h>

#include <fimex/CDM.h>
#include <fimex/CDMException.h>
#include <fimex/CDMExtractor.h>
#include <fimex/CDMFileReaderFactory.h>
#include <fimex/CDMInterpolator.h>
#include <fimex/CDMReader.h>
#include <fimex/CoordinateSystemSliceBuilder.h>
#include <fimex/Data.h>
#include <fimex/UnitsConverter.h>
#include <fimex/coordSys/CoordinateSystem.h>

#if MIFI_VERSION_CURRENT_INT >= MIFI_VERSION_INT(0, 64, 0)
#define FIMEX_HAS_VERTICALCONVERTER 1
#endif

#ifdef FIMEX_HAS_VERTICALCONVERTER
#include <fimex/coordSys/verticalTransform/VerticalConverter.h>
#include <fimex/coordSys/verticalTransform/VerticalTransformationUtils.h>
#else // !FIMEX_HAS_VERTICALCONVERTER
#include <fimex/coordSys/verticalTransform/ToVLevelConverter.h>
#endif // !FIMEX_HAS_VERTICALCONVERTER

#include <sstream>

#define MILOGGER_CATEGORY "vcross.FimexSource"
#include "miLogger/miLogging.h"

using namespace MetNoFimex;

namespace /* anonymous */ {

#define THROW(ex, text)                                                 \
  do {                                                                  \
    std::string msg = (miutil::StringBuilder() << text).str();          \
    METLIBS_LOG_DEBUG("throwing exception: " << msg);                   \
    throw ex(msg);                                                      \
  } while(false)

const char STANDARD_NAME[] = "standard_name";
const char TIME[] = "time";
const char FORECAST_REFERENCE_TIME[] = "forecast_reference_time";
const char LONGITUDE[] = "longitude";
const char LATITUDE[]  = "latitude";
const char COORD_Y[]  = "y";

const char WAVE_FREQ[] = "freq";
const char WAVE_DIRECTION[]  = "direction";

const char VCROSS_NAME[]   = "vcross_name";
const char VCROSS_BOUNDS[] = "bounds";

bool isPositiveUp(std::string zAxisName, const CDM& cdm)
{
  CDMAttribute attr;
  bool positive = true;
  if (cdm.getAttribute(zAxisName, "positive", attr))
    positive = (attr.getStringValue() == "up");
  return positive;
}

template<class F>
struct is_near {
  F value, tolerance;
  is_near(F v, F t = 1e-6)
    : value(v), tolerance(t) { }
  bool operator()(F v) const
    { return std::abs(v-value) < tolerance; }
};

bool isHttpUrl(const std::string& pattern)
{
  return diutil::startswith(pattern, "http://") || diutil::startswith(pattern, "https://");
}

int findTimeIndex(vcross::Inventory_p inv, const vcross::Time& time)
{
  METLIBS_LOG_SCOPE(LOGVAL(time.unit) << LOGVAL(time.value));
  if (not time.valid())
    THROW(std::runtime_error, "time not valid");

  const vcross::Times& times = inv->times;
  vcross::Time::timevalue_t t = time.value;
  if (!diutil::unitsIdentical(time.unit, times.unit)) {
    auto uconv = diutil::unitConverter(time.unit, times.unit);
    if (not uconv) {
      METLIBS_LOG_DEBUG("unit mismatch: '" << time.unit << "' -- '" << times.unit << "'");
      return -1;
    }
    t = uconv->convert1(time.value);
    METLIBS_LOG_DEBUG(LOGVAL(t));
  }
  const vcross::Times::timevalue_v::const_iterator itT
      = std::find_if(times.values.begin(), times.values.end(), is_near<vcross::Time::timevalue_t>(t));
  if (itT == times.values.end())
    THROW(std::runtime_error, "time not found");

  const int tIdx = (itT - times.values.begin());
  METLIBS_LOG_DEBUG(LOGVAL(tIdx));
  return tIdx;
}

int verticalTypeFromId(const std::string& id)
{
  const size_t doubleslash = id.find("//");
  if (doubleslash == std::string::npos)
    return -1;

  const std::string verticalTypeText = id.substr(doubleslash+2);
  if (verticalTypeText == "altitude")
    return MIFI_VINT_ALTITUDE;
  else if (verticalTypeText == "height")
    return MIFI_VINT_HEIGHT;
  if (verticalTypeText == "depth")
    return MIFI_VINT_DEPTH;
  else if (verticalTypeText == "pressure")
    return MIFI_VINT_PRESSURE;
  else
    return -1;
}

std::string transformedZAxisName(const std::string& id)
{
  const size_t doubleslash = id.find("//");
  if (doubleslash == std::string::npos)
    return id;
  return id.substr(0, doubleslash);
}

diutil::Values::Shape shapeFromCDM(const CDM& cdm, const std::string& vName)
{
  METLIBS_LOG_SCOPE(LOGVAL(vName));
  diutil::Values::Shape shape;
  if (not cdm.hasVariable(vName) and cdm.hasDimension(vName)) {
    const size_t length = cdm.getDimension(vName).getLength();
    shape.add(vName, length);
  } else {
    const diutil::Values::Shape::string_v& shapeCdm = cdm.getVariable(vName).getShape();
    for (diutil::Values::Shape::string_v::const_iterator it = shapeCdm.begin(); it != shapeCdm.end(); ++it) {
      const size_t length = cdm.getDimension(*it).getLength();
      shape.add(*it, length);
      METLIBS_LOG_DEBUG(LOGVAL(*it) << LOGVAL(length));
    }
  }
  return shape;
}

void addAxisToShape(diutil::Values::Shape& shape, const CDM& cdm, CoordinateAxis_cp ax)
{
  if (ax)
    shape.add(ax->getName(), cdm.getDimension(ax->getName()).getLength());
}

diutil::Values::Shape shapeFromCS(const CDM& cdm, CoordinateSystem_cp cs)
{
  METLIBS_LOG_SCOPE();

  diutil::Values::Shape shape;
  addAxisToShape(shape, cdm, cs->getGeoXAxis());
  addAxisToShape(shape, cdm, cs->getGeoYAxis());
  addAxisToShape(shape, cdm, cs->getGeoZAxis());
  addAxisToShape(shape, cdm, cs->getTimeAxis());
  return shape;
}

diutil::Values::Shape shapeFromCDM(const CDM& cdm, CoordinateSystem_cp cs, vcross::InventoryBase_cp v)
{
  METLIBS_LOG_SCOPE(LOGVAL(v->id()));
  if (cs and verticalTypeFromId(v->id()) >= 0)
    return shapeFromCS(cdm, cs);
  return shapeFromCDM(cdm, v->id());
}

CoordinateSystem_cp findGeoZTransformed(const CoordinateSystem_cp_v& coordinateSystems,
    const std::string& ztid, const std::string& csid)
{
  METLIBS_LOG_SCOPE(LOGVAL(ztid) << LOGVAL(csid));
  for (CoordinateSystem_cp cs : coordinateSystems) {
    if (not (cs->isSimpleSpatialGridded()))
      continue;
    if (cs->id() == csid) {
      return cs;
    }

    CoordinateAxis_cp zAxis = cs->getGeoZAxis();
    if (not zAxis or zAxis->getName() != ztid)
      continue;
    if (!cs->getGeoXAxis() || !cs->getTimeAxis())
      continue;

    if (MetNoFimex::VerticalTransformation_cp vt = cs->getVerticalTransformation())
      return cs;
  }
  return CoordinateSystem_cp();
}

CoordinateSystem_cp findCsForVariable(const CDM& cdm,
    const CoordinateSystem_cp_v& coordinateSystems, const std::string& vName)
{
  METLIBS_LOG_SCOPE(LOGVAL(vName));
  const std::vector<CoordinateSystem_cp>::const_iterator it =
      std::find_if(coordinateSystems.begin(), coordinateSystems.end(),
          MetNoFimex::CompleteCoordinateSystemForComparator(vName));
  if (it == coordinateSystems.end()) {
    if (cdm.hasDimension(vName)) {
      METLIBS_LOG_DEBUG("has dimension '" << vName << "'");
      return CoordinateSystem_cp();
    }
    THROW(std::runtime_error, "no cs found for '" << vName << "'");
  }
  if (not *it or not (*it)->isSimpleSpatialGridded())
    THROW(std::runtime_error, "cs for '" << vName << "' does not exist or is not a simple spatial grid");

  return *it;
}

CoordinateAxis_cp findAxisOfType(CoordinateSystem_cp cs, CoordinateAxis::AxisType axisType)
{
  if (axisType == CoordinateAxis::GeoX)
    return cs->getGeoXAxis();
  else if (axisType == CoordinateAxis::GeoY)
    return cs->getGeoYAxis();
  else if (axisType == CoordinateAxis::GeoZ)
    return cs->getGeoZAxis();
  else if (axisType == CoordinateAxis::Time)
    return cs->getTimeAxis();
  // if asking for GeoZ, this would only return z axis, but not altitude or pressure even if altitude or pressure are the z axis
  return cs->findAxisOfType(axisType);
}

int findShapeIndex(CoordinateAxis::AxisType type, CoordinateSystem_cp cs, const diutil::Values::Shape& shapeCdm)
{
  if (not cs)
    return -1;
  CoordinateAxis_cp axisCdm = findAxisOfType(cs, type);
  if (not axisCdm)
    return -1;
  return shapeCdm.position(axisCdm->getName());
}

diutil::Values::Shape translateAxisNames(const diutil::Values::Shape& shapeReq, const CDM& cdm, CoordinateSystem_cp cs)
{
  const int N_AXES = 5;
  const CoordinateAxis::AxisType axesCDM[N_AXES] = { CoordinateAxis::GeoX,  CoordinateAxis::GeoY,  CoordinateAxis::GeoZ,  CoordinateAxis::Time, CoordinateAxis::Realization };
  const char* axesV[N_AXES]                      = { diutil::Values::GEO_X, diutil::Values::GEO_Y, diutil::Values::GEO_Z, diutil::Values::TIME, diutil::Values::REALIZATION };

  diutil::Values::Shape shape;
  for (size_t p = 0; p < shapeReq.rank(); ++p) {
    std::string name = shapeReq.name(p);
    size_t length    = shapeReq.length(p);
    const int i = std::find(axesV, axesV + N_AXES, name) - axesV;
    if (i != N_AXES) {
      // standard axis, replace name
      CoordinateAxis_cp axisCdm = findAxisOfType(cs, axesCDM[i]);
      if (axisCdm) {
        name = axisCdm->getName();
      } else if (length != 1) {
        THROW(std::runtime_error, "no axis for '" << name << "' with non-1 length");
      }
    }
    shape.add(name, length);
  }
  return shape;
}

std::string standardNameOrName(const CDM& cdm, const std::string& vName)
{
  CDMAttribute aStandardName;
  if (cdm.getAttribute(vName, STANDARD_NAME, aStandardName))
    return aStandardName.getStringValue();
  else
    return vName;
}

typedef std::vector<std::string> string_v;

string_v findVariablesByStandardNameOrName(const CDM& cdm, const std::string& sName)
{
  string_v variables = cdm.findVariables(STANDARD_NAME, sName);
  if (variables.empty() and cdm.hasVariable(sName))
    variables.push_back(sName);
  return variables;
}

std::string findVariableByStandardNameOrName(const CDM& cdm, const std::string& sName)
{
  string_v variables = findVariablesByStandardNameOrName(cdm, sName);
  if (variables.size() == 1)
    return variables.front();
  else
    return std::string();
}

vcross::LonLat_v makeCrossectionPoints(const MetNoFimex::shared_array<float> vLon, const MetNoFimex::shared_array<float> vLat,
    size_t lola_begin, size_t lola_end)
{
  vcross::LonLat_v points;
  points.reserve(lola_end - lola_begin);
  for (size_t i=lola_begin; i<lola_end; ++i)
    points.push_back(LonLat::fromDegrees(vLon[i], vLat[i]));
  return points;
}

vcross::LonLat_v makePointsRequested(const vcross::LonLat_v& points)
{
  vcross::LonLat_v pointsRequested;
  const size_t n = points.size();
  if (n >= 1)
    pointsRequested.push_back(points.front());
  if (n >= 2)
    pointsRequested.push_back(points.back());
  return pointsRequested;
}

} // namespace anonymous

// ========================================================================

namespace vcross {

FimexReftimeSource::FimexReftimeSource(const std::string& filename, const std::string& filetype, const std::string& fileconfig,
                                       diutil::CharsetConverter_p csNameCharsetConverter, const Time& reftime)
    : mFileName(filename)
    , mFileType(filetype)
    , mFileConfig(fileconfig)
    , mCsNameCharsetConverter(csNameCharsetConverter)
    , mModificationTime(0)
    , mSupportsDynamic(false)
{
  METLIBS_LOG_SCOPE(LOGVAL(filename) << LOGVAL(filetype) << LOGVAL(fileconfig));

  mReferenceTime = reftime;
  update();
}

FimexReftimeSource::~FimexReftimeSource()
{
}

ReftimeSource::Update_t FimexReftimeSource::update()
{
  METLIBS_LOG_SCOPE();

  Update_t u = UNCHANGED;

  if (isHttpUrl(mFileName)) {
    // TODO implement modification time from HEAD
  } else {
    const long modificationTime = miutil::path_ctime(mFileName);
    METLIBS_LOG_DEBUG(LOGVAL(modificationTime));

    if (modificationTime == 0)
      u = GONE;
    else if (modificationTime != mModificationTime)
      u = CHANGED;
    mModificationTime = modificationTime;
  }

  if (u != UNCHANGED) {
    mReader = CDMReader_p();
    mInventory = Inventory_p();
  }

  if (!mReferenceTime.valid()) {
    if (mReader || makeReader()) {
      const CDM& cdm = mReader->getCDM();
      std::string reftime_var = findVariableByStandardNameOrName(cdm, FORECAST_REFERENCE_TIME);
      if (reftime_var.empty())
        reftime_var = findVariableByStandardNameOrName(cdm, TIME);
      METLIBS_LOG_DEBUG(LOGVAL(reftime_var));
      if (!reftime_var.empty()) {
        METLIBS_LOG_DEBUG("using variable '" << reftime_var << "' to extract reference time");
        if (DataPtr rtData = mReader->getScaledData(reftime_var)) {
          if (rtData->size() > 0) {
            if (MetNoFimex::shared_array<Time::timevalue_t> rtValues = rtData->asDouble()) {
              mReferenceTime = Time(mReader->getCDM().getUnits(reftime_var), rtValues[0]);
              METLIBS_LOG_DEBUG(LOGVAL(util::to_miTime(mReferenceTime)));
            }
          }
        }
      } else if (makeInventory() && mInventory->times.npoint() > 0) {
        mReferenceTime = mInventory->times.at(0);
        METLIBS_LOG_WARN("using first time " << util::to_miTime(mReferenceTime) << " as reference time for vcross file '" << mFileName << "'");
      }
    }
  }

  return u;
}

Inventory_cp FimexReftimeSource::getInventory()
{
  if (not mInventory)
    makeInventory();
  return mInventory;
}

CoordinateSystem_cp FimexReftimeSource::findCsForVariable(const CDM& cdm,
    const CoordinateSystem_cp_v& coordinateSystems, vcross::InventoryBase_cp v)
{
  METLIBS_LOG_SCOPE(LOGVAL(v->id()) << LOGVAL(v->dataType()));
  CoordinateSystem_cp cs;
  const int verticalType = verticalTypeFromId(v->id());
  if (verticalType >= 0) {
    const std::string& ztid = transformedZAxisName(v->id());
    zaxis_cs_m::const_iterator itCsId = zaxis_cs.find(ztid);
    if (itCsId != zaxis_cs.end()) {
      cs = findGeoZTransformed(coordinateSystems, ztid, itCsId->second);
    }
  }
  if (not cs)
    cs = ::findCsForVariable(cdm, coordinateSystems, v->id());
  return cs;
}


void FimexReftimeSource::prepareGetValues(Crossection_cp cs,
    FimexCrossection_cp& fcs, CDMReader_p& reader, CoordinateSystem_cp_v& coordinateSystems)
{
  METLIBS_LOG_TIME();
  if (not mInventory and not makeInventory())
    THROW(std::runtime_error, "no inventory");

  fcs = std::dynamic_pointer_cast<const FimexCrossection>(cs);
  if (not fcs)
    THROW(std::runtime_error, "no crossection given, or not a fimex crossection");

  reader = makeReader(fcs);
  if (reader == mReader)
    coordinateSystems = mCoordinateSystems; // FIXME avoid copy
  else
    coordinateSystems = MetNoFimex::listCoordinateSystems(reader);
}

void FimexReftimeSource::getCrossectionValues(Crossection_cp crossection, const Time& time,
    const InventoryBase_cps& data, name2value_t& n2v, int realization)
{
  METLIBS_LOG_TIME();
  FimexCrossection_cp fcs;
  CDMReader_p reader;
  CoordinateSystem_cp_v coordinateSystems;
  prepareGetValues(crossection, fcs, reader, coordinateSystems);
  const CDM& cdm = reader->getCDM();

  const size_t t_start = findTimeIndex(mInventory, time);

  for (InventoryBase_cp b : data) {
    METLIBS_LOG_DEBUG(LOGVAL(b->id()) << LOGVAL(b->nlevel()));
    try {
      CoordinateSystem_cp cs = findCsForVariable(cdm, coordinateSystems, b);
      diutil::Values::Shape shapeCdm = shapeFromCDM(cdm, cs, b);
      diutil::Values::ShapeSlice sliceCdm(shapeCdm);
      sliceCdm.cut(findShapeIndex(CoordinateAxis::GeoX, cs, shapeCdm), fcs->start_index, fcs->length()) // cut on x axis
          .    cut(findShapeIndex(CoordinateAxis::Time, cs, shapeCdm), t_start, 1)                      // single time
          .    cut(findShapeIndex(CoordinateAxis::Realization, cs, shapeCdm), realization, 1);          // single point on ensemble_member axis

      diutil::Values::Shape shapeOut(diutil::Values::GEO_X, fcs->length(), diutil::Values::GEO_Z, b->nlevel());

      if (auto v = getSlicedValues(reader, cs, sliceCdm, shapeOut, b))
        n2v[b->id()] = v;
    } catch (std::exception& ex) {
      METLIBS_LOG_WARN("exception: " << ex.what());
    }
  }
}

void FimexReftimeSource::getTimegraphValues(Crossection_cp crossection,
    size_t crossection_index, const InventoryBase_cps& data, name2value_t& n2v, int realization)
{
  METLIBS_LOG_TIME();
  FimexCrossection_cp fcs;
  CDMReader_p reader;
  CoordinateSystem_cp_v coordinateSystems;
  prepareGetValues(crossection, fcs, reader, coordinateSystems);
  const CDM& cdm = reader->getCDM();

  for (InventoryBase_cp b : data) {
    try {
      CoordinateSystem_cp cs = findCsForVariable(cdm, coordinateSystems, b);
      diutil::Values::Shape shapeCdm = shapeFromCDM(cdm, cs, b);
      diutil::Values::ShapeSlice sliceCdm(shapeCdm);
      sliceCdm.cut(findShapeIndex(CoordinateAxis::GeoX, cs, shapeCdm), fcs->start_index + crossection_index, 1) // single point on x axis
          .    cut(findShapeIndex(CoordinateAxis::Realization, cs, shapeCdm), realization, 1);                  // single point on ensemble_member axis

      diutil::Values::Shape shapeOut(diutil::Values::TIME, mInventory->times.npoint(), diutil::Values::GEO_Z, b->nlevel());

      if (auto v = getSlicedValues(reader, cs, sliceCdm, shapeOut, b)) {
        METLIBS_LOG_DEBUG("values for '" << b->id() << " has npoint=" << v->shape().length(0) << " and nlevel=" << v->shape().length(1));
        n2v[b->id()] = v;
      }
    } catch (std::exception& ex) {
      METLIBS_LOG_WARN("exception: " << ex.what());
    }
  }
}

void FimexReftimeSource::getPointValues(Crossection_cp crossection,
    size_t crossection_index, const Time& time, const InventoryBase_cps& data, name2value_t& n2v, int realization)
{
  METLIBS_LOG_TIME();
  FimexCrossection_cp fcs;
  CDMReader_p reader;
  CoordinateSystem_cp_v coordinateSystems;
  prepareGetValues(crossection, fcs, reader, coordinateSystems);
  const CDM& cdm = reader->getCDM();

  const size_t t_start = findTimeIndex(mInventory, time);

  for (InventoryBase_cp b : data) {
    try {
      CoordinateSystem_cp cs = findCsForVariable(cdm, coordinateSystems, b);
      diutil::Values::Shape shapeCdm = shapeFromCDM(cdm, cs, b);
      diutil::Values::ShapeSlice sliceCdm(shapeCdm);
      sliceCdm.cut(findShapeIndex(CoordinateAxis::GeoX, cs, shapeCdm), fcs->start_index + crossection_index, 1) // single point on x axis
          .    cut(findShapeIndex(CoordinateAxis::Time, cs, shapeCdm), t_start, 1)                              // single time
          .    cut(findShapeIndex(CoordinateAxis::Realization, cs, shapeCdm), realization, 1);                  // single point on ensemble_member axis

      diutil::Values::Shape shapeOut(diutil::Values::GEO_Z, b->nlevel());

      if (auto v = getSlicedValues(reader, cs, sliceCdm, shapeOut, b))
        n2v[b->id()] = v;
    } catch (std::exception& ex) {
      METLIBS_LOG_WARN("exception: " << ex.what());
    }
  }
}

void FimexReftimeSource::getWaveSpectrumValues(Crossection_cp crossection, size_t crossection_index,
    const Time& time, const InventoryBase_cps& data, name2value_t& n2v, int realization)
{
  METLIBS_LOG_TIME();
  FimexCrossection_cp fcs;
  CDMReader_p reader;
  CoordinateSystem_cp_v coordinateSystems;
  prepareGetValues(crossection, fcs, reader, coordinateSystems);
  const CDM& cdm = reader->getCDM();

  const size_t t_start = findTimeIndex(mInventory, time);

  for (InventoryBase_cp b : data) {
    try {
      CoordinateSystem_cp cs = findCsForVariable(cdm, coordinateSystems, b);
      diutil::Values::Shape shapeCdm = shapeFromCDM(reader->getCDM(), cs, b);
      diutil::Values::ShapeSlice sliceCdm(shapeCdm);
      diutil::Values::Shape shapeOut;
      if (not cs) {
        shapeOut = shapeCdm;
      } else {
        sliceCdm.cut(findShapeIndex(CoordinateAxis::GeoX, cs, shapeCdm), fcs->start_index + crossection_index, 1) // single point on x axis
            .    cut(findShapeIndex(CoordinateAxis::Time, cs, shapeCdm), t_start, 1);                     // single time
        shapeOut.add(WAVE_DIRECTION, shapeCdm.length(WAVE_DIRECTION));
        shapeOut.add(WAVE_FREQ,      shapeCdm.length(WAVE_FREQ));
      }

      if (auto v = getSlicedValues(reader, cs, sliceCdm, shapeOut, b))
        n2v[b->id()] = v;
    } catch (std::exception& ex) {
      METLIBS_LOG_WARN("exception: " << ex.what());
    }
  }
}

// converted must be one of zaxis->getField(...)
diutil::Values_p FimexReftimeSource::getSlicedValuesGeoZTransformed(CDMReader_p reader, CoordinateSystem_cp cs, const diutil::Values::ShapeSlice& sliceCdm,
                                                                    const diutil::Values::Shape& shapeOut, InventoryBase_cp converted)
{
  METLIBS_LOG_TIME();

  const int verticalType = verticalTypeFromId(converted->id());
  if (verticalType < 0)
    return diutil::Values_p();

  METLIBS_LOG_DEBUG(LOGVAL(converted->id()));

  // this function does not support requested dimensions other than GEO_X, GEO_Y and TIME
  const int px = shapeOut.position(diutil::Values::GEO_X);
  const int py = shapeOut.position(diutil::Values::GEO_Y);
  const int pz = shapeOut.position(diutil::Values::GEO_Z);
  const int pt = shapeOut.position(diutil::Values::TIME);
  const int rk = shapeOut.rank();
  METLIBS_LOG_DEBUG(LOGVAL(px) << LOGVAL(py) << LOGVAL(pz) << LOGVAL(pt));
  if (pz < 0)
    THROW(std::runtime_error, "requested shape for '" << converted->id() << "' lacks GEO_Z");
  for (int p = 0; p < rk; ++p) {
    if (p != px and p != py and p != pz and p != pt and shapeOut.length(p) != 1) {
      THROW(std::runtime_error, "problem with vertical transformations for '" << converted->id()
          << "': only GEO_X, GEO_Y, and TIME are supported, not '" << shapeOut.name(p) << "'");
    }
  }

  const int pcx = findShapeIndex(CoordinateAxis::GeoX, cs, sliceCdm.shape());
  const int pcy = findShapeIndex(CoordinateAxis::GeoY, cs, sliceCdm.shape());
  const int pcz = findShapeIndex(CoordinateAxis::GeoZ, cs, sliceCdm.shape());
  const int pct = findShapeIndex(CoordinateAxis::Time, cs, sliceCdm.shape());
  METLIBS_LOG_DEBUG(LOGVAL(pcx) << LOGVAL(pcy) << LOGVAL(pcz) << LOGVAL(pct));

  auto v = std::make_shared<diutil::Values>(shapeOut);
  diutil::Values::ShapeIndex idx(v->shape());

  const size_t t_begin = sliceCdm.start(pct), t_length = sliceCdm.length(pct);
  const size_t x_begin = sliceCdm.start(pcx), x_length = sliceCdm.length(pcx);
  const size_t y_begin = sliceCdm.start(pcy), y_length = sliceCdm.length(pcy);
  const size_t z_begin = sliceCdm.start(pcz), z_length = sliceCdm.length(pcz);
  METLIBS_LOG_DEBUG(LOGVAL(t_begin) << LOGVAL(t_length)
      << LOGVAL(x_begin) << LOGVAL(x_length)
      << LOGVAL(y_begin) << LOGVAL(y_length)
      << LOGVAL(z_begin) << LOGVAL(z_length));

  std::shared_ptr<CDMExtractor> extractor = std::make_shared<CDMExtractor>(reader);
  if (cs->getTimeAxis())
    extractor->reduceDimension(cs->getTimeAxis()->getName(), t_begin, t_length);
  if (cs->getGeoXAxis())
    extractor->reduceDimension(cs->getGeoXAxis()->getName(), x_begin, x_length);
  if (cs->getGeoYAxis())
    extractor->reduceDimension(cs->getGeoYAxis()->getName(), y_begin, y_length);
  if (cs->getGeoZAxis())
    extractor->reduceDimension(cs->getGeoZAxis()->getName(), z_begin, z_length);
  if (CoordinateAxis_cp rAxis = cs->findAxisOfType(CoordinateAxis::Realization)) {
    const int pcr = findShapeIndex(CoordinateAxis::Realization, cs, sliceCdm.shape());
    const size_t r_begin = sliceCdm.start(pcr), r_length = sliceCdm.length(pcr);
    extractor->reduceDimension(rAxis->getName(), r_begin, r_length);
  }

  const std::string& ztid = transformedZAxisName(converted->id());
  zaxis_cs_m::const_iterator itCsId = zaxis_cs.find(ztid);
  if (itCsId == zaxis_cs.end())
    THROW(std::runtime_error, "no z-axis '" << ztid << "' for '" << converted->id() << "'");

  // this is almost the same as cs, except that it may be for a different CDMReader
  CoordinateSystem_cp ex_cs = findGeoZTransformed
      (MetNoFimex::listCoordinateSystems(extractor), ztid, itCsId->second);
  if (!ex_cs)
    THROW(std::runtime_error, "no cs with z-axis '" << ztid << "' for '" << converted->id() << "'");

  VerticalTransformation_cp ex_vt = ex_cs->getVerticalTransformation();
  if (!ex_vt)
    THROW(std::runtime_error, "no vertical transformation for z-axis '" << ztid << "' for '" << converted->id() << "'");

  METLIBS_LOG_DEBUG(LOGVAL(ex_vt->getName()));

#ifdef FIMEX_HAS_VERTICALCONVERTER
  if (VerticalConverter_p vc_p = ex_vt->getConverter(extractor, ex_cs, verticalType)) {
    const SliceBuilder sb = createSliceBuilder(extractor->getCDM(), vc_p);
    DataPtr dataCdm = vc_p->getDataSlice(sb);
    if (!dataCdm)
      THROW(std::runtime_error, "no vertical data for z-axis '" << ztid << "' for '" << converted->id() << "'");
    MetNoFimex::shared_array<float> floats = dataCdm->asFloat();

    const diutil::Values::Shape shapeVC(vc_p->getShape(), getDimSizes(extractor->getCDM(), vc_p->getShape()));
    const diutil::Values::Shape shapeCdmOut = translateAxisNames(shapeOut, extractor->getCDM(), cs);
    floats = reshape(shapeVC, shapeCdmOut, floats);

    METLIBS_LOG_DEBUG(LOGVAL(sb) << LOGVAL(shapeVC) << LOGVAL(shapeCdmOut) << LOGVAL(shapeOut));

    v = std::make_shared<diutil::Values>(shapeOut, floats);
  }
#else // !FIMEX_HAS_VERTICALCONVERTER
  const bool timeIsUnlimitedDim = isTimeAxisUnlimited(extractor->getCDM(), ex_cs);
  for (size_t t=0; t < t_length; ++t) {
    idx.set(pt, t);
    const size_t t_vlc = timeIsUnlimitedDim ? t : 0 /* FIXME 0 is not necessarily correct */;
    const size_t t_up  = timeIsUnlimitedDim ? 0 : t;
    if (ToVLevelConverter_p vlc = ex_vt->getConverter
        (extractor, verticalType, t_vlc, ex_cs))
    {
      for (size_t y = 0; y < y_length; ++y) {
        idx.set(py, y);
        for (size_t x = 0; x < x_length; ++x) {
          idx.set(px, x);
          const std::vector<double> up_at_x = (*vlc)(x, y, t_up);
          for (size_t z = 0; z < z_length; ++z) {
            idx.set(pz, z);
            v->setValue(up_at_x.at(z), idx);
          }
        }
      }
    }
  }
#endif // !FIMEX_HAS_VERTICALCONVERTER
  return v;
}

diutil::Values_p FimexReftimeSource::getSlicedValues(CDMReader_p reader, CoordinateSystem_cp cs, const diutil::Values::ShapeSlice& sliceCdm,
                                                     const diutil::Values::Shape& shapeOut, InventoryBase_cp b)
{
  METLIBS_LOG_SCOPE(LOGVAL(b->id()) << LOGVAL(b->dataType()));

  if (not cs) {
    METLIBS_LOG_SCOPE("slice without cs, reading directly");
    DataPtr dataCdm = reader->getScaledData(b->id());
    if (not dataCdm)
      THROW(std::runtime_error, "no data for '" << b->id() << "'");
    if (b->dataType() == ZAxisData::DATA_TYPE()) {
      METLIBS_LOG_SCOPE("z levels, using GEO_Z" << LOGVAL(dataCdm->size()) << LOGVAL(b->nlevel()));
      return std::make_shared<diutil::Values>(diutil::Values::Shape(diutil::Values::GEO_Z, b->nlevel()), dataCdm->asFloat());
    } else {
      return std::make_shared<diutil::Values>(shapeOut, dataCdm->asFloat());
    }
  }

  if (auto v = getSlicedValuesGeoZTransformed(reader, cs, sliceCdm, shapeOut, b))
    return v;

  const CDM& cdm = reader->getCDM();

  const diutil::Values::Shape shapeCdmOut = translateAxisNames(shapeOut, cdm, cs);

  CoordinateSystemSliceBuilder sb(cdm, cs);
  for (size_t p = 0; p < sliceCdm.shape().rank(); ++p)
    sb.setStartAndSize(sliceCdm.shape().name(p), sliceCdm.start(p), sliceCdm.length(p));

  DataPtr dataCdm = reader->getScaledDataSlice(b->id(), sb);
  if (not dataCdm)
    THROW(std::runtime_error, "no sliced data for '" << b->id() << "'");

  MetNoFimex::shared_array<float> floatsReshaped = reshape(sliceCdm, shapeCdmOut, dataCdm->asFloat());
  return std::make_shared<diutil::Values>(shapeOut, floatsReshaped);
}

bool FimexReftimeSource::makeReader()
{
  METLIBS_LOG_SCOPE(LOGVAL(mFileName) << LOGVAL(mFileType) << LOGVAL(mFileConfig));
  METLIBS_LOG_INFO(LOGVAL(mFileName));
  try {
    mReader = CDMFileReaderFactory::create(mFileType, mFileName, mFileConfig);
    return true;
  } catch (std::exception& ex) {
    METLIBS_LOG_WARN("Problem creating reader for '" << mFileName << "': " << ex.what());
    mReader = CDMReader_p();
    return false;
  }
}

CDMReader_p FimexReftimeSource::makeReader(FimexCrossection_cp cs)
{
  METLIBS_LOG_SCOPE();
  if (not (cs and cs->reader)) {
    return mReader;
  }

  return cs->reader;
}

bool FimexReftimeSource::makeInventory()
{
  METLIBS_LOG_TIME();

  try {
    if (not mReader and not makeReader())
      return false;

    mInventory = std::make_shared<Inventory>();
    zaxis_cs.clear();

    const CDM& cdm = mReader->getCDM();

    typedef std::map< std::pair<std::string,std::string>, ZAxisData_cp> knownZAxes_t;
    knownZAxes_t knownZAxes;
    std::string knownTAxis;

    // get all coordinate systems from file, usually one, but may be a few (theoretical limit: # of variables)
    mCoordinateSystems = MetNoFimex::listCoordinateSystems(mReader);

    const std::vector<CDMVariable>& variables = cdm.getVariables();
    for (const CDMVariable& var : variables) {
      const std::string& vName = var.getName(), vsName = standardNameOrName(cdm, vName);
      METLIBS_LOG_DEBUG(LOGVAL(vName) << LOGVAL(vsName));

      if (vsName == VCROSS_BOUNDS or vsName == VCROSS_NAME or vsName == LONGITUDE or vsName == LATITUDE) {
        METLIBS_LOG_DEBUG("stepping over special variable '" << vName << "'");
        continue;
      }

      FieldData_p field(new FieldData(vName, cdm.getUnits(vName)));

      const std::vector<CoordinateSystem_cp>::const_iterator itVarCS =
          std::find_if(mCoordinateSystems.begin(), mCoordinateSystems.end(),
              MetNoFimex::CompleteCoordinateSystemForComparator(vName));
      if (itVarCS != mCoordinateSystems.end()) {
        CoordinateSystem_cp cs = *itVarCS;

#if 0
        if (!cs->getGeoXAxis() || !cs->getGeoYAxis()) {
          METLIBS_LOG_DEBUG("variable '" << vName << "' has different no x or y axis, ignoring");
          continue;
        }
#endif

        METLIBS_LOG_DEBUG("variable '" << vName << "' looks good");

        CoordinateAxis_cp tAxis = cs->getTimeAxis();
        if (tAxis) {
          if (not knownTAxis.empty() and tAxis->getName() != knownTAxis) {
            METLIBS_LOG_DEBUG("variable '" << vName << "' has different time axis, ignoring");
            continue;
          }

          if (knownTAxis.empty()) {
            knownTAxis = tAxis->getName();
            Times& times = mInventory->times;
            const std::string tUnit = cdm.getUnits(tAxis->getName());
            mInventory->times.unit = tUnit;

            if (DataPtr tData = mReader->getScaledData(tAxis->getName())) {
              if (MetNoFimex::shared_array<Time::timevalue_t> tValues = tData->asDouble()) {
                times.values.insert(times.values.end(), tValues.get(), tValues.get()+tData->size());
              }
            }
          }
        }

        if (CoordinateAxis_cp rAxis = cs->findAxisOfType(CoordinateAxis::Realization))
          miutil::maximize(mInventory->realizationCount, cdm.getDimension(rAxis->getShape().front()).getLength());

        CoordinateAxis_cp zAxis = cs->getGeoZAxis();
        if (zAxis) {
          const std::string& zName = zAxis->getName();
          METLIBS_LOG_DEBUG("variable '" << vName << "' has z axis '" << zName << "'");

          ZAxisData_cp zlevels;
          const std::pair<std::string,std::string> zName_csId(zName, cs->id());
          const knownZAxes_t::const_iterator it = knownZAxes.find(zName_csId);
          if (it == knownZAxes.end()) {
            ZAxisData_p znew = std::make_shared<ZAxisData>(zName, cdm.getUnits(zName));
            znew->setZDirection(isPositiveUp(zName, cdm) ? Z_DIRECTION_UP : Z_DIRECTION_DOWN);
            znew->setNlevel(cdm.getDimension(zName).getLength());
            if (VerticalTransformation_cp vt = cs->getVerticalTransformation()) {
              METLIBS_LOG_DEBUG("z axis '" << zName << "' has vertical transformation '" <<  vt->getName() << "'");
              const int n_z_types = 4;
              const int mifi_z_types[n_z_types] = {MIFI_VINT_PRESSURE, MIFI_VINT_ALTITUDE, MIFI_VINT_HEIGHT, MIFI_VINT_DEPTH};
              const Z_AXIS_TYPE vcross_z_types[n_z_types] = {Z_TYPE_PRESSURE, Z_TYPE_ALTITUDE, Z_TYPE_HEIGHT, Z_TYPE_DEPTH};
              const std::string vcross_z_ids[n_z_types] = {"pressure", "altitude", "height", "depth"};
              const std::string vcross_z_units[n_z_types] = {"hPa", "m", "m", "m"};
              for (int i = 0; i < n_z_types; ++i) {
                try {
                  if (ToVLevelConverter_p pc = vt->getConverter(mReader, mifi_z_types[i], 0, cs)) {
                    METLIBS_LOG_DEBUG("got " << vcross_z_ids[i] << " converter for '" << zName << "'");
                    FieldData_p zfield(new FieldData(zName + "//" + vcross_z_ids[i], vcross_z_units[i]));
                    zfield->setNlevel(znew->nlevel());
                    znew->setField(vcross_z_types[i], zfield);
                  }
                } catch (CDMException& ex) {
                  METLIBS_LOG_DEBUG("problem with " << vcross_z_ids[i] << " converter for '" << zName << "', no " << vcross_z_ids[i]
                                                    << " field; exception=" << ex.what());
                }
              }
            }

            zlevels = znew;
            knownZAxes.insert(std::make_pair(zName_csId, zlevels));
            zaxis_cs.insert(zName_csId);
            METLIBS_LOG_DEBUG("z axis '" << zName << "' has " << zlevels->nlevel()
                << " levels, unit '" << zlevels->unit()
                << "' and is " << (zlevels->zdirection() == Z_DIRECTION_UP ? "up" : "down"));
          } else {
            zlevels = it->second;
          }
          field->setZAxis(zlevels);
        }
      }
      mInventory->fields.push_back(field);
    }

    makeCrossectionInventory();

    return true;
  } catch (std::exception& ex) {
    METLIBS_LOG_WARN("Problem creating inventory, exception is: " << ex.what());
    return false;
  }
}

void FimexReftimeSource::makeCrossectionInventory()
{
  METLIBS_LOG_TIME();

  const CDM& cdm = mReader->getCDM();

  const std::string vc_name = findVariableByStandardNameOrName(cdm, VCROSS_NAME);
  std::string vc_bounds;
  if (!vc_name.empty()) {
    CDMAttribute a_bounds;
    if (cdm.getAttribute(vc_name, VCROSS_BOUNDS, a_bounds)) {
      vc_bounds = a_bounds.getStringValue();
    } else {
      METLIBS_LOG_WARN(LOGVAL(vc_name) << " without '" << VCROSS_BOUNDS
          << "' attribute");
    }
  }

  // FIXME find a variable, get x, get longitude/latitude, but also
  // check that vcross_name::bounds applies to this variable and not
  // to something else, plus other checks that may be necessary
  const std::string longitude = findVariableByStandardNameOrName(cdm, LONGITUDE),
      latitude = findVariableByStandardNameOrName(cdm, LATITUDE);

  bool has_longitude_latitude = false;
  DataPtr dataLon, dataLat;
  MetNoFimex::shared_array<float> vLon, vLat;
  if (!longitude.empty() && !latitude.empty()) {
    dataLon = mReader->getScaledData(longitude);
    dataLat = mReader->getScaledData(latitude);
    if (dataLon and dataLat and dataLon->size() == dataLat->size()) {
      vLon = dataLon->asFloat();
      vLat = dataLat->asFloat();
      if (vLon and vLat) {
        has_longitude_latitude = true;
      }
    }
  }

  const bool has_y_1 = !cdm.hasDimension(COORD_Y)
      || cdm.getDimension(COORD_Y).getLength() == 1;

  if (!vc_name.empty() && !vc_bounds.empty() && has_longitude_latitude) {
    mSupportsDynamic = false;
    const CDMVariable &varVCNames = cdm.getVariable(vc_name);
    const std::vector<std::string>& vc_shape = varVCNames.getShape();
    const size_t nvcross = cdm.getDimension(vc_shape.back()).getLength();

    for (size_t ivc=0; ivc<nvcross; ++ivc) {
      SliceBuilder sb_bounds(cdm, vc_bounds);
      sb_bounds.setStartAndSize(vc_shape.back(), ivc, 1);
      DataPtr dataBounds = mReader->getScaledDataSlice(vc_bounds, sb_bounds);
      if (not dataBounds or dataBounds->size() != 2) {
        METLIBS_LOG_WARN(vc_bounds << " size != 2");
        continue;
      }
      MetNoFimex::shared_array<int> valuesBounds = dataBounds->asInt();
      if (not valuesBounds) {
        METLIBS_LOG_WARN(vc_bounds << " values missing");
        continue;
      }
      const size_t lola_begin = valuesBounds[0], lola_end = valuesBounds[1]+1;
      if (lola_begin >= dataLon->size() or lola_end > dataLon->size()) {
        METLIBS_LOG_WARN(vc_bounds << " values out of range (b:" << lola_begin
            << " >= or e:" << lola_end << " > " << dataLon->size() << ")");
        continue;
      }

      SliceBuilder sb_name(cdm, vc_name);
      sb_name.setStartAndSize(vc_shape.back(), ivc, 1);
      DataPtr dataName = mReader->getDataSlice(vc_name, sb_name);
      if (not dataName) {
        METLIBS_LOG_WARN(vc_name << " data missing");
        continue;
      }
      const std::string csname_encoded = dataName->asString();
      const std::string csname = mCsNameCharsetConverter->convert(&csname_encoded[0]);

      const LonLat_v csPoints = makeCrossectionPoints(vLon, vLat, lola_begin, lola_end);
      FimexCrossection_p cs = std::make_shared<FimexCrossection>
          (csname, csPoints, makePointsRequested(csPoints), lola_begin);
      mInventory->crossections.push_back(cs);
      METLIBS_LOG_DEBUG("added segment crossection '" << cs->label()
          << "' with '" << cs->length() << "' points");
    }
  } else if (has_longitude_latitude and has_y_1) {
    // assuming it is a file with a single readymade cross-section
    mSupportsDynamic = false;
    const LonLat_v csPoints = makeCrossectionPoints(vLon, vLat, 0, dataLon->size());
    FimexCrossection_p cs = std::make_shared<FimexCrossection>
        ("single_cs", csPoints, makePointsRequested(csPoints), 0);
    mInventory->crossections.push_back(cs);
    METLIBS_LOG_DEBUG("added single crossection '" << cs->label()
        << "' with '" << cs->length() << "' points");
  } else {
    mSupportsDynamic = true;
  }
}

Crossection_cp FimexReftimeSource::addDynamicCrossection(std::string label, const LonLat_v& positions)
{
  METLIBS_LOG_SCOPE();
  if (not mSupportsDynamic or positions.empty())
    return Crossection_cp();

  typedef std::pair<double, double> LonLatPair;
  typedef std::vector<LonLatPair> LonLatPairs;

  LonLatPairs lonLatCoordinates;
  lonLatCoordinates.reserve(positions.size());
  for (LonLat_v::const_iterator it = positions.begin(); it != positions.end(); ++it)
    lonLatCoordinates.push_back(LonLatPair(it->lonDeg(), it->latDeg()));

  CDMInterpolator_p interpolator = std::make_shared<CDMInterpolator>(mReader);
  const std::vector<CrossSectionDefinition> csd1(1, CrossSectionDefinition(label, lonLatCoordinates));
  interpolator->changeProjectionToCrossSections(MIFI_INTERPOL_BILINEAR, csd1);

  // fetch back points from CDM to get all the intermediate points
  // that CDMInterpolator inserts
  const DataPtr dataLon = interpolator->getScaledData(LONGITUDE);
  const DataPtr dataLat = interpolator->getScaledData(LATITUDE);
  if (not (dataLon and dataLat and dataLon->size() == dataLat->size()))
    return Crossection_cp();
  const MetNoFimex::shared_array<float> vLon = dataLon->asFloat(), vLat = dataLat->asFloat();
  LonLat_v actualPoints;
  actualPoints.reserve(dataLon->size());
  for (size_t i=0; i<dataLon->size(); ++i)
    actualPoints.push_back(LonLat::fromDegrees(vLon[i], vLat[i]));

  FimexCrossection_p cs = std::make_shared<FimexCrossection>
      (label, actualPoints, positions, 0, interpolator);

  // if a cross-section with the same name is known, replace it
  Crossection_cpv& ics =  mInventory->crossections;
  Crossection_cpv::iterator it;
  for (it = ics.begin(); it != ics.end(); ++it) {
    if ((*it)->label() == label) {
      *it = cs;
      break;
    }
  }
  if (it == ics.end())
    mInventory->crossections.push_back(cs);

  METLIBS_LOG_DEBUG("added/replaced dynamic crossection '" << cs->label()
      << "' with '" << cs->length() << "' points");

  return cs;
}

void FimexReftimeSource::dropDynamicCrossection(Crossection_cp cs)
{
  METLIBS_LOG_SCOPE();

  if (FimexCrossection_cp fcs = std::dynamic_pointer_cast<const FimexCrossection>(cs)) {
    Crossection_cpv& ics =  mInventory->crossections;
    for (Crossection_cpv::iterator it = ics.begin(); it != ics.end(); ++it) {
      if (*it == fcs) {
        mInventory->crossections.erase(it);
        return;
      }
    }
  } else {
    METLIBS_LOG_DEBUG("no crossection given, or not a fimex crossection");
  }
}

void FimexReftimeSource::dropDynamicCrossections()
{
  METLIBS_LOG_SCOPE();
  Crossection_cpv crossections;
  for (Crossection_cpv::iterator it = mInventory->crossections.begin(); it != mInventory->crossections.end(); ) {
    FimexCrossection_cp fcs = std::static_pointer_cast<const FimexCrossection>(*it);
    if (not fcs->dynamic())
      crossections.push_back(fcs);
  }
  mInventory->crossections = crossections;
}

// ########################################################################

FimexSource::FimexSource(const std::string& filename_pattern,
    const std::string& filetype, const std::string& config,
    diutil::CharsetConverter_p csNameCharsetConverter)
  : mFilePattern(filename_pattern)
  , mFileType(filetype)
  , mFileConfig(config)
  , mCsNameCharsetConverter(csNameCharsetConverter)
{
  METLIBS_LOG_SCOPE();
}

FimexSource::~FimexSource()
{
}

Source::ReftimeUpdate FimexSource::update()
{
  METLIBS_LOG_SCOPE();
  Source::ReftimeUpdate u;

  ReftimeSource_pv sources;

  for (ReftimeSource_p rts : mReftimeSources) {
    METLIBS_LOG_DEBUG(LOGVAL(util::to_miTime(rts->getReferenceTime())));
    ReftimeSource::Update_t ru = rts->update();
    METLIBS_LOG_DEBUG(LOGVAL(u));
    if (ru == ReftimeSource::GONE) {
      // file disappeared
      u.gone.insert(rts->getReferenceTime());
      continue;
    }
    if (ru == ReftimeSource::CHANGED) {
      // file modified
      u.changed.insert(rts->getReferenceTime());
    }
    sources.push_back(rts);
  }
  std::swap(mReftimeSources, sources);

  if (isHttpUrl(mFilePattern)) {
    Time reftime;
    if (addSource(mFilePattern, reftime))
      u.appeared.insert(reftime);
  } else {
    // init time filter and replace yyyy etc. with ????
    std::string before_slash, after_slash;
    const size_t last_slash = mFilePattern.find_last_of("/");
    if (last_slash != std::string::npos) {
      before_slash = mFilePattern.substr(0, last_slash+1);
      after_slash = mFilePattern.substr(last_slash+1);
    } else {
      after_slash = mFilePattern;
    }
    const miutil::TimeFilter tf(after_slash);
    std::string pattern = before_slash + after_slash;
    METLIBS_LOG_DEBUG(LOGVAL(pattern));

    // expand filenames, even if there is no wildcard
    const diutil::string_v matches = diutil::glob(pattern, GLOB_BRACE);
    for (const std::string& path : matches) {
      Time reftime;
      miutil::miTime rt;
      if (tf.getTime(path, rt)) {
        reftime = vcross::util::from_miTime(rt);
      }
      if (addSource(path, reftime))
        u.appeared.insert(reftime);
    }
  }

  return u;
}

bool FimexSource::addSource(const std::string& path, Time& reftime)
{
  if (reftime.valid() && findSource(reftime))
    return false;
  ReftimeSource_p s = std::make_shared<FimexReftimeSource>(path, mFileType, mFileConfig, mCsNameCharsetConverter, reftime);
  if (!reftime.valid()) {
    reftime = s->getReferenceTime();
    if (findSource(reftime))
      return false;
  }
  mReftimeSources.push_back(s);
  METLIBS_LOG_DEBUG(LOGVAL(path) << LOGVAL(util::to_miTime(reftime)));
  return true;
}

} // namespace vcross
