/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2019 met.no

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

#include "diana_config.h"

#include "diSat.h"

#include "diSatPlotCommand.h"

#include <puTools/miStringFunctions.h>

#include <sstream>

#define MILOGGER_CATEGORY "diana.Sat"
#include <miLogger/miLogging.h>

using namespace::miutil;

//static values that should be set from SatManager
namespace {
const float defaultCut = -1;
const int defaultAlphacut = 0;
const int defaultAlpha = 255;
const int defaultTimediff = 60;
const bool defaultClasstable = false;
} // namespace

Sat::Sat()
    : approved(false)
    , autoFile(true)
    , cut(defaultCut)
    , alphacut(defaultAlphacut)
    , alpha(defaultAlpha)
    , maxDiff(defaultTimediff)
    , classtable(defaultClasstable)
    , palette(false)
    , mosaic(false)
    , commonColourStretch(false)
    , image_rgba_(0)
    , channelschanged(true)
    , rgboperchanged(true)
    , alphaoperchanged(true)
{
  METLIBS_LOG_SCOPE();
  for (int i = 0; i < maxch; i++)
    rawimage[i] = 0;
  for (int i = 0; i < 3; i++)
    origimage[i] = 0;
}

Sat::Sat(SatPlotCommand_cp cmd)
    : Sat()
{
  METLIBS_LOG_SCOPE(LOGVAL(cmd) << LOGVAL(cut));

  image_name = cmd->image_name;
  subtype_name = cmd->subtype_name;
  plotChannels = cmd->plotChannels;
  filename = cmd->filename;
  if (!filename.empty())
    autoFile = false;
  mosaic = cmd->mosaic;
  cut = cmd->cut;
  maxDiff = cmd->timediff;
  alphacut = int(cmd->alphacut * 255);
  alpha = (int)(cmd->alpha * 255);
  classtable = cmd->classtable;
  hideColour = cmd->coloursToHideInLegend;

  METLIBS_LOG_DEBUG(LOGVAL(cut) << LOGVAL(alphacut) << LOGVAL(alpha) << LOGVAL(maxDiff) << LOGVAL(classtable));
}

Sat::~Sat()
{
  cleanup();
}

/* * PURPOSE:   calculate and print the temperature or the albedo of
 *            the pixel pointed at
 */
void Sat::values(int x, int y, std::vector<SatValues>& satval) const
{
  if (x>=0 && x<area.nx && y>=0 && y<area.ny && approved) { // inside image/legal image
    int index = area.nx*(area.ny-y-1) + x;

    //return value from  all channels

    std::map<int, table_cal>::const_iterator p = calibrationTable.begin();
    std::map<int, table_cal>::const_iterator q = calibrationTable.end();
    for (; p!=q && rawimage[p->first]!=NULL; p++) {
      SatValues sv;
      sv.value = -999.99;
      int pvalue;
      // For satellite pictures, get the original value
      if(hdf5type!=0) {
        if(origimage[p->first][index] <= -32000.0)
          continue;
        pvalue = (int)origimage[p->first][index];
      }
      else {
        pvalue = rawimage[p->first][index];
      }
      //return if colour is hidden
      const auto ithc = hideColour.find(pvalue);
      if (ithc != hideColour.end() && ithc->second == 0) {
        return;
      }
      if (pvalue != 0) {
        const int nv = p->second.val.size();
        if (nv > 0) {
          if (nv > pvalue) {
            if (palette) {
              sv.text = p->second.val[pvalue];
            } else {
              sv.value = atof(p->second.val[pvalue].c_str());
              if (miutil::contains(p->second.channel, "TEMP"))
                sv.value -= 273.0;//use degrees celsius instead of Kelvin
            }
          }
        } else {
          sv.value = p->second.a * pvalue + p->second.b;
        }
        sv.channel=p->second.channel;
        satval.push_back(sv);
      }
    }
  }
}

void Sat::setCalibration()
{
  //decode calibration strings from TIFF file

  METLIBS_LOG_SCOPE(LOGVAL(satellite_name));

  cal_channels.clear();
  calibrationTable.clear();

  //palette tiff, but no palette info
  if (palette && !paletteInfo.clname.size() )
    return;

  std::string start = satellite_name + " " + subtype_name + "|";

  METLIBS_LOG_DEBUG(LOGVAL(palette));

  if (palette) {
    std::string name = start + paletteInfo.name;
    table_cal ct;
    ct.channel = name;
    ct.val.push_back(" ");
    ct.val.insert(ct.val.end(), paletteInfo.clname.begin(),
        paletteInfo.clname.end());
    calibrationTable[0]=ct;
    cal_channels.push_back(name);
    return;
  }

  METLIBS_LOG_DEBUG(LOGVAL(cal_table.size()));

  //Table
  if (cal_table.size()>0) {
    for (unsigned int i=0; i<cal_table.size(); i++) {
      table_cal ct;
      std::vector<std::string> token = miutil::split(cal_table[i], ",");
      if (token.size()!= 5)
        continue;
      int m = vch.size();
      int j=0;
      while (j<m && token[0]!=vch[j])
        j++;
      if (j==m)
        continue;
      std::string name = start + token[1]+"("+token[0]+")";
      cal_channels.push_back(name);
      miutil::remove(token[4], '[');
      miutil::remove(token[4], ']');
      ct.channel = name;
      ct.val =miutil::split(token[4], " ");
      if (ct.val.size()!=256)
        continue;
      calibrationTable[j]=ct;
    }
    return;
  }

  METLIBS_LOG_DEBUG(LOGVAL(!cal_vis.empty()));

  //Visual
  bool vis=false;
  float AVis = 0.0, BVis = 0.0;
  if (not cal_vis.empty()) {
    std::vector<std::string> cal = miutil::split(cal_vis, "+");
    if (cal.size()==2) {
      std::string str = cal[0].substr(cal[0].find_first_of("(")+1);
      BVis = atof(str.c_str());
      str = cal[1].substr(cal[1].find_first_of("(")+1);
      AVis = atof(str.c_str());
      vis=true;
    }
  }

  METLIBS_LOG_DEBUG(LOGVAL(!cal_ir.empty()));

  //Infrared
  bool ir = false;
  float AIr = 0.0, BIr = 0.0;
  if (not cal_ir.empty()) {
    std::vector<std::string> cal = miutil::split(cal_ir, "+");
    if (cal.size()==2) {
      std::string str = cal[0].substr(cal[0].find_first_of("(")+1);
      BIr = atof(str.c_str());
      str = cal[1].substr(cal[1].find_first_of("(")+1);
      AIr = atof(str.c_str());
      ir=true;
    }
  }

  if ((plotChannels == "IR_CAL" || plotChannels == "IR+V") && ir) {
    table_cal ct;
    ct.channel = start + plotChannels + ":";
    ct.a= AIr;
    ct.b= BIr;
    calibrationTable[0]=ct;
    cal_channels.push_back(ct.channel);

  } else if ((plotChannels == "VIS_RAW" || plotChannels == "IR+V") && vis) {
    table_cal ct;
    ct.channel = start + plotChannels + ":";
    ct.a= AVis;
    ct.b= BVis;
    if (plotChannels == "VIS_RAW")
      calibrationTable[0]=ct;
    else
      calibrationTable[1]=ct;
    cal_channels.push_back(ct.channel);
  }
  else if (plotChannels == "IR") {
      table_cal ct;
      ct.channel = start + "Infrared (" + plotChannels + "):";
      ct.a= AIr;
      ct.b= BIr -273.0;
      // Set in map 1, 0 is the image
      calibrationTable[1]=ct;
      cal_channels.push_back(ct.channel);

    }

    METLIBS_LOG_DEBUG(LOGVAL(vch.size()));

    // channel "3", "4" and "5" are infrared, the rest are visual
    int n = vch.size();
    for (int j = 0; j < n; j++) {

      METLIBS_LOG_DEBUG("vch[" << j << "]: " << vch[j]);

      /* hdf5type == 0 => radar or mitiff
       * hdf5type == 1 => NOAA (HDF5)
       * hdf5type == 2 => MSG (HDF5)
       * and channels 4 and 7-11 ar infrared
       */
      if (miutil::contains(vch[j], "-")) {
        ir = false;
        vis = false;
      }
      bool isIRchannel = (((hdf5type == 0) && (vch[j] == "3" || vch[j] == "4" || vch[j] == "5")) ||
                          (hdf5type == 2 && (miutil::contains(vch[j], "4") || miutil::contains(vch[j], "7") || miutil::contains(vch[j], "8") ||
                                             miutil::contains(vch[j], "9") || miutil::contains(vch[j], "10") || miutil::contains(vch[j], "11"))) ||
                          (hdf5type == 1 && (miutil::contains(vch[j], "4"))));
      table_cal ct;
      if (ir && isIRchannel) {
        ct.channel = start + "Infrared (" + vch[j] + "):";
        ct.a = AIr;
        ct.b = BIr - 273.0; // use degrees celsius instead of Kelvin

        METLIBS_LOG_DEBUG(LOGVAL(ct.a) << LOGVAL(ct.b));

        calibrationTable[j] = ct;
        cal_channels.push_back(ct.channel);
      } else if (vis) {
        ct.channel = start + "Visual (" + vch[j] + "):";
        ct.a = AVis;
        ct.b = BVis;

        METLIBS_LOG_DEBUG(LOGVAL(ct.a) << LOGVAL(ct.b));

        calibrationTable[j] = ct;
        cal_channels.push_back(ct.channel);
      }
  }
}

void Sat::setAnnotation()
{
  annotation = satellite_name;
  miutil::trim(annotation);
  if (mosaic)
    annotation += " MOSAIKK ";
  else
    annotation += " ";
  annotation += subtype_name;
  annotation += " ";
  annotation += plotChannels;
  annotation += " ";
  annotation += time.format("%D %H:%M", "", true);
  if (mosaic) {
    //add info about first/last mosaic-file
    std::string temp = " (";
    temp += firstMosaicFileTime.format("%H:%M", "", true);
    temp +=" - " + lastMosaicFileTime.format("%H:%M", "", true);
    temp+= ")";
    annotation+=temp;
  }
}

void Sat::setPlotName()
{
  plotname = satellite_name;
  miutil::trim(plotname);
  if (mosaic)
    plotname += " MOSAIKK ";
  else
    plotname += " ";
  plotname += subtype_name;
  plotname += " ";
  plotname += plotChannels;
  if (!autoFile)
    plotname += " "+time.isoTime();
}

void Sat::setArea()
{
  METLIBS_LOG_SCOPE(LOGVAL(Ax) << LOGVAL(Ay) << LOGVAL(Bx) << LOGVAL(By) << LOGVAL(proj_string));
  Projection p(proj_string);
  area.setP(p);
  float x0 = Bx, y0 = By;
  float x1 = x0 + area.nx * Ax, y1 = y0 + area.ny * Ay;
  if (Ay < 0) {
    std::swap(y0, y1);
    Ay = -Ay;
  }
  area.resolutionX = Ax;
  area.resolutionY = Ay;
  const Rectangle r(x0, y0, x1, y1);
  area.setR(r);
}

void Sat::cleanup()
{
  METLIBS_LOG_SCOPE(LOGVAL(area.nx) << LOGVAL(area.ny));

  area.nx = 0;
  area.ny = 0;

  delete[] image_rgba_;
  image_rgba_ = 0;

  for (int i=0; i<maxch; i++) {
    delete[] rawimage[i];
    rawimage[i]= 0;
  }

  for(int j=0; j<3; j++) {
    delete[] origimage[j];
    origimage[j] = 0;
  }

  annotation.erase();
  plotname.erase();
  approved= false;
}
