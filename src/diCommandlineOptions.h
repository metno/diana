/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2021 met.no

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

#ifndef DIANA_COMMANDLINEOPTIONS_H
#define DIANA_COMMANDLINEOPTIONS_H 1

#include <mi_programoptions.h>

#include <map>
#include <string>
#include <vector>

class QString;

namespace diutil {
namespace po = miutil::program_options;

extern const po::option op_help;
extern const po::option op_setup;
extern const po::option op_logger;

std::map<std::string, std::string> parse_user_variables(const std::vector<std::string>& positional);

bool value_if_set(const po::value_set& vm, const po::option& op, std::string& value);
bool value_if_set(const po::value_set& vm, const po::option& op, QString& value);

void printVersion(std::ostream& out, const std::string& argv0);
void printUsage(std::ostream& out, const po::option_set& config, const std::string& details = std::string());

extern const QString titleDianaVersion;

} // namespace diutil

#endif // DIANA_COMMANDLINEOPTIONS_H
