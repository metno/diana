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

class SatManagerBase;

class DianaSatDialogData : public SatDialogData
{
public:
  DianaSatDialogData(SatManagerBase* sm);

  const SatImage_v& initSatDialog() override;
  SatFile_v getSatFiles(const std::string& image_name, const std::string& subtype_name, bool update) override;
  std::vector<std::string> getSatChannels(const std::string& image_name, const std::string& subtype_name, int index) override;
  std::vector<Colour> getSatColours(const std::string& image_name, const std::string& subtype_name) override;

private:
  SatManagerBase* sm_;
};

#endif // dianasatdialogdata_h
