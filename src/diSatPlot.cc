/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#include "diSatPlot.h"

#include "diGLPainter.h"
#include "diKVListPlotCommand.h"
#include "diPaintGLPainter.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.SatPlot"
#include <miLogger/miLogging.h>

using namespace::miutil;
using namespace std;

SatPlot::SatPlot()
  : satdata(0)
{
}

SatPlot::~SatPlot()
{
  delete satdata;
}

std::string SatPlot::getEnabledStateKey() const
{
  return miutil::mergeKeyValue(getPlotInfo(4));
}

void SatPlot::getAnnotation(std::string &str, Colour &col) const
{
  if (satdata->approved){
    str = satdata->annotation;
    col = Colour("black");
  } else
    str.erase();
}

void SatPlot::getSatName(std::string &str)
{
  if (satdata->approved) {
    str = miutil::trimmed(satdata->satellite_name + satdata->filetype);
    if (satdata->mosaic)
      str+=" MOSAIKK ";
    else
      str+= " ";
    str += satdata->time.isoTime();
  } else
    str.erase();
}

void SatPlot::setData(Sat *data)
{
  clearData();
  delete satdata;
  satdata = data;
}

void SatPlot::clearData()
{
}

void SatPlot::getCalibChannels(std::vector<std::string>& channels)
{
  channels.insert(channels.end(),
      satdata->cal_channels.begin(),
      satdata->cal_channels.end());
}

void SatPlot::values(float x, float y, std::vector<SatValues>& satval)
{
  if (not isEnabled())
    return;

  if (!satdata || !satdata->image || !satdata->approved)
    return;

  //x, y in map coordinates
  //Convert to satellite proj coordiantes
  getStaticPlot()->MapToProj(satdata->area.P(), 1, &x, &y);
  // convert to satellite pixel
  int xpos = satdata->area.toGridX(x);
  int ypos = satdata->area.toGridY(y);

  satdata->values(xpos,ypos,satval);
}

void SatPlot::plot(DiGLPainter* gl, PlotOrder porder)
{
  METLIBS_LOG_TIME();

  if (porder != SHADE_BACKGROUND)
    return;

  if (!isEnabled())
    return;

  if (!satdata || !satdata->image || !satdata->approved)
    return;

  gl->drawScreenImage(QPointF(0,0), rasterPaint());
}

void SatPlot::rasterPixels(int n, const diutil::PointD &xy0, const diutil::PointD &dxy, QRgb* pixels)
{
  const int nx = satdata->area.nx, ny = satdata->area.ny;

  const diutil::PointD fxy0(satdata->area.R().x1, satdata->area.R().y1);
  const diutil::PointD res(satdata->area.resolutionX, satdata->area.resolutionY);
  diutil::PointD ixy = (xy0 - fxy0) / res, step = dxy/res;
  int li = -1, lx = -1, ly = -1;
  for (int i=0; i<n; ++i, ixy += step) {
    const float dxy = 0.5f;
    const int ix = int(ixy.x() - dxy), iy = int(ixy.y() + dxy);
    if (ix<0 || ix>=nx || iy<0 || iy>=ny)
      continue;
    if (li >= 0 && ix == lx && iy == ly) {
      pixels[i] = pixels[li];
    } else {
      const unsigned char* p = &satdata->image[(ix + iy * nx)*4];
      pixels[i] = qRgba(p[0], p[1], p[2], p[3]);
      li = i;
      lx = ix;
      ly = iy;
    }
  }
}

bool SatPlot::getAnnotations(vector<string>& anno)
{
  if (!isEnabled())
    return false;

  if(satdata == NULL || satdata->image == NULL || !satdata->approved)
    return false;

  int nanno = anno.size();

  for(int i=0; i<nanno; i++){
    if (miutil::contains(anno[i], "$sat"))
      miutil::replace(anno[i], "$sat", satdata->satellite_name);
  }

  //Colour table
  if (!satdata->palette || !satdata->classtable)
    return false;

  for(int i=0; i<nanno; i++){

    if(! miutil::contains(anno[i], "table"))
      continue;

    std::string satName = satdata->paletteInfo.name;
    miutil::trim(satName);

    std::string endString;
    std::string startString;
    if (miutil::contains(anno[i], ",")){
      size_t nn = anno[i].find_first_of(",");
      endString = anno[i].substr(nn);
      startString =anno[i].substr(0,nn);
    } else {
      startString =anno[i];
    }

    if (miutil::contains(anno[i], "table=")) {
      std::string name = startString.substr(startString.find_first_of("=")+1);
      if( name[0]=='"' )
        miutil::remove(name, '"');
      miutil::trim(name);
      if (not miutil::contains(satName, name))
        continue;
    }

    std::string str  = "table=\"";
    str += satName;

    int n = satdata->paletteInfo.noofcl;
    //NB: better solution: get step from gui
    int step = n/50 +1;
    for (int j = 0;j< n;j+=step){
      std::string rgbstr;
      for( int k=0; k<3; k++){
        rgbstr+=miutil::from_number(satdata->paletteInfo.cmap[k][j+1]);
        if(k<2) rgbstr+=":";
      }
      str +=";";
      str +=rgbstr;
      str +=";;";
      str +=satdata->paletteInfo.clname[j];
    }

    str += "\" ";
    str += endString;

    anno.push_back(str);
  }

  return true;
}

void SatPlot::setSatAuto(bool autoFile,const string& satellite, const string& file)
{
  if (satdata->satellite == satellite && satdata->filetype == file)
    satdata->autoFile=autoFile;
}
