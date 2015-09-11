/* -*- c++ -*-
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2013 met.no

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

#include "diField.h"
#include "diCommonFieldTypes.h"
#include "diFieldFunctions.h"
#include "diGridConverter.h"
#include "diFieldCache.h"
#include "GridDataKey.h"
#include "GridInventoryTypes.h"

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
  typedef boost::shared_array<float> FloatArray;

public:
  FieldManager();

  ~FieldManager();

  enum FieldCacheOptions {
    NO_CACHE = 1,
    READ_RESULT = 2,
    READ_ALL = 4,
    WRITE_RESULT = 8,
    WRITE_ALL = 16
  };

  /// read Field sections in setup
  bool parseSetup(const std::vector<std::string>& lines,
      const std::string& token, std::vector<std::string>& errors);

  std::string section() const
  {
    return "FIELD_FILES";
  }
  std::vector<std::string> subsections();

  void setFieldNames(const std::vector<std::string>& vfieldname);

  /// parse <FIELD_FILES> from setup, or updates
  bool updateFileSetup(const std::vector<std::string>& lines,
      std::vector<std::string>& errors, bool clearSources = false, bool top = false);

  /// add new model to filesetup
  bool addModels(const std::vector<std::string>& configInfo);

  bool addGridCollection(const std::string& fileType,
      const std::string& modelName, const std::vector<std::string>& filenames,
      const std::vector<std::string>& format,
      const std::vector<std::string>& config,
      const std::vector<std::string>& option);

  /// return info about model/file groups (to FieldDialog)
  const std::vector<FieldDialogInfo>& getFieldDialogInfo() const
  {
    return fieldDialogInfo;
  }

  /// check if model exists
  bool modelOK(const std::string& modelName);

  /// return all variables/levels for one model/file (to FieldDialog)
  void getFieldInfo(const std::string& modelName, const std::string& refTime,
      std::map<std::string,FieldInfo>& fieldInfo);

  /// return grid from requested model, if the model contains more grids, the first one is returned
  gridinventory::Grid getGrid(const std::string& modelName);

  /// return available times for the requested models and fields
  std::vector<miutil::miTime> getFieldTime(const std::vector<FieldRequest>& fieldrequest,
      bool updateSource = false);

  /// return the reference time, time of analysis if a forecast
  std::set<std::string> getReferenceTimes(const std::string& modelName);

  std::string getBestReferenceTime(const std::string& modelName, int refOffset,
      int refHour);

  /// read and compute one field
  bool makeField(Field*& fout, FieldRequest fieldrequest, int cacheOptions = 0);
  bool writeField(FieldRequest fieldrequest, const Field* field);
  bool freeField(Field* field);
  void flushCache()
    { fieldcache->flush(); }

  /// read and compute a difference field (fv1 = fv1-fv2)
  bool makeDifferenceFields(std::vector<Field*>& fv1, std::vector<Field*>& fv2);

  void updateSources();
  void updateSource(const std::string& modelName);

  std::vector<std::string> getFileNames(const std::string& modelName);

protected:
  static GridConverter gc;   // gridconverter class

private:
  typedef boost::shared_ptr<GridCollection> GridCollectionPtr;
  typedef boost::shared_ptr<GridIOsetup> GridIOsetupPtr;

  typedef std::map<std::string, GridCollectionPtr> GridSources_t;

private:
  FieldCachePtr fieldcache;

  std::vector<FieldDialogInfo> fieldDialogInfo;

  GridSources_t gridSources;

  std::map<std::string, std::string> defaultConfig;
  std::map<std::string, std::string> defaultFile;

  typedef std::map<std::string, GridIOsetupPtr> gridio_setups_t;
  gridio_setups_t gridio_setups;
  typedef std::map<std::string, std::string> gridio_sections_t;
  gridio_sections_t gridio_sections;

  GridCollectionPtr getGridCollection(const std::string& modelName,
      const std::string& refTime = "", bool rescan = false,
      bool checkSourceChanges = true);

  void addComputedParameters(gridinventory::ReftimeInventory& inventory);

  std::string mergeTaxisNames(gridinventory::ReftimeInventory& inventory,
      const std::string& taxsis1, const std::string& taxsis2);

  void writeToCache(Field*& fout);

  /// remove parentheses from end of string
  std::string removeParenthesesFromString(const std::string& origName);

  Field* getField(GridCollectionPtr gridCollection,
      gridinventory::ReftimeInventory& inventory, FieldRequest fieldrequest,
      int cacheOptions);
  bool getAllFields_timeInterval(GridCollectionPtr gridCollection,
      gridinventory::ReftimeInventory& inventory, std::vector<Field*>& vfield,      FieldRequest fieldrequest,
      int fch, bool accumulate_flux, int cacheOptions);
  bool getAllFields(GridCollectionPtr gridCollection,
      gridinventory::ReftimeInventory& inventory, std::vector<Field*>& vfield,      FieldRequest fieldrequest,
      const std::vector<float>& constants, int cacheOptions);
  bool multiplyFieldByTimeStep(Field* f, float sec_diff);
  bool paramExist(gridinventory::ReftimeInventory& inventory,
      const FieldRequest& fieldrequest, gridinventory::GridParameter& param);
};

#endif
