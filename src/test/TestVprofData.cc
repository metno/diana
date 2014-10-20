
#include <vcross_v2/VcrossEvaluate.h>
#include <puTools/mi_boost_compatibility.hh>

#include <gtest/gtest.h>

#include <fstream>
#include <string>

#define MILOGGER_CATEGORY "diField.test.VprofDataTest"
#include "miLogger/miLogging.h"

using namespace vcross;

//#define DEBUG_MESSAGES
#ifdef DEBUG_MESSAGES
#include <log4cpp/Category.hh>
#define configureLogging()                                              \
  milogger::LoggingConfig lc("kjlhlkjH");                               \
  log4cpp::Category::getRoot().setPriority(log4cpp::Priority::DEBUG)
#else
#define configureLogging() /* empty */
#endif // !DEBUG_MESSAGES

static const char AROME_FILE[] = "arome_vprof.nc";
static const int AROME_N_CS = 5;
static const int AROME_CS_LEN[AROME_N_CS] = { 1, 1, 1, 1, 1 };
static const int AROME_N_TIME = 2;
static const int AROME_N_Z = 65;

static const char modelName[] = "testmodel";

TEST(VprofDataTest, TestSetup)
{
  configureLogging();

  string_v sources;
  sources.push_back("m=" + std::string(modelName)
      + " f=" + std::string(TOP_SRCDIR) + "/test/" + std::string(AROME_FILE)
      + " t=netcdf");

  //parameters and computations should be defined in setup
  Setup_p setup = miutil::make_shared<vcross::Setup>();
  string_v fields;
  fields.push_back("air_temperature_celsius_ml");
  fields.push_back("dew_point_temperature_celsius_ml");
  fields.push_back("x_wind_ml");
  fields.push_back("y_wind_ml");
  fields.push_back("relative_humidity_ml");
  fields.push_back("upward_air_velocity_ml");
  string_v plots;
  string_v plot_defs;
  for ( size_t i = 0; i < fields.size(); ++i ) {
    plots.push_back(std::string("name=") + fields[i] + std::string(" plot=CONTOUR(") + fields[i] + std::string(")"));
    plot_defs.push_back("VCROSS " + std::string(" model=") + std::string(modelName) + std::string(" field=") + fields[i]);
  }

  string_v computations;
  computations.push_back("relative_humidity_ml=rh_from_tk_q(air_temperature_ml,specific_humidity_ml)");
  computations.push_back("tdk=tdk_from_tk_q(air_temperature_ml,specific_humidity_ml)");
  computations.push_back("air_temperature_celsius_ml=convert_unit(air_temperature_ml,celsius)");
  computations.push_back("dew_point_temperature_celsius_ml=convert_unit(tdk,celsius)");
  vc_configure(setup, sources, computations, plots);

  Collector_p collector = miutil::make_shared<Collector>(setup);
  ASSERT_TRUE(collector);
}
