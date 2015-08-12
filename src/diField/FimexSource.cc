
#include "FimexSource.h"

#include "DataReshape.h"
#include "FimexIO.h" // for miutil::path_ctime
#include "TimeFilter.h"
#include "VcrossUtil.h"

#include <puTools/mi_boost_compatibility.hh>
#include <puTools/miStringBuilder.h>

#include <puCtools/glob_cache.h>
#include <puCtools/puCglob.h>

#include <fimex/CDM.h>
#include <fimex/CDMExtractor.h>
#include <fimex/CDMFileReaderFactory.h>
#include <fimex/CDMInterpolator.h>
#include <fimex/Data.h>
#include <fimex/CoordinateSystemSliceBuilder.h>
#include <fimex/coordSys/verticalTransform/ToVLevelConverter.h>

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

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

DataPtr getScaledDataSlice(vcross::FimexReftimeSource::CDMReader_p reader, const SliceBuilder& sb,
    const std::string& varName, const std::string& unit)
{
  if (unit.empty())
    return reader->getScaledDataSlice(varName, sb);
  else
    return reader->getScaledDataSliceInUnit(varName, unit, sb);
}

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

namespace vcutil {
typedef std::vector<std::string> string_v;
string_v glob(const std::string& pattern, int glob_flags, bool& error)
{
  glob_t globBuf;
  error = (glob(pattern.c_str(), glob_flags, 0, &globBuf) != 0);

  string_v matches;
  if (not error)
    matches = string_v(globBuf.gl_pathv, globBuf.gl_pathv + globBuf.gl_pathc);

  globfree(&globBuf);
  return matches;
}
inline string_v glob(const std::string& pattern, int glob_flags=0)
{ bool error; return glob(pattern, glob_flags, error); }
} // namespace vcutil

// ========== FIXME start copy from diana/src/diUtilities.cc ==========
static bool startsOrEndsWith(const std::string& txt, const std::string& sub, int startcompare)
{
  if (sub.empty())
    return true;
  if (txt.size() < sub.size())
    return false;
  return txt.compare(startcompare, sub.size(), sub) == 0;
}

bool startswith(const std::string& txt, const std::string& start)
{
  return startsOrEndsWith(txt, start, 0);
}

bool endswith(const std::string& txt, const std::string& end)
{
  return startsOrEndsWith(txt, end,
      ((int)txt.size()) - ((int)end.size()));
}
// ========== FIXME end copy from diana/src/diUtilities.cc ==========

bool isHttpUrl(const std::string& pattern)
{
  return startswith(pattern, "http://");
}

int findTimeIndex(vcross::Inventory_p inv, const vcross::Time& time)
{
  METLIBS_LOG_SCOPE(LOGVAL(time.unit) << LOGVAL(time.value));
  if (not time.valid())
    THROW(std::runtime_error, "time not valid");

  const vcross::Times& times = inv->times;
  vcross::Time::timevalue_t t = time.value;
  if (not vcross::util::unitsIdentical(time.unit, times.unit)) {
    vcross::util::UnitsConverterPtr uconv = vcross::util::unitConverter(time.unit, times.unit);
    if (not uconv) {
      METLIBS_LOG_DEBUG("unit mismatch: '" << time.unit << "' -- '" << times.unit << "'");
      return -1;
    }
    t = uconv->convert(time.value);
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

static bool isTimeAxisUnlimited(const CDM& cdm, vcross::FimexReftimeSource::CoordinateSystem_p cs)
{
  CoordinateSystem::ConstAxisPtr tAxis = cs->getTimeAxis();
  if (tAxis and tAxis->getShape().size() == 1)
    return cdm.getDimension(tAxis->getShape().at(0)).isUnlimited();
  else
    return true;
}

int verticalTypeFromId(const std::string& id)
{
  const size_t doubleslash = id.find("//");
  if (doubleslash == std::string::npos)
    return -1;

  const std::string verticalTypeText = id.substr(doubleslash+2);
  if (verticalTypeText == "altitude")
    return MIFI_VINT_ALTITUDE;
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

typedef boost::shared_ptr<const VerticalTransformation> VerticalTransformation_cp;
typedef boost::shared_ptr<ToVLevelConverter> ToVLevelConverter_p;

vcross::Values::Shape shapeFromCDM(const CDM& cdm, const std::string& vName)
{
  METLIBS_LOG_SCOPE(LOGVAL(vName));
  vcross::Values::Shape shape;
  if (not cdm.hasVariable(vName) and cdm.hasDimension(vName)) {
    const size_t length = cdm.getDimension(vName).getLength();
    shape.add(vName, length);
  } else {
    const vcross::Values::Shape::string_v& shapeCdm = cdm.getVariable(vName).getShape();
    for (vcross::Values::Shape::string_v::const_iterator it = shapeCdm.begin(); it != shapeCdm.end(); ++it) {
      const size_t length = cdm.getDimension(*it).getLength();
      shape.add(*it, length);
      METLIBS_LOG_DEBUG(LOGVAL(*it) << LOGVAL(length));
    }
  }
  return shape;
}

void addAxisToShape(vcross::Values::Shape& shape, const CDM& cdm, CoordinateSystem::ConstAxisPtr ax)
{
  if (ax)
    shape.add(ax->getName(), cdm.getDimension(ax->getName()).getLength());
}

vcross::Values::Shape shapeFromCS(const CDM& cdm, vcross::FimexReftimeSource::CoordinateSystem_p cs)
{
  METLIBS_LOG_SCOPE();

  vcross::Values::Shape shape;
  addAxisToShape(shape, cdm, cs->getGeoXAxis());
  addAxisToShape(shape, cdm, cs->getGeoYAxis());
  addAxisToShape(shape, cdm, cs->getGeoZAxis());
  addAxisToShape(shape, cdm, cs->getTimeAxis());
  return shape;
}

vcross::Values::Shape shapeFromCDM(const CDM& cdm, vcross::FimexReftimeSource::CoordinateSystem_p cs, vcross::InventoryBase_cp v)
{
  METLIBS_LOG_SCOPE(LOGVAL(v->id()));
  if (cs and verticalTypeFromId(v->id()) >= 0)
    return shapeFromCS(cdm, cs);
  return shapeFromCDM(cdm, v->id());
}

vcross::FimexReftimeSource::CoordinateSystem_p findGeoZTransformed(const vcross::FimexReftimeSource::CoordinateSystem_pv& coordinateSystems,
    const std::string& id)
{
  METLIBS_LOG_SCOPE(LOGVAL(id));
  const std::string& zid = transformedZAxisName(id);
  BOOST_FOREACH(vcross::FimexReftimeSource::CoordinateSystem_p cs, coordinateSystems) {
    if (not (cs->isSimpleSpatialGridded()))
      continue;

    CoordinateSystem::ConstAxisPtr zAxis = cs->getGeoZAxis();
    if (not zAxis or zAxis->getName() != zid)
      continue;

    if (VerticalTransformation_cp vt = cs->getVerticalTransformation())
      return cs;
  }
  return vcross::FimexReftimeSource::CoordinateSystem_p();
}

vcross::FimexReftimeSource::CoordinateSystem_p findCsForVariable(const CDM& cdm,
    const vcross::FimexReftimeSource::CoordinateSystem_pv& coordinateSystems, const std::string& vName)
{
  METLIBS_LOG_SCOPE(LOGVAL(vName));
  const std::vector<vcross::FimexReftimeSource::CoordinateSystem_p>::const_iterator it =
      std::find_if(coordinateSystems.begin(), coordinateSystems.end(),
          MetNoFimex::CompleteCoordinateSystemForComparator(vName));
  if (it == coordinateSystems.end()) {
    if (cdm.hasDimension(vName)) {
      METLIBS_LOG_SCOPE("has dimension '" << vName << "'");
      return vcross::FimexReftimeSource::CoordinateSystem_p();
    }
    THROW(std::runtime_error, "no cs found for '" << vName << "'");
  }
  if (not *it or not (*it)->isSimpleSpatialGridded())
    THROW(std::runtime_error, "cs for '" << vName << "' does not exist or is not a simple spatial grid");

  return *it;
}

vcross::FimexReftimeSource::CoordinateSystem_p findCsForVariable(const CDM& cdm,
    const vcross::FimexReftimeSource::CoordinateSystem_pv& coordinateSystems, vcross::InventoryBase_cp v)
{
  METLIBS_LOG_SCOPE(LOGVAL(v->id()) << LOGVAL(v->dataType()));
  vcross::FimexReftimeSource::CoordinateSystem_p cs;
  if (verticalTypeFromId(v->id()) >= 0)
    cs = findGeoZTransformed(coordinateSystems, v->id());
  if (not cs)
    cs = findCsForVariable(cdm, coordinateSystems, v->id());
  return cs;
}

CoordinateSystem::ConstAxisPtr findAxisOfType(vcross::FimexReftimeSource::CoordinateSystem_p cs, CoordinateAxis::AxisType axisType)
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

int findShapeIndex(CoordinateAxis::AxisType type, vcross::FimexReftimeSource::CoordinateSystem_p cs, const vcross::Values::Shape& shapeCdm)
{
  if (not cs)
    return -1;
  CoordinateSystem::ConstAxisPtr axisCdm = findAxisOfType(cs, type);
  if (not axisCdm)
    return -1;
  return shapeCdm.position(axisCdm->getName());
}

vcross::Values::Shape translateAxisNames(const vcross::Values::Shape& shapeReq, const CDM& cdm, vcross::FimexReftimeSource::CoordinateSystem_p cs)
{
  const int N_AXES = 4;
  const CoordinateAxis::AxisType axesCDM[N_AXES] = { CoordinateAxis::GeoX,  CoordinateAxis::GeoY,  CoordinateAxis::GeoZ,  CoordinateAxis::Time };
  const char* axesV[N_AXES]                      = { vcross::Values::GEO_X, vcross::Values::GEO_Y, vcross::Values::GEO_Z, vcross::Values::TIME };

  vcross::Values::Shape shape;
  for (size_t p = 0; p < shapeReq.rank(); ++p) {
    std::string name = shapeReq.name(p);
    size_t length    = shapeReq.length(p);
    const int i = std::find(axesV, axesV + N_AXES, name) - axesV;
    if (i != N_AXES) {
      // standard axis, replace name
      CoordinateSystem::ConstAxisPtr axisCdm = findAxisOfType(cs, axesCDM[i]);
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

bool hasVariablesByStandardNameOrName(const CDM& cdm, const std::string& sName)
{
  return not findVariablesByStandardNameOrName(cdm, sName).empty();
}

std::string findVariableByStandardNameOrName(const CDM& cdm, const std::string& sName)
{
  string_v variables = findVariablesByStandardNameOrName(cdm, sName);
  if (variables.size() == 1)
    return variables.front();
  else
    return std::string();
}

vcross::LonLat_v makeCrossectionPoints(const boost::shared_array<float> vLon, const boost::shared_array<float> vLat,
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

FimexReftimeSource::FimexReftimeSource(std::string filename, std::string filetype, std::string fileconfig,
  const Time& reftime)
  : mFileName(filename)
  , mFileType(filetype)
  , mFileConfig(fileconfig)
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
            if (boost::shared_array<Time::timevalue_t> rtValues = rtData->asDouble()) {
              mReferenceTime = Time(mReader->getCDM().getUnits(reftime_var), rtValues[0]);
              METLIBS_LOG_DEBUG(LOGVAL(util::to_miTime(mReferenceTime)));
            }
          }
        }
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

void FimexReftimeSource::prepareGetValues(Crossection_cp cs,
    FimexCrossection_cp& fcs, CDMReader_p& reader, CoordinateSystem_pv& coordinateSystems)
{
  METLIBS_LOG_TIME();
  if (not mInventory and not makeInventory())
    THROW(std::runtime_error, "no inventory");

  fcs = boost::dynamic_pointer_cast<const FimexCrossection>(cs);
  if (not fcs)
    THROW(std::runtime_error, "no crossection given, or not a fimex crossection");

  reader = makeReader(fcs);
  if (reader == mReader)
    coordinateSystems = mCoordinateSystems; // FIXME avoid copy
  else
    coordinateSystems = MetNoFimex::listCoordinateSystems(reader);
}

void FimexReftimeSource::getCrossectionValues(Crossection_cp crossection, const Time& time,
    const InventoryBase_cps& data, name2value_t& n2v)
{
  METLIBS_LOG_TIME();
  FimexCrossection_cp fcs;
  CDMReader_p reader;
  CoordinateSystem_pv coordinateSystems;
  prepareGetValues(crossection, fcs, reader, coordinateSystems);
  const CDM& cdm = reader->getCDM();

  const size_t t_start = findTimeIndex(mInventory, time);

  BOOST_FOREACH(InventoryBase_cp b, data) {
    METLIBS_LOG_DEBUG(LOGVAL(b->id()) << LOGVAL(b->nlevel()));
    try {
      CoordinateSystem_p cs = findCsForVariable(cdm, coordinateSystems, b);
      Values::Shape shapeCdm = shapeFromCDM(cdm, cs, b);
      Values::ShapeSlice sliceCdm(shapeCdm);
      sliceCdm.cut(findShapeIndex(CoordinateAxis::GeoX, cs, shapeCdm), fcs->start_index, fcs->length()) // cut on x axis
          .    cut(findShapeIndex(CoordinateAxis::Time, cs, shapeCdm), t_start, 1);                     // single time

      Values::Shape shapeOut(Values::GEO_X, fcs->length(), Values::GEO_Z, b->nlevel());

      if (Values_p v = getSlicedValues(reader, cs, sliceCdm, shapeOut, b))
        n2v[b->id()] = v;
    } catch (std::exception& ex) {
      METLIBS_LOG_WARN("exception: " << ex.what());
    }
  }
}

void FimexReftimeSource::getTimegraphValues(Crossection_cp crossection,
    size_t crossection_index, const InventoryBase_cps& data, name2value_t& n2v)
{
  METLIBS_LOG_TIME();
  FimexCrossection_cp fcs;
  CDMReader_p reader;
  CoordinateSystem_pv coordinateSystems;
  prepareGetValues(crossection, fcs, reader, coordinateSystems);
  const CDM& cdm = reader->getCDM();

  BOOST_FOREACH(InventoryBase_cp b, data) {
    try {
      CoordinateSystem_p cs = findCsForVariable(cdm, coordinateSystems, b);
      Values::Shape shapeCdm = shapeFromCDM(cdm, cs, b);
      Values::ShapeSlice sliceCdm(shapeCdm);
      sliceCdm.cut(findShapeIndex(CoordinateAxis::GeoX, cs, shapeCdm), fcs->start_index + crossection_index, 1); // single point on x axis

      Values::Shape shapeOut(Values::TIME, mInventory->times.npoint(), Values::GEO_Z, b->nlevel());

      if (Values_p v = getSlicedValues(reader, cs, sliceCdm, shapeOut, b)) {
        METLIBS_LOG_DEBUG("values for '" << b->id() << " has npoint=" << v->npoint() << " and nlevel=" << v->nlevel());
        n2v[b->id()] = v;
      }
    } catch (std::exception& ex) {
      METLIBS_LOG_WARN("exception: " << ex.what());
    }
  }
}

void FimexReftimeSource::getPointValues(Crossection_cp crossection,
    size_t crossection_index, const Time& time, const InventoryBase_cps& data, name2value_t& n2v)
{
  METLIBS_LOG_TIME();
  FimexCrossection_cp fcs;
  CDMReader_p reader;
  CoordinateSystem_pv coordinateSystems;
  prepareGetValues(crossection, fcs, reader, coordinateSystems);
  const CDM& cdm = reader->getCDM();

  const size_t t_start = findTimeIndex(mInventory, time);

  BOOST_FOREACH(InventoryBase_cp b, data) {
    try {
      CoordinateSystem_p cs = findCsForVariable(cdm, coordinateSystems, b);
      Values::Shape shapeCdm = shapeFromCDM(cdm, cs, b);
      Values::ShapeSlice sliceCdm(shapeCdm);
      sliceCdm.cut(findShapeIndex(CoordinateAxis::GeoX, cs, shapeCdm), fcs->start_index + crossection_index, 1) // single point on x axis
          .    cut(findShapeIndex(CoordinateAxis::Time, cs, shapeCdm), t_start, 1);                     // single time

      Values::Shape shapeOut(Values::GEO_Z, b->nlevel());

      if (Values_p v = getSlicedValues(reader, cs, sliceCdm, shapeOut, b))
        n2v[b->id()] = v;
    } catch (std::exception& ex) {
      METLIBS_LOG_WARN("exception: " << ex.what());
    }
  }
}

void FimexReftimeSource::getWaveSpectrumValues(Crossection_cp crossection, size_t crossection_index,
    const Time& time, const InventoryBase_cps& data, name2value_t& n2v)
{
  METLIBS_LOG_TIME();
  FimexCrossection_cp fcs;
  CDMReader_p reader;
  CoordinateSystem_pv coordinateSystems;
  prepareGetValues(crossection, fcs, reader, coordinateSystems);
  const CDM& cdm = reader->getCDM();

  const size_t t_start = findTimeIndex(mInventory, time);

  BOOST_FOREACH(InventoryBase_cp b, data) {
    try {
      CoordinateSystem_p cs = findCsForVariable(cdm, coordinateSystems, b);
      Values::Shape shapeCdm = shapeFromCDM(reader->getCDM(), cs, b);
      Values::ShapeSlice sliceCdm(shapeCdm);
      Values::Shape shapeOut;
      if (not cs) {
        shapeOut = shapeCdm;
      } else {
        sliceCdm.cut(findShapeIndex(CoordinateAxis::GeoX, cs, shapeCdm), fcs->start_index + crossection_index, 1) // single point on x axis
            .    cut(findShapeIndex(CoordinateAxis::Time, cs, shapeCdm), t_start, 1);                     // single time
        shapeOut.add(WAVE_DIRECTION, shapeCdm.length(WAVE_DIRECTION));
        shapeOut.add(WAVE_FREQ,      shapeCdm.length(WAVE_FREQ));
      }

      if (Values_p v = getSlicedValues(reader, cs, sliceCdm, shapeOut, b))
        n2v[b->id()] = v;
    } catch (std::exception& ex) {
      METLIBS_LOG_WARN("exception: " << ex.what());
    }
  }
}

// converted must be one of zaxis->altitudeField() or pressureField()
Values_p FimexReftimeSource::getSlicedValuesGeoZTransformed(CDMReader_p reader, CoordinateSystem_p cs,
    const Values::ShapeSlice& sliceCdm, const Values::Shape& shapeOut, InventoryBase_cp converted)
{
  METLIBS_LOG_TIME();

  const int verticalType = verticalTypeFromId(converted->id());
  if (verticalType < 0)
    return Values_p();

  METLIBS_LOG_DEBUG(LOGVAL(converted->id()));

  // this function does not support requested dimensions other than GEO_X, GEO_Y and TIME
  const int px = shapeOut.position(Values::GEO_X);
  const int py = shapeOut.position(Values::GEO_Y);
  const int pz = shapeOut.position(Values::GEO_Z);
  const int pt = shapeOut.position(Values::TIME);
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

  Values_p v = miutil::make_shared<Values>(shapeOut);
  Values::ShapeIndex idx(v->shape());

  const size_t t_begin = sliceCdm.start(pct), t_length = sliceCdm.length(pct);
  const size_t x_begin = sliceCdm.start(pcx), x_length = sliceCdm.length(pcx);
  const size_t y_begin = sliceCdm.start(pcy), y_length = sliceCdm.length(pcy);
  const size_t z_begin = sliceCdm.start(pcz), z_length = sliceCdm.length(pcz);
  METLIBS_LOG_DEBUG(LOGVAL(t_begin) << LOGVAL(t_length)
      << LOGVAL(x_begin) << LOGVAL(x_length)
      << LOGVAL(y_begin) << LOGVAL(y_length)
      << LOGVAL(z_begin) << LOGVAL(z_length));

  boost::shared_ptr<CDMExtractor> extractor(new CDMExtractor(reader));
  if (cs->getTimeAxis())
    extractor->reduceDimension(cs->getTimeAxis()->getName(), t_begin, t_length);
  if (cs->getGeoXAxis())
    extractor->reduceDimension(cs->getGeoXAxis()->getName(), x_begin, x_length);
  if (cs->getGeoYAxis())
    extractor->reduceDimension(cs->getGeoYAxis()->getName(), y_begin, y_length);
  if (cs->getGeoZAxis())
    extractor->reduceDimension(cs->getGeoZAxis()->getName(), z_begin, z_length);

  CoordinateSystem_p ex_cs = findGeoZTransformed
      (MetNoFimex::listCoordinateSystems(extractor),
          transformedZAxisName(converted->id()));
  if (!ex_cs)
    return Values_p();

  const bool timeIsUnlimitedDim = isTimeAxisUnlimited(extractor->getCDM(), ex_cs);

  VerticalTransformation_cp ex_vt = ex_cs->getVerticalTransformation();
  if (not ex_vt)
    return Values_p();

  METLIBS_LOG_DEBUG(LOGVAL(ex_vt->getName()));

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
  return v;
}

Values_p FimexReftimeSource::getSlicedValues(CDMReader_p reader, CoordinateSystem_p cs,
    const Values::ShapeSlice& sliceCdm, const Values::Shape& shapeOut, InventoryBase_cp b)
{
  METLIBS_LOG_SCOPE(LOGVAL(b->id()) << LOGVAL(b->dataType()));

  if (not cs) {
    METLIBS_LOG_SCOPE("slice without cs, reading directly");
    DataPtr dataCdm = reader->getScaledData(b->id());
    if (not dataCdm)
      THROW(std::runtime_error, "no data for '" << b->id() << "'");
    if (b->dataType() == ZAxisData::DATA_TYPE()) {
      METLIBS_LOG_SCOPE("z levels, using GEO_Z" << LOGVAL(dataCdm->size()) << LOGVAL(b->nlevel()));
      return boost::make_shared<Values>(Values::Shape(Values::GEO_Z, b->nlevel()), dataCdm->asFloat());
    } else {
      return boost::make_shared<Values>(shapeOut, dataCdm->asFloat());
    }
  }

  if (Values_p v = getSlicedValuesGeoZTransformed(reader, cs, sliceCdm, shapeOut, b))
    return v;

  const CDM& cdm = reader->getCDM();

  const Values::Shape shapeCdmOut = translateAxisNames(shapeOut, cdm, cs);

  CoordinateSystemSliceBuilder sb(cdm, cs);
  for (size_t p = 0; p < sliceCdm.shape().rank(); ++p)
    sb.setStartAndSize(sliceCdm.shape().name(p), sliceCdm.start(p), sliceCdm.length(p));

  DataPtr dataCdm = reader->getScaledDataSlice(b->id(), sb);
  if (not dataCdm)
    THROW(std::runtime_error, "no sliced data for '" << b->id() << "'");

  boost::shared_array<float> floatsReshaped = reshape(sliceCdm, shapeCdmOut, dataCdm->asFloat());
  return boost::make_shared<Values>(shapeOut, floatsReshaped);
}

bool FimexReftimeSource::makeReader()
{
  METLIBS_LOG_SCOPE(LOGVAL(mFileName) << LOGVAL(mFileType) << LOGVAL(mFileConfig));
  try {
    int ftype = mifi_get_filetype(mFileType.c_str());
    if (ftype == MIFI_FILETYPE_UNKNOWN) {
      METLIBS_LOG_ERROR("unknown source_type:" << mFileType);
      return CDMReader_p();
    }

    mReader = CDMReader_p(CDMFileReaderFactory::create(ftype, mFileName, mFileConfig));
    return true;
  } catch (std::exception& ex) {
    METLIBS_LOG_WARN("Problem creating reader for '" << mFileName << "': " << ex.what());
    mReader = CDMReader_p();
    return false;
  }
}

FimexReftimeSource::CDMReader_p FimexReftimeSource::makeReader(FimexCrossection_cp cs)
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

    mInventory = boost::make_shared<Inventory>();

    const CDM& cdm = mReader->getCDM();

    typedef std::map< std::pair<std::string,std::string>, ZAxisData_cp> knownZAxes_t;
    knownZAxes_t knownZAxes;
    std::string knownTAxis;

    // get all coordinate systems from file, usually one, but may be a few (theoretical limit: # of variables)
    mCoordinateSystems = MetNoFimex::listCoordinateSystems(mReader);

    const std::vector<CDMVariable>& variables = cdm.getVariables();
    BOOST_FOREACH(const CDMVariable& var, variables) {
      const std::string& vName = var.getName(), vsName = standardNameOrName(cdm, vName);
      METLIBS_LOG_DEBUG(LOGVAL(vName) << LOGVAL(vsName));

      if (vsName == VCROSS_BOUNDS or vsName == VCROSS_NAME or vsName == LONGITUDE or vsName == LATITUDE) {
        METLIBS_LOG_DEBUG("stepping over special variable '" << vName << "'");
        continue;
      }

      FieldData_p field(new FieldData(vName, cdm.getUnits(vName)));

      const std::vector<CoordinateSystem_p>::const_iterator itVarCS =
          std::find_if(mCoordinateSystems.begin(), mCoordinateSystems.end(),
              MetNoFimex::CompleteCoordinateSystemForComparator(vName));
      if (itVarCS != mCoordinateSystems.end()) {
        CoordinateSystem_p cs = *itVarCS;

#if 0
        if (!cs->getGeoXAxis() || !cs->getGeoYAxis()) {
          METLIBS_LOG_DEBUG("variable '" << vName << "' has different no x or y axis, ignoring");
          continue;
        }
#endif

        METLIBS_LOG_DEBUG("variable '" << vName << "' looks good");

        CoordinateSystem::ConstAxisPtr tAxis = cs->getTimeAxis();
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
              if (boost::shared_array<Time::timevalue_t> tValues = tData->asDouble()) {
                times.values.insert(times.values.end(), tValues.get(), tValues.get()+tData->size());
              }
            }
          }
        }

        CoordinateSystem::ConstAxisPtr zAxis = cs->getGeoZAxis();
        if (zAxis) {
          const std::string& zName = zAxis->getName();
          METLIBS_LOG_DEBUG("variable '" << vName << "' has z axis '" << zName << "'");

          ZAxisData_cp zlevels;
          const std::pair<std::string,std::string> zName_csId(zName, cs->id());
          const knownZAxes_t::const_iterator it = knownZAxes.find(zName_csId);
          if (it == knownZAxes.end()) {
            ZAxisData_p znew = boost::make_shared<ZAxisData>(zName, cdm.getUnits(zName));
            znew->setZDirection(isPositiveUp(zName, cdm) ? Z_DIRECTION_UP : Z_DIRECTION_DOWN);
            znew->setNlevel(cdm.getDimension(zName).getLength());
            if (VerticalTransformation_cp vt = cs->getVerticalTransformation()) {
              METLIBS_LOG_DEBUG("z axis '" << zName << "' has vertical transformation '" <<  vt->getName() << "'");
              try {
                if (ToVLevelConverter_p pc = vt->getConverter(mReader, MIFI_VINT_PRESSURE, 0, cs)) {
                  METLIBS_LOG_DEBUG("got pressure converter for '" << zName << "'");
                  FieldData_p pressure(new FieldData(zName + "//pressure", "hPa"));
                  pressure->setNlevel(znew->nlevel());
                  znew->setPressureField(pressure);
                }
              } catch (CDMException& ex) {
                METLIBS_LOG_DEBUG("problem with pressure converter for '" << zName << "', no pressure field; exception=" << ex.what());
              }
              try {
                if (ToVLevelConverter_p ac = vt->getConverter(mReader, MIFI_VINT_ALTITUDE, 0, cs)) {
                  METLIBS_LOG_DEBUG("got altitude converter for '" << zName << "'");
                  FieldData_p altitude(new FieldData(zName + "//altitude", "m"));
                  altitude->setNlevel(znew->nlevel());
                  znew->setAltitudeField(altitude);
                }
              } catch (CDMException& ex) {
                METLIBS_LOG_DEBUG("problem with altitude converter for '" << zName << "', no altitude field");
              }
            }

            zlevels = znew;
            knownZAxes.insert(std::make_pair(zName_csId, zlevels));
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
  boost::shared_array<float> vLon, vLat;
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

  const bool has_y_1 = cdm.hasDimension(COORD_Y)
      && cdm.getDimension(COORD_Y).getLength() == 1;

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
      boost::shared_array<int> valuesBounds = dataBounds->asInt();
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
      DataPtr dataName = mReader->getScaledDataSlice(vc_name, sb_name);
      if (not dataName) {
        METLIBS_LOG_WARN(vc_name << " data missing");
        continue;
      }
      boost::shared_array<char> valuesName = dataName->asChar();
      if (not valuesName) {
        METLIBS_LOG_WARN(vc_name << " values missing");
        continue;
      }
      const std::string csname(valuesName.get());

      const LonLat_v csPoints = makeCrossectionPoints(vLon, vLat, lola_begin, lola_end);
      FimexCrossection_p cs = miutil::make_shared<FimexCrossection>
          (csname, csPoints, makePointsRequested(csPoints), lola_begin);
      mInventory->crossections.push_back(cs);
      METLIBS_LOG_DEBUG("added segment crossection '" << cs->label()
          << "' with '" << cs->length() << "' points");
    }
  } else if (has_longitude_latitude and has_y_1) {
    // assuming it is a file with a single readymade cross-section
    mSupportsDynamic = false;
    const LonLat_v csPoints = makeCrossectionPoints(vLon, vLat, 0, dataLon->size());
    FimexCrossection_p cs = miutil::make_shared<FimexCrossection>
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

  boost::shared_ptr<CDMInterpolator> interpolator(new CDMInterpolator(mReader));
  const std::vector<CrossSectionDefinition> csd1(1, CrossSectionDefinition(label, lonLatCoordinates));
  interpolator->changeProjectionToCrossSections(MIFI_INTERPOL_BILINEAR, csd1);

  // fetch back points from CDM to get all the intermediate points
  // that CDMInterpolator inserts
  const DataPtr dataLon = interpolator->getScaledData(LONGITUDE);
  const DataPtr dataLat = interpolator->getScaledData(LATITUDE);
  if (not (dataLon and dataLat and dataLon->size() == dataLat->size()))
    return Crossection_cp();
  const boost::shared_array<float> vLon = dataLon->asFloat(), vLat = dataLat->asFloat();
  LonLat_v actualPoints;
  actualPoints.reserve(dataLon->size());
  for (size_t i=0; i<dataLon->size(); ++i)
    actualPoints.push_back(LonLat::fromDegrees(vLon[i], vLat[i]));

  FimexCrossection_p cs = miutil::make_shared<FimexCrossection>
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

  if (FimexCrossection_cp fcs = boost::dynamic_pointer_cast<const FimexCrossection>(cs)) {
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
    FimexCrossection_cp fcs = boost::static_pointer_cast<const FimexCrossection>(*it);
    if (not fcs->dynamic())
      crossections.push_back(fcs);
  }
  mInventory->crossections = crossections;
}

// ########################################################################

FimexSource::FimexSource(const std::string& filename_pattern,
    const std::string& filetype, const std::string& config)
  : mFilePattern(filename_pattern)
  , mFileType(filetype)
  , mFileConfig(config)
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

  for (ReftimeSource_pv::const_iterator it = mReftimeSources.begin(); it != mReftimeSources.end(); ++it) {
    METLIBS_LOG_DEBUG(LOGVAL(util::to_miTime((*it)->getReferenceTime())));
    ReftimeSource::Update_t ru = (*it)->update();
    METLIBS_LOG_DEBUG(LOGVAL(u));
    if (ru == ReftimeSource::GONE) {
      // file disappeared
      u.gone.insert((*it)->getReferenceTime());
      continue;
    }
    if (ru == ReftimeSource::CHANGED) {
      // file modified
      u.changed.insert((*it)->getReferenceTime());
    }
    sources.push_back(*it);
  }
  std::swap(mReftimeSources, sources);

  if (isHttpUrl(mFilePattern)) {
    Time reftime;
    if (addSource(mFilePattern, reftime))
      u.appeared.insert(reftime);
  } else {
    // init time filter and replace yyyy etc. with ????
    TimeFilter tf;
    std::string before_slash, after_slash;
    const size_t last_slash = mFilePattern.find_last_of("/");
    if (last_slash != std::string::npos) {
      before_slash = mFilePattern.substr(0, last_slash+1);
      after_slash = mFilePattern.substr(last_slash+1, mFilePattern.size());
    } else {
      after_slash = mFilePattern;
    }
    tf.initFilter(after_slash, true);
    std::string pattern = before_slash + after_slash;
    METLIBS_LOG_DEBUG(LOGVAL(pattern));

    // expand filenames, even if there is no wildcard
    const vcutil::string_v matches = vcutil::glob(pattern, GLOB_BRACE);
    for (vcutil::string_v::const_iterator it = matches.begin(); it != matches.end(); ++it) {
      const std::string& path = *it;
      Time reftime;
      if (tf.ok()) {
        const std::string reftime_from_filename = tf.getTimeStr(path);
        METLIBS_LOG_DEBUG(LOGVAL(reftime_from_filename));
        reftime = vcross::util::from_miTime(miutil::miTime(reftime_from_filename));
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
  ReftimeSource_p s = miutil::make_shared<FimexReftimeSource>(path, mFileType, mFileConfig, reftime);
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
