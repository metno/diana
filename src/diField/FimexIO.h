/*
 * FimexIO.h
 *
 *  Created on: Mar 11, 2010
 *      Author: audunc
 */
/*
 Copyright (C) 2006-2021 met.no

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
#include "GridIOsetup.h"

#include <fimex/CDMReader.h>
#include <fimex/coordSys/CoordinateSystem.h>

#include <string>
#include <vector>
#include <map>

namespace MetNoFimex {
  // forward decl
  class CoordinateSystemSliceBuilder;
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
  MetNoFimex::CoordinateSystem_cp_v coordSys;
  bool singleTimeStep;
  int noOfClimateTimes;
  bool writeable;
  bool turnWaveDirection;
  MetNoFimex::CDMReader_p feltReader;

  FimexIOsetup* setup;

  MetNoFimex::CoordinateSystemSliceBuilder createSliceBuilder(MetNoFimex::CDMReader_p reader, const MetNoFimex::CoordinateSystem_cp& varCS,
      const std::string& reftime, const gridinventory::GridParameter& param,
      int taxis_index, int zaxis_index, int extraaxis_index);
  MetNoFimex::CoordinateSystemSliceBuilder createSliceBuilder(const MetNoFimex::CoordinateSystem_cp& varCS, const std::string& reftime,
      const gridinventory::GridParameter& param,
      int taxis_index, int zaxis_index, int extraaxis_index);
  bool paramExists(const std::string& reftime, const gridinventory::GridParameter& param);
  void setHybridParametersIfPresent(const std::string& reftime, const gridinventory::GridParameter& param, const std::string& apVar, const std::string& bVar,
                                    size_t zaxis_index, Field_p field);
  void copyFieldSwapY(const bool y_is_up, const int nx, const int ny, const float* fdataSrc, float* fdataDst);

  typedef std::map<std::string, std::string> name2id_t;

  MetNoFimex::CDMReader_p createReader();

  /*! Find index of time in taxis.
   * \return time index, or -1 if not found
   */
  int findTimeIndex(const gridinventory::Taxis& taxis, const miutil::miTime& time);

  /*! Find index of zlevel in zaxis.
   * \return index if found, or 0 if not found but length 1, or -1 if not found and length != 1
   */
  int findZIndex(const gridinventory::Zaxis& zaxis, const std::string& zlevel);

  /*! Find index of elevel in extraaxis.
   * \return index if found, or -1 if not found
   */
  int findExtraIndex(const gridinventory::ExtraAxis& extraaxis, const std::string& elevel);

  //! helper function for inventoryExtractGrid
  void inventoryExtractGridProjection(MetNoFimex::Projection_cp projection, gridinventory::Grid& grid,
      MetNoFimex::CoordinateAxis_cp xAxis, MetNoFimex::CoordinateAxis_cp yAxis);

  //! helper function for makeInventory
  void inventoryExtractGrid(std::set<gridinventory::Grid>& grids, MetNoFimex::CoordinateSystem_cp cs,
      MetNoFimex::CoordinateAxis_cp xAxis, MetNoFimex::CoordinateAxis_cp yAxis);

  //! helper function for makeInventory
  void inventoryExtractVAxis(std::set<gridinventory::Zaxis>& zaxes, name2id_t& name2id,
      MetNoFimex::CoordinateAxis_cp vAxis, MetNoFimex::CoordinateSystem_cp& cs);

  void inventoryExtractExtraAxes(std::set<gridinventory::ExtraAxis>& extraaxes, name2id_t& name2id,
      const std::vector<std::string>& unsetList, const MetNoFimex::CoordinateAxis_cp_v axes);

  std::string fallbackGetReferenceTime();

  /** Find an appropriate coordinate system for the variable.
   * Also checks that it is a simple spatial grid and that both X/Lon and Y/Lat
   * axes are present.
   */
  MetNoFimex::CoordinateSystem_cp findCoordinateSystem(const gridinventory::GridParameter& param);
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
  bool sourceChanged(bool update) override;

  /**
   * Return referencetime from filename or file
   * @return reftime_from_file
   */
  std::string getReferenceTime() const override { return reftime_from_file; }

  /**
   * Returns true if referencetime matches
   * @return bool
   */
  bool referenceTimeOK(const std::string& refTime) override { return (reftime_from_file.empty() || reftime_from_file == refTime); }

  /**
   * Build the inventory from source
   * @return status
   */
  bool makeInventory(const std::string& reftime) override;

  /**
   * Get data slice as Field
   */
  Field_p getData(const std::string& reftime, const gridinventory::GridParameter& param, const std::string& level, const miutil::miTime& time,
                  const std::string& elevel) override;

  /**
   * Get data
   */
  vcross::Values_p getVariable(const std::string& varName) override;

  /**
   * Put data slice from Field. Assumes that the field was generated by this same
   * FimexIO object.
   */
  bool putData(const std::string& reftime, const gridinventory::GridParameter& param, const std::string& level, const miutil::miTime& time,
               const std::string& run, const std::string& unit, Field_cp field, const std::string& output_time) override;
};

#endif /* FIMEXIO_H_ */
