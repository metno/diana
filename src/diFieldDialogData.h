/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2020 met.no

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
#ifndef difielddialogdata_h
#define difielddialogdata_h

#include "diField/diCommonFieldTypes.h"
#include "diTimeTypes.h"
#include "util/diKeyValue.h"

#include <map>
#include <set>
#include <vector>

class FieldDialogData
{
public:
  typedef std::map<std::string, std::string> attributes_t;

  virtual ~FieldDialogData();

  /// return model/file groups and contents
  virtual FieldModelGroupInfo_v getFieldModelGroups() = 0;

  /// return plot/parameter info for one model to FieldDialog
  virtual void getFieldPlotGroups(const std::string& modelName, const std::string& refTime, bool predefinedPlots, FieldPlotGroupInfo_v& vfgi) = 0;

  /// return GlobalAttributes for one model
  virtual attributes_t getFieldGlobalAttributes(const std::string& modelName, const std::string& refTime) = 0;

  /// return the reference time given by refOffset and refhour or the last reference time for the given model
  virtual std::string getBestFieldReferenceTime(const std::string& model, int refOffset, int refHour) = 0;

  /// update reference times for the given model
  virtual void updateFieldReferenceTimes(const std::string& model) = 0;

  /// return all reference times for the given model
  virtual std::set<std::string> getFieldReferenceTimes(const std::string& model) = 0;

  /// return plot options for all defined plot fields in setup
  virtual void getSetupFieldOptions(std::map<std::string, miutil::KeyValue_v>& fieldoptions) = 0;

  /// Returns available times for the requested fields.
  /** This is only used from FieldDialog::updateTime, and it is not obvious why FieldDialog has to call this function. */
  virtual plottimes_t getFieldTime(std::vector<FieldRequest>& request) = 0;
};

#endif // difielddialogdata_h
