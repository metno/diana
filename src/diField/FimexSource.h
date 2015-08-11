
#ifndef FIMEXSOURCE_HH
#define FIMEXSOURCE_HH

#include "VcrossSource.h"

#include <fimex/CDMReader.h>
#include <fimex/coordSys/CoordinateSystem.h>

namespace vcross {

class FimexReftimeSource : public ReftimeSource {
public:
  typedef boost::shared_ptr<MetNoFimex::CDMReader> CDMReader_p;
  typedef boost::shared_ptr<const MetNoFimex::CoordinateSystem> CoordinateSystem_p;
  typedef std::vector<CoordinateSystem_p> CoordinateSystem_pv;

  class FimexCrossection : public Crossection {
  public:
    FimexCrossection(const std::string& label, const LonLat_v& points,
        const LonLat_v& requestedPoints, size_t si, CDMReader_p r = CDMReader_p())
      : Crossection(label, points, requestedPoints) , start_index(si), reader(r) { }

    bool dynamic() const
      { return reader; }

    size_t start_index;
    CDMReader_p reader;
  };
  typedef boost::shared_ptr<FimexCrossection> FimexCrossection_p;
  typedef boost::shared_ptr<const FimexCrossection> FimexCrossection_cp;

public:
  FimexReftimeSource(std::string filename, std::string filetype, std::string config, const Time& reftime);
  ~FimexReftimeSource();

  Update_t update();
  Inventory_cp getInventory();

  void getCrossectionValues(Crossection_cp cs, const Time& time,
      const InventoryBase_cps& data, name2value_t& n2v);
  void getTimegraphValues(Crossection_cp cs, size_t crossection_index,
      const InventoryBase_cps& data, name2value_t& n2v);
  void getPointValues(Crossection_cp crossection, size_t crossection_index, const Time& time,
      const InventoryBase_cps& data, name2value_t& n2v);
  void getWaveSpectrumValues(Crossection_cp crossection, size_t crossection_index, const Time& time,
      const InventoryBase_cps& data, name2value_t& n2v);

  bool supportsDynamicCrossections()
    { return mSupportsDynamic; }

  /*! Add a dynamic cross-section, if possible.
   * Replaces the first existing cross-section with the same label.
   *
   * The new cross-section may have different points than the ones
   * passed in, e.g. some intermediate points.
   *
   * @return pointer to a new Crossection object, or a null pointer
   */
  Crossection_cp addDynamicCrossection(std::string label, const LonLat_v& positions);
  void dropDynamicCrossection(Crossection_cp cs);
  void dropDynamicCrossections();

private:
  bool makeReader();

  /**
   * Returns a CDMInterpolator if cs is dynamic, or mReader.
   */
  CDMReader_p makeReader(FimexCrossection_cp cs);

  bool makeInventory();
  void makeCrossectionInventory();

  void prepareGetValues(Crossection_cp cs,
      FimexCrossection_cp& fcs, CDMReader_p& reader, CoordinateSystem_pv& coordinateSystems);
  Values_p getSlicedValues(CDMReader_p reader, CoordinateSystem_p cs,
      const Values::ShapeSlice& sliceCdm, const Values::Shape& shapeOut, InventoryBase_cp b);
  Values_p getSlicedValuesGeoZTransformed(CDMReader_p csReader, CoordinateSystem_p cs,
      const Values::ShapeSlice& slice, const Values::Shape& shapeOut, InventoryBase_cp b);

private:
  std::string mFileName, mFileType, mFileConfig;
  long mModificationTime;
  Time mReftime;
  CDMReader_p mReader;
  Inventory_p mInventory;
  CoordinateSystem_pv mCoordinateSystems;
  bool mSupportsDynamic;
};

// ########################################################################

class FimexSource : public Source {
public:
  FimexSource(const std::string& filename_pattern, const std::string& filetype,
      const std::string& config = std::string());
  ~FimexSource();

  ReftimeUpdate update();

private:
  bool addSource(const std::string& path, Time& reftime);

private:
  std::string mFilePattern;
  std::string mFileType;
  std::string mFileConfig;
};

// ########################################################################

typedef boost::shared_ptr<FimexSource> FimexSource_p;

} // namespace vcross

#endif /* FIMEXSOURCE_HH */
