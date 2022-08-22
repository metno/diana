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

#ifndef DIANA_ObsGRPCServiceUtils_h
#define DIANA_ObsGRPCServiceUtils_h

#include "diObsGRPCServiceDecls.h"

#include <memory>

#include "diana_obs_v0_svc.grpc.pb.h"

namespace diutil {
namespace grpc {
namespace obs {

struct ObsServiceGRPCClient
{
  ObsServiceGRPCClient(const std::string& addr);

  std::unique_ptr<diana_obs_v0::ObservationsService::Stub> stub;
};

} // namespace obs
} // namespace grpc
} // namespace diutil

#endif // !DIANA_ObsGRPCServiceUtils_h
