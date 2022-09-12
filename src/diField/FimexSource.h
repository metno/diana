/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2022 met.no

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

#ifndef FIMEXSOURCE_HH
#define FIMEXSOURCE_HH

#include "VcrossSource.h"

#include "../util/charsets.h"

#include <fimex/CDMReaderDecl.h>

#include <memory>
#include <vector>

namespace MetNoFimex {
// forward decl
class CDM;
class CoordinateSystem;
typedef std::shared_ptr<const CoordinateSystem> CoordinateSystem_cp;
typedef std::vector<CoordinateSystem_cp> CoordinateSystem_cp_v;
} // namespace MetNoFimex

namespace vcross {

class FimexReftimeSource : public ReftimeSource {
public:
  class FimexCrossection : public Crossection {
  public:
    FimexCrossection(const std::string& label, const LonLat_v& points,
        const LonLat_v& requestedPoints, size_t si, MetNoFimex::CDMReader_p r = MetNoFimex::CDMReader_p())
      : Crossection(label, points, requestedPoints) , start_index(si), reader(r) { }

    bool dynamic() const
      { return !!reader; }

    size_t start_index;
    MetNoFimex::CDMReader_p reader;
  };
  typedef std::shared_ptr<FimexCrossection> FimexCrossection_p;
  typedef std::shared_ptr<const FimexCrossection> FimexCrossection_cp;

public:
  FimexReftimeSource(const std::string& filename, const std::string& filetype, const std::string& config, diutil::CharsetConverter_p csNameCharsetConverter,
                     const Time& reftime);
  ~FimexReftimeSource();

  Update_t update() override;
  Inventory_cp getInventory() override;

  void getCrossectionValues(Crossection_cp cs, const Time& time,
      const InventoryBase_cps& data, name2value_t& n2v, int realization) override;
  void getTimegraphValues(Crossection_cp cs, size_t crossection_index,
      const InventoryBase_cps& data, name2value_t& n2v, int realization) override;
  void getPointValues(Crossection_cp crossection, size_t crossection_index, const Time& time,
      const InventoryBase_cps& data, name2value_t& n2v, int realization) override;
  void getWaveSpectrumValues(Crossection_cp crossection, size_t crossection_index, const Time& time,
      const InventoryBase_cps& data, name2value_t& n2v, int realization) override;

  bool supportsDynamicCrossections() override
    { return mSupportsDynamic; }

  /*! Add a dynamic cross-section, if possible.
   * Replaces the first existing cross-section with the same label.
   *
   * The new cross-section may have different points than the ones
   * passed in, e.g. some intermediate points.
   *
   * @return pointer to a new Crossection object, or a null pointer
   */
  Crossection_cp addDynamicCrossection(std::string label, const LonLat_v& positions) override;
  void dropDynamicCrossection(Crossection_cp cs) override;
  void dropDynamicCrossections() override;

private:
  bool makeReader();

  /**
   * Returns a CDMInterpolator if cs is dynamic, or mReader.
   */
  MetNoFimex::CDMReader_p makeReader(FimexCrossection_cp cs);

  bool makeInventory();
  void makeCrossectionInventory();

  void prepareGetValues(Crossection_cp cs,
      FimexCrossection_cp& fcs, MetNoFimex::CDMReader_p& reader, MetNoFimex::CoordinateSystem_cp_v& coordinateSystems);
  diutil::Values_p getSlicedValues(MetNoFimex::CDMReader_p reader, MetNoFimex::CoordinateSystem_cp cs, const diutil::Values::ShapeSlice& sliceCdm,
                                   const diutil::Values::Shape& shapeOut, InventoryBase_cp b);
  diutil::Values_p getSlicedValuesGeoZTransformed(MetNoFimex::CDMReader_p csReader, MetNoFimex::CoordinateSystem_cp cs, const diutil::Values::ShapeSlice& slice,
                                                  const diutil::Values::Shape& shapeOut, InventoryBase_cp b);
  MetNoFimex::CoordinateSystem_cp findCsForVariable(const MetNoFimex::CDM& cdm,
      const MetNoFimex::CoordinateSystem_cp_v& coordinateSystems, InventoryBase_cp v);

private:
  std::string mFileName, mFileType, mFileConfig;
  diutil::CharsetConverter_p mCsNameCharsetConverter;
  long mModificationTime;
  MetNoFimex::CDMReader_p mReader;
  Inventory_p mInventory;
  typedef std::map<std::string, std::string> zaxis_cs_m;
  zaxis_cs_m zaxis_cs;
  MetNoFimex::CoordinateSystem_cp_v mCoordinateSystems;
  bool mSupportsDynamic;
};

// ########################################################################

class FimexSource : public Source {
public:
  FimexSource(const std::string& filename_pattern, const std::string& filetype,
      const std::string& config, diutil::CharsetConverter_p csNameCharsetConverter);
  ~FimexSource();

  ReftimeUpdate update();

private:
  bool addSource(const std::string& path, Time& reftime);

private:
  std::string mFilePattern;
  std::string mFileType;
  std::string mFileConfig;
  diutil::CharsetConverter_p mCsNameCharsetConverter;
};

// ########################################################################

typedef std::shared_ptr<FimexSource> FimexSource_p;

} // namespace vcross

#endif /* FIMEXSOURCE_HH */
