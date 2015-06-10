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

#include <set>
#include <string>
#include <vector>

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
    std::set< std::string > vcoord;
  };

  FieldPlotManager(FieldManager* fm);

  void getAllFieldNames(std::vector<std::string>& fieldNames);

  /// return lists of inputfields
  std::vector<std::string> getFields();

  /// read setup section for field plots
  bool parseSetup();
  bool parseFieldPlotSetup();
  bool parseFieldGroupSetup();

  bool makeDifferenceField(const std::string& fspec1,
      const std::string& fspec2, const miutil::miTime& ptime,
      std::vector<Field*>& fv);

  bool makeFields(const std::string& pin, const miutil::miTime& ptime,
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
  void getFieldGroups(const std::string& modelName, std::string refTime, bool plotGroups, std::vector<FieldGroupInfo>& vfgi);

  ///return referencetime given by refoffset and refhour or last referencetime for given model
  std::string getBestFieldReferenceTime(const std::string& model, int refOffset, int refHour);

  ///return grid info from model
  gridinventory::Grid getFieldGrid(const std::string& model);

  /// Returns the available times for the selected models and fields.
  std::vector<miutil::miTime> getFieldTime(const std::vector<std::string>& pinfos,
      bool updateSources=false);

  /// Returns the union or intersection of plot times from all pinfos.
  void getCapabilitiesTime(std::vector<miutil::miTime>& normalTimes,
      int& timediff, const std::string& pinfo,
      bool updateSources=false);

  ///return levels
  std::vector<std::string> getFieldLevels(const std::string& pinfo);

  /// Parse plotInfo string into FieldReqests and plotName
  bool parsePin(std::string& pin, std::vector<FieldRequest>& fieldrequest, std::string& plotName);

  /// Write field to file
  bool writeField(FieldRequest fieldrequest, const Field* field);

  void freeFields(const std::vector<Field*>& fields);

private:
  std::vector<PlotField> vPlotField;

  std::vector<std::string> splitComStr(const std::string& s, bool splitall);

  std::vector<FieldRequest> getParamNames(const std::string& plotName, FieldRequest fieldrequest);

  bool splitDifferenceCommandString(const std::string& pin, std::string& fspec1, std::string& fspec2);

  void parseString(std::string& pin, FieldRequest& fieldrequest, std::vector<std::string>& paramNames, std::string& plotName );

  void flightlevel2pressure(FieldRequest& frq);

  std::map<std::string, std::string> groupNames;


  FieldManager* fieldManager;
};

#endif
