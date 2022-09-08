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

#include "diObsReaderGRPC.h"

#include "diObsDataVector.h"
#include "service/diGRPCUtils.h"
#include "service/diObsGRPCServiceUtils.h"
#include "service/diObsGRPCUtils.h"
#include "util/debug_timer.h"
#include "util/time_util.h"

#define MILOGGER_CATEGORY "diana.ObsReaderGRPC"
#include <miLogger/miLogging.h>

namespace {
ObsDialogInfo::ParType toParType(const diana_obs_v0::Par::Type& t)
{
  switch (t) {
  // clang-format off
  case diana_obs_v0::Par::pt_std:  return ObsDialogInfo::pt_std;  break;
  case diana_obs_v0::Par::pt_knot: return ObsDialogInfo::pt_knot; break;
  case diana_obs_v0::Par::pt_temp: return ObsDialogInfo::pt_temp; break;
  case diana_obs_v0::Par::pt_rrr:  return ObsDialogInfo::pt_rrr;  break;
  // clang-format on
  default:
    throw std::runtime_error("unexpected parameter type");
  }
}
} // namespace

// ------------------------------------------------------------------------

ObsReaderGRPC::ObsReaderGRPC()
    : updated_times_(0)
    , updated_parameters_(0)
{
}

ObsReaderGRPC::~ObsReaderGRPC() {}

bool ObsReaderGRPC::configure(const std::string& key, const std::string& value)
{
  METLIBS_LOG_SCOPE(LOGVAL(key) << LOGVAL(value));
  if (key == "grpc_obs") {
    if (client_) {
      METLIBS_LOG_ERROR("client already configured");
      return false;
    }
    client_.reset(new diutil::grpc::obs::ObsServiceGRPCClient(value));
    METLIBS_LOG_DEBUG("grpc_obs client configured");
  } else if (key == "name") {
    name_ = value;
  } else {
    return ObsReader::configure(key, value);
  }
  return true;
}

bool ObsReaderGRPC::checkForUpdates(bool /*useArchive*/)
{
  return true;
}

std::set<miutil::miTime> ObsReaderGRPC::getTimes(bool useArchive, bool update)
{
  METLIBS_LOG_SCOPE();
  if (update || updated_times_ == 0)
    updateTimes(useArchive);
  return cached_times_;
}

void ObsReaderGRPC::updateTimes(bool useArchive)
{
  METLIBS_LOG_SCOPE();
  if (!client_)
    return;

  diana_obs_v0::TimesRequest req;
  req.set_provider(name_);
  req.set_ifupdatedsince(updated_times_);
  req.set_usearchive(useArchive);
  diana_obs_v0::TimesResult res;

  grpc::ClientContext context;
  diutil::grpc::set_timeout(context);
  const auto gstatus = client_->stub->GetTimes(&context, req, &res);
  if (!gstatus.ok()) {
    METLIBS_LOG_ERROR("gRPC GetTimes request unsuccessful: " << gstatus.error_message());
    return;
  }

  if (res.status().code() == diana_obs_v0::Status::Code::Status_Code_ERROR) {
    METLIBS_LOG_ERROR("gRPC GetTimes request failed: " << res.status().what());
    return;
  }

  updated_times_ = res.lastupdate();

  const bool status_unchanged = (res.status().code() == diana_obs_v0::Status::Code::Status_Code_UNCHANGED);
  METLIBS_LOG_DEBUG(LOGVAL(res.timespans_size()) << LOGVAL(updated_times_) << LOGVAL(res.lastupdate()) << LOGVAL(status_unchanged));
  if (status_unchanged)
    return;

  cached_times_ = diutil::grpc::obs::timesFromTimeSpans(res);
}

std::vector<ObsDialogInfo::Par> ObsReaderGRPC::getParameters()
{
  METLIBS_LOG_SCOPE();
  updateParameters();
  return cached_parameters_;
}

void ObsReaderGRPC::updateParameters()
{
  METLIBS_LOG_SCOPE();
  if (!client_)
    return;

  diana_obs_v0::ParametersRequest req;
  req.set_provider(name_);
  req.set_ifupdatedsince(updated_parameters_);
  diana_obs_v0::ParametersResult res;

  grpc::ClientContext context;
  diutil::grpc::set_timeout(context);
  const auto gstatus = client_->stub->GetParameters(&context, req, &res);
  if (!gstatus.ok()) {
    METLIBS_LOG_ERROR("gRPC GetParameters request unsuccessful: " << gstatus.error_message());
    return;
  }

  if (res.status().code() == diana_obs_v0::Status::Code::Status_Code_ERROR) {
    METLIBS_LOG_ERROR("gRPC GetParameters request failed: " << res.status().what());
    return;
  }

  updated_parameters_ = res.lastupdate();

  const bool status_unchanged = (res.status().code() == diana_obs_v0::Status::Code::Status_Code_UNCHANGED);
  METLIBS_LOG_DEBUG(LOGVAL(res.parameters_size()) << LOGVAL(updated_parameters_) << LOGVAL(res.lastupdate()) << LOGVAL(status_unchanged));
  if (status_unchanged)
    return;

  cached_parameters_.clear();

  cached_parameters_.reserve(res.parameters_size());
  for (int i = 0; i < res.parameters_size(); ++i) {
    const auto& rp = res.parameters(i);
    cached_parameters_.push_back(
        ObsDialogInfo::Par(rp.name(), toParType(rp.type()), rp.symbol(), rp.precision(), rp.description(), rp.criteria_min(), rp.criteria_max()));
  }
}

void ObsReaderGRPC::getData(ObsDataRequest_cp request, ObsDataResult_p result)
{
  METLIBS_LOG_TIME();
  if (!client_)
    return;

  diana_obs_v0::DataRequest req;
  req.set_provider(name_);
  diutil::grpc::obs::copyFromMiTime(*req.mutable_obstime(), request->obstime);
  req.set_timediff(60 * request->timeDiff); // convert from minutes to seconds
  req.set_level(request->level);
  req.set_usearchive(request->useArchive);
  diana_obs_v0::DataResult res;

  grpc::ClientContext context;
  diutil::grpc::set_timeout(context);
  grpc::Status gstatus;
  {
    METLIBS_LOG_TIME("reading obs data from grpc");
    gstatus = client_->stub->GetData(&context, req, &res);
  }
  if (!gstatus.ok()) {
    METLIBS_LOG_ERROR("gRPC GetData rerequest unsuccessful: " << gstatus.error_message());
    result->setComplete(false);
    return;
  }

  if (res.status().code() == diana_obs_v0::Status::ERROR) {
    METLIBS_LOG_ERROR("gRPC GetData request failed: " << res.status().what());
    result->setComplete(false);
    return;
  }

  METLIBS_LOG_DEBUG(LOGVAL(res.data_size()));

  auto obsdata = std::make_shared<ObsDataVector>();
  obsdata->reserve(res.data_size());

  std::map<size_t, size_t> data_keys;
  for (int ki = 0; ki < res.data_keys_size(); ++ki) {
    data_keys[ki] = obsdata->add_key(res.data_keys(ki));
  }

  const miutil::miTime time = diutil::grpc::obs::toMiTime(res.time());
  result->setTime(time);

  for (int i = 0; i < res.data_size(); ++i) {
    const auto& rd = res.data(i);
    const size_t oi = obsdata->size();

    auto& odb = obsdata->basic(oi);
    const auto& rdb = rd.basic();
    odb.id = rdb.id();
    odb.obsTime = miutil::addSec(time, rdb.obs_time_delta().seconds());
    odb.xpos = rdb.xpos();
    odb.ypos = rdb.ypos();
    odb.dataType = res.data_type();

    if (rd.has_metar()) {
      auto& odm = obsdata->metar(oi);
      const auto& rdm = rd.metar();
      odm.ship_buoy = rdm.ship_buoy();
      odm.metarId = rdm.metarid();
      odm.CAVOK = rdm.cavok();
      odm.REww.reserve(rdm.reww_size());
      for (int j = 0; j < rdm.reww_size(); ++j)
        odm.REww.push_back(rdm.reww(j));
      odm.ww.reserve(rdm.ww_size());
      for (int j = 0; j < rdm.ww_size(); ++j)
        odm.ww.push_back(rdm.ww(j));
      odm.cloud.reserve(rdm.cloud_size());
      for (int j = 0; j < rdm.cloud_size(); ++j)
        odm.cloud.push_back(rdm.cloud(j));
    }

    for (int j = 0; j < rd.fdata_size(); ++j) {
      const auto& fd = rd.fdata(j);
      obsdata->put_float(oi, data_keys[fd.key_index()], fd.value());
    }
    for (int j = 0; j < rd.sdata_size(); ++j) {
      const auto& sd = rd.sdata(j);
      obsdata->put_string(oi, data_keys[sd.key_index()], sd.value());
    }
  }

  result->add(obsdata);
  result->setComplete(true);
}

PlotCommand_cpv ObsReaderGRPC::getExtraAnnotations()
{
  return PlotCommand_cpv();
}
