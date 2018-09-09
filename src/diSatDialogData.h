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
#ifndef disatdialogdata_h
#define disatdialogdata_h

#include "diSatTypes.h"
#include "diTimeTypes.h"
#include "util/diKeyValue.h"

#include <map>
#include <set>
#include <vector>

class SatDialogData
{
public:
  virtual ~SatDialogData();

  /// return button names for SatDialog
  virtual const SatDialogInfo& initSatDialog() = 0;

  /// satellite follows main plot time
  virtual void setSatAuto(bool, const std::string&, const std::string&) = 0;

  /// get list of satfiles of class satellite and subclass file. if update is true read new list from disk
  virtual const std::vector<SatFileInfo>& getSatFiles(const std::string& satellite, const std::string& file, bool update) = 0;

  /// returns channels for subproduct of class satellite and subclass file
  virtual const std::vector<std::string>& getSatChannels(const std::string& satellite, const std::string& file, int index) = 0;

  /// returns colour palette for subproduct of class satellite and subclass file
  virtual const std::vector<Colour>& getSatColours(const std::string& satellite, const std::string& file) = 0;
};

#endif // disatdialogdata_h
