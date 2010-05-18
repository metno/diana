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

#include <diSetupParser.h>
#include <diField/diCommonFieldTypes.h>
#include <diField/diFieldManager.h>

#include <puTools/miString.h>

#include <vector>

using namespace std;


/**
   \brief Field
*/
class FieldPlotManager {


  // Constructor
public:
  struct PlotField {
    miutil::miString name;            ///< the field name in dialog etc.
    miutil::miString fieldgroup;      ///< special fieldgroup name (to separate some fields)
//     miutil::miString text;            ///< not used, yet...
    miutil::miString plot;            ///< the plot type, for debugging only
    vector<miutil::miString> input;   ///< the input fields, read or computed
  };

  FieldPlotManager(FieldManager* fm);

  void getAllFieldNames(vector<miutil::miString>& fieldNames);
  /// return lists of inputfields
  vector<miutil::miString>  getFields();
  vector<miutil::miString>  getPlotFields();
  /// read setup section for field plots
  bool parseSetup(SetupParser& sp);
  bool makeDifferenceField(const miutil::miString& fspec1, const miutil::miString& fspec2,
			   const miutil::miTime& ptime,
			   vector<Field*>& fv,
			   const miutil::miString& levelSpec, const miutil::miString& levelSet,
			   const miutil::miString& idnumSpec, const miutil::miString& idnumSet,
			   int vectorIndex);

  bool makeFields(const miutil::miString& pin, const miutil::miTime& ptime,
		  vector<Field*>& vfout,
		  const miutil::miString& levelSpec, const miutil::miString& levelSet,
		  const miutil::miString& idnumSpec, const miutil::miString& idnumSet,
		  bool toCache=false);
  /// return available times for the requested models and fields

  void makeFieldText(Field* fout,
		     const miutil::miString& plotName,
		     const miutil::miString& levelSpecified,
		     const miutil::miString& idnumSpecified);

  vector<miutil::miTime> getFieldTime(vector<FieldTimeRequest>& request,
			      bool allTimeSteps,
			      bool& constTimes);

  /// return all field groups for one model/file (to FieldDialog)
  void getFieldGroups(const miutil::miString& modelNameRequest,
		      miutil::miString& modelName, vector<FieldGroupInfo>& vfgi);

  /// return available times for the selceted models and fields
  vector<miutil::miTime> getFieldTime(const vector<miutil::miString>& pinfos,
			      bool& constTimes);

  ///returns union or intersection of plot times from all pinfos
  void getCapabilitiesTime(vector<miutil::miTime>& normalTimes,
			   miutil::miTime& constTimes,
			   int& timediff,
			   const miutil::miString& pinfo);

  ///return levels 
  vector<miutil::miString> getFieldLevels(const miutil::miString& pinfo);



  /// return all defined field plot names from setup
  void getAllFieldNames(vector<miutil::miString>& fieldNames,
			set<miutil::miString>& fieldprefixes,
			set<miutil::miString>& fieldsuffixes);


private:


  vector<PlotField>  vPlotField;
  map<miutil::miString, PlotField> mapPlotField;
  set<miutil::miString> fieldprefixes;
  set<miutil::miString> fieldsuffixes;

  vector<miutil::miString> splitComStr(const miutil::miString& s, bool splitall);

  bool splitSuffix(miutil::miString& plotName,
		   miutil::miString& suffix);

  bool parsePin(const miutil::miString& pin,
		miutil::miString& modelName,
		miutil::miString& plotName,
		vector<miutil::miString>& fieldName,
		miutil::miString& levelName,
		miutil::miString& idnumName,
		int& hourOffset,
		int& hourDiff,
		miutil::miTime& time);


  FieldManager* fieldManager;

};



#endif
