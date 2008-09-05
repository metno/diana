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
#include <diCommonFieldTypes.h>
#include <diFieldManager.h>

#include <miString.h>

#include <vector>

using namespace std;


/**
   \brief Field
*/
class FieldPlotManager {


  // Constructor
public:
  struct PlotField {
    miString name;            ///< the field name in dialog etc.
    miString fieldgroup;      ///< special fieldgroup name (to separate some fields)
//     miString text;            ///< not used, yet...
    miString plot;            ///< the plot type, for debugging only
    vector<miString> input;   ///< the input fields, read or computed
  };

  FieldPlotManager(FieldManager* fm);

  void getAllFieldNames(vector<miString>& fieldNames);
  /// return lists of inputfields
  vector<miString>  getFields();
  vector<miString>  getPlotFields();
  /// read setup section for field plots
  bool parseSetup(SetupParser& sp);
  bool makeDifferenceField(const miString& fspec1, const miString& fspec2,
			   const miTime& ptime,
			   vector<Field*>& fv,
			   const miString& levelSpec, const miString& levelSet,
			   const miString& idnumSpec, const miString& idnumSet,
			   int vectorIndex);

  bool makeFields(const miString& pin, const miTime& ptime,
		  vector<Field*>& vfout,
		  const miString& levelSpec, const miString& levelSet,
		  const miString& idnumSpec, const miString& idnumSet,
		  bool toCache=false);
  /// return available times for the requested models and fields

  void makeFieldText(Field* fout,
		     const miString& plotName,
		     const miString& levelSpecified,
		     const miString& idnumSpecified);

  vector<miTime> getFieldTime(vector<FieldTimeRequest>& request,
			      bool allTimeSteps,
			      bool& constTimes);

  /// return all field groups for one model/file (to FieldDialog)
  void getFieldGroups(const miString& modelNameRequest,
		      miString& modelName, vector<FieldGroupInfo>& vfgi);

  /// return available times for the selceted models and fields
  vector<miTime> getFieldTime(const vector<miString>& pinfos,
			      bool& constTimes);

  ///returns union or intersection of plot times from all pinfos
  void getCapabilitiesTime(vector<miTime>& normalTimes,
			   miTime& constTimes,
			   int& timediff,
			   const miString& pinfo);

  ///return levels 
  vector<miString> getFieldLevels(const miString& pinfo);



  /// return all defined field plot names from setup
  void getAllFieldNames(vector<miString>& fieldNames,
			set<miString>& fieldprefixes,
			set<miString>& fieldsuffixes);


private:


  vector<PlotField>  vPlotField;
  map<miString, PlotField> mapPlotField;
  set<miString> fieldprefixes;
  set<miString> fieldsuffixes;

  vector<miString> splitComStr(const miString& s, bool splitall);

  bool splitSuffix(miString& plotName,
		   miString& suffix);

  bool parsePin(const miString& pin,
		miString& modelName,
		miString& plotName,
		vector<miString>& fieldName,
		miString& levelName,
		miString& idnumName,
		int& hourOffset,
		int& hourDiff,
		miTime& time);


  FieldManager* fieldManager;

};



#endif
