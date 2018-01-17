#include "diObsPlotType.h"

namespace {
const std::string T_SYNOP = "synop";
const std::string T_METAR = "metar";
const std::string T_LIST = "list";
const std::string T_PRESSURE = "pressure";
const std::string T_TIDE = "tide";
const std::string T_OCEAN = "ocean";
const std::string T_OTHER = "other";
}

ObsPlotType obsPlotTypeFromText(const std::string& opt)
{
  if (opt == T_SYNOP)
    return OPT_SYNOP;
  else if (opt == T_METAR)
    return OPT_METAR;
  else if (opt == T_LIST)
    return OPT_LIST;
  else if (opt == T_PRESSURE)
    return OPT_PRESSURE;
  else if (opt == T_TIDE)
    return OPT_TIDE;
  else if (opt == T_OCEAN)
    return OPT_OCEAN;
  else
    return OPT_OTHER;
}

const std::string& obsPlotTypeToText(ObsPlotType opt)
{
  if (opt == OPT_SYNOP)
    return T_SYNOP;
  else if (opt == OPT_METAR)
    return T_METAR;
  else if (opt == OPT_LIST)
    return T_LIST;
  else if (opt == OPT_PRESSURE)
    return T_PRESSURE;
  else if (opt == OPT_TIDE)
    return T_TIDE;
  else if (opt == OPT_OCEAN)
    return T_OCEAN;
  else
    return T_OTHER;
}
