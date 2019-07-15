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

#ifndef dianasatdialogdata_h
#define dianasatdialogdata_h

#include "diSatDialogData.h"

class SatManager;
class SatPlotCluster;

class DianaSatDialogData : public SatDialogData
{
public:
  DianaSatDialogData(SatManager* sm, SatPlotCluster* spc);

  const SatDialogInfo& initSatDialog() override;
  void setSatAuto(bool, const std::string&, const std::string&) override;
  const std::vector<SatFileInfo>& getSatFiles(const std::string& satellite, const std::string& file, bool update) override;
  const std::vector<std::string>& getSatChannels(const std::string& satellite, const std::string& file, int index) override;
  const std::vector<Colour>& getSatColours(const std::string& satellite, const std::string& file) override;

private:
  SatManager* sm_;
  SatPlotCluster* spc_;
};

#endif // dianasatdialogdata_h
