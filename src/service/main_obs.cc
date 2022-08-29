/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2022 met.no

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

#include "diCommandlineOptions.h"
#include "diObsReaderFactory.h"
#include "diObsReaderMutex.h"
#include "diObsService.h"
#include "diUtilities.h"

#include <puTools/miStringFunctions.h>

#include "diana_config.h"
#ifdef DIANA_GRPC_INCLUDES_IN_GRPCPP
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#else // !DIANA_GRPC_INCLUDES_IN_GRPCPP
#include <grpc++/security/server_credentials.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#endif // !DIANA_GRPC_INCLUDES_IN_GRPCPP

#include <yaml-cpp/yaml.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <regex>
#include <string>

#include "diana_obs_v0_svc.grpc.pb.h"

#define MILOGGER_CATEGORY "diana.main_obs"
#include <miLogger/miLogging.h>

namespace {

void replaceEnv(std::string& text)
{
  static const std::regex env_regex("\\$\\{([a-zA-Z0-9_]+)\\}");
  std::smatch env_match;
  while (std::regex_search(text, env_match, env_regex)) {
    const std::string env = env_match.str(1);
    const std::string val = miutil::from_c_str(getenv(env.c_str()));
    text.replace(env_match[0].first, env_match[0].second, val);
  }
}

class ObservationsServiceGRPC final : public diana_obs_v0::ObservationsService::Service
{
public:
  ObservationsServiceGRPC(ObservationsService_p obs)
      : obs_(obs)
  {
  }

  grpc::Status GetTimes(grpc::ServerContext* /*context*/, const diana_obs_v0::TimesRequest* request, diana_obs_v0::TimesResult* result) override
  {
    obs_->GetTimes(request, result);
    return grpc::Status::OK;
  }

  grpc::Status GetParameters(grpc::ServerContext* /*context*/, const diana_obs_v0::ParametersRequest* request, diana_obs_v0::ParametersResult* result) override
  {
    obs_->GetParameters(request, result);
    return grpc::Status::OK;
  }

  grpc::Status GetData(grpc::ServerContext* /*context*/, const ::diana_obs_v0::DataRequest* request, diana_obs_v0::DataResult* result) override
  {
    obs_->GetData(request, result);
    return grpc::Status::OK;
  }

private:
  ObservationsService_p obs_;
};

namespace po = miutil::program_options;
const po::option op_service_address = po::option("service-address", "gRPC service address for listening") //
                                          .set_default_value("localhost:50051");
const po::option op_obs_config = po::option("obs-config", "yaml configuration for obs reader") //
                                     .set_default_value("obsconfig.yml");

int RunServer(int argc, char** argv)
{
  setlocale(LC_NUMERIC, "C");
  setenv("LC_NUMERIC", "C", 1);

  po::option_set cmdline_options;
  // clang-format off
  cmdline_options
      << diutil::op_help
      << diutil::op_logger
      << op_service_address
      << op_obs_config;
  // clang-format on

  std::vector<std::string> positional;
  po::value_set vm;
  try {
    vm = po::parse_command_line(argc, argv, cmdline_options, positional);
  } catch (po::option_error& e) {
    std::cerr << "ERROR while parsing commandline options: " << e.what() << std::endl;
    return 1;
  }
  if (vm.is_set(diutil::op_help)) {
    diutil::printUsage(std::cout, cmdline_options, "This is a Diana gRPC observations service.");
    return 0;
  }

  milogger::LoggingConfig log4cpp(vm.value(diutil::op_logger));

  const std::string& yaml_url = vm.value(op_obs_config);

  std::string content;
  if (!diutil::getFromAny(yaml_url, content)) {
    METLIBS_LOG_ERROR("Could not fetch obs config yaml from '" << yaml_url << "'.");
    return 1;
  }
  const YAML::Node node = YAML::Load(content);

  ObservationsService_p os = std::make_shared<ObservationsService>();

  for (const auto& r : node["readers"]) {
    const std::string name = r["name"].as<std::string>();
    if (name.empty()) {
      METLIBS_LOG_ERROR("Empty reader name not permitted.");
      return 1;
    }
    const std::string type = r["type"].as<std::string>();
    ObsReader_p reader = makeObsReader(type);
    if (!reader) {
      METLIBS_LOG_ERROR("Could not create reader '" << name << "' of type '" << type << "'.");
      return 1;
    }

    for (const auto& s : r["options"]) {
      const std::string& k = s["name"].as<std::string>();
      std::string v = s["value"].as<std::string>();
      replaceEnv(v);
      if (!reader->configure(k, v)) {
        METLIBS_LOG_ERROR("Could not set option '" << k << "' to value '" << v << "' for reader '" << name << ".");
        return 1;
      }
    }
    os->add(name, std::make_shared<ObsReaderMutex>(reader));
  }

  ObservationsServiceGRPC osg(os);

  const std::string service_address = vm.value(op_service_address);
  grpc::ServerBuilder builder;
  builder.SetDefaultCompressionLevel(GRPC_COMPRESS_LEVEL_HIGH);
  builder.SetMaxSendMessageSize(INT_MAX);
  builder.AddListeningPort(service_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&osg);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  if (server) {
    std::cout << "Server listening on '" << service_address << "'" << std::endl;
    server->Wait();
    return 0;
  } else {
    std::cerr << "No server created, exiting." << std::endl;
    return 1;
  }
}

} // namespace

int main(int argc, char** argv)
{
  METLIBS_LOG_SCOPE();
  return RunServer(argc, argv);
}
