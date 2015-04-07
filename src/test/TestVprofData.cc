
#include <vcross_v2/VcrossEvaluate.h>
#include <vcross_v2/VcrossComputer.h>

#include <diField/VcrossUtil.h>

#include <puTools/mi_boost_compatibility.hh>
#include <puTools/miTime.h>

#include <gtest/gtest.h>

#include <fstream>
#include <string>

#define MILOGGER_CATEGORY "diana.test.VprofData"
#include "miLogger/miLogging.h"

using namespace vcross;

const char AROME_FILE[] = "arome_vprof.nc";
const int AROME_N_CS = 6;
const int AROME_N_TIME = 2;
const char AROME_RTT[] = "2014-10-20 00:00:00";

static const char BANGKOK_FILE[] = "bangkok_sonde.nc";
static const int BANGKOK_N_CS = 1;
static const int BANGKOK_N_TIME = 30;
static const int BANGKOK_N_Z = 15;

static const char modelName[] = "testmodel";

TEST(VprofDataTest, TestSetup)
{
  Setup_p setup = miutil::make_shared<vcross::Setup>();
  Collector_p collector = miutil::make_shared<Collector>(setup);

  string_v sources;
  sources.push_back("m=" + std::string(modelName)
      + " f=" + std::string(TEST_SRCDIR) + "/" + std::string(AROME_FILE)
      + " t=netcdf");
  EXPECT_EQ(0, setup->configureSources(sources).size()) << "syntax errors in sources";

  Source_p src = setup->findSource(modelName);
  ASSERT_TRUE(src);
  EXPECT_EQ(1, src->getReferenceTimes().size());

  //parameters and computations should be defined in setup
  string_v computations;
  computations.push_back("relative_humidity_ml=rh_from_tk_q(air_temperature_ml,specific_humidity_ml)");
  computations.push_back("tdk=tdk_from_tk_q(air_temperature_ml,specific_humidity_ml)");
  computations.push_back("air_temperature_celsius_ml=convert_unit(air_temperature_ml,celsius)");
  computations.push_back("dew_point_temperature_celsius_ml=convert_unit(tdk,celsius)");
  EXPECT_EQ(0, setup->configureComputations(computations).size()) << "syntax errors in computations";
  collector->setupChanged();

  const vcross::Time AROME_RT = util::from_miTime(miutil::miTime(AROME_RTT));
  vcross::Inventory_cp inv = collector->getResolver()->getInventory(ModelReftime(modelName, AROME_RT));
  ASSERT_TRUE(inv);
  ASSERT_EQ(AROME_N_TIME, inv->times.npoint());
  ASSERT_EQ(AROME_N_CS, inv->crossections.size());
  Crossection_cp cs = inv->crossections.at(3);
  ASSERT_EQ(1, cs->length());

  string_v field_ids;
  field_ids.push_back("air_temperature_celsius_ml");
  field_ids.push_back("dew_point_temperature_celsius_ml");
  field_ids.push_back("x_wind_ml");
  field_ids.push_back("y_wind_ml");
  field_ids.push_back("relative_humidity_ml");
  field_ids.push_back("upward_air_velocity_ml");
  const ModelReftime mr(modelName, AROME_RT);
  for (string_v::const_iterator it = field_ids.begin(); it != field_ids.end(); ++it)
    collector->requireField(mr, *it);
  collector->requireVertical(vcross::Z_TYPE_PRESSURE);

  FieldData_cp air_temperature = boost::dynamic_pointer_cast<const FieldData>(collector->getResolvedField(mr, field_ids[0]));
  ASSERT_TRUE(air_temperature);
  ZAxisData_cp zaxis = air_temperature->zaxis();
  ASSERT_TRUE(zaxis);

  model_values_m model_values = vc_fetch_pointValues(collector, cs->point(0), inv->times.at(0));
  model_values_m::iterator itM = model_values.find(mr);
  ASSERT_TRUE(itM != model_values.end());
  name2value_t& n2v = itM->second;

  Values_cp zvalues;
  if (util::unitsConvertible(zaxis->unit(), "hPa"))
    zvalues = vc_evaluate_field(zaxis, n2v);
  else if (InventoryBase_cp pfield = zaxis->pressureField())
    zvalues = vc_evaluate_field(pfield, n2v);
  ASSERT_TRUE(zvalues);

  vc_evaluate_fields(collector, model_values, mr, field_ids);

  { name2value_t::const_iterator itN = n2v.find("x_wind_ml");
    ASSERT_TRUE(itN != n2v.end());
    Values_cp xwind_values = itN->second;
    ASSERT_TRUE(xwind_values);
    EXPECT_EQ(1, xwind_values->shape().rank());
  }

  { name2value_t::const_iterator itN = n2v.find("air_temperature_celsius_ml");
    ASSERT_TRUE(itN != n2v.end());
    Values_cp t_values = itN->second;
    ASSERT_TRUE(t_values);
    EXPECT_EQ(1, t_values->shape().rank());
  }

  { name2value_t::const_iterator itN = n2v.find("tdk");
    ASSERT_TRUE(itN != n2v.end());
    Values_cp tdk_values = itN->second;
    ASSERT_TRUE(tdk_values);
    EXPECT_EQ(1, tdk_values->shape().rank());
  }
}

TEST(VprofDataTest, TestBangkok)
{
  Setup_p setup = miutil::make_shared<vcross::Setup>();
  Collector_p collector = miutil::make_shared<Collector>(setup);

  string_v sources;
  sources.push_back("m=" + std::string(modelName)
      + " f=" TEST_SRCDIR "/" + std::string(BANGKOK_FILE)
      + " t=netcdf");
  EXPECT_EQ(0, setup->configureSources(sources).size()) << "syntax errors in sources";
  Source_p src = setup->findSource(modelName);
  ASSERT_TRUE(src);
  const Time reftime = src->getLatestReferenceTime();
  EXPECT_EQ("2014-11-10 12:00:00", util::to_miTime(reftime).isoTime())
      << reftime.value << " u=" << reftime.unit;

  //parameters and computations should be defined in setup
  string_v computations;
  computations.push_back("specific_humidity=q_from_tk_rh(air_temperature_pl,relative_humidity_pl)");
  computations.push_back("air_temperature=convert_unit(air_temperature_pl,celsius)");
  computations.push_back("dew_point_temperature_kelvin=tdk_from_tk_rh(air_temperature_pl,relative_humidity_pl)");
  computations.push_back("dew_point_temperature_celsius=convert_unit(dew_point_temperature_kelvin,celsius)");
  EXPECT_EQ(0, setup->configureComputations(computations).size()) << "syntax errors in computations";
  collector->setupChanged();

  const ModelReftime mr(modelName, reftime);
  vcross::Inventory_cp inv = collector->getResolver()->getInventory(mr);
  ASSERT_TRUE(inv);
  ASSERT_EQ(BANGKOK_N_TIME, inv->times.npoint());
  ASSERT_EQ(BANGKOK_N_CS, inv->crossections.size());
  Crossection_cp cs = inv->crossections.at(0);
  ASSERT_EQ(1, cs->length());

  string_v field_ids;
  field_ids.push_back("air_temperature");
  field_ids.push_back("dew_point_temperature_celsius");
  field_ids.push_back("x_wind_pl");
  field_ids.push_back("y_wind_pl");
  field_ids.push_back("relative_humidity_pl");
  for (string_v::const_iterator it = field_ids.begin(); it != field_ids.end(); ++it)
    collector->requireField(mr, *it);

  FieldData_cp air_temperature = boost::dynamic_pointer_cast<const FieldData>(collector->getResolvedField(mr, field_ids[0]));
  ASSERT_TRUE(air_temperature);
  InventoryBase_cp zaxis = air_temperature->zaxis();
  ASSERT_TRUE(zaxis);
  EXPECT_EQ("isobaric", zaxis->id());
  collector->requireField(mr, zaxis);

  model_values_m model_values = vc_fetch_pointValues(collector, cs->point(0), inv->times.at(0));
  model_values_m::iterator itM = model_values.find(mr);
  ASSERT_TRUE(itM != model_values.end());
  name2value_t& n2v = itM->second;

  Values_cp zvalues = util::unitConversion(vc_evaluate_field(zaxis, n2v), zaxis->unit(), "hPa");
  ASSERT_TRUE(zvalues and zvalues->values());
  EXPECT_EQ(1, zvalues->shape().rank());
  EXPECT_EQ(BANGKOK_N_Z, zvalues->shape().length(vcross::Values::GEO_Z));
  const float BANGKOK_Z_VALUES[BANGKOK_N_Z] = { 10, 30, 50, 70, 100, 150, 200, 250, 300, 400, 500, 700, 850, 925, 1000 };
  for (int i=0; i<BANGKOK_N_Z; ++i)
    EXPECT_EQ(BANGKOK_Z_VALUES[i], zvalues->values()[i]) << "i=" << i;

  vc_evaluate_fields(collector, model_values, mr, field_ids);

  if(0){ name2value_t::const_iterator itN = n2v.find("x_wind_pl");
    ASSERT_TRUE(itN != n2v.end());
    Values_cp xwind_values = itN->second;
    ASSERT_TRUE(xwind_values);
    EXPECT_EQ(1, xwind_values->shape().rank());
  }

  { name2value_t::const_iterator itN = n2v.find("air_temperature");
    ASSERT_TRUE(itN != n2v.end());
    Values_cp t_values = itN->second;
    ASSERT_TRUE(t_values);
    EXPECT_EQ(1, t_values->shape().rank());
    EXPECT_EQ(BANGKOK_N_Z, t_values->shape().length(0));
  }

  { name2value_t::const_iterator itN = n2v.find("dew_point_temperature_celsius");
    ASSERT_TRUE(itN != n2v.end());
    Values_cp v_values = itN->second;
    ASSERT_TRUE(v_values);
    EXPECT_EQ(1, v_values->shape().rank());
    EXPECT_NEAR(20.6, v_values->values()[BANGKOK_N_Z-1], 0.1);
  }
}
