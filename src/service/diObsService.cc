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

#include "diObsService.h"

#include <chrono>
#include <fstream>
#include <memory>
#include <puTools/miTime.h>
#include <string>

#define MILOGGER_CATEGORY "diana.ObsService"
#include <miLogger/miLogging.h>

namespace {

typedef std::unique_lock<std::mutex> scoped_lock;

long periodUpdateTimes = 60;       // seconds between time updates
long periodUpdateParameters = 120; // seconds between parameter updates

long timestamp()
{
  using namespace std::chrono;
  return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

diana_obs_v0::Par::Type fromParType(const ObsDialogInfo::ParType& t)
{
  switch (t) {
  // clang-format off
  case ObsDialogInfo::pt_std:  return diana_obs_v0::Par::pt_std;
  case ObsDialogInfo::pt_knot: return diana_obs_v0::Par::pt_knot;
  case ObsDialogInfo::pt_temp: return diana_obs_v0::Par::pt_temp;
  case ObsDialogInfo::pt_rrr:  return diana_obs_v0::Par::pt_rrr;
  // clang-format on
  }
}

void copyFromMiTime(diana_obs_v0::Time& ot, const miutil::miTime& mt)
{
  ot.set_year(mt.year());
  ot.set_month(mt.month());
  ot.set_day(mt.day());
  ot.set_hour(mt.hour());
  ot.set_minute(mt.min());
  ot.set_second(mt.sec());
}

miutil::miTime toMiTime(const diana_obs_v0::Time& ot)
{
  return miutil::miTime(ot.year(), ot.month(), ot.day(), ot.hour(), ot.minute(), ot.second());
}

void setStatusFromException(diana_obs_v0::Status& status, std::exception& ex)
{
  status.set_code(diana_obs_v0::Status::ERROR);
  status.set_what(ex.what());
}

} // namespace

ObservationsService::ObservationsService()
    : cvt(diutil::findConverter(diutil::ISO_8859_1, diutil::UTF_8))
{
  METLIBS_LOG_SCOPE();
}

void ObservationsService::add(const std::string& name, ObsReader_p reader)
{
  if (name.empty())
    throw std::runtime_error("empty reader name not permitted");
  if (!readers_.insert(std::make_pair(name, ReaderUpdate{reader})).second)
    throw std::runtime_error("reader with name '" + name + "' already configured");
}

ObservationsService::ReaderUpdate& ObservationsService::find(const std::string& name)
{
  const auto it = readers_.find(name);
  if (it == readers_.end())
    throw std::runtime_error("reader with name '" + name + "' not known");
  return it->second;
}

void ObservationsService::GetTimes(const diana_obs_v0::TimesRequest* request, diana_obs_v0::TimesResult* result)
{
  METLIBS_LOG_SCOPE();
  try {
    ObsReader_p reader;
    bool update = false;
    {
      scoped_lock lock(mutex_);
      ReaderUpdate& ru = find(request->provider());
      const long now = timestamp();
      const bool outdatedHere = (ru.updatedTimes == 0 || ru.updatedTimes + periodUpdateTimes < now);
      const bool changedRequest = (request->usearchive() != ru.tUseArchive);
      const bool outdatedRequest = (request->ifupdatedsince() < ru.updatedTimes);
      METLIBS_LOG_DEBUG(LOGVAL(outdatedHere) << LOGVAL(outdatedRequest) << LOGVAL(changedRequest));
      if (outdatedHere || changedRequest) {
        update = true;
        ru.updatedTimes = now;
        ru.tUseArchive = request->usearchive();
      }
      result->set_lastupdate(ru.updatedTimes);
      if (!(outdatedHere || changedRequest || outdatedRequest)) {
        result->mutable_status()->set_code(::diana_obs_v0::Status_Code::Status_Code_UNCHANGED);
        return;
      }

      reader = ru.reader;
    }

    const std::set<miutil::miTime> times = reader->getTimes(request->usearchive(), update);
    miutil::miTime spanBegin, spanEnd;
    float spanStep = -1;
    for (const auto& t : times) {
      if (spanBegin.undef()) {
        spanBegin = t;
        spanStep = -1;
      } else if (spanStep < 0) {
        spanEnd = t;
        spanStep = miutil::miTime::secDiff(spanEnd, spanBegin);
      } else {
        const float step = miutil::miTime::secDiff(t, spanEnd);
        if (step == spanStep) {
          spanEnd = t;
        } else {
          diana_obs_v0::TimeSpan& ts = *result->add_timespans();
          copyFromMiTime(*ts.mutable_begin(), spanBegin);
          ts.set_step(spanStep);
          copyFromMiTime(*ts.mutable_end(), spanEnd);

          spanBegin = t;
          spanStep = -1;
        }
      }
    }
    if (!spanBegin.undef()) {
      diana_obs_v0::TimeSpan& ts = *result->add_timespans();
      copyFromMiTime(*ts.mutable_begin(), spanBegin);
      if (spanStep > 0) {
        ts.set_step(spanStep);
        copyFromMiTime(*ts.mutable_end(), spanEnd);
      }
    }
  } catch (std::exception& ex) {
    setStatusFromException(*result->mutable_status(), ex);
  }
}

void ObservationsService::GetParameters(const diana_obs_v0::ParametersRequest* request, diana_obs_v0::ParametersResult* result)
{
  METLIBS_LOG_SCOPE();
  try {
    ObsReader_p reader;
    {
      scoped_lock lock(mutex_);
      ReaderUpdate& ru = find(request->provider());
      const long now = timestamp();
      const bool outdatedHere = (ru.updatedParameters == 0 || ru.updatedParameters + periodUpdateParameters < now);
      const bool changedRequest = false;
      const bool outdatedRequest = (request->ifupdatedsince() < ru.updatedParameters);
      METLIBS_LOG_DEBUG(LOGVAL(outdatedHere) << LOGVAL(outdatedRequest) << LOGVAL(changedRequest));
      if (outdatedHere || changedRequest) {
        ru.updatedParameters = now;
      }
      result->set_lastupdate(ru.updatedParameters);
      if (!(outdatedHere || changedRequest || outdatedRequest)) {
        result->mutable_status()->set_code(::diana_obs_v0::Status_Code::Status_Code_UNCHANGED);
        return;
      }

      reader = ru.reader;
    }

    const std::vector<ObsDialogInfo::Par> parameters = reader->getParameters();
    for (const auto& p : parameters) {
      diana_obs_v0::Par* par = result->add_parameters();
      par->set_name(p.name);
      par->set_type(fromParType(p.type));
      par->set_symbol(p.symbol);
      par->set_precision(p.precision);
      par->set_description(p.button_tip);
      par->set_criteria_min(p.button_low);
      par->set_criteria_max(p.button_high);
    }
  } catch (std::exception& ex) {
    setStatusFromException(*result->mutable_status(), ex);
  }
}

static int key_index(diana_obs_v0::DataResult& dr, const std::string& key, std::map<std::string, int>& key_index_cache)
{
  const auto it = key_index_cache.find(key);
  if (it != key_index_cache.end())
    return it->second;

  const int idx = dr.data_keys_size();
  key_index_cache.insert(std::make_pair(key, idx));
  dr.add_data_keys(key);
  return idx;
}

void ObservationsService::GetData(const diana_obs_v0::DataRequest* request, diana_obs_v0::DataResult* result)
{
  METLIBS_LOG_TIME();
  try {
    ObsReader_p reader = find(request->provider()).reader;

    ObsDataRequest_p req = std::make_shared<ObsDataRequest>();
    req->obstime = toMiTime(request->obstime());
    req->timeDiff = request->timediff() / 60; // convert from seconds to minutes
    req->level = request->level();
    req->useArchive = request->usearchive();

    ObsDataResult_p res = std::make_shared<ObsDataResult>();
    reader->getData(req, res);
    if (!res->success())
      throw std::runtime_error("error reading obs data");

    copyFromMiTime(*result->mutable_time(), res->time());

    std::map<std::string, int> key_index_cache;

    ObsDataContainer_cp data = res->data();
    if (!data->empty())
      result->set_data_type(data->basic(0).dataType);

    const auto key_names = data->get_keys();

    for (size_t oi = 0; oi < data->size(); ++oi) {
      diana_obs_v0::ObsData& rd = *result->add_data();

      const auto& odb = data->basic(oi);
      auto rdb = rd.mutable_basic();
      rdb->set_id(cvt->convert(odb.id));
      rdb->set_xpos(odb.xpos);
      rdb->set_ypos(odb.ypos);

      auto otd = rdb->mutable_obs_time_delta();
      otd->set_seconds(miutil::miTime::secDiff(odb.obsTime, req->obstime));

      for (const auto& key : key_names) {
        if (const float* f = data->get_float(oi, key)) {
          diana_obs_v0::FloatKV& fv = *rd.add_fdata();
          fv.set_key_index(key_index(*result, cvt->convert(key), key_index_cache));
          fv.set_value(*f);
        }
        if (const std::string* s = data->get_string(oi, key)) {
          diana_obs_v0::StringKV& sv = *rd.add_sdata();
          sv.set_key_index(key_index(*result, cvt->convert(key), key_index_cache));
          sv.set_value(cvt->convert(*s));
        }
      }

      // BUFR only
      const auto& odm = data->metar(oi);
      /* always send, not clear how to detect absent metar */ {
        auto rdm = rd.mutable_metar();
        rdm->set_ship_buoy(odm.ship_buoy);
        rdm->set_metarid(odm.metarId);
        rdm->set_cavok(odm.CAVOK);
        for (const auto& reww : odm.REww)
          rdm->add_reww(reww);
        for (const auto& ww : odm.ww)
          rdm->add_ww(ww);
        for (const auto& cloud : odm.cloud)
          rdm->add_cloud(cloud);
      }
    }

#if 0
    std::ofstream pbf("dump.pbf");
    result->SerializeToOstream(&pbf);
#endif
  } catch (std::exception& ex) {
    setStatusFromException(*result->mutable_status(), ex);
  }
}
