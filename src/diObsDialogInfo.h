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

#ifndef DIOBSDIALOGINFO_H
#define DIOBSDIALOGINFO_H

#include "diObsPlotType.h"
#include "util/diKeyValue.h"

#include <string>
#include <vector>

/**
   \brief GUI data for all observations
*/
struct ObsDialogInfo
{

  enum ParType { pt_std, pt_knot, pt_temp, pt_rrr };
  struct Par
  {
    std::string name;
    ParType type;
    int symbol;
    int precision;
    std::string button_tip;
    int button_low;
    int button_high;

    Par(const std::string& name);
    Par(const std::string& name, ParType type, int symbol, int precision, const std::string& button_tip, int button_low, int button_high);
  };

  static Par findPar(const std::string& name);
  static const std::vector<Par>& vparam();

  /// list of prioritized stations
  struct PriorityList
  {
    std::string file;
    std::string name;
  };

  /// list of plotting criterias
  struct CriteriaList
  {
    std::string name;
    std::vector<std::string> criteria;
  };
  typedef std::vector<CriteriaList> CriteriaList_v;

  /// data button info for observation dialogue
  struct Button
  {
    std::string name;
    std::string tooltip;
    int high, low;
  };

  enum Misc {
    none = 0,
    dev_field_button = 1 << 0,
    tempPrecision = 1 << 1,
    unit_ms = 1 << 2,
    orientation = 1 << 3,
    parameterName = 1 << 4,
    popup = 1 << 5,
    qualityflag = 1 << 6,
    wmoflag = 1 << 7,
    markerboxVisible = 1 << 8,
    criteria = 1 << 9,
    asFieldButton = 1 << 10,
    leveldiffs = 1 << 11,
    show_VV_as_code = 1 << 12,
  };
  static Misc miscFromText(const std::string& text);

  /// observation plot type
  struct PlotType
  {
    std::string name;
    ObsPlotType plottype;
    std::vector<std::string> readernames;
    std::vector<Button> button;
    int misc;
    std::vector<int> verticalLevels;
    std::vector<CriteriaList> criteriaList;

    PlotType();
    void addButton(const std::string& name, const std::string& tip, int low = -50, int high = 50);
    void setAllActive(const std::vector<std::string>& parameter, const std::string& name, const std::vector<Button>& b);
    void addExtraParameterButtons();
  };
  typedef std::vector<PlotType> PlotType_v;

  void addPlotType(const ObsDialogInfo::PlotType& p, bool addIfNoReaders);

  PlotType_v plottype;
  std::vector<PriorityList> priority;
};

#endif // DIOBSDIALOGINFO_H
