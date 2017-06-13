/*
 Diana - A Free Meteorological Visualisation Tool

 $Id: diPlotOptions.h 369 2007-11-02 08:55:24Z lisbethb $

 Copyright (C) 2006 met.no

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

#include <diField/diCommonFieldTypes.h>
#include <diField/diFieldManager.h>
#include "diPlotCommand.h"
#include "diPlotOptions.h"

#include <set>
#include <string>
#include <vector>

class FieldPlot;

/**
 \brief FieldPlotManager
 */
class FieldPlotManager {

public:
  struct PlotField {
    std::string name; ///< the field name in dialog etc.
    std::string fieldgroup; ///< special fieldgroup name (to separate some fields)
    //miutil::std::string text;            ///< not used, yet...
    std::string plot; ///< the plot type, for debugging only
    std::vector<std::string> input; ///< the input fields, read or computed
    std::string inputstr; //same as above, used as tooltip
    std::set< std::string > vcoord;
    FieldFunctions::VerticalType vctype;
  };

  FieldPlotManager(FieldManager* fm);

  FieldPlot* createPlot(const PlotCommand_cp& cmd);

  void getAllFieldNames(std::vector<std::string>& fieldNames);

  /// return lists of inputfields
  std::vector<std::string> getFields();

  /// read setup section for field plots
  bool parseSetup();
  bool parseFieldPlotSetup();
  bool parseFieldGroupSetup();

  bool makeDifferenceField(const miutil::KeyValue_v& fspec1,
      const miutil::KeyValue_v& fspec2, const miutil::miTime& ptime,
      std::vector<Field*>& fv);

  bool makeFields(const miutil::KeyValue_v& pin, const miutil::miTime& ptime,
      std::vector<Field*>& vfout);

  bool addGridCollection(const std::string fileType,
      const std::string& modelName,
      const std::vector<std::string>& filenames,
      const std::vector<std::string>& format,
      std::vector<std::string> config,
      const std::vector<std::string>& option);

  /// return available times for the requested models and fields
  void makeFieldText(Field* fout, const std::string& plotName, bool flightlevel=false);

  std::vector<miutil::miTime> getFieldTime(std::vector<FieldRequest>& request, bool updateSources=false);

  /// return all field groups for one model/file (to FieldDialog)
  void getFieldGroups(const std::string& modelName, std::string refTime, bool plotdefienitions, std::vector<FieldGroupInfo>& vfgi);

  ///return referencetime given by refoffset and refhour or last referencetime for given model
  std::string getBestFieldReferenceTime(const std::string& model, int refOffset, int refHour);

  ///return grid info from model
  gridinventory::Grid getFieldGrid(const std::string& model);

  /// Returns the available times for the selected models and fields.
  std::vector<miutil::miTime> getFieldTime(const std::vector<miutil::KeyValue_v>& pinfos,
      bool updateSources=false);

  miutil::miTime getFieldReferenceTime(const miutil::KeyValue_v& pinfo);

  /// Returns the union or intersection of plot times from all pinfos.
  void getCapabilitiesTime(std::vector<miutil::miTime>& normalTimes,
      int& timediff, const PlotCommand_cp& pc);

  ///return levels
  std::vector<std::string> getFieldLevels(const miutil::KeyValue_v& pinfo);

  /// Parse plotInfo string into FieldReqests and plotName
  void parsePin(const miutil::KeyValue_v& pin, std::vector<FieldRequest>& fieldrequest, std::string& plotName);

  /// helper function to extract plot name using parsePin
  std::string extractPlotName(const miutil::KeyValue_v& pin);

  /// Write field to file
  bool writeField(FieldRequest fieldrequest, const Field* field);

  void freeFields(const std::vector<Field*>& fields);

  /// update static fieldplotoptions
  static bool updateFieldPlotOptions(const std::string& name, const miutil::KeyValue_v& optstr);
  static void getAllFieldOptions(const std::vector<std::string>&,
      std::map<std::string, miutil::KeyValue_v> &fieldoptions);
  /** fill a field's PlotOptions from static map, and substitute values
      from a string containing plotoptions */
  static void getFieldPlotOptions(const std::string& name, PlotOptions& po, miutil::KeyValue_v &fdo);
  static std::string getFieldClassSpecs(const std::string& fieldplotname);

  static bool splitDifferenceCommandString(const miutil::KeyValue_v& pin, miutil::KeyValue_v& fspec1, miutil::KeyValue_v& fspec2);

private:
  std::vector<PlotField> vPlotField;

  std::vector<std::string> splitComStr(const std::string& s, bool splitall);

  std::vector<FieldRequest> getParamNames(const std::string& plotName, FieldRequest fieldrequest);

  void parseString(const miutil::KeyValue_v &pin, FieldRequest& fieldrequest, std::vector<std::string>& paramNames, std::string& plotName );

  void flightlevel2pressure(FieldRequest& frq);

  std::map<std::string, std::string> groupNames;

  static std::map<std::string, PlotOptions> fieldPlotOptions;
  static std::map<std::string, miutil::KeyValue_v> fieldDataOptions;

  FieldManager* fieldManager;
};

#endif
