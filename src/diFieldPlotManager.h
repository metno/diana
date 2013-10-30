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

#include <puTools/miString.h>

#include <vector>

using namespace std;

/**
 \brief FieldPlotManager
 */
class FieldPlotManager {

  // Constructor
public:
  struct PlotField {
    std::string name; ///< the field name in dialog etc.
    std::string fieldgroup; ///< special fieldgroup name (to separate some fields)
    //miutil::std::string text;            ///< not used, yet...
    std::string plot; ///< the plot type, for debugging only
    vector<std::string> input; ///< the input fields, read or computed
    set< std::string > vcoord;
  };

  FieldPlotManager(FieldManager* fm);

  void getAllFieldNames(vector<std::string>& fieldNames);

  /// return lists of inputfields
  vector<std::string> getFields();
  vector<std::string> getPlotFields();

  /// read setup section for field plots
  bool parseSetup();
  bool parseFieldPlotSetup();
  bool parseFieldGroupSetup();

  bool makeDifferenceField(const std::string& fspec1,
      const std::string& fspec2, const miutil::miTime& ptime,
      vector<Field*>& fv);

  bool makeFields(const std::string& pin, const miutil::miTime& ptime,
      vector<Field*>& vfout,bool toCache = false);

  bool addGridCollection(const std::string fileType,
      const std::string& modelName,
      const std::vector<std::string>& filenames,
      const std::vector<std::string>& format,
      std::vector<std::string> config,
      const std::vector<std::string>& option);

  /// return available times for the requested models and fields
  void makeFieldText(Field* fout, const std::string& plotName);

  vector<miutil::miTime> getFieldTime(vector<FieldRequest>& request, bool& constTimes, bool updateSources=false);

  /// return all field groups for one model/file (to FieldDialog)
  void getFieldGroups(const std::string& modelNameRequest,
      std::string& modelName, std::string refTime, bool plotGroups, vector<FieldGroupInfo>& vfgi);

  ///return referencetime given by refoffset and refhour or last referencetime for given model
  std::string getBestFieldReferenceTime(const std::string& model, int refOffset, int refHour);

  /// Returns the available times for the selected models and fields.
  vector<miutil::miTime> getFieldTime(const std::vector<std::string>& pinfos,
      bool& constTimes, bool updateSources=false);

  /// Returns the union or intersection of plot times from all pinfos.
  void getCapabilitiesTime(vector<miutil::miTime>& normalTimes,
      miutil::miTime& constTimes, int& timediff, const std::string& pinfo,
      bool updateSources=false);

  ///return levels
  vector<std::string> getFieldLevels(const std::string& pinfo);

  /// return all defined field plot names from setup
  void getAllFieldNames(vector<std::string>& fieldNames,
      set<std::string>& fieldprefixes, set<std::string>& fieldsuffixes);

  /// Parse plotInfo string into FieldReqests and plotName
  bool parsePin(std::string& pin, vector<FieldRequest>& fieldrequest, std::string& plotName);

private:

  vector<PlotField> vPlotField;
  set<std::string> fieldprefixes;
  set<std::string> fieldsuffixes;

  vector<std::string>
      splitComStr(const std::string& s, bool splitall);

  bool splitSuffix(std::string& plotName, std::string& suffix);
  vector<std::string> getParamNames(std::string plotName, std::string vcoord, bool& standard_name);

  bool splitDifferenceCommandString(std::string pin, std::string& fspec1, std::string& fspec2);

  void parseString(std::string& pin, FieldRequest& fieldrequest, vector<std::string>& paramNames, std::string& plotName );

  map<std::string, std::string> groupNames;


  FieldManager* fieldManager;

};

#endif
