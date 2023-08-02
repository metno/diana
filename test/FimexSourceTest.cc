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

#include <FimexSource.h>
#include <VcrossUtil.h>

#include <gtest/gtest.h>

#include <fstream>
#include <string>

#define MILOGGER_CATEGORY "diField.test.FimexSourceTest"
#include "miLogger/miLogging.h"

using diutil::Values;
using namespace vcross;

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

static const char HIRLAM_DYN_FILE[] = "hirlam_dyn.nc";
static const int HIRLAM_DYN_N_TIME = 6;

static const char AROMESMHI_FILE[] = "arome_smhi.nc";
static const int AROMESMHI_N_TIME = 4;
static const int AROMESMHI_N_Z = 65;

static const char BANGLADESH_FILE[] = "bangladesh_vc.nc";
static const int BANGLADESH_N_CS = 4;
static const int BANGLADESH_CS_LEN[BANGLADESH_N_CS] = { 1, 1, 96, 124 };
static const int BANGLADESH_N_TIME = 5;
static const int BANGLADESH_N_Z = 15;

static const char NORKYST_FILE[] = "norkyst_z.nc";
static const int NORKYST_N_CS = 4;
static const int NORKYST_CS_LEN[NORKYST_N_CS] = {59, 24, 1, 1};
static const int NORKYST_N_TIME = 3;
static const int NORKYST_N_Z = 12;

static const char NORDIC_FILE[] = "nordic_sg.nc";
static const int NORDIC_N_CS = 5;
static const int NORDIC_CS_LEN[NORDIC_N_CS] = {11, 5, 1, 1, 1};
static const int NORDIC_N_TIME = 3;
static const int NORDIC_N_Z = 35;

static const char WAVE_FILE[] = "wavespec.nc";

static const char S1970[] = "seconds since 1970-01-01 00:00:00";

static diutil::CharsetConverter_p csc()
{
  return diutil::findConverter(diutil::CHARSET_READ(), diutil::CHARSET_INTERNAL());
}

static ReftimeSource_p openFimexFile(const std::string& file)
{
  const std::string fileName = (TEST_SRCDIR "/") + file;
  std::ifstream inputfile(fileName.c_str());
  if (not inputfile)
    return ReftimeSource_p();

  return ReftimeSource_p(new FimexReftimeSource(fileName, "netcdf", "", csc(), Time()));
}

TEST(FimexReftimeSourceTest, TestSimraVcross0)
{
  ReftimeSource_p fs = openFimexFile(SIMRA_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(bool(inv));
  ASSERT_EQ(SIMRA_N_TIME, inv->times.npoint());
  ASSERT_EQ(SIMRA_N_CS,   inv->crossections.size());

  Crossection_cp cs0 = inv->crossections.at(0);
  ASSERT_TRUE(bool(cs0));
  EXPECT_EQ(SIMRA_CS_LEN[0], cs0->length());

  FieldData_cp turbulence = inv->findFieldById("turbulence");
  ASSERT_TRUE(bool(turbulence));
  EXPECT_EQ("FIELD", turbulence->dataType());
  EXPECT_EQ(SIMRA_N_Z,      turbulence->nlevel());

  const Time& time = inv->times.at(0);

  InventoryBase_cps request;
  request.insert(turbulence);
  name2value_t n2v;
  fs->getCrossectionValues(cs0, time, request, n2v, 0);

  auto turbulence_values = n2v[turbulence->id()];
  ASSERT_TRUE(bool(turbulence_values));

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
  ReftimeSource_p fs = openFimexFile(SIMRA_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(bool(inv));
  ASSERT_EQ(SIMRA_N_TIME, inv->times.npoint());
  ASSERT_EQ(SIMRA_N_CS, inv->crossections.size());

  Crossection_cp cs1 = inv->crossections.at(1);
  ASSERT_TRUE(bool(cs1));
  EXPECT_EQ(SIMRA_CS_LEN[1], cs1->length());

  FieldData_cp turbulence = inv->findFieldById("turbulence");
  ASSERT_TRUE(bool(turbulence));

  const Time& time = inv->times.at(1);

  InventoryBase_cps request;
  request.insert(turbulence);
  name2value_t n2v;
  fs->getCrossectionValues(cs1, time, request, n2v, 0);

  auto turbulence_values = n2v[turbulence->id()];
  ASSERT_TRUE(bool(turbulence_values));

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
  ReftimeSource_p fs = openFimexFile(SIMRA_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(bool(inv));
  ASSERT_EQ(SIMRA_N_TIME, inv->times.npoint());
  ASSERT_EQ(SIMRA_N_CS,   inv->crossections.size());

  Crossection_cp cs1 = inv->crossections.at(1);
  ASSERT_TRUE(bool(cs1));
  EXPECT_EQ(SIMRA_CS_LEN[1], cs1->length());

  FieldData_cp turbulence = inv->findFieldById("turbulence");
  ASSERT_TRUE(bool(turbulence));
  ZAxisData_cp vertical = turbulence->zaxis();
  ASSERT_TRUE(bool(vertical));
  InventoryBase_cp vertical_pressure = turbulence->zaxis()->pressureField();
  ASSERT_TRUE(bool(vertical_pressure));
  InventoryBase_cp vertical_altitude = turbulence->zaxis()->altitudeField();
  ASSERT_TRUE(bool(vertical_altitude));

  const Time& time = inv->times.at(1);

  InventoryBase_cps request;
  request.insert(turbulence);
  request.insert(vertical_pressure);
  request.insert(vertical_altitude);
  name2value_t n2v;
  fs->getCrossectionValues(cs1, time, request, n2v, 0);

  auto vertical_p_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(bool(vertical_p_values));
  auto vertical_h_values = n2v[vertical_altitude->id()];
  ASSERT_TRUE(bool(vertical_h_values));

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
  ReftimeSource_p fs = openFimexFile(SIMRA_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(bool(inv));
  ASSERT_EQ(SIMRA_N_TIME, inv->times.npoint());
  ASSERT_EQ(SIMRA_N_CS,   inv->crossections.size());

  Crossection_cp cs0 = inv->crossections.at(0);
  ASSERT_TRUE(bool(cs0));
  EXPECT_EQ(SIMRA_CS_LEN[0], cs0->length());

  FieldData_cp turbulence = inv->findFieldById("turbulence");
  ASSERT_TRUE(bool(turbulence));

  InventoryBase_cps request;
  request.insert(turbulence);
  name2value_t n2v;
  fs->getTimegraphValues(cs0, 5, request, n2v, 0);

  auto turbulence_values = n2v[turbulence->id()];
  ASSERT_TRUE(bool(turbulence_values));

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
  ReftimeSource_p fs = openFimexFile(SIMRA_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(bool(inv));
  ASSERT_EQ(SIMRA_N_TIME, inv->times.npoint());
  ASSERT_EQ(SIMRA_N_CS,   inv->crossections.size());

  Crossection_cp cs1 = inv->crossections.at(1);
  ASSERT_TRUE(bool(cs1));

  FieldData_cp turbulence = inv->findFieldById("turbulence");
  ASSERT_TRUE(bool(turbulence));

  const Time& time = inv->times.at(1);
  InventoryBase_cps request;
  request.insert(turbulence);
  name2value_t n2v;
  fs->getPointValues(cs1, 17, time, request, n2v, 0);

  auto turbulence_values = n2v[turbulence->id()];
  ASSERT_TRUE(bool(turbulence_values));

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
  ReftimeSource_p fs = openFimexFile(AROME_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(bool(inv));
  ASSERT_EQ(AROME_N_TIME, inv->times.npoint());
  ASSERT_EQ(AROME_N_CS, inv->crossections.size());

  Crossection_cp cs1 = inv->crossections.at(1);
  ASSERT_TRUE(bool(cs1));
  EXPECT_EQ(AROME_CS_LEN[1], cs1->length());

  FieldData_cp temperature = inv->findFieldById("air_temperature_ml");
  ASSERT_TRUE(bool(temperature));

  const Time& time = inv->times.at(1);

  InventoryBase_cps request;
  request.insert(temperature);
  name2value_t n2v;
  fs->getCrossectionValues(cs1, time, request, n2v, 0);

  auto temperature_values = n2v[temperature->id()];
  ASSERT_TRUE(bool(temperature_values));

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
  ReftimeSource_p fs = openFimexFile(AROME_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(bool(inv));
  ASSERT_EQ(AROME_N_TIME, inv->times.npoint());
  ASSERT_EQ(AROME_N_CS,   inv->crossections.size());

  Crossection_cp cs1 = inv->crossections.at(1);
  ASSERT_TRUE(bool(cs1));
  EXPECT_EQ(AROME_CS_LEN[1], cs1->length());

  FieldData_cp temperature = inv->findFieldById("air_temperature_ml");
  ASSERT_TRUE(bool(temperature));
  ZAxisData_cp vertical = temperature->zaxis();
  ASSERT_TRUE(bool(vertical));
  InventoryBase_cp vertical_pressure = temperature->zaxis()->pressureField();
  ASSERT_TRUE(bool(vertical_pressure));
  InventoryBase_cp vertical_altitude = temperature->zaxis()->altitudeField();
  ASSERT_TRUE(bool(vertical_altitude));

  const Time& time = inv->times.at(1);

  InventoryBase_cps request;
  request.insert(temperature);
  request.insert(vertical_pressure);
  request.insert(vertical_altitude);
  name2value_t n2v;
  fs->getCrossectionValues(cs1, time, request, n2v, 0);

  auto vertical_p_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(bool(vertical_p_values));
  auto vertical_h_values = n2v[vertical_altitude->id()];
  ASSERT_TRUE(bool(vertical_h_values));

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
  ReftimeSource_p fs = openFimexFile(AROME_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(bool(inv));
  ASSERT_EQ(AROME_N_TIME, inv->times.npoint());
  ASSERT_EQ(AROME_N_CS,   inv->crossections.size());

  Crossection_cp cs1 = inv->crossections.at(1);
  ASSERT_TRUE(bool(cs1));
  EXPECT_EQ(AROME_CS_LEN[1], cs1->length());

  FieldData_cp temperature = inv->findFieldById("air_temperature_ml");
  ASSERT_TRUE(bool(temperature));
  ZAxisData_cp vertical = temperature->zaxis();
  ASSERT_TRUE(bool(vertical));
  InventoryBase_cp vertical_pressure = temperature->zaxis()->pressureField();
  ASSERT_TRUE(bool(vertical));

  InventoryBase_cps request;
  request.insert(temperature);
  request.insert(vertical_pressure);
  name2value_t n2v;
  fs->getTimegraphValues(cs1, 43, request, n2v, 0);

  auto temperature_values = n2v[temperature->id()];
  ASSERT_TRUE(bool(temperature_values));
  auto vertical_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(bool(vertical_values));

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
  ReftimeSource_p fs = openFimexFile(HIRLAM_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(bool(inv));
  ASSERT_EQ(HIRLAM_N_TIME, inv->times.npoint());
  ASSERT_EQ(HIRLAM_N_CS,   inv->crossections.size());

  Crossection_cp cs1 = inv->crossections.at(1);
  ASSERT_TRUE(bool(cs1));
  EXPECT_EQ(HIRLAM_CS_LEN[1], cs1->length());

  FieldData_cp air_pt = inv->findFieldById("air_potential_temperature_ml");
  ASSERT_TRUE(bool(air_pt));
  ZAxisData_cp vertical = air_pt->zaxis();
  ASSERT_TRUE(bool(vertical));
  InventoryBase_cp vertical_pressure = air_pt->zaxis()->pressureField();
  ASSERT_TRUE(bool(vertical_pressure));
  InventoryBase_cp vertical_altitude = air_pt->zaxis()->altitudeField();
  ASSERT_TRUE(bool(vertical_altitude));

  const Time& time = inv->times.at(0);

  InventoryBase_cps request;
  request.insert(air_pt);
  request.insert(vertical_pressure);
  request.insert(vertical_altitude);
  name2value_t n2v;
  fs->getCrossectionValues(cs1, time, request, n2v, 0);

  auto vertical_p_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(bool(vertical_p_values));
  auto vertical_h_values = n2v[vertical_altitude->id()];
  ASSERT_TRUE(bool(vertical_h_values));
  auto air_pt_values = n2v[air_pt->id()];
  ASSERT_TRUE(bool(air_pt_values));

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

#if 0 // disabled due to a problem with fimex
TEST(FimexReftimeSourceTest, TestEmepDynVcross)
{
  ReftimeSource_p fs = openFimexFile(EMEP_ETNA_FILE);
  if (not fs)
    return;

  { Inventory_cp inv = fs->getInventory();
    ASSERT_TRUE(bool(inv));
    ASSERT_EQ(EMEP_ETNA_N_TIME, inv->times.npoint());
    ASSERT_EQ(0, inv->crossections.size());
  }

  ASSERT_TRUE(bool(fs->supportsDynamicCrossections()));
  LonLat_v positions;
  positions.push_back(LonLat::fromDegrees(14.995, 37.755));
  positions.push_back(LonLat::fromDegrees(16, 39));
  positions.push_back(LonLat::fromDegrees(13, 39));
  positions.push_back(LonLat::fromDegrees(13, 36));
  positions.push_back(LonLat::fromDegrees(16, 36));
  ASSERT_TRUE(bool(fs->addDynamicCrossection("etna", positions)));

  Inventory_cp inv = fs->getInventory();
  Crossection_cp csdyn = inv->findCrossectionByLabel("etna");
  ASSERT_TRUE(bool(csdyn));
  ASSERT_LE(positions.size(), csdyn->length());
  ASSERT_EQ(positions.size(), csdyn->lengthRequested());
  for (size_t i=0; i<positions.size(); ++i)
    ASSERT_LE(positions.at(i).distanceTo(csdyn->pointRequested(i)), 10) << "i=" << i;

  FieldData_cp ash_c = inv->findFieldById("ash_concentration");
  ASSERT_TRUE(bool(ash_c));
  ZAxisData_cp vertical = ash_c->zaxis();
  ASSERT_TRUE(bool(vertical));
  InventoryBase_cp vertical_pressure = ash_c->zaxis()->pressureField();
  ASSERT_TRUE(bool(vertical_pressure));
  InventoryBase_cp vertical_altitude = ash_c->zaxis()->altitudeField();
  ASSERT_TRUE(bool(vertical_altitude));

  const Time& time = inv->times.at(0);

  InventoryBase_cps request;
  request.insert(ash_c);
  request.insert(vertical_pressure);
  request.insert(vertical_altitude);
  name2value_t n2v;
  fs->getCrossectionValues(csdyn, time, request, n2v, 0);

  auto vertical_p_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(bool(vertical_p_values));
  auto vertical_h_values = n2v[vertical_altitude->id()];
  ASSERT_TRUE(bool(vertical_h_values));
  auto ash_c_values = n2v[ash_c->id()];
  ASSERT_TRUE(bool(ash_c_values));

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

  idx.set(Values::GEO_X, positions.size()-1);
  idx.set(Values::GEO_Z, 13);

  float ex_p = 505.81207, ac_p = vertical_p_values->value(idx);
  if (fabsf(ex_p - ac_p) >= 1e-4) {
    // newer fimex versions (after svn rev 1908) do not insert duplicate
    // points between line segments
    idx.set(Values::GEO_X, positions.size()-2);
  }
  EXPECT_FLOAT_EQ(ex_p, vertical_p_values->value(idx));
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
#endif // disabled due to a problem with fimex

TEST(FimexReftimeSourceTest, TestHirlamVcrossDyn)
{
  ReftimeSource_p fs = openFimexFile(HIRLAM_DYN_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(bool(inv));
  ASSERT_EQ(HIRLAM_DYN_N_TIME, inv->times.npoint());
  ASSERT_EQ(0, inv->crossections.size());

  ASSERT_TRUE(fs->supportsDynamicCrossections());
  LonLat_v positions;
  positions.push_back(LonLat::fromDegrees(15.46,78.25));
  Crossection_cp cs0 = fs->addDynamicCrossection("LONGYEARBYEN", positions);
  ASSERT_TRUE(bool(cs0));

  FieldData_cp air_pt = inv->findFieldById("air_potential_temperature_ml");
  ASSERT_TRUE(bool(air_pt));
  ZAxisData_cp vertical = air_pt->zaxis();
  ASSERT_TRUE(bool(vertical));
  InventoryBase_cp vertical_pressure = air_pt->zaxis()->pressureField();
  ASSERT_TRUE(bool(vertical_pressure));
  InventoryBase_cp vertical_altitude = air_pt->zaxis()->altitudeField();
  ASSERT_TRUE(bool(vertical_altitude));

  InventoryBase_cps request;
  request.insert(air_pt);
  request.insert(vertical_pressure);
  request.insert(vertical_altitude);
  name2value_t n2v;
  fs->getTimegraphValues(cs0, 0, request, n2v, 0);

  auto vertical_p_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(bool(vertical_p_values));
  auto vertical_h_values = n2v[vertical_altitude->id()];
  ASSERT_TRUE(bool(vertical_h_values));
  auto air_pt_values = n2v[air_pt->id()];
  ASSERT_TRUE(bool(air_pt_values));

  const Values::Shape& shape(vertical_p_values->shape());
  ASSERT_EQ(2, shape.rank());
  EXPECT_EQ(Values::TIME,      shape.name(0));
  EXPECT_EQ(HIRLAM_DYN_N_TIME, shape.length(0));
  EXPECT_EQ(Values::GEO_Z,     shape.name(1));
  EXPECT_EQ(HIRLAM_N_Z,        shape.length(1));

  ASSERT_EQ(shape.names(),   vertical_h_values->shape().names());
  ASSERT_EQ(shape.lengths(), vertical_h_values->shape().lengths());

  ASSERT_EQ(shape.names(),   air_pt_values->shape().names());
  ASSERT_EQ(shape.lengths(), air_pt_values->shape().lengths());

  Values::ShapeIndex idx(vertical_p_values->shape());
  idx.set(Values::TIME, 0);
  idx.set(Values::GEO_Z, 39);
  EXPECT_FLOAT_EQ(747.15155, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(2405,      vertical_h_values->value(idx));
  EXPECT_FLOAT_EQ(293,       air_pt_values->value(idx));

  idx.set(Values::GEO_Z, 3);
  EXPECT_FLOAT_EQ(71.458359, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(18212,     vertical_h_values->value(idx));
  EXPECT_FLOAT_EQ(451.5,     air_pt_values->value(idx));

  idx.set(Values::TIME, 3);
  EXPECT_FLOAT_EQ(71.487221, vertical_p_values->value(idx));
  EXPECT_FLOAT_EQ(18205,     vertical_h_values->value(idx));
  EXPECT_FLOAT_EQ(449.39999, air_pt_values->value(idx));
}

TEST(FimexReftimeSourceTest, TestAromeSmhiDynVcross)
{
  ReftimeSource_p fs = openFimexFile(AROMESMHI_FILE);
  if (not fs)
    return;

  { Inventory_cp inv = fs->getInventory();
    ASSERT_TRUE(bool(inv));
    ASSERT_EQ(AROMESMHI_N_TIME, inv->times.npoint());
    ASSERT_EQ(0, inv->crossections.size());
  }

  ASSERT_TRUE(bool(fs->supportsDynamicCrossections()));
  LonLat_v positions;
  positions.push_back(LonLat::fromDegrees(16.4, 59.3));
  positions.push_back(LonLat::fromDegrees(16.4, 59.4));
  positions.push_back(LonLat::fromDegrees(16.3, 59.4));
  ASSERT_TRUE(bool(fs->addDynamicCrossection("smhi", positions)));

  Inventory_cp inv = fs->getInventory();
  Crossection_cp csdyn = inv->findCrossectionByLabel("smhi");
  ASSERT_TRUE(bool(csdyn));
  ASSERT_LE(positions.size(), csdyn->length());
  ASSERT_EQ(positions.size(), csdyn->lengthRequested());

  FieldData_cp airtemp = inv->findFieldById("air_temperature_ml");
  ASSERT_TRUE(bool(airtemp));
  ZAxisData_cp vertical = airtemp->zaxis();
  ASSERT_TRUE(bool(vertical));
  InventoryBase_cp vertical_pressure = airtemp->zaxis()->pressureField();
  ASSERT_TRUE(bool(vertical_pressure));
  InventoryBase_cp vertical_altitude = airtemp->zaxis()->altitudeField();
  ASSERT_TRUE(bool(vertical_altitude));

  const Time& time = inv->times.at(0);

  InventoryBase_cps request;
  request.insert(airtemp);
  request.insert(vertical_pressure);
  request.insert(vertical_altitude);
  name2value_t n2v;
  fs->getCrossectionValues(csdyn, time, request, n2v, 0);

  auto vertical_p_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(bool(vertical_p_values));
  auto vertical_h_values = n2v[vertical_altitude->id()];
  ASSERT_TRUE(bool(vertical_h_values));
  auto airtemp_values = n2v[airtemp->id()];
  ASSERT_TRUE(bool(airtemp_values));

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

  // --------------------------------------------------

  Crossection_cp csdyn1 = inv->findCrossectionByLabel("smhi");
  ASSERT_TRUE(bool(csdyn1));
  const Time& time1 = inv->times.at(1);
  name2value_t n2v1;
  fs->getCrossectionValues(csdyn1, time1, request, n2v1, 0);

  vertical_p_values = n2v1[vertical_pressure->id()];
  ASSERT_TRUE(bool(vertical_p_values));
  vertical_h_values = n2v1[vertical_altitude->id()];
  ASSERT_TRUE(bool(vertical_h_values));
  airtemp_values = n2v1[airtemp->id()];
  ASSERT_TRUE(bool(airtemp_values));

  const Values::Shape& shape1(vertical_p_values->shape());
  ASSERT_EQ(2, shape.rank());
  EXPECT_EQ(Values::GEO_X,    shape1.name(0));
  EXPECT_LE(positions.size(), shape1.length(0));
  EXPECT_EQ(Values::GEO_Z,    shape1.name(1));
  EXPECT_EQ(AROMESMHI_N_Z,    shape1.length(1));

  ASSERT_EQ(shape1.names(),   vertical_h_values->shape().names());
  ASSERT_EQ(shape1.lengths(), vertical_h_values->shape().lengths());

  ASSERT_EQ(shape1.names(),   airtemp_values->shape().names());
  ASSERT_EQ(shape1.lengths(), airtemp_values->shape().lengths());

  Values::ShapeIndex idx1(vertical_p_values->shape());
  idx1.set(Values::GEO_X, positions.size()-1);
  idx1.set(Values::GEO_Z, 13);
  EXPECT_FLOAT_EQ(276.90195, vertical_p_values->value(idx1));
  EXPECT_FLOAT_EQ(10941.716, vertical_h_values->value(idx1));
  EXPECT_FLOAT_EQ(219.45538, airtemp_values->value(idx1));

  idx1.set(Values::GEO_Z, 3);
  EXPECT_FLOAT_EQ(70.039642,  vertical_p_values->value(idx1));
  EXPECT_FLOAT_EQ(22535.818,  vertical_h_values->value(idx1));
}

TEST(FimexReftimeSourceTest, TestBangladeshVcross)
{
  ReftimeSource_p fs = openFimexFile(BANGLADESH_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(bool(inv));
  ASSERT_EQ(BANGLADESH_N_TIME, inv->times.npoint());
  ASSERT_EQ(BANGLADESH_N_CS,   inv->crossections.size());

  Crossection_cp cs3 = inv->crossections.at(3);
  ASSERT_TRUE(bool(cs3));
  EXPECT_EQ(BANGLADESH_CS_LEN[3], cs3->length());

  FieldData_cp temperature = inv->findFieldById("air_temperature_pl");
  ASSERT_TRUE(bool(temperature));
  ZAxisData_cp vertical = temperature->zaxis();
  ASSERT_TRUE(bool(vertical));
  InventoryBase_cp vertical_pressure = temperature->zaxis()->pressureField();
  ASSERT_TRUE(bool(vertical_pressure));
  InventoryBase_cp vertical_altitude = temperature->zaxis()->altitudeField();
  ASSERT_TRUE(bool(vertical_altitude));

  const Time& time = inv->times.at(1);

  InventoryBase_cps request;
  request.insert(temperature);
  request.insert(vertical);
  request.insert(vertical_pressure);
  request.insert(vertical_altitude);
  name2value_t n2v;
  fs->getCrossectionValues(cs3, time, request, n2v, 0);

  auto vertical_values = n2v[vertical->id()];
  ASSERT_TRUE(bool(vertical_values));
  auto vertical_p_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(bool(vertical_p_values));
  auto vertical_h_values = n2v[vertical_altitude->id()];
  ASSERT_TRUE(bool(vertical_h_values));
  auto temperature_values = n2v[temperature->id()];
  ASSERT_TRUE(bool(temperature_values));

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
  ReftimeSource_p fs = openFimexFile(BANGLADESH_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(bool(inv));
  ASSERT_EQ(BANGLADESH_N_TIME, inv->times.npoint());
  ASSERT_EQ(BANGLADESH_N_CS,   inv->crossections.size());

  Crossection_cp cs0 = inv->crossections.at(0);
  ASSERT_TRUE(bool(cs0));
  EXPECT_EQ(BANGLADESH_CS_LEN[0], cs0->length());

  FieldData_cp temperature = inv->findFieldById("air_temperature_pl");
  ASSERT_TRUE(bool(temperature));
  ZAxisData_cp vertical = temperature->zaxis();
  ASSERT_TRUE(bool(vertical));
  InventoryBase_cp vertical_pressure = temperature->zaxis()->pressureField();
  ASSERT_TRUE(bool(vertical_pressure));

  const Time& time = inv->times.at(1);

  InventoryBase_cps request;
  request.insert(temperature);
  request.insert(vertical);
  request.insert(vertical_pressure);
  name2value_t n2v;
  fs->getPointValues(cs0, 0, time, request, n2v, 0);

  auto vertical_values = n2v[vertical->id()];
  ASSERT_TRUE(bool(vertical_values));
  auto vertical_p_values = n2v[vertical_pressure->id()];
  ASSERT_TRUE(bool(vertical_p_values));
  auto temperature_values = n2v[temperature->id()];
  ASSERT_TRUE(bool(temperature_values));

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
  ReftimeSource_p fs = openFimexFile(WAVE_FILE);
  if (not fs)
    return;

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(bool(inv));
  ASSERT_EQ(1, inv->times.npoint());
  ASSERT_EQ(1, inv->crossections.size());

  Crossection_cp cs0 = inv->crossections.at(0);
  ASSERT_TRUE(bool(cs0));
  EXPECT_EQ(5, cs0->length());

  FieldData_cp spec = inv->findFieldById("SPEC");
  FieldData_cp freq = inv->findFieldById("freq");
  ASSERT_TRUE(bool(spec));
  ASSERT_TRUE(bool(freq));
  EXPECT_FALSE(spec->hasZAxis());

  InventoryBase_cps request;
  request.insert(spec);
  request.insert(freq);
  name2value_t n2v;
  fs->getWaveSpectrumValues(cs0, 2, inv->times.at(0), request, n2v, 0);

  {
    auto spec_values = n2v[spec->id()];
    ASSERT_TRUE(bool(spec_values));

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
  {
    auto freq_values = n2v[freq->id()];
    ASSERT_TRUE(bool(freq_values));
    const Values::Shape& shape(freq_values->shape());

    ASSERT_EQ(1, shape.rank());
    EXPECT_EQ("freq", shape.name(0));
    EXPECT_EQ(36,     shape.length(0));

    Values::ShapeIndex idx(freq_values->shape());
    idx.set("freq", 2);
    EXPECT_FLOAT_EQ(0.04177283, freq_values->value(idx));
  }
}

TEST(FimexReftimeSourceTest, DepthProfileZ)
{
  ReftimeSource_p fs = openFimexFile(NORKYST_FILE);
  ASSERT_TRUE(bool(fs));

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(bool(inv));
  ASSERT_EQ(NORKYST_N_TIME, inv->times.npoint());
  ASSERT_EQ(NORKYST_N_CS, inv->crossections.size());

  Crossection_cp cs0 = inv->crossections.at(0);
  ASSERT_TRUE(bool(cs0));
  EXPECT_EQ(NORKYST_CS_LEN[0], cs0->length());

  FieldData_cp temperature = inv->findFieldById("temperature");
  ASSERT_TRUE(bool(temperature));
  ZAxisData_cp vertical = temperature->zaxis();
  ASSERT_TRUE(bool(vertical));
  EXPECT_EQ("depth", vertical->id());

  const Time& time = inv->times.at(1);

  InventoryBase_cps request;
  request.insert(temperature);
  request.insert(vertical);
  name2value_t n2v;
  fs->getPointValues(cs0, 0, time, request, n2v, 0);

  auto vertical_values = n2v[vertical->id()];
  ASSERT_TRUE(bool(vertical_values));
  auto temperature_values = n2v[temperature->id()];
  ASSERT_TRUE(bool(temperature_values));

  const Values::Shape& shape(vertical_values->shape());
  ASSERT_EQ(1, shape.rank());
  EXPECT_EQ(Values::GEO_Z, shape.name(0));
  EXPECT_EQ(NORKYST_N_Z, shape.length(0));

  ASSERT_EQ(shape.names(), vertical_values->shape().names());
  ASSERT_EQ(shape.lengths(), vertical_values->shape().lengths());

  Values::ShapeIndex idx(vertical_values->shape());
  idx.set(Values::GEO_Z, 5);
  EXPECT_FLOAT_EQ(50, vertical_values->value(idx));
  EXPECT_FLOAT_EQ(8.93, temperature_values->value(idx));

  idx.set(Values::GEO_Z, 9);
  EXPECT_FLOAT_EQ(200, vertical_values->value(idx));
  EXPECT_FLOAT_EQ(6.78, temperature_values->value(idx));
}

TEST(FimexReftimeSourceTest, DepthProfileSG)
{
  ReftimeSource_p fs = openFimexFile(NORDIC_FILE);
  ASSERT_TRUE(bool(fs));

  Inventory_cp inv = fs->getInventory();
  ASSERT_TRUE(bool(inv));
  ASSERT_EQ(NORDIC_N_TIME, inv->times.npoint());
  ASSERT_EQ(NORDIC_N_CS, inv->crossections.size());

  Crossection_cp cs0 = inv->crossections.at(0);
  ASSERT_TRUE(bool(cs0));
  EXPECT_EQ(NORDIC_CS_LEN[0], cs0->length());

  FieldData_cp temperature = inv->findFieldById("temp");
  ASSERT_TRUE(bool(temperature));
  ZAxisData_cp vertical = temperature->zaxis();
  ASSERT_TRUE(bool(vertical));
  EXPECT_EQ("s_rho", vertical->id());
  InventoryBase_cp vertical_depth = temperature->zaxis()->depthField();
  ASSERT_TRUE(bool(vertical_depth));
  EXPECT_EQ("s_rho//depth", vertical_depth->id());

  const Time& time = inv->times.at(1);

  InventoryBase_cps request;
  request.insert(temperature);
  request.insert(vertical_depth);
  name2value_t n2v;
  fs->getPointValues(cs0, 0, time, request, n2v, 0);

  auto vertical_depth_values = n2v[vertical_depth->id()];
  ASSERT_TRUE(bool(vertical_depth_values));
  auto temperature_values = n2v[temperature->id()];
  ASSERT_TRUE(bool(temperature_values));

  const Values::Shape& shape(vertical_depth_values->shape());
  ASSERT_EQ(1, shape.rank());
  EXPECT_EQ(Values::GEO_Z, shape.name(0));
  EXPECT_EQ(NORDIC_N_Z, shape.length(0));

  ASSERT_EQ(shape.names(), vertical_depth_values->shape().names());
  ASSERT_EQ(shape.lengths(), vertical_depth_values->shape().lengths());

  Values::ShapeIndex idx(vertical_depth_values->shape());
  idx.set(Values::GEO_Z, 5);
  EXPECT_FLOAT_EQ(133.85757, vertical_depth_values->value(idx));
  EXPECT_FLOAT_EQ(7.2303705, temperature_values->value(idx));

  idx.set(Values::GEO_Z, 25);
  EXPECT_FLOAT_EQ(11.73487, vertical_depth_values->value(idx));
  EXPECT_FLOAT_EQ(12.436813, temperature_values->value(idx));
}

TEST(FimexSourceTest, TestAromeReftimes)
{
  unlink(TEST_BUILDDIR "/test_arome_reftimes/arome_vc_20150125_12.nc");
  unlink(TEST_BUILDDIR "/test_arome_reftimes/arome_vc_20150125_18.nc");
  unlink(TEST_BUILDDIR "/test_arome_reftimes/arome_vc_20150126_00.nc");
  unlink(TEST_BUILDDIR "/test_arome_reftimes/arome_vc_20150126_06.nc");
  rmdir(TEST_BUILDDIR "/test_arome_reftimes");

  ASSERT_EQ(0, mkdir(TEST_BUILDDIR "/test_arome_reftimes", 0700));

  FimexSource s(TEST_BUILDDIR "/test_arome_reftimes/arome_vc_[yyyymmdd_HH].nc", "netcdf", "", csc());
  EXPECT_FALSE(s.update());

  { ASSERT_EQ(0, symlink(TEST_SRCDIR "/arome_vc_20150126_06.nc", TEST_BUILDDIR "/test_arome_reftimes/arome_vc_20150126_06.nc"));
    const Source::ReftimeUpdate u = s.update();
    EXPECT_EQ(0, u.gone.size());
    EXPECT_EQ(0, u.changed.size());
    EXPECT_EQ(1, u.appeared.size());
  }

  { ASSERT_EQ(0, symlink(TEST_SRCDIR "/arome_vc_20150125_12.nc", TEST_BUILDDIR "/test_arome_reftimes/arome_vc_20150125_12.nc"));
    ASSERT_EQ(0, symlink(TEST_SRCDIR "/arome_vc_20150125_18.nc", TEST_BUILDDIR "/test_arome_reftimes/arome_vc_20150125_18.nc"));
    ASSERT_EQ(0, symlink(TEST_SRCDIR "/arome_vc_20150126_00.nc", TEST_BUILDDIR "/test_arome_reftimes/arome_vc_20150126_00.nc"));
    const Source::ReftimeUpdate u = s.update();
    EXPECT_EQ(0, u.gone.size());
    EXPECT_EQ(0, u.changed.size());
    EXPECT_EQ(3, u.appeared.size());
  }

  { unlink(TEST_BUILDDIR "/test_arome_reftimes/arome_vc_20150125_12.nc");
    const Source::ReftimeUpdate u = s.update();
    EXPECT_EQ(1, u.gone.size());
    EXPECT_EQ(0, u.changed.size());
    EXPECT_EQ(0, u.appeared.size());
  }
}

TEST(FimexSourceTest, TestAromeReftimesStar)
{
  unlink(TEST_BUILDDIR "/test_arome_reftimes/arome_vc_20150125_12.nc");
  unlink(TEST_BUILDDIR "/test_arome_reftimes/arome_vc_20150125_18.nc");
  unlink(TEST_BUILDDIR "/test_arome_reftimes/arome_vc_20150126_00.nc");
  unlink(TEST_BUILDDIR "/test_arome_reftimes/arome_vc_20150126_06.nc");
  rmdir(TEST_BUILDDIR "/test_arome_reftimes");

  ASSERT_EQ(0, mkdir(TEST_BUILDDIR "/test_arome_reftimes", 0700));

  FimexSource s(TEST_BUILDDIR "/test_arome_reftimes/arome_vc_*.nc", "netcdf", "", csc());
  EXPECT_FALSE(s.update());

  { ASSERT_EQ(0, symlink(TEST_SRCDIR "/arome_vc_20150126_06.nc", TEST_BUILDDIR "/test_arome_reftimes/arome_vc_20150126_06.nc"));
    const Source::ReftimeUpdate u = s.update();
    EXPECT_EQ(0, u.gone.size());
    EXPECT_EQ(0, u.changed.size());
    ASSERT_EQ(1, u.appeared.size());
    EXPECT_TRUE(u.appeared.count(Time(S1970, 1422252000)));
  }

  { ASSERT_EQ(0, symlink(TEST_SRCDIR "/arome_vc_20150125_12.nc", TEST_BUILDDIR "/test_arome_reftimes/arome_vc_20150125_12.nc"));
    ASSERT_EQ(0, symlink(TEST_SRCDIR "/arome_vc_20150125_18.nc", TEST_BUILDDIR "/test_arome_reftimes/arome_vc_20150125_18.nc"));
    ASSERT_EQ(0, symlink(TEST_SRCDIR "/arome_vc_20150126_00.nc", TEST_BUILDDIR "/test_arome_reftimes/arome_vc_20150126_00.nc"));
    const Source::ReftimeUpdate u = s.update();
    EXPECT_EQ(0, u.gone.size());
    EXPECT_EQ(0, u.changed.size());
    ASSERT_EQ(3, u.appeared.size());
    EXPECT_TRUE(u.appeared.count(Time(S1970, 1422187200)));
    EXPECT_TRUE(u.appeared.count(Time(S1970, 1422208800)));
    EXPECT_TRUE(u.appeared.count(Time(S1970, 1422230400)));
  }

  { unlink(TEST_BUILDDIR "/test_arome_reftimes/arome_vc_20150125_12.nc");
    const Source::ReftimeUpdate u = s.update();
    ASSERT_EQ(1, u.gone.size());
    EXPECT_TRUE(u.gone.count(Time(S1970, 1422187200)));
    EXPECT_EQ(0, u.changed.size());
    EXPECT_EQ(0, u.appeared.size());
  }
}

// add test for 1-d value
// add test for dynamic cuts
