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

#ifndef diSatPlotBase_h
#define diSatPlotBase_h

#include "diPlotOptionsPlot.h"

#include "diArea.h"
#include "diSatTypes.h"

class SatPlotCommand;
typedef std::shared_ptr<const SatPlotCommand> SatPlotCommand_cp;

/**
  \brief Plot satellite and radar images
*/
class SatPlotBase : public PlotOptionsPlot
{
public:
  SatPlotBase(SatPlotCommand_cp cmd);

  std::string getEnabledStateKey() const override;

  SatPlotCommand_cp command() const { return command_; }
  void setCommand(SatPlotCommand_cp cmd);

  virtual GridArea& getSatArea() = 0;
  virtual void getCalibChannels(std::vector<std::string>& channels) const = 0;

  /// get pixel value
  virtual void values(float x, float y, std::vector<SatValues>& satval) const = 0;

private:
  SatPlotBase(const SatPlotBase& rhs) = delete;
  SatPlotBase& operator=(const SatPlotBase& rhs) = delete;

  SatPlotCommand_cp command_;
};

#endif // diSatPlotBase_h
