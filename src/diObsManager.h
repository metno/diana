/*
  Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2018 met.no

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
#ifndef diObsManager_h
#define diObsManager_h

#include "diObsData.h"
#include "diObsDialogInfo.h"
#include "diObsPlotType.h"
#include "diObsReader.h"
#include "diPlotCommand.h"
#include "diTimeTypes.h"

#include <puTools/TimeFilter.h>

#include <string>
#include <set>
#include <vector>

class ObsData;
class ObsMetaData;
class ObsPlot;
struct ObsDialogInfo;

/**
  \brief Managing observations
  
  - parse setup
  - send plot info strings to ObsPlot objects
  - managing file/time info
  - read data
*/
class ObsManager {
public:
  struct ProdInfo {
    std::string name;
    std::set<ObsPlotType> plottypes; //! for which style of plotting should this be reader be available?
    ObsReader_p reader;              //! observation data reader
    int sort_order;
  };

private:
  typedef std::map<std::string, ProdInfo> string_ProdInfo_m;
  string_ProdInfo_m Prod;

  ObsDialogInfo::PriorityList_v priority;

  // one  criterialist pr plot type
  std::map<std::string, ObsDialogInfo::CriteriaList_v> criteriaList;

  std::vector<std::string> popupSpec; // Parameter data from setupfile

  ObsDialogInfo::PlotType_v setupPlotTypes_;

  bool useArchive; //read archive files too.

  //--------------------------------------

  void addReaders(ObsDialogInfo::PlotType& dialogInfo);

  std::vector<ObsReader_p> readers(ObsPlot* oplot);

  bool parseFilesSetup();
  bool parsePrioritySetup();
  bool parseCriteriaSetup();
  bool parsePopupWindowSetup();
  bool parsePlotTypeSetup();

public:
  ObsManager();

  bool parseSetup();

  void archiveMode(bool on) { useArchive = on; }

  ObsDialogInfo initDialog();
  void updateDialog(ObsDialogInfo::PlotType& pt, const std::string& readername);

  void setPlotDefaults(ObsPlot* oplot);

  //! read data into an obsplot
  bool prepare(ObsPlot*, const miutil::miTime&);

  //! \return observation times for a list of "prod"
  plottimes_t getTimes(const std::vector<std::string>& obsTypes, bool update);

  //! \return union or intersection of plot times from all pinfos
  void getCapabilitiesTime(plottimes_t& normalTimes, int& timediff, const PlotCommand_cp& pinfo);

  //! return observation times for list of obsTypes
  plottimes_t getObsTimes(const std::set<std::string>& readernames);

  bool updateTimes(ObsPlot* op);
};

#endif
