/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017 met.no

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

#include "diObsReaderHqc.h"

ObsReaderHqc::ObsReaderHqc()
    : hqc_from(-2)
    , timeListChanged(true)
{
}

#if 0
namespace /* anonymous */{
std::vector<std::string> split_on_comma(const std::string& txt, const char* comma = ",")
{
  std::vector<std::string> s;
  boost::algorithm::split(s, txt, boost::algorithm::is_any_of(comma));
  return s;
}
} /* anonymous namespace */

bool ObsManager::initHqcdata(int from, const string& commondesc,
    const string& common, const string& desc,
    const std::vector<std::string>& data)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(commondesc) << LOGVAL(common) << LOGVAL(desc));

  //   if(desc == "remove" && from != hqc_from)
  //     return false;

  if (common == "remove") {
    hqcdata.clear();
    hqc_synop_parameter.clear();
    return true;
  }

  if (desc == "time") {
    int n = data.size();

    Prod["hqc_synop"].fileInfo.clear();
    Prod["hqc_list"].fileInfo.clear();

    for (int i = 0; i < n; i++) {
      METLIBS_LOG_DEBUG("time = " << data[i]);
      FileInfo finfo;
      finfo.time = miTime(data[i]);
      Prod["hqc_synop"].fileInfo.push_back(finfo);
      Prod["hqc_list"].fileInfo.push_back(finfo);
    }
    return true;
  }

  hqc_from = from;
  const std::vector<std::string> descstr = split_on_comma(commondesc),
      commonstr = split_on_comma(common);
  if (commonstr.size() != descstr.size()) {
    METLIBS_LOG_ERROR("different size of commondesc and common");
    return false;
  }

  for (size_t i = 0; i < descstr.size(); i++) {
    const std::string value = miutil::to_lower(descstr[i]);
    if (value == "time") {
      hqcTime = miTime(commonstr[i]);
    }
  }

  hqcdata.clear();
  hqc_synop_parameter = split_on_comma(desc);
  METLIBS_LOG_DEBUG(LOGVAL(desc) << LOGVAL(hqc_synop_parameter.size()));
  for (const std::string& d : data){
    const std::vector<std::string> tokens = split_on_comma(d);

    ObsData obsd;
    if (!changeHqcdata(obsd, hqc_synop_parameter, tokens))
      return false;

    hqcdata.push_back(obsd);
  }
  return true;
}

bool ObsManager::sendHqcdata(ObsPlot* oplot)
{
  METLIBS_LOG_SCOPE(LOGVAL(hqcTime));
  if (!oplot->timeOK(hqcTime))
    return false;

  oplot->replaceObsData(hqcdata);
  oplot->setSelectedStation(selectedStation);
  oplot->setHqcFlag(hqcFlag);
  oplot->changeParamColour(hqcFlag_old, false);
  oplot->changeParamColour(hqcFlag, true);
  if (!oplot->setData())
    return false;

  std::string anno = "Hqc " + hqcTime.format("%D %H%M", "", true);
  oplot->setObsAnnotation(anno);
  oplot->setPlotName(anno);
  return true;
}

bool ObsManager::updateHqcdata(const string& commondesc, const string& common,
    const string& desc, const vector<string>& data)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(commondesc) << LOGVAL(common) << LOGVAL(desc));

  const std::vector<std::string> descstr = split_on_comma(commondesc),
      commonstr = split_on_comma(common);
  if (commonstr.size() != descstr.size()) {
    METLIBS_LOG_ERROR("different size of commondesc and common");
    return false;
  }

  for (size_t i = 0; i < descstr.size(); i++) {
    const std::string value = boost::algorithm::to_lower_copy(descstr[i]);
    if (value == "time") {
      const miutil::miTime t(commonstr[i]);
      hqcTime = t;
      //  if( t != hqcTime ) return false; //time doesn't match
    }
  }

  const std::vector<std::string> param = split_on_comma(desc);
  if (param.size() < 2)
    return false;

  for (const std::string& d : data){
    METLIBS_LOG_DEBUG(LOGVAL(d));

    const std::vector<std::string> datastr = split_on_comma(d);

    // find station
    size_t i;
    for (i=0; i<hqcdata.size() && hqcdata[i].id != datastr[0]; ++i) {}
    if (i == hqcdata.size())
      continue; // station not found
    METLIBS_LOG_DEBUG(LOGVAL(i) << LOGVAL(datastr[0]));

    if (!changeHqcdata(hqcdata[i], param, datastr))
      return false;
  }
  return true;
}

bool ObsManager::changeHqcdata(ObsData& odata, const vector<string>& param,
    const vector<string>& data)
{
  METLIBS_LOG_SCOPE();
  if (param.size() != data.size()) {
    METLIBS_LOG_ERROR(
        "No. of parameters: "<<param.size()<<" != no. of data: " <<data.size());
    return false;
  }

  for (unsigned int i = 0; i < param.size(); i++) {
    const std::string& key = param[i];
    const std::string value = miutil::to_lower(data[i]);

    if (key == "id") {
      odata.id = data[i]; // no lower case!
    } else if (key == "lon") {
      odata.xpos = miutil::to_double(value);
    } else if (key == "lat") {
      odata.ypos = miutil::to_double(value);
    } else if (key == "auto") {
      if (value == "a")
        odata.fdata["ix"] = 4;
      else if (value == "n")
        odata.fdata["ix"] = -1;
    } else if (key == "St.type") {
      if (value != "none" && value != "")
        odata.dataType = value;
    } else {
      const std::vector<std::string> vstr = split_on_comma(data[i], ";");
      if (vstr.empty())
        continue;

      const float fvalue = miutil::to_double(vstr[0]);
      if (vstr.size() >= 2) {
        odata.flag[key] = vstr[1];
        if (vstr.size() == 3)
          odata.flagColour[key] = Colour(vstr[2]);
      }

      const char* simple_keys[] = { "h", "VV", "N", "dd", "ff", "TTT", "TdTdTd",
          "PPPP", "ppp", "RRR", "Rt", "ww", "Nh", "TwTwTw", "PwaPwa", "HwaHwa",
          "Pw1Pw1", "Hw1Hw1", "s", "fxfx", "TxTn", "sss" };
      if (std::find(simple_keys, boost::end(simple_keys), key) != boost::end(simple_keys))
      {
        odata.fdata[key] = fvalue;
        continue;
      }

      if (key == "a") {
        if (fvalue >= 0 && fvalue < 10)
          odata.fdata["a"] = fvalue;
        // FIXME else { do not keep old value }
      } else if (key == "W1" or key == "W2") {
        if (fvalue > 2 && fvalue < 10)
          odata.fdata[key] = fvalue;
        // FIXME else { do not keep old value }
      } else if (key == "Cl" or key == "Cm" or key == "Ch") {
        if (fvalue > 0 && fvalue < 10)
          odata.fdata[key] = fvalue;
        // FIXME else { do not keep old value }
      } else if (key == "vs") {
        if (fvalue >= 0 && fvalue < 10)
          odata.fdata[key] = fvalue;
        // FIXME else { do not keep old value }
      } else if (key == "ds") {
        if (fvalue > 0 && fvalue < 9)
          odata.fdata[key] = fvalue;
        // FIXME else { do not keep old value }
      } else if (key == "dw1dw1") {
        if (fvalue > 0 && fvalue < 37)
          odata.fdata[key] = fvalue;
        // FIXME else { do not keep old value }
      } else if (key == "TxTxTx" or key == "TnTnTn") {
        odata.fdata["TxTn"] = fvalue;
      } else if (key == "911ff" or key == "ff_911") {
        odata.fdata["ff_911"] = fvalue;
      } else {
        METLIBS_LOG_INFO("unknown key '" << key << '\'');
      }
    }
  }
  return true;
}

void ObsManager::processHqcCommand(const std::string& command,
    const std::string& str)
{
  if (command == "remove") {
    hqcdata.clear();
    hqc_synop_parameter.clear();
  } else if (command == "flag") {
    hqcFlag_old = hqcFlag;
    hqcFlag = str;
    //    hqcFlag = miutil::to_lower(str);
  } else if (command == "station") {
    const std::vector<std::string> vstr = split_on_comma(str);
    if (not vstr.empty())
      selectedStation = vstr[0];
  }
}
#endif

std::vector<ObsDialogInfo::Par> ObsReaderHqc::getParameters()
{
#if 0
  // called each time initHqcData() is called
  //  METLIBS_LOG_DEBUG("updateHqcDialog");

  const std::string plotType;

  int nd = dialog.plottype.size();
  int id = 0;
  while (id < nd && dialog.plottype[id].name != plotType) {
    id++;
  }
  if (id == nd)
    return dialog;

  ObsDialogInfo::PlotType& pt = dialog.plottype[id];

  pt.button.clear();
  pt.datatype[0].active.clear();
  if (plotType == "Hqc_synop") {
    int wind = 0;
    for (const std::string& p : hqc_synop_parameter){
      if (p=="dd" || p=="ff") {
        wind++;
        continue;
      }
      if (p == "lon" or p == "lat" or p == "auto")
        continue;
      pt.button.push_back(addButton(p," ",0,0,true));
      pt.datatype[0].active.push_back(true);
    }
    if (wind == 2) {
      pt.button.push_back(addButton("Wind", " ", 0, 0, true));
      pt.datatype[0].active.push_back(true);
    }
    if (pt.button.size()) {
      pt.button.push_back(addButton("Flag", " ", 0, 0, true));
      pt.datatype[0].active.push_back(true);
    }
  } else if (plotType == "Hqc_list") {
    int wind = 0;
    for (const std::string& p : hqc_synop_parameter){
      if (p == "auto")
        continue;
      if (p == "DD" or p == "FF") {
        wind++;
      } else {
        pt.button.push_back(addButton(p," ",0,0,true));
        pt.datatype[0].active.push_back(true);
      }
      if (wind == 2) {
        wind = 0;
        pt.button.push_back(addButton("Wind"," ",0,0,true));
        pt.datatype[0].active.push_back(true);
      }
    }
  }
  return dialog;
#endif
}
