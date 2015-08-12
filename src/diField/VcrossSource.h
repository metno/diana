
#ifndef VCROSSSOURCE_HH
#define VCROSSSOURCE_HH 1

#include "VcrossData.h"

namespace vcross {

/*! Data source with a single reference time. */
class ReftimeSource
{
public:
  enum Update_t {
    UNCHANGED = 0,
    CHANGED = 1,
    GONE = 2
  };

  virtual ~ReftimeSource();

  virtual Update_t update() = 0;

  virtual Inventory_cp getInventory() = 0;

  virtual const Time& getReferenceTime() const
    { return mReferenceTime; }

  // for line_data: vcvalues has nlevel=0

  /*!
   * Get values for a cross-section. Output shape: (z-axis, x-axis)
   */
  virtual void getCrossectionValues(Crossection_cp cs, const Time& time,
      const InventoryBase_cps& data, name2value_t& n2v) = 0;

  /*!
   * Get values for a time-graph. Output shape: (z-axis, t-axis)
   */
  virtual void getTimegraphValues(Crossection_cp cs, size_t crossection_index,
      const InventoryBase_cps& data, name2value_t& n2v) = 0;

  /*!
   * Get values for a single point. Output shape: (z-axis)
   */
  virtual void getPointValues(Crossection_cp crossection, size_t crossection_index, const Time& time,
      const InventoryBase_cps& data, name2value_t& n2v) = 0;

  /*!
   * Get wave spectrum values for a single point. Output shape: ("direction", "freq")
   */
  virtual void getWaveSpectrumValues(Crossection_cp crossection, size_t crossection_index, const Time& time,
      const InventoryBase_cps& data, name2value_t& n2v) = 0;

  // try to release some memory
  virtual void flush();

  virtual bool supportsDynamicCrossections();

  // the following functions change getInventory()

  virtual Crossection_cp addDynamicCrossection(std::string name, const LonLat_v& positions);
  virtual void dropDynamicCrossection(Crossection_cp cs);
  virtual void dropDynamicCrossections();

protected:
  Time mReferenceTime;
};

typedef boost::shared_ptr<ReftimeSource> ReftimeSource_p;

// ########################################################################

/*! Data source for multiple refernce times.
 */
class Source
{
public:
  struct ReftimeUpdate {
    Time_s gone, changed, appeared;
    operator bool() const
      { return !(gone.empty() && changed.empty() && appeared.empty()); }
  };

  virtual ~Source();

  virtual ReftimeUpdate update() = 0;

  virtual Time_s getReferenceTimes();
  virtual Inventory_cp getInventory(const Time& reftime);

  Time getLatestReferenceTime();

  // for line_data: vcvalues has nlevel=0

  /*!
   * Get values for a cross-section. Output shape: (z-axis, x-axis)
   */
  virtual void getCrossectionValues(const Time& reftime, Crossection_cp cs, const Time& time,
      const InventoryBase_cps& data, name2value_t& n2v);

  /*!
   * Get values for a time-graph. Output shape: (z-axis, t-axis)
   */
  virtual void getTimegraphValues(const Time& reftime, Crossection_cp cs, size_t crossection_index,
      const InventoryBase_cps& data, name2value_t& n2v);

  /*!
   * Get values for a single point. Output shape: (z-axis)
   */
  virtual void getPointValues(const Time& reftime, Crossection_cp crossection, size_t crossection_index, const Time& time,
      const InventoryBase_cps& data, name2value_t& n2v);

  /*!
   * Get wave spectrum values for a single point. Output shape: ("direction", "freq")
   */
  virtual void getWaveSpectrumValues(const Time& reftime, Crossection_cp crossection, size_t crossection_index, const Time& time,
      const InventoryBase_cps& data, name2value_t& n2v);

  // try to release some memory
  virtual void flush();

  virtual bool supportsDynamicCrossections(const Time& reftime);

  // the following functions change getInventory()

  virtual Crossection_cp addDynamicCrossection(const Time& reftime, std::string name, const LonLat_v& positions);
  virtual void dropDynamicCrossection(const Time& reftime, Crossection_cp cs);
  virtual void dropDynamicCrossections(const Time& reftime);

protected:
  ReftimeSource_p findSource(const Time& reftime) const;

private:
  void updateIfNoReftimeSources();

protected:
  typedef std::vector<ReftimeSource_p> ReftimeSource_pv;
  ReftimeSource_pv mReftimeSources;
};

typedef boost::shared_ptr<Source> Source_p;

} // namespace vcross

#endif
