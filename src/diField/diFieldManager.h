/* -*- c++ -*-
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2013-2020 met.no

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
#ifndef diFieldManager_h
#define diFieldManager_h

#include "GridDataKey.h"
#include "GridInventoryTypes.h"
#include "diCommonFieldTypes.h"
#include "diField.h"
#include "diFieldFunctions.h"
#include "diGridConverter.h"
#include "diTimeTypes.h"

#include "puTools/miTime.h"

#include <boost/shared_array.hpp>

#include <vector>
#include <map>
#include <set>

class GridIOsetup;
class GridCollection;

/**
 \brief Managing fields and "models"

 Parse setup
 Prepare field data and information (Field) for FieldPlot
 Initiate field reading and computations.
 */
class FieldManager {
private:
  FieldFunctions ffunc;

public:
  FieldManager();

  ~FieldManager();

  /// read Field sections in setup
  bool parseSetup();

  /// parse <FIELD_FILES> from setup, or updates
  bool updateFileSetup(const std::vector<std::string>& lines,
      std::vector<std::string>& errors, bool clearSources = false, bool top = false);

  /// add new model to filesetup -- only used from unit tests
  bool addModels(const std::vector<std::string>& configInfo);

  bool addGridCollection(const std::string& fileType,
      const std::string& modelName, const std::vector<std::string>& filenames,
      const std::vector<std::string>& format,
      const std::vector<std::string>& config,
      const std::vector<std::string>& option);

  /// return info about model/file groups (to FieldDialog)
  const FieldModelGroupInfo_v& getFieldModelGroups() const { return fieldModelGroups; }

  std::map<std::string,std::string> getGlobalAttributes(const std::string& modelName, const std::string& refTime);

  /// return all variables/levels for one model/file (to FieldDialog)
  std::map<std::string, FieldPlotInfo> getFieldPlotInfo(const std::string& modelName, const std::string& refTime);

  /// return grid from requested model, if the model contains more grids, the first one is returned
  gridinventory::Grid getGrid(const std::string& modelName);

  /// return available times for the requested models and fields
  plottimes_t getFieldTime(const std::vector<FieldRequest>& fieldrequests, bool updateSources);
  plottimes_t getFieldTime(const FieldRequest& fieldrequest, bool updateSources);

  /// return the reference time, time of analysis if a forecast
  std::set<std::string> getReferenceTimes(const std::string& modelName);

  std::string getBestReferenceTime(const std::string& modelName, int refOffset,
      int refHour);

  /// read and compute one field
  Field_p makeField(const FieldRequest& fieldrequest);
  bool writeField(const FieldRequest& fieldrequest, Field_cp field);

  /// read and compute a difference field (fv1 = fv1-fv2)
  static bool makeDifferenceFields(Field_pv& fv1, Field_pv& fv2);

protected:
  static GridConverter gc;   // gridconverter class

private:
  typedef std::shared_ptr<GridCollection> GridCollectionPtr;
  typedef std::shared_ptr<GridIOsetup> GridIOsetupPtr;

  typedef std::map<std::string, GridCollectionPtr> GridSources_t;

private:
  bool parseSetup(const std::vector<std::string>& lines, const std::string& token, std::vector<std::string>& errors);
  std::vector<std::string> subsections();

private:
  FieldModelGroupInfo_v fieldModelGroups;

  GridSources_t gridSources;

  std::map<std::string, std::string> defaultConfig;
  std::map<std::string, std::string> defaultFile;

  typedef std::map<std::string, GridIOsetupPtr> gridio_setups_t;
  gridio_setups_t gridio_setups;
  typedef std::map<std::string, std::string> gridio_sections_t;
  gridio_sections_t gridio_sections;

  GridCollectionPtr getGridCollection(const std::string& modelName, const std::string& refTime, bool updateSources);
};

#endif
