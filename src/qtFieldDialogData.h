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

#ifndef dianafielddialogdata_h
#define dianafielddialogdata_h

#include "diFieldDialogData.h"

class FieldPlotManager;

class DianaFieldDialogData : public FieldDialogData
{
public:
  DianaFieldDialogData(FieldPlotManager* fpm);

  FieldModelGroupInfo_v getFieldModelGroups() override;
  void getFieldPlotGroups(const std::string& modelName, const std::string& refTime, bool predefinedPlots, FieldPlotGroupInfo_v& vfgi) override;
  attributes_t getFieldGlobalAttributes(const std::string& modelName, const std::string& refTime) override;
  std::string getBestFieldReferenceTime(const std::string& model, int refOffset, int refHour) override;
  plottimes_t getFieldTime(std::vector<FieldRequest>& request) override;
  void updateFieldReferenceTimes(const std::string& model) override;
  std::set<std::string> getFieldReferenceTimes(const std::string& model) override;
  void getSetupFieldOptions(std::map<std::string, miutil::KeyValue_v>& fieldOptions) override;

private:
  FieldPlotManager* fpm_;
};

#endif // dianafielddialogdata_h
