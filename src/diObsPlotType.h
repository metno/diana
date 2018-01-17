#ifndef DIOBSPLOTTYPE_H
#define DIOBSPLOTTYPE_H

#include <string>

enum ObsPlotType { OPT_SYNOP, OPT_METAR, OPT_LIST, OPT_PRESSURE, OPT_TIDE, OPT_OCEAN, OPT_OTHER };

ObsPlotType obsPlotTypeFromText(const std::string& opt);
const std::string& obsPlotTypeToText(ObsPlotType opt);

#endif // DIOBSPLOTTYPE_H
