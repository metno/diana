/*
 Diana - A Free Meteorological Visualisation Tool

 $Id$

 Copyright (C) 2006 met.no

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
//#define DEBUGPRINT

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diSat.h>
#include <iostream>

using namespace::miutil;

//static values that should be set from SatManager
float Sat::defaultCut=-1;
int Sat::defaultAlphacut=0;
int Sat::defaultAlpha=255;
int Sat::defaultTimediff=60;
bool Sat::defaultClasstable=false;

// Default constructor
Sat::Sat() :
  approved(false),  autoFile(true),
  cut(defaultCut), alphacut(defaultAlphacut), alpha(defaultAlpha),
  maxDiff(defaultTimediff), classtable(defaultClasstable),
  nx(0), ny(0), palette(false), mosaic(false),
  commonColourStretch(false), image(0), calibidx(-1),
  channelschanged(true), rgboperchanged(true),
  alphaoperchanged(true),mosaicchanged(true)
  {

#ifdef DEBUGPRINT
  cerr << "Sat constructor" << endl;
#endif

  for (int i=0; i<maxch; i ++)
    rawimage[i]= 0;
  for (int i=0; i<3; i++)
    origimage[i]= NULL;
  }

// Copy constructor
Sat::Sat (const Sat &rhs)
{
#ifdef DEBUGPRINT
  cerr << "Sat copy constructor  ";
#endif
  // elementwise copy
  memberCopy(rhs);
}

Sat::Sat (const miString &pin) :
  approved(false),  autoFile(true),
  cut(defaultCut), alphacut(defaultAlphacut), alpha(defaultAlpha),
  maxDiff(defaultTimediff), classtable(defaultClasstable),
  nx(0), ny(0), palette(false), mosaic(false),
  commonColourStretch(false), image(0), calibidx(-1),
  channelschanged(true), rgboperchanged(true),
  alphaoperchanged(true),mosaicchanged(true)
  {

#ifdef DEBUGPRINT
  cerr << "Sat constructor(pin)" << endl;
#endif

  for (int i=0; i<maxch; i++)
    rawimage[i]= 0;
  for (int i=0; i<3; i++)
    origimage[i]= NULL;

  vector<miString> tokens= pin.split();
  int n= tokens.size();
  if (n < 4) {
    cerr << "Wrong syntax: "<<pin<<endl;
  }

  satellite = tokens[1];
  filetype = tokens[2];
  plotChannels = tokens[3];

  miString key, value;

  for (int i=4; i<n; i++) { // search through plotinfo
    vector<miString> stokens= tokens[i].split('=');
    if (stokens.size()==2) {
      key = stokens[0].downcase();
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
        vector <miString> stokens=value.split(',');
        int m= stokens.size();
        for (int j=0; j<m; j++) {
          hideColor.push_back(atoi(stokens[j].c_str()));
        }
      }
    }
  }

#ifdef DEBUGPRINT
  cerr << "cut = " << cut << endl; cerr<< "alphaCut = " << alphacut <<endl;
  cerr << "alpha = " << alpha << endl;
  cerr << "maxDiff = " << maxDiff << endl;
  cerr<< "classtable = " << classtable << endl;
#endif

  }

// Destructor
Sat::~Sat() {
#ifdef DEBUGPRINT
  cerr << "Sat destructor  nx=" << nx << " ny=" << ny << endl;
#endif
  cleanup();
}

// Assignment operato r
Sat& Sat::operator=(const Sat &rhs) {
#ifdef DEBUGPRINT
  cerr <<"Sat assignment operator  " ;
#endif

  if (this == &rhs) return *this;
  // elementwise copy
  memberCopy(rhs);

  return *this;
}

// Equality operator
bool Sat::operator==(const Sat &rhs) const {
  return false;
}

void Sat::memberCopy(const Sat& rhs)
{
#ifdef DEBUGPRINT
  cerr << "Sat memberCopy nx=" << rhs.nx << " ny=" << rhs.ny << endl;
#endif
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
  hideColor=rhs.hideColor;
  nx = rhs.nx;
  ny = rhs.ny;
  area= rhs.area;
  gridResolutionX = rhs.gridResolutionX;
  gridResolutionY = rhs.gridResolutionY;
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
  long size= nx*ny;

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
void Sat::values(int x, int y, vector<SatValues>& satval)
{

  if (x>=0 && x<nx && y>=0 && y<ny && approved) { // inside image/legal image
    int index = nx*(ny-y-1) + x;

    //return value from  all channels

    map<int,table_cal>::iterator p=calibrationTable.begin();
    map<int,table_cal>::iterator q=calibrationTable.end();
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
      for (unsigned int i=0; i<hideColor.size(); i++) {
        if (hideColor[i] ==pvalue)
          return;
      }
      if (pvalue!=0) {
        if (p->second.val.size()>0) {
          if (int(p->second.val.size())>pvalue) {
            if (palette) {
              sv.text = p->second.val[(int)pvalue];
            } else {
              sv.value = atof(p->second.val[(int)pvalue].cStr());
              if (p->second.channel.contains("TEMP"))
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

#ifdef DEBUGPRINT
  cerr << "Sat::setCalibration satellite_name: "<< satellite_name << endl;
#endif

  cal_channels.clear();
  calibrationTable.clear();

  //palette tiff, but no palette info
  if (palette && !paletteInfo.clname.size() )
    return;

  miString start = satellite_name + " " + filetype +"|";

#ifdef DEBUGPRINT
  cerr << "Sat::setCalibration -- palette: " << palette << endl;
#endif

  if (palette) {
    miString name = start + paletteInfo.name;
    table_cal ct;
    ct.channel = name;
    ct.val.push_back(" ");
    ct.val.insert(ct.val.end(), paletteInfo.clname.begin(),
        paletteInfo.clname.end());
    calibrationTable[0]=ct;
    cal_channels.push_back(name);
    return;
  }

#ifdef DEBUGPRINT
  cerr << "Sat::setCalibration -- cal_table.size(): " << cal_table.size() << endl;
#endif

  //Table
  if (cal_table.size()>0) {
    for (unsigned int i=0; i<cal_table.size(); i++) {
      table_cal ct;
      vector<miString> token = cal_table[i].split(",");
      if (token.size()!= 5)
        continue;
      int m = vch.size();
      int j=0;
      while (j<m && token[0]!=vch[j])
        j++;
      if (j==m)
        continue;
      miString name = start + token[1]+"("+token[0]+")";
      cal_channels.push_back(name);
      token[4].remove('[');
      token[4].remove(']');
      ct.channel = name;
      ct.val =token[4].split(" ");
      if (ct.val.size()!=256)
        continue;
      calibrationTable[j]=ct;
    }
    return;
  }

#ifdef DEBUGPRINT
  cerr << "Sat::setCalibration -- cal_vis.exists(): " << cal_vis.exists() << endl;
#endif

  //Visual
  bool vis=false;
  float AVis = 0.0, BVis = 0.0;
  if (cal_vis.exists()) {
    vector<miString> cal = cal_vis.split("+");
    if (cal.size()==2) {
      miString str = cal[0].substr(cal[0].find_first_of("(")+1);
      BVis = atof(str.c_str());
      str = cal[1].substr(cal[1].find_first_of("(")+1);
      AVis = atof(str.c_str());
      vis=true;
    }
  }

#ifdef DEBUGPRINT
  cerr << "Sat::setCalibration -- cal_ir.exists(): " << cal_ir.exists() << endl;
#endif

  //Infrared
  bool ir = false;
  float AIr = 0.0, BIr = 0.0;
  if (cal_ir.exists()) {
    vector<miString> cal = cal_ir.split("+");
    if (cal.size()==2) {
      miString str = cal[0].substr(cal[0].find_first_of("(")+1);
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

#ifdef DEBUGPRINT
  cerr << "Sat::setCalibration -- vch.size(): " << vch.size() << endl;
#endif

  //channel "3", "4" and "5" are infrared, the rest are visual
  int n = vch.size();
  for (int j=0; j<n; j++) {

#ifdef DEBUGPRINT
    cerr << "Sat::setCalibration -- vch["<< j << "]: " << vch[j] << endl;
#endif

    /* hdf5type == 0 => radar or mitiff
     * hdf5type == 1 => NOAA (HDF5)
     * hdf5type == 2 => MSG (HDF5)
     * and channels 4 and 7-11 ar infrared
     */
    if(vch[j].contains("-")) {
      ir=false;
      vis=false;
    }
    bool isIRchannel=(((hdf5type == 0) && (vch[j]=="3" ||vch[j]=="4" || vch[j]=="5"))
        || (hdf5type == 2 && (vch[j].contains("4") || vch[j].contains("7") ||
            vch[j].contains("8") || vch[j].contains("9") || vch[j].contains("10") ||
            vch[j].contains("11"))) || (hdf5type == 1 && (vch[j].contains("4"))));
    table_cal ct;
    if (ir && isIRchannel) {
      ct.channel = start + "Infrared (" + vch[j] + "):";
      ct.a= AIr;
      ct.b= BIr-273.0;//use degrees celsius instead of Kelvin

#ifdef DEBUGPRINT
      cerr << "Sat::setCalibration -- ir ct.a: " << ct.a << endl;
      cerr << "Sat::setCalibration -- ir ct.b: " << ct.a << endl;
#endif

      calibrationTable[j]=ct;
      cal_channels.push_back(ct.channel);
    } else if (vis) {
      ct.channel = start + "Visual (" + vch[j] + "):";;
      ct.a= AVis;
      ct.b= BVis;

#ifdef DEBUGPRINT
      cerr << "Sat::setCalibration -- vis ct.a: " << ct.a << endl;
      cerr << "Sat::setCalibration -- vis ct.b: " << ct.a << endl;
#endif

      calibrationTable[j]=ct;
      cal_channels.push_back(ct.channel);
    }

  }
}

void Sat::setAnnotation()
{
  annotation = satellite_name;
  annotation.trim();
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
    miString temp = " (";
    temp += firstMosaicFileTime.format("%H:%M");
    temp +=" - " + lastMosaicFileTime.format("%H:%M");
    temp+= ")";
    annotation+=temp;
  }
}

void Sat::setPlotName()
{
  plotname = satellite_name;
  plotname.trim();
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

#ifdef DEBUGPRINT
  cerr << "Sat::setArea: " << endl;
#endif


  if ( proj_string == "" ) {
       std::stringstream tmp_proj_string;
       tmp_proj_string << "+proj=stere";
       tmp_proj_string << " +lon_0=" << GridRot;
       tmp_proj_string << " +lat_ts=" << TrueLat;
       tmp_proj_string << " +lat_0=90";
       tmp_proj_string << " +R=6371000";
       tmp_proj_string << " +units=km";
       tmp_proj_string << " +x_0=" << (Bx*-1000.);
       tmp_proj_string << " +y_0=" << (By*-1000.)+(Ay*ny*1000.);
       tmp_proj_string << " +towgs84=0,0,0 +no_defs";
       proj_string = tmp_proj_string.str();
  }

  Projection p(proj_string, Ax, Ay);
  area.setP(p);
  gridResolutionX = Ax;
  gridResolutionY = Ay;
  Rectangle r(0., 0., nx*gridResolutionX, ny*gridResolutionY);

  area.setR(r);

}

void Sat::cleanup()
{

#ifdef DEBUGPRINT
  cerr << "Sat cleanup nx=" << nx << " ny=" << ny << endl;
#endif

  nx = 0;
  ny = 0;

  // delete plot image
  if (image!=0)
    delete[] image;
  image = 0;

  // delete raw images
  for (int i=0; i<maxch; i++) {
    if (rawimage[i]!=0)
      delete[] rawimage[i];
    rawimage[i]= 0;
  }

  for(int j=0; j<3; j++) {
    if(origimage[j] != NULL) {
      delete[] origimage[j];
    }
    origimage[j] = NULL;
  }

  calibidx= -1;
  annotation.erase();
  plotname.erase();
  approved= false;
}
