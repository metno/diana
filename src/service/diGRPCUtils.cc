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

#include "diGRPCUtils.h"

#include <chrono>
#include <climits>

#include "diana_config.h"
#ifdef DIANA_GRPC_INCLUDES_IN_GRPCPP
#include <grpcpp/create_channel.h>
#else // !DIANA_GRPC_INCLUDES_IN_GRPCPP
#include <grpc++/create_channel.h>
#endif // !DIANA_GRPC_INCLUDES_IN_GRPCPP

namespace diutil {
namespace grpc {

void set_timeout(::grpc::ClientContext& context, int milliseconds)
{
  std::chrono::system_clock::time_point deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(milliseconds);
  context.set_deadline(deadline);
}

void set_timeout(::grpc::ClientContext& context)
{
  set_timeout(context, 2000);
}

std::shared_ptr<::grpc::Channel> make_channel(const std::string& addr)
{
  // https://github.com/tensorflow/serving/issues/1382#issuecomment-730428996
  auto cargs = ::grpc::ChannelArguments();
  const int max_size = INT_MAX;
  cargs.SetMaxReceiveMessageSize(max_size);
  cargs.SetMaxSendMessageSize(max_size);
  return ::grpc::CreateCustomChannel(addr, ::grpc::InsecureChannelCredentials(), cargs);
}

} // namespace grpc
} // namespace diutil
