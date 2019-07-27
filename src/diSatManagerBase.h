/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2019 met.no

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

#ifndef diSatManagerBase_h
#define diSatManagerBase_h

#include "diColour.h"
#include "diSatDialogInfo.h"
#include "diSatPlotCommand.h"
#include "diTimeTypes.h"

#include <vector>

class Sat;
class SatPlotBase;

class SatManagerBase
{
public:
  SatManagerBase();
  virtual ~SatManagerBase();

  virtual SatPlotBase* createPlot(SatPlotCommand_cp cmd) = 0;

  //! Try to reuse plot for showing cmd
  //! return true iff the plot has been reused
  virtual bool reusePlot(SatPlotBase* plot, SatPlotCommand_cp cmd, bool first) = 0;

  virtual SatFile_v getFiles(const SatImageAndSubType& sist, bool update) = 0;

  //! Returns colour palette for this subproduct.
  virtual const std::vector<Colour>& getColours(const SatImageAndSubType& sist) = 0;

  virtual const std::vector<std::string>& getChannels(const SatImageAndSubType& sist, int index) = 0;

  virtual const SatImage_v& initDialog() = 0;

  virtual plottimes_t getSatTimes(const SatImageAndSubType& sist) = 0;

  virtual bool parseSetup() = 0;

  virtual void archiveMode(bool on) = 0;

  /// returns union or intersection of plot times from all pinfos
  void getCapabilitiesTime(plottimes_t& progTimes, int& timediff, const PlotCommand_cp& pinfo);

private:
  SatManagerBase(const SatManagerBase&) = delete;
  SatManagerBase& operator=(const SatManagerBase&) = delete;
};

#endif // diSatManagerBase_h
