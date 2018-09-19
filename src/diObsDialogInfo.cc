/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018 met.no

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

#include "diObsDialogInfo.h"

ObsDialogInfo::PlotType::PlotType()
    : misc(ObsDialogInfo::criteria)
{
}

void ObsDialogInfo::PlotType::addButton(const std::string& name, const std::string& tip, int low, int high)
{
  Button b;
  b.name = name;
  b.tooltip = tip;
  b.low = low;
  b.high = high;
  button.push_back(b);
}

void ObsDialogInfo::PlotType::addExtraParameterButtons()
{
  bool wind_speed = false;
  bool wind_direction = false;
  for (const ObsDialogInfo::Button& b : button) {
    if (b.name == "dd")
      wind_direction = true;
    if (b.name == "ff")
      wind_speed = true;
  }
  if (wind_speed && wind_direction) {
    ObsDialogInfo::Button b;
    b.name = "Wind";
    button.insert(button.begin(), b);
  }
}

ObsDialogInfo::Par::Par(const std::string& name_)
    : name(name_)
    , type(ObsDialogInfo::pt_std)
    , symbol(-1)
    , precision(0)
    , button_tip(name_)
    , button_low(-50)
    , button_high(50)
{
}

ObsDialogInfo::Par::Par(const std::string& name_, ParType type_, int symbol_, int precision_, const std::string& button_tip_, int button_low_, int button_high_)
    : name(name_)
    , type(type_)
    , symbol(symbol_)
    , precision(precision_)
    , button_tip(button_tip_)
    , button_low(button_low_)
    , button_high(button_high_)
{
}

const std::vector<ObsDialogInfo::Par> pars = {{"pos", ObsDialogInfo::pt_std, -1, 0, "Position", 0, 0},
                                              {"dd", ObsDialogInfo::pt_std, -1, 0, "wind direction", 0, 360},
                                              {"ff", ObsDialogInfo::pt_knot, -1, 0, "wind speed", 0, 100},
                                              {"TTT", ObsDialogInfo::pt_temp, -1, 1, "Temperature", -50, 50},
                                              {"TdTdTd", ObsDialogInfo::pt_temp, -1, 1, "Dew point temperature", -50, 50},
                                              {"PPPP", ObsDialogInfo::pt_std, -1, 1, "Pressure ", 100, 1050},
                                              {"PPPP_mslp", ObsDialogInfo::pt_std, -1, 1, "", -50, -50},
                                              {"ppp", ObsDialogInfo::pt_std, -1, 1, " 3 hour pressure change", -10, 10},
                                              {"a", ObsDialogInfo::pt_std, 201, 1, "Characteristic of pressure tendency", 0, 9},
                                              {"h", ObsDialogInfo::pt_std, -1, 0, "height of base of cloud", 1, 9},
                                              {"VV", ObsDialogInfo::pt_std, -1, 0, "horizontal visibility", 0, 10000},
                                              {"N", ObsDialogInfo::pt_std, -1, 0, "total cloud cover", 0, 8},
                                              {"RRR", ObsDialogInfo::pt_rrr, -1, 1, "precipitation", -1, 100},
                                              {"ww", ObsDialogInfo::pt_std, 0, 1, "Significant weather", 0, 0},
                                              {"W1", ObsDialogInfo::pt_std, 0, 1, "past weather (1)", 3, 9},
                                              {"W2", ObsDialogInfo::pt_std, 0, 1, "past weather (2)", 3, 9},
                                              {"Nh", ObsDialogInfo::pt_std, -1, 0, "cloud amount", 0, 9},
                                              {"Cl", ObsDialogInfo::pt_std, 170, 0, "cloud type, low", 1, 9},
                                              {"Cm", ObsDialogInfo::pt_std, 180, 0, "cloud type, medium", 1, 9},
                                              {"Ch", ObsDialogInfo::pt_std, 190, 0, "cloud type, high", 1, 9},
                                              {"vs", ObsDialogInfo::pt_std, -1, 0, "speed of motion of moving observing platform", 0, 9},
                                              {"ds", ObsDialogInfo::pt_std, 0, 0, "direction of motion of moving observing platform", 1, 360},
                                              {"TwTwTw", ObsDialogInfo::pt_temp, -1, 1, "sea/water temperature", -50, 50},
                                              {"PwaHwa", ObsDialogInfo::pt_std, 0, 0, "period/height of waves", 0, 0},
                                              {"dw1dw1", ObsDialogInfo::pt_std, 0, 0, "direction of swell waves", 0, 360},
                                              {"Pw1Hw1", ObsDialogInfo::pt_std, 0, 0, "period/height of swell waves", 0, 0},
                                              {"TxTn", ObsDialogInfo::pt_temp, -1, 1, "max/min temperature at 2m", -50, 50},
                                              {"sss", ObsDialogInfo::pt_std, -1, 0, "Snow depth", 0, 100},
                                              {"911ff", ObsDialogInfo::pt_knot, -1, 0, "maximum wind speed (gusts)", 0, 100},
                                              {"s", ObsDialogInfo::pt_std, -1, 0, "state of the sea", 0, 20},
                                              {"fxfx", ObsDialogInfo::pt_knot, -1, 0, "maximum wind speed (10 min mean wind)", 0, 100},
                                              {"Id", ObsDialogInfo::pt_std, -1, 0, "Identifier", 0, 0},
                                              {"Date", ObsDialogInfo::pt_std, -1, 0, "Date(mm-dd)", 0, 100},
                                              {"Time", ObsDialogInfo::pt_std, -1, 0, "hh.mm", 0, 0},
                                              {"Height", ObsDialogInfo::pt_std, -1, 0, "height of station", 0, 5000},
                                              {"Zone", ObsDialogInfo::pt_std, -1, 0, "Zone", 1, 99},
                                              {"Name", ObsDialogInfo::pt_std, -1, 0, "Name", 0, 0},
                                              {"RRR_6", ObsDialogInfo::pt_rrr, -1, 0, "precipitation past 6 hours", -1, 100},
                                              {"RRR_12", ObsDialogInfo::pt_rrr, -1, 0, "precipitation past 12 hours", -1, 100},
                                              {"RRR_24", ObsDialogInfo::pt_rrr, -1, 0, "precipitation past 24 hours", -1, 100},
                                              {"quality", ObsDialogInfo::pt_std, -1, 0, "quality", -1, 100},
                                              {"HHH", ObsDialogInfo::pt_std, -1, 0, "geopotential", 0, 50},
                                              {"QI", ObsDialogInfo::pt_std, -1, 2, "Percent confidence", 0, 100},
                                              {"QI_NM", ObsDialogInfo::pt_std, -1, 2, "Percent confidence no model", 0, 100},
                                              {"QI_RFF", ObsDialogInfo::pt_std, -1, 2, "Percent confidence recursive filter flag", 0, 100},
                                              {"RRR_1", ObsDialogInfo::pt_rrr, -1, 0, "precipitation past hour", -1, 100},
                                              {"depth", ObsDialogInfo::pt_std, -1, 0, "depth", 0, 100},
                                              {"TTTT", ObsDialogInfo::pt_temp, -1, 2, "sea/water temperature", -50, 50},
                                              {"SSSS", ObsDialogInfo::pt_std, -1, 2, "Salt", 0, 50},
                                              {"TE", ObsDialogInfo::pt_std, -1, 2, "Tide", -10, 10},
                                              {"T_red", ObsDialogInfo::pt_std, -1, 0, "Potential temperature", -50, 50},
                                              {"ffk", ObsDialogInfo::pt_knot, -1, 0, "wind speed in knots", 0, 100},
                                              {"fmfm", ObsDialogInfo::pt_knot, -1, 0, "Wind gust", 0, 36},
                                              {"NS1", ObsDialogInfo::pt_std, -1, 0, "Cloud cover - cloud layer 1 (lowest) [manual]", 0, 8},
                                              {"CC1", ObsDialogInfo::pt_std, 170, 0, "Cloud type - cloud layer 1 (lowest) [manual]", 1, 9},
                                              {"HS1", ObsDialogInfo::pt_std, -1, 0, "Height of cloud base - cloud layer 1 (lowest) [manual]", 0, 100000},
                                              {"NS2", ObsDialogInfo::pt_std, -1, 0, "Cloud cover - cloud layer 2 [manual]", 0, 8},
                                              {"CC2", ObsDialogInfo::pt_std, 170, 0, "Cloud type - cloud layer 2 [manual]", 1, 9},
                                              {"HS2", ObsDialogInfo::pt_std, -1, 0, "Height of cloud base - cloud layer 2 [manual]", 0, 100000},
                                              {"NS3", ObsDialogInfo::pt_std, -1, 0, "Cloud cover - cloud layer 3 (heighest) [manual]", 0, 8},
                                              {"CC3", ObsDialogInfo::pt_std, 170, 0, "Cloud type - cloud layer 3 (heighest) [manual]", 1, 9},
                                              {"HS3", ObsDialogInfo::pt_std, -1, 0, "Height of cloud base - cloud layer 3 (heighest) [manual]", 0, 100000},
                                              {"NS4", ObsDialogInfo::pt_std, -1, 0, "Cloud cover - cloud layer 4 (Cb) [manual]", 0, 8},
                                              {"CC4", ObsDialogInfo::pt_std, 170, 0, "Cloud type - cloud layes 4 (Cb) [manual]", 1, 9},
                                              {"HS4", ObsDialogInfo::pt_std, -1, 0, "Height of cloud base - cloud layes 4 (Cb) [manual]", 0, 100000},
                                              {"NS_A1", ObsDialogInfo::pt_std, -1, 0, "Cloud cover - cloud layer 1 (lowest) [auto]", 0, 8},
                                              {"HS_A1", ObsDialogInfo::pt_std, -1, 0, "Height of cloud base - cloud layer 1 (lowest) [auto]", 0, 100000},
                                              {"NS_A2", ObsDialogInfo::pt_std, -1, 0, "Cloud cover - cloud layer 2 [auto]", 0, 8},
                                              {"HS_A2", ObsDialogInfo::pt_std, -1, 0, "Height of cloud base - cloud layer 2 [auto]", 0, 100000},
                                              {"NS_A3", ObsDialogInfo::pt_std, -1, 0, "Cloud cover - cloud layer 3[auto]", 0, 8},
                                              {"HS_A3", ObsDialogInfo::pt_std, -1, 0, "Height of cloud base - cloud layer 3 [auto]", 0, 100000},
                                              {"NS_A4", ObsDialogInfo::pt_std, -1, 0, "Cloud cover - cloud layer 4 (heighest) [auto]", 0, 8},
                                              {"HS_A4", ObsDialogInfo::pt_std, -1, 0, "Height of cloud base - cloud layer 4 (heighest) [auto]", 0, 100000},
                                              {"dxdxdx", ObsDialogInfo::pt_std, -1, 0, "Extreme clockwise wind direction of a variable wind", 0, 360},
                                              {"dndndn", ObsDialogInfo::pt_std, -1, 0, "Extreme counterclockwise wind direction of a variable wind", 0, 360},
                                              {"PHPHPHPH", ObsDialogInfo::pt_std, -1, 1, "Altimeter setting (QNH)", 100, 1050},
                                              {"QWSG", ObsDialogInfo::pt_std, -1, 0, "?", 0, 1},
                                              {"GWI", ObsDialogInfo::pt_std, -1, 0, "General weather indicator", 0, 1}};

void ObsDialogInfo::addPlotType(const ObsDialogInfo::PlotType& p, bool addIfNoReaders)
{
  if (addIfNoReaders || !p.readernames.empty())
    plottype.push_back(p);
}

// static
ObsDialogInfo::Par ObsDialogInfo::findPar(const std::string& name)
{
  for (const Par& p : vparam()) {
    if (p.name == name)
      return p;
  }
  return Par(name);
}

// static
const std::vector<ObsDialogInfo::Par>& ObsDialogInfo::vparam()
{
  return pars;
}

// static
ObsDialogInfo::Misc ObsDialogInfo::miscFromText(const std::string& text)
{
  if (text == "dev_field_button")
    return dev_field_button;
  if (text == "tempPrecision")
    return tempPrecision;
  if (text == "unit_ms")
    return unit_ms;
  if (text == "orientation")
    return orientation;
  if (text == "parameterName")
    return parameterName;
  if (text == "popup")
    return popup;
  if (text == "qualityflag")
    return qualityflag;
  if (text == "wmoflag")
    return wmoflag;
  if (text == "markerboxVisible")
    return markerboxVisible;
  if (text == "criteria")
    return criteria;
  if (text == "asFieldButton")
    return asFieldButton;
  if (text == "leveldiffs")
    return leveldiffs;
  if (text == "show_VV_as_code")
    return show_VV_as_code;
  return none;
}
