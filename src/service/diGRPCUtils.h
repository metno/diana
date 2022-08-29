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

#ifndef DIANA_GRPCUtils_h
#define DIANA_GRPCUtils_h

#include <memory>

#include "diana_config.h"
#ifdef DIANA_GRPC_INCLUDES_IN_GRPCPP
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#else // !DIANA_GRPC_INCLUDES_IN_GRPCPP
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#endif // !DIANA_GRPC_INCLUDES_IN_GRPCPP

namespace diutil {
namespace grpc {

//! Set timeout for gRPC context.
void set_timeout(::grpc::ClientContext& context, int milliseconds);

//! Set default timeout for gRPC context (2s).
void set_timeout(::grpc::ClientContext& context);

//! Create a channel with default options (max message size, insecure).
std::shared_ptr<::grpc::Channel> make_channel(const std::string& addr);

} // namespace grpc
} // namespace diutil

#endif // !DIANA_GRPCUtils_h
