
#include <FimexSource.h>
#include <VcrossUtil.h>

#include <gtest/gtest.h>

#include <fstream>
#include <string>

#define MILOGGER_CATEGORY "diField.test.FimexSourceTest"
#include "miLogger/miLogging.h"

using namespace vcross;

//#define DEBUG_MESSAGES
#ifdef DEBUG_MESSAGES
#include <fimex/Logger.h> // required to tell fimex to use log4cpp
#include <log4cpp/Category.hh>
#define configureLogging()                                              \
  MetNoFimex::Logger::setClass(MetNoFimex::Logger::LOG4CPP);            \
  milogger::LoggingConfig lc("kjlhlkjH");                               \
  log4cpp::Category::getRoot().setPriority(log4cpp::Priority::DEBUG)
#else
#define configureLogging() /* empty */
#endif // !DEBUG_MESSAGES

typedef boost::shared_ptr<FimexSource> FimexSource_p;

static const char SIMRA_FILE[] = "simra_vc.nc";
static const int SIMRA_N_CS = 2;
static const int SIMRA_CS_LEN[SIMRA_N_CS] = { 64, 167 };
static const int SIMRA_N_TIME = 2;
static const int SIMRA_N_Z = 41;

static const char AROME_FILE[] = "arome_vc.nc";
static const int AROME_N_CS = 2;
static const int AROME_CS_LEN[AROME_N_CS] = { 233, 222 };
static const int AROME_N_TIME = 2;
static const int AROME_N_Z = 65;

static const char HIRLAM_FILE[] = "hirlam_vc.nc";
static const int HIRLAM_N_CS = 2;
static const int HIRLAM_CS_LEN[HIRLAM_N_CS] = { 19, 10 };
static const int HIRLAM_N_TIME = 3;
static const int HIRLAM_N_Z = 60;

static const char EMEP_ETNA_FILE[] = "emep_etna_dyn.nc";
static const int EMEP_ETNA_N_TIME = 4;
static const int EMEP_ETNA_N_Z = 19;

static const char AROMESMHI_FILE[] = "arome_smhi.nc";
static const int AROMESMHI_N_TIME = 4;
static const int AROMESMHI_N_Z = 65;

static const char BANGLADESH_FILE[] = "bangladesh_vc.nc";
static const int BANGLADESH_N_CS = 4;
static const int BANGLADESH_CS_LEN[BANGLADESH_N_CS] = { 1, 1, 96, 124 };
static const int BANGLADESH_N_TIME = 5;
static const int BANGLADESH_N_Z = 15;

static const char WAVE_FILE[] = "wavespec.nc";

static const char S1970[] = "seconds since 1970-01-01 00:00:00";

static ReftimeSource_p openFimexFile(const std::string& file)
{
  const std::string fileName = (TEST_SRCDIR "/") + file;
  std::ifstream inputfile(fileName.c_str());
  if (not inputfile)
    return ReftimeSource_p();

  return ReftimeSource_p(new FimexReftimeSource(fileName, "netcdf", "", Time()));
}

TEST(FimexReftimeSourceTest, TestSimraVcross0)
{
  configureLogging();
  ReftimeSource_p fs = openFimexFile(SIMRA_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(inv);
  ASSERT_EQ(SIMRA_N_TIME, inv->times.npoint());
  ASSERT_EQ(SIMRA_N_CS,   inv->crossections.size());

  Crossection_cp cs0 = inv->crossections.at(0);
  ASSERT_TRUE(cs0);
  EXPECT_EQ(SIMRA_CS_LEN[0], cs0->length());

  FieldData_cp turbulence = inv->findFieldById("turbulence");
  ASSERT_TRUE(turbulence);
  EXPECT_EQ("FIELD", turbulence->dataType());
  EXPECT_EQ(SIMRA_N_Z,      turbulence->nlevel());

  const Time& time = inv->times.at(0);

  InventoryBase_cps request;
  request.insert(turbulence);
  name2value_t n2v;
  fs->getCrossectionValues(cs0, time, request, n2v);

  Values_cp turbulence_values = n2v[turbulence->id()];
  ASSERT_TRUE(turbulence_values);

  const Values::Shape& shape(turbulence_values->shape());
  ASSERT_EQ(2, shape.rank());
  EXPECT_EQ(Values::GEO_X,   shape.name(0));
  EXPECT_EQ(SIMRA_CS_LEN[0], shape.length(0));
  EXPECT_EQ(Values::GEO_Z,   shape.name(1));
  EXPECT_EQ(SIMRA_N_Z,       shape.length(1));

  Values::ShapeIndex idx(turbulence_values->shape());
  idx.set(Values::GEO_X, 5);
  idx.set(Values::GEO_Z, 12);
  EXPECT_FLOAT_EQ(0.11371478, turbulence_values->value(idx));

  idx.set(Values::GEO_Z, 39);
  EXPECT_FLOAT_EQ(0.49979857, turbulence_values->value(idx));

  idx.set(Values::GEO_X, 37);
  EXPECT_FLOAT_EQ(0.21248914, turbulence_values->value(idx));
}

TEST(FimexReftimeSourceTest, TestSimraVcross1)
{
  configureLogging();
  ReftimeSource_p fs = openFimexFile(SIMRA_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(inv);
  ASSERT_EQ(SIMRA_N_TIME, inv->times.npoint());
  ASSERT_EQ(SIMRA_N_CS, inv->crossections.size());

  Crossection_cp cs1 = inv->crossections.at(1);
  ASSERT_TRUE(cs1);
  EXPECT_EQ(SIMRA_CS_LEN[1], cs1->length());

  FieldData_cp turbulence = inv->findFieldById("turbulence");
  ASSERT_TRUE(turbulence);

  const Time& time = inv->times.at(1);

  InventoryBase_cps request;
  request.insert(turbulence);
  name2value_t n2v;
  fs->getCrossectionValues(cs1, time, request, n2v);

  Values_cp turbulence_values = n2v[turbulence->id()];
  ASSERT_TRUE(turbulence_values);

  const Values::Shape& shape(turbulence_values->shape());

  ASSERT_EQ(2, shape.rank());
  EXPECT_EQ(Values::GEO_X,   shape.name(0));
  EXPECT_EQ(SIMRA_CS_LEN[1], shape.length(0));
  EXPECT_EQ(Values::GEO_Z,   shape.name(1));
  EXPECT_EQ(SIMRA_N_Z,       shape.length(1));

  Values::ShapeIndex idx(turbulence_values->shape());
  idx.set(Values::GEO_X, 43);
  idx.set(Values::GEO_Z, 35);
  EXPECT_FLOAT_EQ(0.85601133, turbulence_values->value(idx));

  idx.set(Values::GEO_Z, 39);
  EXPECT_FLOAT_EQ(0.65781903, turbulence_values->value(idx));

  idx.set(Values::GEO_X, 23);
  EXPECT_FLOAT_EQ(1.1107885, turbulence_values->value(idx));
}

TEST(FimexReftimeSourceTest, TestSimraVcrossVertical)
{
  configureLogging();
  ReftimeSource_p fs = openFimexFile(SIMRA_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(inv);
  ASSERT_EQ(SIMRA_N_TIME, inv->times.npoint());
  ASSERT_EQ(SIMRA_N_CS,   inv->crossections.size());

  Crossection_cp cs1 = inv->crossections.at(1);
  ASSERT_TRUE(cs1);
  EXPECT_EQ(SIMRA_CS_LEN[1], cs1->length());

  FieldData_cp turbulence = inv->findFieldById("turbulence");
  ASSERT_TRUE(turbulence);
  ZAxisData_cp vertical = turbulence->zaxis();
  ASSERT_TRUE(vertical);
  InventoryBase_cp vertical_pressure = turbulence->zaxis()->pressureField();
  ASSERT_TRUE(vertical_pressure);
  InventoryBase_cp vertical_altitude = turbulence->zaxis()->altitudeField();
  ASSERT_TRUE(vertical_altitude);

  const Time& time = inv->times.at(1);

  InventoryBase_cps request;
  request.insert(turbulence);
  request.insert(vertical_pressure);
  request.insert(vertical_altitude);
  name2value_t n2v;
  fs->getCrossectionValues(cs1, time, request, n2v);
  
  Values_cp vertical_p_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(vertical_p_values);
  Values_cp vertical_h_values = n2v[vertical_altitude->id()];
  ASSERT_TRUE(vertical_h_values);

  const Values::Shape& shape(vertical_p_values->shape());
  ASSERT_EQ(2, shape.rank());
  EXPECT_EQ(Values::GEO_X,   shape.name(0));
  EXPECT_EQ(SIMRA_CS_LEN[1], shape.length(0));
  EXPECT_EQ(Values::GEO_Z,   shape.name(1));
  EXPECT_EQ(SIMRA_N_Z,       shape.length(1));

  ASSERT_EQ(shape.names(), vertical_h_values->shape().names());
  ASSERT_EQ(shape.lengths(), vertical_h_values->shape().lengths());

  Values::ShapeIndex idx(vertical_p_values->shape());
  idx.set(Values::GEO_X, 43);
  idx.set(Values::GEO_Z, 39);
  EXPECT_FLOAT_EQ(965.61432, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(450.6684, vertical_h_values->value(idx));

  idx.set(Values::GEO_Z, 3);
  EXPECT_FLOAT_EQ(728.30688, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(2665.3196, vertical_h_values->value(idx));

  idx.set(Values::GEO_X, 23);
  EXPECT_FLOAT_EQ(727.28851, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(2676.1211, vertical_h_values->value(idx));
}

TEST(FimexReftimeSourceTest, TestSimraTimegraph)
{
  configureLogging();
  ReftimeSource_p fs = openFimexFile(SIMRA_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(inv);
  ASSERT_EQ(SIMRA_N_TIME, inv->times.npoint());
  ASSERT_EQ(SIMRA_N_CS,   inv->crossections.size());

  Crossection_cp cs0 = inv->crossections.at(0);
  ASSERT_TRUE(cs0);
  EXPECT_EQ(SIMRA_CS_LEN[0], cs0->length());

  FieldData_cp turbulence = inv->findFieldById("turbulence");
  ASSERT_TRUE(turbulence);

  InventoryBase_cps request;
  request.insert(turbulence);
  name2value_t n2v;
  fs->getTimegraphValues(cs0, 5, request, n2v);

  Values_cp turbulence_values = n2v[turbulence->id()];
  ASSERT_TRUE(turbulence_values);

  const Values::Shape& shape(turbulence_values->shape());
  ASSERT_EQ(2, shape.rank());
  EXPECT_EQ(Values::TIME,         shape.name(0));
  EXPECT_EQ(SIMRA_N_TIME,         shape.length(0));
  EXPECT_EQ(Values::GEO_Z,        shape.name(1));
  EXPECT_EQ(SIMRA_N_Z,            shape.length(1));


  Values::ShapeIndex idx(turbulence_values->shape());
  idx.set(Values::TIME, 0);
  idx.set(Values::GEO_Z, 12);
  EXPECT_FLOAT_EQ(0.11371478, turbulence_values->value(idx));
}

TEST(FimexReftimeSourceTest, TestSimraVprofile)
{
  configureLogging();
  ReftimeSource_p fs = openFimexFile(SIMRA_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(inv);
  ASSERT_EQ(SIMRA_N_TIME, inv->times.npoint());
  ASSERT_EQ(SIMRA_N_CS,   inv->crossections.size());

  Crossection_cp cs1 = inv->crossections.at(1);
  ASSERT_TRUE(cs1);

  FieldData_cp turbulence = inv->findFieldById("turbulence");
  ASSERT_TRUE(turbulence);

  const Time& time = inv->times.at(1);
  InventoryBase_cps request;
  request.insert(turbulence);
  name2value_t n2v;
  fs->getPointValues(cs1, 17, time, request, n2v);

  Values_cp turbulence_values = n2v[turbulence->id()];
  ASSERT_TRUE(turbulence_values);

  const Values::Shape& shape(turbulence_values->shape());
  ASSERT_EQ(1, shape.rank());
  EXPECT_EQ(Values::GEO_Z, shape.name(0));
  EXPECT_EQ(SIMRA_N_Z,     shape.length(0));

  Values::ShapeIndex idx(turbulence_values->shape());
  idx.set(Values::GEO_Z, 36);
  EXPECT_FLOAT_EQ(0.92437315, turbulence_values->value(idx));

  idx.set(Values::GEO_Z, 40);
  EXPECT_FLOAT_EQ(0.20002793, turbulence_values->value(idx));
}

TEST(FimexReftimeSourceTest, TestAromeVcross1)
{
  configureLogging();
  ReftimeSource_p fs = openFimexFile(AROME_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(inv);
  ASSERT_EQ(AROME_N_TIME, inv->times.npoint());
  ASSERT_EQ(AROME_N_CS, inv->crossections.size());

  Crossection_cp cs1 = inv->crossections.at(1);
  ASSERT_TRUE(cs1);
  EXPECT_EQ(AROME_CS_LEN[1], cs1->length());

  FieldData_cp temperature = inv->findFieldById("air_temperature_ml");
  ASSERT_TRUE(temperature);

  const Time& time = inv->times.at(1);

  InventoryBase_cps request;
  request.insert(temperature);
  name2value_t n2v;
  fs->getCrossectionValues(cs1, time, request, n2v);

  Values_cp temperature_values = n2v[temperature->id()];
  ASSERT_TRUE(temperature_values);

  const Values::Shape& shape(temperature_values->shape());

  ASSERT_EQ(2, shape.rank());
  EXPECT_EQ(Values::GEO_X,   shape.name(0));
  EXPECT_EQ(AROME_CS_LEN[1], shape.length(0));
  EXPECT_EQ(Values::GEO_Z,   shape.name(1));
  EXPECT_EQ(AROME_N_Z,       shape.length(1));

  Values::ShapeIndex idx(temperature_values->shape());
  idx.set(Values::GEO_X, 43);
  idx.set(Values::GEO_Z, 35);
  EXPECT_FLOAT_EQ(274.72327, temperature_values->value(idx));

  idx.set(Values::GEO_Z, 39);
  EXPECT_FLOAT_EQ(277.45432, temperature_values->value(idx));

  idx.set(Values::GEO_X, 23);
  EXPECT_FLOAT_EQ(277.99792, temperature_values->value(idx));
}

TEST(FimexReftimeSourceTest, TestAromeVcrossVertical)
{
  configureLogging();
  ReftimeSource_p fs = openFimexFile(AROME_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(inv);
  ASSERT_EQ(AROME_N_TIME, inv->times.npoint());
  ASSERT_EQ(AROME_N_CS,   inv->crossections.size());

  Crossection_cp cs1 = inv->crossections.at(1);
  ASSERT_TRUE(cs1);
  EXPECT_EQ(AROME_CS_LEN[1], cs1->length());

  FieldData_cp temperature = inv->findFieldById("air_temperature_ml");
  ASSERT_TRUE(temperature);
  ZAxisData_cp vertical = temperature->zaxis();
  ASSERT_TRUE(vertical);
  InventoryBase_cp vertical_pressure = temperature->zaxis()->pressureField();
  ASSERT_TRUE(vertical_pressure);
  InventoryBase_cp vertical_altitude = temperature->zaxis()->altitudeField();
  ASSERT_TRUE(vertical_altitude);

  const Time& time = inv->times.at(1);

  InventoryBase_cps request;
  request.insert(temperature);
  request.insert(vertical_pressure);
  request.insert(vertical_altitude);
  name2value_t n2v;
  fs->getCrossectionValues(cs1, time, request, n2v);

  Values_cp vertical_p_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(vertical_p_values);
  Values_cp vertical_h_values = n2v[vertical_altitude->id()];
  ASSERT_TRUE(vertical_h_values);

  const Values::Shape& shape(vertical_p_values->shape());
  ASSERT_EQ(2, shape.rank());
  EXPECT_EQ(Values::GEO_X,   shape.name(0));
  EXPECT_EQ(AROME_CS_LEN[1], shape.length(0));
  EXPECT_EQ(Values::GEO_Z,   shape.name(1));
  EXPECT_EQ(AROME_N_Z,       shape.length(1));

  ASSERT_EQ(shape.names(), vertical_h_values->shape().names());
  ASSERT_EQ(shape.lengths(), vertical_h_values->shape().lengths());

  Values::ShapeIndex idx(vertical_p_values->shape());
  idx.set(Values::GEO_X, 43);
  idx.set(Values::GEO_Z, 39);
  EXPECT_FLOAT_EQ(836.25787, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(1619.2686, vertical_h_values->value(idx));

  idx.set(Values::GEO_Z, 3);
  EXPECT_FLOAT_EQ(70.053238, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(22534.182, vertical_h_values->value(idx));

  idx.set(Values::GEO_X, 23);
  EXPECT_FLOAT_EQ(70.053352, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(22534.168, vertical_h_values->value(idx));
}

TEST(FimexReftimeSourceTest, TestAromeTimegraph)
{
  configureLogging();
  ReftimeSource_p fs = openFimexFile(AROME_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(inv);
  ASSERT_EQ(AROME_N_TIME, inv->times.npoint());
  ASSERT_EQ(AROME_N_CS,   inv->crossections.size());

  Crossection_cp cs1 = inv->crossections.at(1);
  ASSERT_TRUE(cs1);
  EXPECT_EQ(AROME_CS_LEN[1], cs1->length());

  FieldData_cp temperature = inv->findFieldById("air_temperature_ml");
  ASSERT_TRUE(temperature);
  ZAxisData_cp vertical = temperature->zaxis();
  ASSERT_TRUE(vertical);
  InventoryBase_cp vertical_pressure = temperature->zaxis()->pressureField();
  ASSERT_TRUE(vertical);

  InventoryBase_cps request;
  request.insert(temperature);
  request.insert(vertical_pressure);
  name2value_t n2v;
  fs->getTimegraphValues(cs1, 43, request, n2v);

  Values_cp temperature_values = n2v[temperature->id()];
  ASSERT_TRUE(temperature_values);
  Values_cp vertical_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(vertical_values);

  const Values::Shape& shape(vertical_values->shape());
  ASSERT_EQ(2, shape.rank());
  EXPECT_EQ(Values::TIME,  shape.name(0));
  EXPECT_EQ(AROME_N_TIME,  shape.length(0));
  EXPECT_EQ(Values::GEO_Z, shape.name(1));
  EXPECT_EQ(AROME_N_Z,     shape.length(1));

  Values::ShapeIndex idx(temperature_values->shape());
  idx.set(Values::TIME, 1);
  idx.set(Values::GEO_Z, 35);
  EXPECT_FLOAT_EQ(274.72327, temperature_values->value(idx));
  idx.set(Values::GEO_Z, 39);
  EXPECT_FLOAT_EQ(277.45432, temperature_values->value(idx));

  const Values::Shape& shape_z(vertical_values->shape());
  ASSERT_EQ(2, shape_z.rank());
  EXPECT_EQ(Values::TIME,  shape_z.name(0));
  EXPECT_EQ(AROME_N_TIME,  shape_z.length(0));
  EXPECT_EQ(Values::GEO_Z, shape_z.name(1));
  EXPECT_EQ(AROME_N_Z,     shape_z.length(1));

  Values::ShapeIndex idx_z(vertical_values->shape());
  idx_z.set(Values::TIME, 1);
  idx_z.set(Values::GEO_Z, 39);
  EXPECT_FLOAT_EQ(836.25787, vertical_values->value(idx_z));
  idx_z.set(Values::GEO_Z, 3);
  EXPECT_FLOAT_EQ(70.053238, vertical_values->value(idx_z));
}

TEST(FimexReftimeSourceTest, TestHirlamVcrossVertical)
{
  configureLogging();
  ReftimeSource_p fs = openFimexFile(HIRLAM_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(inv);
  ASSERT_EQ(HIRLAM_N_TIME, inv->times.npoint());
  ASSERT_EQ(HIRLAM_N_CS,   inv->crossections.size());

  Crossection_cp cs1 = inv->crossections.at(1);
  ASSERT_TRUE(cs1);
  EXPECT_EQ(HIRLAM_CS_LEN[1], cs1->length());

  FieldData_cp air_pt = inv->findFieldById("air_potential_temperature_ml");
  ASSERT_TRUE(air_pt);
  ZAxisData_cp vertical = air_pt->zaxis();
  ASSERT_TRUE(vertical);
  InventoryBase_cp vertical_pressure = air_pt->zaxis()->pressureField();
  ASSERT_TRUE(vertical_pressure);
  InventoryBase_cp vertical_altitude = air_pt->zaxis()->altitudeField();
  ASSERT_TRUE(vertical_altitude);

  const Time& time = inv->times.at(0);

  InventoryBase_cps request;
  request.insert(air_pt);
  request.insert(vertical_pressure);
  request.insert(vertical_altitude);
  name2value_t n2v;
  fs->getCrossectionValues(cs1, time, request, n2v);

  Values_cp vertical_p_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(vertical_p_values);
  Values_cp vertical_h_values = n2v[vertical_altitude->id()];
  ASSERT_TRUE(vertical_h_values);
  Values_cp air_pt_values = n2v[air_pt->id()];
  ASSERT_TRUE(air_pt_values);

  const Values::Shape& shape(vertical_p_values->shape());
  ASSERT_EQ(2, shape.rank());
  EXPECT_EQ(Values::GEO_X,   shape.name(0));
  EXPECT_EQ(HIRLAM_CS_LEN[1], shape.length(0));
  EXPECT_EQ(Values::GEO_Z,   shape.name(1));
  EXPECT_EQ(HIRLAM_N_Z,       shape.length(1));

  ASSERT_EQ(shape.names(),   vertical_h_values->shape().names());
  ASSERT_EQ(shape.lengths(), vertical_h_values->shape().lengths());

  ASSERT_EQ(shape.names(),   air_pt_values->shape().names());
  ASSERT_EQ(shape.lengths(), air_pt_values->shape().lengths());

  Values::ShapeIndex idx(vertical_p_values->shape());
  idx.set(Values::GEO_X, 3);
  idx.set(Values::GEO_Z, 39);
  EXPECT_FLOAT_EQ(747.15155, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(2405,      vertical_h_values->value(idx));
  EXPECT_FLOAT_EQ(293,       air_pt_values->value(idx));

  idx.set(Values::GEO_Z, 3);
  EXPECT_FLOAT_EQ(71.458359, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(18212,     vertical_h_values->value(idx));
  EXPECT_FLOAT_EQ(451.5,     air_pt_values->value(idx));

  idx.set(Values::GEO_X, 7);
  EXPECT_FLOAT_EQ(71.487221, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(18205,     vertical_h_values->value(idx));
  EXPECT_FLOAT_EQ(449.39999, air_pt_values->value(idx));
}

TEST(FimexReftimeSourceTest, TestEmepDynVcross)
{
  configureLogging();
  ReftimeSource_p fs = openFimexFile(EMEP_ETNA_FILE);
  if (not fs)
    return;

  { Inventory_cp inv = fs->getInventory();
    ASSERT_TRUE(inv);
    ASSERT_EQ(EMEP_ETNA_N_TIME, inv->times.npoint());
    ASSERT_EQ(0, inv->crossections.size());
  }

  ASSERT_TRUE(fs->supportsDynamicCrossections());
  LonLat_v positions;
  positions.push_back(LonLat::fromDegrees(14.995, 37.755));
  positions.push_back(LonLat::fromDegrees(16, 39));
  positions.push_back(LonLat::fromDegrees(13, 39));
  positions.push_back(LonLat::fromDegrees(13, 36));
  positions.push_back(LonLat::fromDegrees(16, 36));
  ASSERT_TRUE(fs->addDynamicCrossection("etna", positions));

  Inventory_cp inv = fs->getInventory();
  Crossection_cp csdyn = inv->findCrossectionByLabel("etna");
  ASSERT_TRUE(csdyn);
  ASSERT_LE(positions.size(), csdyn->length());
  ASSERT_EQ(positions.size(), csdyn->lengthRequested());
  for (size_t i=0; i<positions.size(); ++i)
    ASSERT_LE(positions.at(i).distanceTo(csdyn->pointRequested(i)), 10) << "i=" << i;

  FieldData_cp ash_c = inv->findFieldById("ash_concentration");
  ASSERT_TRUE(ash_c);
  ZAxisData_cp vertical = ash_c->zaxis();
  ASSERT_TRUE(vertical);
  InventoryBase_cp vertical_pressure = ash_c->zaxis()->pressureField();
  ASSERT_TRUE(vertical_pressure);
  InventoryBase_cp vertical_altitude = ash_c->zaxis()->altitudeField();
  ASSERT_TRUE(vertical_altitude);

  const Time& time = inv->times.at(0);

  InventoryBase_cps request;
  request.insert(ash_c);
  request.insert(vertical_pressure);
  request.insert(vertical_altitude);
  name2value_t n2v;
  fs->getCrossectionValues(csdyn, time, request, n2v);

  Values_cp vertical_p_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(vertical_p_values);
  Values_cp vertical_h_values = n2v[vertical_altitude->id()];
  ASSERT_TRUE(vertical_h_values);
  Values_cp ash_c_values = n2v[ash_c->id()];
  ASSERT_TRUE(ash_c_values);

  const Values::Shape& shape(vertical_p_values->shape());
  ASSERT_EQ(2, shape.rank());
  EXPECT_EQ(Values::GEO_X,    shape.name(0));
  EXPECT_LE(positions.size(), shape.length(0));
  EXPECT_EQ(Values::GEO_Z,    shape.name(1));
  EXPECT_EQ(EMEP_ETNA_N_Z,    shape.length(1));

  ASSERT_EQ(shape.names(),   vertical_h_values->shape().names());
  ASSERT_EQ(shape.lengths(), vertical_h_values->shape().lengths());

  ASSERT_EQ(shape.names(),   ash_c_values->shape().names());
  ASSERT_EQ(shape.lengths(), ash_c_values->shape().lengths());

  Values::ShapeIndex idx(vertical_p_values->shape());
  idx.set(Values::GEO_X, positions.size()-2);
  idx.set(Values::GEO_Z, 13);
  EXPECT_FLOAT_EQ(505.81207,    vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(5859.9058,    vertical_h_values->value(idx));
  EXPECT_FLOAT_EQ(4.406802e-26, ash_c_values->value(idx));

  idx.set(Values::GEO_Z, 3);
  EXPECT_FLOAT_EQ(999.0155,   vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(119.33135,  vertical_h_values->value(idx));
  EXPECT_FLOAT_EQ(0.82586122, ash_c_values->value(idx));

  idx.set(Values::GEO_X, 0);
  EXPECT_FLOAT_EQ(982.49335, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(259.99149, vertical_h_values->value(idx));
  EXPECT_FLOAT_EQ(121.61005, ash_c_values->value(idx));
}

TEST(FimexReftimeSourceTest, TestAromeSmhiDynVcross)
{
  configureLogging();
  ReftimeSource_p fs = openFimexFile(AROMESMHI_FILE);
  if (not fs)
    return;

  { Inventory_cp inv = fs->getInventory();
    ASSERT_TRUE(inv);
    ASSERT_EQ(AROMESMHI_N_TIME, inv->times.npoint());
    ASSERT_EQ(0, inv->crossections.size());
  }

  ASSERT_TRUE(fs->supportsDynamicCrossections());
  LonLat_v positions;
  positions.push_back(LonLat::fromDegrees(16.4, 59.3));
  positions.push_back(LonLat::fromDegrees(16.4, 59.4));
  positions.push_back(LonLat::fromDegrees(16.3, 59.4));
  ASSERT_TRUE(fs->addDynamicCrossection("smhi", positions));

  Inventory_cp inv = fs->getInventory();
  Crossection_cp csdyn = inv->findCrossectionByLabel("smhi");
  ASSERT_TRUE(csdyn);
  ASSERT_LE(positions.size(), csdyn->length());
  ASSERT_EQ(positions.size(), csdyn->lengthRequested());

  FieldData_cp airtemp = inv->findFieldById("air_temperature_ml");
  ASSERT_TRUE(airtemp);
  ZAxisData_cp vertical = airtemp->zaxis();
  ASSERT_TRUE(vertical);
  InventoryBase_cp vertical_pressure = airtemp->zaxis()->pressureField();
  ASSERT_TRUE(vertical_pressure);
  InventoryBase_cp vertical_altitude = airtemp->zaxis()->altitudeField();
  ASSERT_TRUE(vertical_altitude);

  const Time& time = inv->times.at(0);

  InventoryBase_cps request;
  request.insert(airtemp);
  request.insert(vertical_pressure);
  request.insert(vertical_altitude);
  name2value_t n2v;
  fs->getCrossectionValues(csdyn, time, request, n2v);

  Values_cp vertical_p_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(vertical_p_values);
  Values_cp vertical_h_values = n2v[vertical_altitude->id()];
  ASSERT_TRUE(vertical_h_values);
  Values_cp airtemp_values = n2v[airtemp->id()];
  ASSERT_TRUE(airtemp_values);

  const Values::Shape& shape(vertical_p_values->shape());
  ASSERT_EQ(2, shape.rank());
  EXPECT_EQ(Values::GEO_X,    shape.name(0));
  EXPECT_LE(positions.size(), shape.length(0));
  EXPECT_EQ(Values::GEO_Z,    shape.name(1));
  EXPECT_EQ(AROMESMHI_N_Z,    shape.length(1));

  ASSERT_EQ(shape.names(),   vertical_h_values->shape().names());
  ASSERT_EQ(shape.lengths(), vertical_h_values->shape().lengths());

  ASSERT_EQ(shape.names(),   airtemp_values->shape().names());
  ASSERT_EQ(shape.lengths(), airtemp_values->shape().lengths());

  Values::ShapeIndex idx(vertical_p_values->shape());
  idx.set(Values::GEO_X, positions.size()-1);
  idx.set(Values::GEO_Z, 13);
  EXPECT_FLOAT_EQ(276.83298, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(10943.816, vertical_h_values->value(idx));
  EXPECT_FLOAT_EQ(218.79533, airtemp_values->value(idx));

  idx.set(Values::GEO_Z, 3);
  EXPECT_FLOAT_EQ(70.039345,  vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(22535.854,  vertical_h_values->value(idx));
  EXPECT_FLOAT_EQ(227.53127,  airtemp_values->value(idx));

  idx.set(Values::GEO_X, 0);
  EXPECT_FLOAT_EQ(70.039894, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(22535.787, vertical_h_values->value(idx));
  EXPECT_FLOAT_EQ(227.47379, airtemp_values->value(idx));
}

TEST(FimexReftimeSourceTest, TestBangladeshVcross)
{
  configureLogging();
  ReftimeSource_p fs = openFimexFile(BANGLADESH_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(inv);
  ASSERT_EQ(BANGLADESH_N_TIME, inv->times.npoint());
  ASSERT_EQ(BANGLADESH_N_CS,   inv->crossections.size());

  Crossection_cp cs3 = inv->crossections.at(3);
  ASSERT_TRUE(cs3);
  EXPECT_EQ(BANGLADESH_CS_LEN[3], cs3->length());

  FieldData_cp temperature = inv->findFieldById("air_temperature_pl");
  ASSERT_TRUE(temperature);
  ZAxisData_cp vertical = temperature->zaxis();
  ASSERT_TRUE(vertical);
  InventoryBase_cp vertical_pressure = temperature->zaxis()->pressureField();
  ASSERT_TRUE(vertical_pressure);
  InventoryBase_cp vertical_altitude = temperature->zaxis()->altitudeField();
  ASSERT_TRUE(vertical_altitude);

  const Time& time = inv->times.at(1);

  InventoryBase_cps request;
  request.insert(temperature);
  request.insert(vertical);
  request.insert(vertical_pressure);
  request.insert(vertical_altitude);
  name2value_t n2v;
  fs->getCrossectionValues(cs3, time, request, n2v);

  Values_cp vertical_values = n2v[vertical->id()];
  ASSERT_TRUE(vertical_values);
  Values_cp vertical_p_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(vertical_p_values);
  Values_cp vertical_h_values = n2v[vertical_altitude->id()];
  ASSERT_TRUE(vertical_h_values);
  Values_cp temperature_values = n2v[temperature->id()];
  ASSERT_TRUE(temperature_values);

  const Values::Shape& shape(vertical_p_values->shape());
  ASSERT_EQ(2, shape.rank());
  EXPECT_EQ(Values::GEO_X, shape.name(0));
  EXPECT_EQ(BANGLADESH_CS_LEN[3], shape.length(0));
  EXPECT_EQ(Values::GEO_Z, shape.name(1));
  EXPECT_EQ(BANGLADESH_N_Z, shape.length(1));

  ASSERT_EQ(shape.names(), vertical_h_values->shape().names());
  ASSERT_EQ(shape.lengths(), vertical_h_values->shape().lengths());

  Values::ShapeIndex idx(vertical_p_values->shape());
  idx.set(Values::GEO_X, 43);
  idx.set(Values::GEO_Z, 13);
  EXPECT_FLOAT_EQ(925, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(768.59125, vertical_h_values->value(idx));
  EXPECT_FLOAT_EQ(294.87329, temperature_values->value(idx));

  idx.set(Values::GEO_Z, 3);
  EXPECT_FLOAT_EQ(70, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(22540.594, vertical_h_values->value(idx));
  EXPECT_FLOAT_EQ(193.71919, temperature_values->value(idx));

  idx.set(Values::GEO_X, 2);
  EXPECT_FLOAT_EQ(70, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(22540.594, vertical_h_values->value(idx));
  EXPECT_FLOAT_EQ(197.3261, temperature_values->value(idx));

  // check vertical axis (isobaric)
  const Values::Shape& shape_v(vertical_values->shape());
  ASSERT_EQ(1, shape_v.rank());
  EXPECT_EQ(Values::GEO_Z, shape_v.name(0));
  EXPECT_EQ(BANGLADESH_N_Z, shape_v.length(0));

  Values::ShapeIndex idx_v(shape_v);
  idx_v.set(Values::GEO_Z, 13);
  EXPECT_FLOAT_EQ(925, vertical_values->value(idx_v));

  idx_v.set(Values::GEO_Z, 3);
  EXPECT_FLOAT_EQ(70, vertical_values->value(idx_v));
}

TEST(FimexReftimeSourceTest, TestBangladeshVprof)
{
  configureLogging();
  ReftimeSource_p fs = openFimexFile(BANGLADESH_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(inv);
  ASSERT_EQ(BANGLADESH_N_TIME, inv->times.npoint());
  ASSERT_EQ(BANGLADESH_N_CS,   inv->crossections.size());

  Crossection_cp cs0 = inv->crossections.at(0);
  ASSERT_TRUE(cs0);
  EXPECT_EQ(BANGLADESH_CS_LEN[0], cs0->length());

  FieldData_cp temperature = inv->findFieldById("air_temperature_pl");
  ASSERT_TRUE(temperature);
  ZAxisData_cp vertical = temperature->zaxis();
  ASSERT_TRUE(vertical);
  InventoryBase_cp vertical_pressure = temperature->zaxis()->pressureField();
  ASSERT_TRUE(vertical_pressure);

  const Time& time = inv->times.at(1);

  InventoryBase_cps request;
  request.insert(temperature);
  request.insert(vertical);
  request.insert(vertical_pressure);
  name2value_t n2v;
  fs->getPointValues(cs0, 0, time, request, n2v);

  Values_cp vertical_values = n2v[vertical->id()];
  ASSERT_TRUE(vertical_values);
  Values_cp vertical_p_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(vertical_p_values);
  Values_cp temperature_values = n2v[temperature->id()];
  ASSERT_TRUE(temperature_values);

  const Values::Shape& shape(vertical_p_values->shape());
  ASSERT_EQ(1, shape.rank());
  EXPECT_EQ(Values::GEO_Z, shape.name(0));
  EXPECT_EQ(BANGLADESH_N_Z, shape.length(0));

  ASSERT_EQ(shape.names(),   vertical_values->shape().names());
  ASSERT_EQ(shape.lengths(), vertical_values->shape().lengths());

  Values::ShapeIndex idx(vertical_p_values->shape());
  idx.set(Values::GEO_Z, 13);
  EXPECT_FLOAT_EQ(925, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(925, vertical_values->value(idx));
  EXPECT_FLOAT_EQ(291.81891, temperature_values->value(idx));

  idx.set(Values::GEO_Z, 3);
  EXPECT_FLOAT_EQ(70, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(70, vertical_values->value(idx));
  EXPECT_FLOAT_EQ(198.53999, temperature_values->value(idx));
}

TEST(FimexReftimeSourceTest, WaveSpectra1)
{
  configureLogging();
  ReftimeSource_p fs = openFimexFile(WAVE_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(inv);
  ASSERT_EQ(1, inv->times.npoint());
  ASSERT_EQ(1, inv->crossections.size());

  Crossection_cp cs0 = inv->crossections.at(0);
  ASSERT_TRUE(cs0);
  EXPECT_EQ(5, cs0->length());

  FieldData_cp spec = inv->findFieldById("SPEC");
  FieldData_cp freq = inv->findFieldById("freq");
  ASSERT_TRUE(spec);
  ASSERT_TRUE(freq);
  EXPECT_FALSE(spec->hasZAxis());

  InventoryBase_cps request;
  request.insert(spec);
  request.insert(freq);
  name2value_t n2v;
  fs->getWaveSpectrumValues(cs0, 2, inv->times.at(0), request, n2v);

  { Values_cp spec_values = n2v[spec->id()];
    ASSERT_TRUE(spec_values);

    const Values::Shape& shape(spec_values->shape());

    ASSERT_EQ(2, shape.rank());
    EXPECT_EQ("direction",   shape.name(0));
    EXPECT_EQ(36,            shape.length(0));
    EXPECT_EQ("freq",        shape.name(1));
    EXPECT_EQ(36,            shape.length(1));

    Values::ShapeIndex idx(spec_values->shape());
    idx.set("direction", 1);
    idx.set("freq", 5);
    EXPECT_FLOAT_EQ(0.0675519, spec_values->value(idx));
    idx.set("freq", 31);
    EXPECT_FLOAT_EQ(0.0008872001, spec_values->value(idx));
  }
  { Values_cp freq_values = n2v[freq->id()];
    ASSERT_TRUE(freq_values);
    const Values::Shape& shape(freq_values->shape());

    ASSERT_EQ(1, shape.rank());
    EXPECT_EQ("freq", shape.name(0));
    EXPECT_EQ(36,     shape.length(0));

    Values::ShapeIndex idx(freq_values->shape());
    idx.set("freq", 2);
    EXPECT_FLOAT_EQ(0.04177283, freq_values->value(idx));
  }
}

TEST(FimexSourceTest, TestAromeReftimes)
{
  configureLogging();

  unlink(TEST_BUILDDIR "test_arome_reftimes/arome_vc_20150125_12.nc");
  unlink(TEST_BUILDDIR "test_arome_reftimes/arome_vc_20150125_18.nc");
  unlink(TEST_BUILDDIR "test_arome_reftimes/arome_vc_20150126_00.nc");
  unlink(TEST_BUILDDIR "test_arome_reftimes/arome_vc_20150126_06.nc");
  rmdir(TEST_BUILDDIR "test_arome_reftimes");

  ASSERT_EQ(0, mkdir(TEST_BUILDDIR "test_arome_reftimes", 0700));

  FimexSource s(TEST_BUILDDIR "test_arome_reftimes/arome_vc_[yyyymmdd_HH].nc", "netcdf", "");
  EXPECT_FALSE(s.update());

  { ASSERT_EQ(0, symlink(TEST_SRCDIR "/arome_vc_20150126_06.nc", TEST_BUILDDIR "test_arome_reftimes/arome_vc_20150126_06.nc"));
    const Source::ReftimeUpdate u = s.update();
    EXPECT_EQ(0, u.gone.size());
    EXPECT_EQ(0, u.changed.size());
    EXPECT_EQ(1, u.appeared.size());
  }

  { ASSERT_EQ(0, symlink(TEST_SRCDIR "/arome_vc_20150125_12.nc", TEST_BUILDDIR "test_arome_reftimes/arome_vc_20150125_12.nc"));
    ASSERT_EQ(0, symlink(TEST_SRCDIR "/arome_vc_20150125_18.nc", TEST_BUILDDIR "test_arome_reftimes/arome_vc_20150125_18.nc"));
    ASSERT_EQ(0, symlink(TEST_SRCDIR "/arome_vc_20150126_00.nc", TEST_BUILDDIR "test_arome_reftimes/arome_vc_20150126_00.nc"));
    const Source::ReftimeUpdate u = s.update();
    EXPECT_EQ(0, u.gone.size());
    EXPECT_EQ(0, u.changed.size());
    EXPECT_EQ(3, u.appeared.size());
  }

  { unlink(TEST_BUILDDIR "test_arome_reftimes/arome_vc_20150125_12.nc");
    const Source::ReftimeUpdate u = s.update();
    EXPECT_EQ(1, u.gone.size());
    EXPECT_EQ(0, u.changed.size());
    EXPECT_EQ(0, u.appeared.size());
  }
}

TEST(FimexSourceTest, TestAromeReftimesStar)
{
  configureLogging();

  unlink(TEST_BUILDDIR "test_arome_reftimes/arome_vc_20150125_12.nc");
  unlink(TEST_BUILDDIR "test_arome_reftimes/arome_vc_20150125_18.nc");
  unlink(TEST_BUILDDIR "test_arome_reftimes/arome_vc_20150126_00.nc");
  unlink(TEST_BUILDDIR "test_arome_reftimes/arome_vc_20150126_06.nc");
  rmdir(TEST_BUILDDIR "test_arome_reftimes");

  ASSERT_EQ(0, mkdir(TEST_BUILDDIR "test_arome_reftimes", 0700));

  FimexSource s(TEST_BUILDDIR "test_arome_reftimes/arome_vc_*.nc", "netcdf", "");
  EXPECT_FALSE(s.update());

  { ASSERT_EQ(0, symlink(TEST_SRCDIR "/arome_vc_20150126_06.nc", TEST_BUILDDIR "test_arome_reftimes/arome_vc_20150126_06.nc"));
    const Source::ReftimeUpdate u = s.update();
    EXPECT_EQ(0, u.gone.size());
    EXPECT_EQ(0, u.changed.size());
    ASSERT_EQ(1, u.appeared.size());
    EXPECT_TRUE(u.appeared.count(Time(S1970, 1422252000)));
  }

  { ASSERT_EQ(0, symlink(TEST_SRCDIR "/arome_vc_20150125_12.nc", TEST_BUILDDIR "test_arome_reftimes/arome_vc_20150125_12.nc"));
    ASSERT_EQ(0, symlink(TEST_SRCDIR "/arome_vc_20150125_18.nc", TEST_BUILDDIR "test_arome_reftimes/arome_vc_20150125_18.nc"));
    ASSERT_EQ(0, symlink(TEST_SRCDIR "/arome_vc_20150126_00.nc", TEST_BUILDDIR "test_arome_reftimes/arome_vc_20150126_00.nc"));
    const Source::ReftimeUpdate u = s.update();
    EXPECT_EQ(0, u.gone.size());
    EXPECT_EQ(0, u.changed.size());
    ASSERT_EQ(3, u.appeared.size());
    EXPECT_TRUE(u.appeared.count(Time(S1970, 1422187200)));
    EXPECT_TRUE(u.appeared.count(Time(S1970, 1422208800)));
    EXPECT_TRUE(u.appeared.count(Time(S1970, 1422230400)));
  }

  { unlink(TEST_BUILDDIR "test_arome_reftimes/arome_vc_20150125_12.nc");
    const Source::ReftimeUpdate u = s.update();
    ASSERT_EQ(1, u.gone.size());
    EXPECT_TRUE(u.gone.count(Time(S1970, 1422187200)));
    EXPECT_EQ(0, u.changed.size());
    EXPECT_EQ(0, u.appeared.size());
  }
}

// add test for 1-d value
// add test for dynamic cuts
