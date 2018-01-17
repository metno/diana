
#include "diObsReaderRoad.h"

#include "diObsDialogInfo.h"

// includes for road specific implementation
#include <diObsRoad.h>

#include <puTools/miStringFunctions.h>

using namespace miutil;
using namespace std;

ObsReaderRoad::ObsReaderRoad()
{
  setTimeRange(-180, 180);
}

bool ObsReaderRoad::configure(const std::string& key, const std::string& value)
{
  if (key == "roadobs" || key == "archive_roadobs") {
    addPattern(value, key == "archive_roadobs");
  } else if (key == "headerfile") {
    headerfile = value;
  } else if (key == "databasefile") {
    databasefile = value;
  } else if (key == "stationfile") {
    stationfile = value;
  } else if (key == "daysback") {
    daysback = miutil::to_int(value);
  } else {
    return ObsReaderFile::configure(key, value);
  }
  return true;
}

void ObsReaderRoad::getDataFromFile(const FileInfo& fi, ObsDataRequest_cp request, ObsDataResult_p result)
{
#ifdef OBSROAD
  ObsRoad obsRoad(fi.filename, databasefile, stationfile, headerfile, fi.time, dynamic_cast<RoadObsPlot*>(oplot), false);
  // initData inits the internal data structures in oplot, eg the roadobsp and stationlist.
  obsRoad.initData(dynamic_cast<RoadObsPlot*>(oplot));
  // readData reads the data from road.
  // it shoukle be complemented by a method on the ObsPlot objects that reads data from a single station from road,
  // after ObsPlt has computed wich stations to plot.
  std::vector<ObsData> obsdata = obsRoad.readData(dynamic_cast<RoadObsPlot*>(oplot));
  for (ObsData& obs : obsdata)
    obs.dataType = request->dataType;
  result->add(obsdata);
#endif
}

std::vector<ObsDialogInfo::Par> ObsReaderRoad::getParameters()
{
#ifdef OBSROAD
  // The road format must have a header file, defined in prod
  // This file, defines the parameters as well as the mapping
  // between diana and road parameter space.

  std::string filename; // just dummy here
  miTime filetime;      // just dummy here
  ObsRoad obsRoad = ObsRoad(filename, databasefile, stationfile, headerfile, filetime, NULL, false);
  bool found = obsRoad.asciiOK();

  if (obsRoad.parameterType("dd") && obsRoad.parameterType("ff")) {
    pt.addButton("Wind", "");
    pt.datatype[0].active.push_back(true); // only one datatype, yet!
  }
  for (size_t c = 0; c < obsRoad.columnCount(); c++) {
    pt.addButton(obsRoad.columnName(c), obsRoad.columnTooltip(c), -100, 100, true);
    pt.datatype[0].active.push_back(true); // only one datatype, yet!
  }
#else
  return ObsReader::getParameters();
#endif
}
