/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018-2022 met.no

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

#include "diObsReaderFactory.h"

#include "diana_config.h"

#include "diObsReaderAscii.h"
#include "diObsReaderBufr.h"
#include "diObsReaderMetnoUrl.h"
#include "diObsReaderRoad.h"

#ifdef ENABLE_GRPC
#include "diObsReaderGRPC.h"
#endif

#include <puTools/miStringFunctions.h> // string split

#include <map>

#define MILOGGER_CATEGORY "diana.ObsReaderFactory"
#include <miLogger/miLogging.h>

namespace {

#if 0
typedef std::map<std::string, ObsManager::ProdInfo> string_ProdInfo_m;
string_ProdInfo_m defProd;
void initProductDefaults()
{
  if (!defProd.empty())
    return;

  // defaults for all observation types
  std::string parameter;
  defProd["synop"].obsformat = ObsManager::ofmt_synop;
  defProd["synop"].timeRangeMin = -30;
  defProd["synop"].timeRangeMax = 30;
  defProd["synop"].synoptic = true;
  parameter = "Wind,TTT,TdTdTd,PPPP,ppp,a,h,VV,N,RRR,ww,W1,W2,Nh,Cl,Cm,Ch,vs,ds,TwTwTw"
              ",PwaHwa,dw1dw1,Pw1Hw1,TxTn,sss,911ff,s,fxfx,Id,Name,St.no(3),St.no(5)"
              ",Pos,dd,ff,T_red,Date,Time,Height,Zone,RRR_1,RRR_6,RRR_12,RRR_24,quality";
  defProd["synop"].parameter = miutil::split(parameter, ",");

  defProd["aireps"].obsformat = ObsManager::ofmt_aireps;
  defProd["aireps"].timeRangeMin = -30;
  defProd["aireps"].timeRangeMax = 30;
  defProd["aireps"].synoptic = false;
  parameter = "Pos,dd,ff,Wind,TTT,TdTdTd,PPPP,Id,Date,Time,HHH";
  defProd["aireps"].parameter = miutil::split(parameter, ",");

  defProd["satob"].obsformat = ObsManager::ofmt_satob;
  defProd["satob"].timeRangeMin = -180;
  defProd["satob"].timeRangeMax = 180;
  defProd["satob"].synoptic = false;
  parameter = "Pos,dd,ff,Wind,Id,Date,Time";
  defProd["satob"].parameter = miutil::split(parameter, ",");

  defProd["dribu"].obsformat = ObsManager::ofmt_dribu;
  defProd["dribu"].timeRangeMin = -90;
  defProd["dribu"].timeRangeMax = 90;
  defProd["dribu"].synoptic = false;
  parameter = "Pos,dd,ff,Wind,TTT,TdTdTd,PPPP,ppp,a,TwTwTw,Id,Date,Time";
  defProd["dribu"].parameter = miutil::split(parameter, ",");

  defProd["temp"].obsformat = ObsManager::ofmt_temp;
  defProd["temp"].timeRangeMin = -30;
  defProd["temp"].timeRangeMax = 30;
  defProd["temp"].synoptic = false;
  parameter = "Pos,dd,ff,Wind,TTT,TdTdTd,PPPP,Id,Date,Time,HHH,QI,QI_NM,QI_RFF";
  defProd["temp"].parameter = miutil::split(parameter, ",");

  defProd["ocea"].obsformat = ObsManager::ofmt_ocea;
  defProd["ocea"].timeRangeMin = -180;
  defProd["ocea"].timeRangeMax = 180;
  defProd["ocea"].synoptic = true;

  defProd["tide"].obsformat = ObsManager::ofmt_tide;
  defProd["tide"].timeRangeMin = -180;
  defProd["tide"].timeRangeMax = 180;
  defProd["tide"].synoptic = true;

  defProd["pilot"].obsformat = ObsManager::ofmt_pilot;
  defProd["pilot"].timeRangeMin = -30;
  defProd["pilot"].timeRangeMax = 30;
  defProd["pilot"].synoptic = false;
  parameter = "Pos,dd,ff,Wind,TTT,TdTdTd,PPPP,Id,Date,Time,HHH";
  defProd["pilot"].parameter = miutil::split(parameter, ",");

  defProd["metar"].obsformat = ObsManager::ofmt_metar;
  defProd["metar"].timeRangeMin = -15;
  defProd["metar"].timeRangeMax = 15;
  defProd["metar"].synoptic = true;

  parameter = "Pos,dd,ff,Wind,dndx,fmfm,TTT,TdTdTd,ww,REww,VVVV/Dv,VxVxVxVx/Dvx,Clouds,PHPHPHPH,Id,Date,Time";
  defProd["metar"].parameter = miutil::split(parameter, ",");
  defProd["ascii"].obsformat = ObsManager::ofmt_ascii;
  defProd["ascii"].timeRangeMin = -180;
  defProd["ascii"].timeRangeMax = 180;
  defProd["ascii"].synoptic = false;

  defProd["url"].obsformat = ObsManager::ofmt_url;

#ifdef ROADOBS
  defProd["roadobs"].obsformat= ObsManager::ofmt_roadobs;
  defProd["roadobs"].timeRangeMin=-180;
  defProd["roadobs"].timeRangeMax= 180;
  defProd["roadobs"].synoptic= true;
#endif
}
#endif

} // namespace

ObsReader_p makeObsReader(const std::string& key)
{
  METLIBS_LOG_SCOPE(LOGVAL(key));
  if (key == "bufr" || key == "archive_bufr")
    return std::make_shared<ObsReaderBufr>();
  if (key == "file" || key == "archivefile" || key == "ascii" || key == "archive_ascii")
    return std::make_shared<ObsReaderAscii>();
  if (key == "grpc_obs") {
#ifdef ENABLE_GRPC
    return std::make_shared<ObsReaderGRPC>();
#else
    METLIBS_LOG_WARN("grpc_obs reader disabled");
    return ObsReader_p();
#endif
  }
  if (key == "url")
    return std::make_shared<ObsReaderMetnoUrl>();
  if (key == "roadobs" || key == "archive_roadobs")
    return std::make_shared<ObsReaderRoad>();
  METLIBS_LOG_WARN("unknown reader type '" << key << "'");
  return ObsReader_p();
}
