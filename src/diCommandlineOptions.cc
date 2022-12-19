
#include "diCommandlineOptions.h"

#include "diBuild.h"
#include "diana_config.h"

#include <puTools/miStringFunctions.h>

#include <QString>

#define MILOGGER_CATEGORY "diana.CommandlineOptions"
#include <miLogger/miLogging.h>

namespace diutil {

const po::option op_help = std::move(po::option("help", "show help message").set_shortkey("h").set_narg(0));
const po::option op_setup = std::move(po::option("setup", "name of setupfile (default diana.setup)")
                                          .set_shortkey("s") //
                                          .set_default_value("diana.setup")
                                          .set_overwriting());
const po::option op_logger = std::move(po::option("logger", "logging configuration file")
                                           .set_shortkey("L") //
                                           .set_default_value(SYSCONFDIR "/" PACKAGE_NAME "/" PVERSION "/log4cpp.properties")
                                           .set_overwriting());

std::map<std::string, std::string> parse_user_variables(const std::vector<std::string>& positional)
{
  std::map<std::string, std::string> user_variables{{"PVERSION", PVERSION}, {"SYSCONFDIR", SYSCONFDIR}};
  for (const std::string& sarg : positional) {
    const std::vector<std::string> ks = miutil::split(sarg, "=");
    if (ks.size() == 2) {
      user_variables[ks[0]] = ks[1]; // overwrite if given multiple times
    } else {
      METLIBS_LOG_WARN("ignoring unknown commandline argument '" << sarg << "'");
    }
  }
  return user_variables;
}

bool value_if_set(const po::value_set& vm, const po::option& op, std::string& value)
{
  const bool is_set = vm.is_set(op);
  if (is_set)
    value = vm.value(op);
  return is_set;
}

bool value_if_set(const po::value_set& vm, const po::option& op, QString& value)
{
  const bool is_set = vm.is_set(op);
  if (is_set)
    value = QString::fromStdString(vm.value(op));
  return is_set;
}

void printVersion(std::ostream& out, const std::string& argv0)
{
  out << argv0 << " : DIANA version: " << VERSION
      << "  build: " << diana_build_string
      << "  commit: " << diana_build_commit
      << std::endl;
}

void printUsage(std::ostream& out, const po::option_set& config, const std::string& details)
{
  const std::string line = "----------------------------------------------------------\n";
  out << line
      << "Diana - a 2D presentation system for meteorological data,\n"
      << "including fields, observations, satellite- and radarimages,\n"
      << "vertical profiles and cross sections. Diana has tools for\n"
      << "on-screen fieldediting and drawing of objects (fronts, areas,"
      << "symbols etc.\n"
      << "Copyright (C) 2006-2021 met.no\n"
      << line;

  if (!details.empty())
    out << details << line;

  config.help(out);
}

const QString titleDianaVersion = QString("Diana %1").arg(VERSION);

} // namespace diutil
