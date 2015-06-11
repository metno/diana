/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2015 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diSat.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.Sat"
#include <miLogger/miLogging.h>

using namespace::miutil;

//static values that should be set from SatManager
float Sat::defaultCut=-1;
int Sat::defaultAlphacut=0;
int Sat::defaultAlpha=255;
int Sat::defaultTimediff=60;
bool Sat::defaultClasstable=false;

Sat::Sat() :
  approved(false),  autoFile(true),
  cut(defaultCut), alphacut(defaultAlphacut), alpha(defaultAlpha),
  maxDiff(defaultTimediff), classtable(defaultClasstable),
  palette(false), mosaic(false),
  commonColourStretch(false), image(0), calibidx(-1),
  channelschanged(true), rgboperchanged(true),
  alphaoperchanged(true),mosaicchanged(true)
{
  METLIBS_LOG_SCOPE();
  for (int i=0; i<maxch; i++)
    rawimage[i] = 0;
  for (int i=0; i<3; i++)
    origimage[i] = 0;
}

Sat::Sat(const Sat &rhs)
{
  METLIBS_LOG_SCOPE();
  memberCopy(rhs);
}

Sat::Sat (const std::string &pin) :
  approved(false),  autoFile(true),
  cut(defaultCut), alphacut(defaultAlphacut), alpha(defaultAlpha),
  maxDiff(defaultTimediff), classtable(defaultClasstable),
  palette(false), mosaic(false),
  commonColourStretch(false), image(0), calibidx(-1),
  channelschanged(true), rgboperchanged(true),
  alphaoperchanged(true),mosaicchanged(true)
{
  METLIBS_LOG_SCOPE(LOGVAL(pin)<<LOGVAL(cut));

  for (int i=0; i<maxch; i++)
    rawimage[i] = 0;
  for (int i=0; i<3; i++)
    origimage[i] = 0;

  std::vector<std::string> tokens= miutil::split(pin);
  int n= tokens.size();
  if (n < 4) {
    METLIBS_LOG_WARN("Wrong syntax: "<<pin);
  }

  satellite = tokens[1];
  filetype = tokens[2];
  plotChannels = tokens[3];

  std::string key, value;

  for (int i=4; i<n; i++) { // search through plotinfo
    std::vector<std::string> stokens= miutil::split(tokens[i], 0, "=");
    if (stokens.size()==2) {
      key = miutil::to_lower(stokens[0]);
      value = stokens[1];
      if (key=="file") {
        filename = value;
        autoFile = false;
      } else if (key=="mosaic")
        mosaic = (atoi(value.c_str())!=0);
      else if (key=="cut")
        cut = atof(value.c_str());
      else if (key == "timediff")
        maxDiff = atoi(value.c_str());
      else if (key=="alphacut" || key=="alfacut")
        alphacut = (int) (atof(value.c_str())*255);
      else if (key=="alpha" || key=="alpha")
        alpha = (int) (atof(value.c_str())*255);
      else if (key=="table")
        classtable = (atoi(value.c_str())!=0);
      else if (key=="hide") {
        std::vector<std::string> stokens = miutil::split(value, 0, ",");
        int m= stokens.size();
        for (int j=0; j<m; j++) {
          std::vector <std::string> sstokens=miutil::split(stokens[j], 0, ":");
          if(sstokens.size()==1) {
            hideColour[atoi(sstokens[0].c_str())] = 0;
          } else {
            hideColour[atoi(sstokens[0].c_str())] = atoi(sstokens[1].c_str());
          }
        }
      }
    }
  }

  METLIBS_LOG_DEBUG(LOGVAL(cut) << LOGVAL(alphacut) << LOGVAL(alpha) << LOGVAL(maxDiff) << LOGVAL(classtable));
}

Sat::~Sat()
{
  cleanup();
}

Sat& Sat::operator=(const Sat &rhs)
{
  METLIBS_LOG_SCOPE();

  if (this != &rhs)
    memberCopy(rhs);
  return *this;
}

bool Sat::operator==(const Sat &rhs) const
{
  return false;
}

void Sat::memberCopy(const Sat& rhs)
{
  METLIBS_LOG_SCOPE(LOGVAL(rhs.area.nx) << LOGVAL(rhs.area.ny));

  // first clean up images etc.
  cleanup();

  // copy members
  approved= rhs.approved;
  satellite= rhs.satellite;
  filetype= rhs.filetype;
  filename= rhs.filename;
  actualfile= rhs.actualfile;
  cut= rhs.cut;
  alphacut= rhs.alphacut;
  alpha= rhs.alpha;
  maxDiff= rhs.maxDiff;
  classtable= rhs.classtable;
  hideColour=rhs.hideColour;

  area= rhs.area;

  time= rhs.time;
  annotation= rhs.annotation;
  plotname= rhs.plotname;
  palette= rhs.palette;
  plotChannels= rhs.plotChannels;
  calibidx= rhs.calibidx;
  for (int i=0; i<3; i++)
    rgbindex[i]= rhs.rgbindex[i];

  autoFile = rhs.autoFile;
  channelschanged= rhs.channelschanged;
  mosaicchanged=rhs.mosaicchanged;
  rgboperchanged= rhs.rgboperchanged;
  alphaoperchanged= rhs.alphaoperchanged;
  mosaic=rhs.mosaic;
  firstMosaicFileTime=rhs.firstMosaicFileTime;
  lastMosaicFileTime=rhs.lastMosaicFileTime;
  commonColourStretch=rhs.commonColourStretch;
  // copy images
  long size= area.gridSize();

  if (size) {
    image = new unsigned char[size];
    for (int i=0; i<maxch; i++) {
      if (rhs.rawimage[i]!=0) {
        rawimage[i]= new unsigned char[size];
        rawchannels[i]= rhs.rawchannels[i];
      }
    }
    for (int j=0; j<size; j++) {
      image[j]=rhs.image[j];
      for (int i=0; i<maxch; i++)
        if (rawimage[i]!=0)
          rawimage[i][j]= rhs.rawimage[i][j];
    }
    for (int k=0; k<3; k++) {
      origimage[k]= new float[size];
    }
    for (int j=0; j<size; j++) {
      for (int i=0; i<3; i++)
        if (origimage[i]!=NULL)
          origimage[i][j]= rhs.origimage[i][j];
    }
  }

}

void Sat::setDefaultValues(const SatDialogInfo & Dialog)
{
  //default values used if nothing is set in OKstring
  defaultCut=Dialog.cut.value*Dialog.cut.scale;
  defaultAlphacut = int (Dialog.alphacut.value*Dialog.alphacut.scale*255);
  defaultAlpha=int (Dialog.alpha.value*Dialog.alpha.scale*255);
  defaultTimediff=int (Dialog.timediff.value*Dialog.timediff.scale);
  defaultClasstable=true;
  //defaultHideColor;
}

/* * PURPOSE:   calculate and print the temperature or the albedo of
 *            the pixel pointed at
 */
void Sat::values(int x, int y, std::vector<SatValues>& satval)
{
  if (x>=0 && x<area.nx && y>=0 && y<area.ny && approved) { // inside image/legal image
    int index = area.nx*(area.ny-y-1) + x;

    //return value from  all channels

    std::map<int,table_cal>::iterator p=calibrationTable.begin();
    std::map<int,table_cal>::iterator q=calibrationTable.end();
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
      if ( hideColour.count(pvalue) && hideColour[pvalue] == 0) {
          return;
      }
      if (pvalue!=0) {
        if (p->second.val.size()>0) {
          if (int(p->second.val.size())>pvalue) {
            if (palette) {
              sv.text = p->second.val[(int)pvalue];
            } else {
              sv.value = atof(p->second.val[(int)pvalue].c_str());
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

  METLIBS_LOG_DEBUG("Sat::setCalibration satellite_name: "<< satellite_name);

  cal_channels.clear();
  calibrationTable.clear();

  //palette tiff, but no palette info
  if (palette && !paletteInfo.clname.size() )
    return;

  std::string start = satellite_name + " " + filetype +"|";

  METLIBS_LOG_DEBUG("Sat::setCalibration -- palette: " << palette);

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

  METLIBS_LOG_DEBUG("Sat::setCalibration -- cal_table.size(): " << cal_table.size());

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

  METLIBS_LOG_DEBUG("Sat::setCalibration -- cal_vis.exists(): " << !cal_vis.empty());

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

  METLIBS_LOG_DEBUG("Sat::setCalibration -- cal_ir.exists(): " << !cal_ir.empty());

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

  METLIBS_LOG_DEBUG("Sat::setCalibration -- vch.size(): " << vch.size());

  //channel "3", "4" and "5" are infrared, the rest are visual
  int n = vch.size();
  for (int j=0; j<n; j++) {

    METLIBS_LOG_DEBUG("Sat::setCalibration -- vch["<< j << "]: " << vch[j]);

    /* hdf5type == 0 => radar or mitiff
     * hdf5type == 1 => NOAA (HDF5)
     * hdf5type == 2 => MSG (HDF5)
     * and channels 4 and 7-11 ar infrared
     */
    if(miutil::contains(vch[j], "-")) {
      ir=false;
      vis=false;
    }
    bool isIRchannel=(((hdf5type == 0) && (vch[j]=="3" ||vch[j]=="4" || vch[j]=="5"))
        || (hdf5type == 2 && (miutil::contains(vch[j], "4") || miutil::contains(vch[j], "7") ||
            miutil::contains(vch[j], "8") || miutil::contains(vch[j], "9") || miutil::contains(vch[j], "10") ||
            miutil::contains(vch[j], "11"))) || (hdf5type == 1 && (miutil::contains(vch[j], "4"))));
    table_cal ct;
    if (ir && isIRchannel) {
      ct.channel = start + "Infrared (" + vch[j] + "):";
      ct.a= AIr;
      ct.b= BIr-273.0;//use degrees celsius instead of Kelvin

      METLIBS_LOG_DEBUG("Sat::setCalibration -- ir ct.a: " << ct.a);
      METLIBS_LOG_DEBUG("Sat::setCalibration -- ir ct.b: " << ct.a);

      calibrationTable[j]=ct;
      cal_channels.push_back(ct.channel);
    } else if (vis) {
      ct.channel = start + "Visual (" + vch[j] + "):";
      ct.a= AVis;
      ct.b= BVis;

      METLIBS_LOG_DEBUG("Sat::setCalibration -- vis ct.a: " << ct.a);
      METLIBS_LOG_DEBUG("Sat::setCalibration -- vis ct.b: " << ct.a);

      calibrationTable[j]=ct;
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
  annotation += filetype;
  annotation += " ";
  annotation += plotChannels;
  annotation += " ";
  annotation += time.format("%D %H:%M");
  if (mosaic) {
    //add info about first/last mosaic-file
    std::string temp = " (";
    temp += firstMosaicFileTime.format("%H:%M");
    temp +=" - " + lastMosaicFileTime.format("%H:%M");
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
  plotname += filetype;
  plotname += " ";
  plotname += plotChannels;
  if (!autoFile)
    plotname += " "+time.isoTime();
}

void Sat::setArea()
{
  METLIBS_LOG_SCOPE(Ax<<" : "<<Ay<<" : "<<Bx<<" : "<<By << " : " << proj_string);

  // If the mitiff image contains no proj string, it is probably transformed to +R=6371000
  // and adjusted to fit nwp-data and maps.
  //These adjustments require no conversion between +R=6371000 and ellps=WGS84,
  // and therefore no +datum or +towgs84 are given.
  if (proj_string == "") {
    std::stringstream tmp_proj_string;
    tmp_proj_string << "+proj=stere";
    tmp_proj_string << " +lon_0=" << GridRot;
    tmp_proj_string << " +lat_ts=" << TrueLat;
    tmp_proj_string << " +lat_0=90";
    tmp_proj_string << " +R=6371000";
    tmp_proj_string << " +units=km";
    tmp_proj_string << " +x_0=" << (Bx*-1000.);
    tmp_proj_string << " +y_0=" << (By*-1000.)+(Ay*area.ny*1000.);
    proj_string = tmp_proj_string.str();
  }
  Projection p(proj_string);
  area.setP(p);
  area.resolutionX = Ax;
  area.resolutionY = Ay;
  Rectangle r(0., 0., area.nx*area.resolutionX, area.ny*area.resolutionY);
  area.setR(r);
}

void Sat::cleanup()
{
  METLIBS_LOG_SCOPE(LOGVAL(area.nx) << LOGVAL(area.ny));

  area.nx = 0;
  area.ny = 0;

  delete[] image;
  image = 0;

  for (int i=0; i<maxch; i++) {
    delete[] rawimage[i];
    rawimage[i]= 0;
  }

  for(int j=0; j<3; j++) {
    delete[] origimage[j];
    origimage[j] = 0;
  }

  calibidx= -1;
  annotation.erase();
  plotname.erase();
  approved= false;
}
