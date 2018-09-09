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
#ifndef diFieldPlotManager_h
#define diFieldPlotManager_h

#include "diField/GridInventoryTypes.h"
#include "diField/diCommonFieldTypes.h"
#include "diPlotCommand.h"
#include "diPlotOptions.h"
#include "diTimeTypes.h"

#include <set>
#include <string>
#include <vector>

class Field;
class FieldPlot;
class FieldManager;

struct FieldPlotManagerPlotField;

/**
 \brief FieldPlotManager
 */
class FieldPlotManager {

public:
  FieldPlotManager();
  ~FieldPlotManager();

  FieldPlot* createPlot(const PlotCommand_cp& cmd);
  void flushPlotCache();

  /// read setup section for field plots
  bool parseSetup();

  bool makeDifferenceField(const miutil::KeyValue_v& fspec1,
      const miutil::KeyValue_v& fspec2, const miutil::miTime& ptime,
      std::vector<Field*>& fv);

  bool makeFields(const miutil::KeyValue_v& pin, const miutil::miTime& ptime,
      std::vector<Field*>& vfout);

  bool updateFieldFileSetup(const std::vector<std::string>& lines, std::vector<std::string>& errors);

  bool addGridCollection(const std::string& modelname, const std::string& filename, bool writeable);

  FieldModelGroupInfo_v getFieldModelGroups();
  std::set<std::string> getFieldReferenceTimes(const std::string& model);

  void getSetupFieldOptions(std::map<std::string, miutil::KeyValue_v>& fieldoptions) const;

  /** fill a field's PlotOptions from static map, and substitute values
      from a string containing plotoptions */
  void getFieldPlotOptions(const std::string& name, PlotOptions& po, miutil::KeyValue_v& fdo) const;

  std::map<std::string, std::string> getFieldGlobalAttributes(const std::string& modelName, const std::string& refTime);

  plottimes_t getFieldTime(std::vector<FieldRequest>& request, bool updateSources = false);

  /// return all field groups for one model/file (to FieldDialog)
  void getFieldPlotGroups(const std::string& modelName, const std::string& refTime, bool predefinedPlots, FieldPlotGroupInfo_v& vfgi);

  ///return referencetime given by refoffset and refhour or last referencetime for given model
  std::string getBestFieldReferenceTime(const std::string& model, int refOffset, int refHour);

  ///return grid info from model
  gridinventory::Grid getFieldGrid(const std::string& model);

  /// Returns the available times for the selected models and fields.
  plottimes_t getFieldTime(const std::vector<miutil::KeyValue_v>& pinfos, bool updateSources = false);

  miutil::miTime getFieldReferenceTime(const miutil::KeyValue_v& pinfo);

  /// Returns the union or intersection of plot times from all pinfos.
  void getCapabilitiesTime(plottimes_t& normalTimes, int& timediff, const PlotCommand_cp& pc);

  ///return levels
  std::vector<std::string> getFieldLevels(const miutil::KeyValue_v& pinfo);

  /// Parse plotInfo string into FieldReqests and plotName
  void parsePin(const miutil::KeyValue_v& pin, std::vector<FieldRequest>& fieldrequest, std::string& plotName);

  /// helper function to extract plot name using parsePin
  std::string extractPlotName(const miutil::KeyValue_v& pin);

  /// Write field to file
  bool writeField(const FieldRequest& fieldrequest, const Field* field);
  void freeFields(const std::vector<Field*>& fields);

private:
  typedef std::shared_ptr<FieldPlotManagerPlotField> PlotField_p;
  std::vector<PlotField_p> vPlotField;

  bool parseFieldPlotSetup();
  bool parseFieldGroupSetup();

  std::vector<std::string> splitComStr(const std::string& s, bool splitall);

  /// update static fieldplotoptions
  bool updateFieldPlotOptions(const std::string& name, const miutil::KeyValue_v& optstr);

  /// return lists of inputfields
  std::vector<std::string> getFields();

  std::vector<FieldRequest> getParamNames(const std::string& plotName, FieldRequest fieldrequest);

  void parseString(const miutil::KeyValue_v &pin, FieldRequest& fieldrequest, std::vector<std::string>& paramNames, std::string& plotName );

  std::map<std::string, std::string> groupNames;

  std::map<std::string, PlotOptions> fieldPlotOptions;
  std::map<std::string, miutil::KeyValue_v> fieldDataOptions;

  std::unique_ptr<FieldManager> fieldManager;
};

#endif
