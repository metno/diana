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
    miutil::miString name; ///< the field name in dialog etc.
    miutil::miString fieldgroup; ///< special fieldgroup name (to separate some fields)
    //miutil::miString text;            ///< not used, yet...
    miutil::miString plot; ///< the plot type, for debugging only
    vector<miutil::miString> input; ///< the input fields, read or computed
    set< miutil::miString > vcoord;
  };

  FieldPlotManager(FieldManager* fm);

  void getAllFieldNames(vector<miutil::miString>& fieldNames);

  /// return lists of inputfields
  vector<miutil::miString> getFields();
  vector<miutil::miString> getPlotFields();

  /// read setup section for field plots
  bool parseSetup();
  bool parseFieldPlotSetup();
  bool parseFieldGroupSetup();

  bool makeDifferenceField(const miutil::miString& fspec1,
      const miutil::miString& fspec2, const miutil::miTime& ptime,
      vector<Field*>& fv);

  bool makeFields(const miutil::miString& pin, const miutil::miTime& ptime,
      vector<Field*>& vfout,bool toCache = false);

  bool addGridCollection(const miutil::miString fileType,
      const miutil::miString& modelName,
      const std::vector<miutil::miString>& filenames,
      const std::vector<std::string>& format,
      std::vector<std::string> config,
      const std::vector<miutil::miString>& option);

  /// return available times for the requested models and fields
  void makeFieldText(Field* fout, const miutil::miString& plotName);

  vector<miutil::miTime> getFieldTime(vector<FieldRequest>& request, bool& constTimes);

  /// return all field groups for one model/file (to FieldDialog)
  void getFieldGroups(const miutil::miString& modelNameRequest,
      miutil::miString& modelName, std::string refTime, bool plotGroups, vector<FieldGroupInfo>& vfgi);

  ///return referencetime given by refoffset and refhour or last referencetime for given model
  std::string getBestFieldReferenceTime(const std::string& model, int refOffset, int refHour);

  /// return available times for the selceted models and fields
  vector<miutil::miTime> getFieldTime(const vector<miutil::miString>& pinfos,
      bool& constTimes);

  ///returns union or intersection of plot times from all pinfos
  void getCapabilitiesTime(vector<miutil::miTime>& normalTimes,
      miutil::miTime& constTimes, int& timediff, const miutil::miString& pinfo);

  ///return levels
  vector<miutil::miString> getFieldLevels(const miutil::miString& pinfo);

  /// return all defined field plot names from setup
  void getAllFieldNames(vector<miutil::miString>& fieldNames,
      set<std::string>& fieldprefixes, set<std::string>& fieldsuffixes);

  /// Parse plotInfo string into FieldReqests and plotName
  bool parsePin(std::string& pin, vector<FieldRequest>& fieldrequest, std::string& plotName);

private:

  vector<PlotField> vPlotField;
  set<std::string> fieldprefixes;
  set<std::string> fieldsuffixes;

  vector<miutil::miString>
      splitComStr(const miutil::miString& s, bool splitall);

  bool splitSuffix(std::string& plotName, std::string& suffix);
  vector<std::string> getParamNames(std::string plotName, std::string vcoord, bool& standard_name);

  bool splitDifferenceCommandString(miutil::miString pin, miutil::miString& fspec1, miutil::miString& fspec2);

  void parseString(std::string& pin, FieldRequest& fieldrequest, vector<std::string>& paramNames, std::string& plotName );

  map<miutil::miString, miutil::miString> groupNames;


  FieldManager* fieldManager;

};

#endif
