/*
 * FimexIO.h
 *
 *  Created on: Mar 11, 2010
 *      Author: audunc
 */
/*
 $Id:$

 Copyright (C) 2006 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 email: diana@met.no

 This is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef FIMEXIO_H_
#define FIMEXIO_H_

#include "GridIO.h"

#include <fimex/CDMReader.h>
#include <fimex/coordSys/CoordinateSystem.h>

#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>
#include <map>

namespace MetNoFimex {
  // forward decl
  class CoordinateSystemSliceBuilder;
}

namespace miutil {
long path_ctime(const std::string& path);
}

class FimexIOsetup: public GridIOsetup {
public:
  FimexIOsetup()
  {
  }
  virtual ~FimexIOsetup()
  {
  }
  /**
   * Return name of section in setup file
   * @return name
   */
  static std::string getSectionName()
  {
    return "FIMEX_SETUP";
  }

  /**
   * Parse setup information
   * @param lines
   * @param errors
   * @return status
   */
  bool parseSetup(std::vector<std::string> lines, std::vector<std::string>& errors);

  // map for supplementary option, for example reprojections
  std::map<std::string, std::map<std::string, std::string> > optionMap; // Contains options from FIMEX_SETUP section

};

class FimexIO: public GridIO {
public:
  typedef boost::shared_ptr<const MetNoFimex::CoordinateSystem> CoordinateSystemPtr;
  typedef std::vector<CoordinateSystemPtr> CoordinateSystems_t;
  typedef boost::shared_ptr<MetNoFimex::CDMReader> CDMReaderPtr;


private:
  //! begin and end indices of a named cross-section
  struct VcrossBeginEnd {
    size_t begin, end;
    VcrossBeginEnd(size_t b, size_t e) : begin(b), end(e) { }
  };

  typedef std::map<std::string, VcrossBeginEnd> vcross_indices_t;

private:
  bool sourceOk;
  long modificationTime;
  std::string model_name;
  std::string source_type;
  std::string config_filename;
  std::string reftime_from_file; //! reference time from filename or from file

  std::string projDef; //proj4 definition from setup. If defined, use this projection.
  CoordinateSystems_t coordSys;
  bool singleTimeStep;
  int noOfClimateTimes;
  bool writeable;
  bool turnWaveDirection;
  CDMReaderPtr feltReader;

  FimexIOsetup* setup;

  vcross_indices_t mVcrossIndices; //! begin+end indices for vertical cross sections, set in makeInventory

  void findAxisIndices(gridinventory::Taxis taxis, gridinventory::Zaxis zaxis,gridinventory::ExtraAxis extraaxis,
      const miutil::miTime& time, const std::string& level, const std::string& elevel,
      size_t& taxis_index, size_t& zaxis_index, size_t& extraaxis_index);
  MetNoFimex::CoordinateSystemSliceBuilder createSliceBuilder(CDMReaderPtr reader, const CoordinateSystemPtr& varCS,
      const std::string& reftime, const gridinventory::GridParameter& param,
      const std::string& zlevel, const miutil::miTime& time, const std::string& elevel, size_t& zaxis_index);
  MetNoFimex::CoordinateSystemSliceBuilder createSliceBuilder(const CoordinateSystemPtr& varCS,
      const std::string& reftime, const gridinventory::GridParameter& param,
      const std::string& zlevel, const miutil::miTime& time, const std::string& elevel, size_t& zaxis_index);
  bool paramExists(const std::string& reftime, const gridinventory::GridParameter& param);
  void setHybridParametersIfPresent(const std::string& reftime, const gridinventory::GridParameter& param,
      const std::string& apVar, const std::string& bVar, size_t zaxis_index, Field* field);
  void copyFieldSwapY(const bool y_is_up, const int nx, const int ny, const float* fdataSrc, float* fdataDst);

  typedef std::map<std::string, std::string> name2id_t;

  CDMReaderPtr createReader();

  size_t findTimeIndex(const gridinventory::Taxis& taxis, const miutil::miTime& time);
  size_t findZIndex(const gridinventory::Zaxis& zaxis, const std::string& zlevel);
  size_t findExtraIndex(const gridinventory::ExtraAxis& extraaxis, const std::string& elevel);

  //! helper function for inventoryExtractGrid
  void inventoryExtractGridProjection(boost::shared_ptr<const MetNoFimex::Projection> projection, gridinventory::Grid& grid,
      MetNoFimex::CoordinateSystem::ConstAxisPtr xAxis, MetNoFimex::CoordinateSystem::ConstAxisPtr yAxis);

  //! helper function for makeInventory
  void inventoryExtractGrid(std::set<gridinventory::Grid>& grids, CoordinateSystemPtr cs,
      MetNoFimex::CoordinateSystem::ConstAxisPtr xAxis, MetNoFimex::CoordinateSystem::ConstAxisPtr yAxis);

  //! helper function for makeInventory
  void inventoryExtractVAxis(std::set<gridinventory::Zaxis>& zaxes, name2id_t& name2id,
      MetNoFimex::CoordinateSystem::ConstAxisPtr vAxis, CoordinateSystemPtr& cs);

  void inventoryExtractExtraAxes(std::set<gridinventory::ExtraAxis>& extraaxes, name2id_t& name2id,
      const std::vector<std::string>& unsetList, const MetNoFimex::CoordinateSystem::ConstAxisList axes);

  std::string fallbackGetReferenceTime();

  /** Find an appropriate coordinate system for the variable.
   * Also checks that it is a simple spatial grid and that both X/Lon and Y/Lat
   * axes are present.
   */
  CoordinateSystemPtr findCoordinateSystem(const gridinventory::GridParameter& param);
  const std::string& extractVariableName(const gridinventory::GridParameter& param);

  std::string reproj_name;

public:
  FimexIO(const std::string & modelname, const std::string & sourcename,
      const std::string & reftime,
      const std::string & format, const std::string& config,
      const std::vector<std::string>& options, bool makeFeltReader, FimexIOsetup * s);
  ~FimexIO();

  /**
   * What kind of source is this.
   * @return Sourcetype
   */
  static std::string getSourceType()
  {
    return "fimex";
  }

  // ===================== IMPLEMENTATIONS OF VIRTUAL FUNCTIONS BELOW THIS LINE ============================

  /**
   * Returns whether the source has changed since the last makeInventory
   * @return bool
   */
  virtual bool sourceChanged(bool update);

  /**
   * Return referencetime from filename or file
   * @return reftime_from_file
   */
  virtual std::string getReferenceTime() const
    { return reftime_from_file; }

  /**
   * Returns true if referencetime matches
   * @return bool
   */
  virtual bool referenceTimeOK(const std::string& refTime)
    { return (reftime_from_file.empty() || reftime_from_file == refTime);}

  /**
   * Build the inventory from source
   * @return status
   */
  virtual bool makeInventory(const std::string& reftime);

  /**
   * Get data slice as Field
   */
  virtual Field * getData(const std::string& reftime, const gridinventory::GridParameter& param,
      const std::string& level, const miutil::miTime& time,
      const std::string& run, const std::string& unit);

  /**
   * Get data
   */
  virtual vcross::Values_p getVariable(const std::string& varName);

  /**
   * Put data slice from Field. Assumes that the field was generated by this same
   * FimexIO object.
   */
  virtual bool putData(const std::string& reftime, const gridinventory::GridParameter& param,
      const std::string& level, const miutil::miTime& time,
      const std::string& run, const std::string& unit, const Field* field,
      const std::string& output_time);
};

#endif /* FIMEXIO_H_ */
