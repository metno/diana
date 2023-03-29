/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2022 met.no

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

#include "diSatPlot.h"

#include "diGLPainter.h"
#include "diRasterSat.h"
#include "diSat.h"
#include "diSatManager.h"
#include "diSatPlotCommand.h"
#include "diStaticPlot.h"
#include "util/misc_util.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.SatPlot"
#include <miLogger/miLogging.h>

using namespace::miutil;

SatPlot::SatPlot(SatPlotCommand_cp cmd, SatManager* satm)
    : SatPlotBase(cmd)
    , satm_(satm)
    , satdata(new Sat(command()))
{
}

SatPlot::~SatPlot()
{
}

void SatPlot::changeTime(const miutil::miTime& mapTime)
{
  const auto satP = getSatArea();
  satm_->setData(satdata.get(), mapTime);
  if (satP != getSatArea())
    updateMapSatTransformation();
  setPlotName(satdata->plotname);
  setStatus(hasData() ? P_OK_DATA : P_OK_EMPTY);
}

void SatPlot::changeProjection(const Area& mapArea, const Rectangle& plotSize, const diutil::PointI& physSize)
{
  SatPlotBase::changeProjection(mapArea, plotSize, physSize);
  if (mapArea.P() != mapP_) {
    mapP_ = mapArea.P();
    updateMapSatTransformation();
  }
}

void SatPlot::updateMapSatTransformation()
{
  const auto& satP = getSatArea().P();
  if (satP.isDefined())
    mapT_ = satP.transformationFrom(mapP_);
  else
    mapT_ = nullptr;
}

const GridArea& SatPlot::getSatArea()
{
  return satdata->area;
}

void SatPlot::getAnnotation(std::string &str, Colour &col) const
{
  if (satdata->approved){
    str = satdata->annotation;
    col = Colour("black");
  } else
    str.erase();
}

void SatPlot::getCalibChannels(std::vector<std::string>& channels) const
{
  diutil::insert_all(channels, satdata->cal_channels);
}

void SatPlot::values(float x, float y, std::vector<SatValues>& satval) const
{
  if (!isEnabled() || !hasData() || !mapT_)
    return;

  // x, y in map coordinates

  // convert to satellite proj coordinates
  if (!mapT_->forward(1, &x, &y))
    return;

  // convert to satellite pixel
  int xpos = satdata->area.toGridX(x);
  int ypos = satdata->area.toGridY(y);

  satdata->values(xpos,ypos,satval);
}

void SatPlot::plot(DiGLPainter* gl, PlotOrder porder)
{
  METLIBS_LOG_TIME();

  if (porder != PO_SHADE_BACKGROUND || !isEnabled() || !hasData())
    return;

  RasterSat rs(getStaticPlot()->plotArea(), satdata->area, satdata->image_rgba_);
  gl->drawScreenImage(QPointF(0, 0), rs.rasterPaint());
}

void SatPlot::getDataAnnotations(std::vector<std::string>& anno) const
{
  if (!isEnabled() || !hasData())
    return;

  for(auto& a : anno){
    if (miutil::contains(a, "$sat"))
      miutil::replace(a, "$sat", satdata->satellite_name);
  }

  //Colour table
  if (!satdata->palette || !satdata->classtable)
    return;

  const int nanno = anno.size();
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
}

bool SatPlot::hasData() const
{
  return satdata->approved && satdata->image_rgba_;
}
