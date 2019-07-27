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

#include "qtSatDialogData.h"

#include "diSatManagerBase.h"

DianaSatDialogData::DianaSatDialogData(SatManagerBase* sm)
    : sm_(sm)
{
}

const SatImage_v& DianaSatDialogData::initSatDialog()
{
  return sm_->initDialog();
}

SatFile_v DianaSatDialogData::getSatFiles(const std::string& image_name, const std::string& subtype_name, bool update)
{
  return sm_->getFiles(image_name, subtype_name, update);
}

std::vector<std::string> DianaSatDialogData::getSatChannels(const std::string& image_name, const std::string& subtype_name, int index)
{
  return sm_->getChannels(image_name, subtype_name, index);
}

std::vector<Colour> DianaSatDialogData::getSatColours(const std::string& image_name, const std::string& subtype_name)
{
  return sm_->getColours(image_name, subtype_name);
}
