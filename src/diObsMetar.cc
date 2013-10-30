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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef METNOOBS

#include <diObsMetar.h>

using namespace::miutil;

ObsMetar::ObsMetar(const std::string &file)
 : metar(file)
{
}

void ObsMetar::init(ObsPlot *oplot){


  //It is assumed that each station has only one entry in the metar
  //file, and that fileObsTime == ObsTime

  int numStations= contents.size();

  for(int i=0; i<numStations; i++)
    if(oplot->timeOK(contents[i].desc.obsTime)){
      ObsData &d = oplot->getNextObs();
      putData(i,d);
    }

}


void
ObsMetar::putData(int i,ObsData &d){

  d.dataType="metar";
    //Description
  d.id = miutil::trimmed(contents[i].desc.kjennetegn);
  d.xpos =  contents[i].desc.pos.longitude();
  d.ypos = contents[i].desc.pos.latitude();
  d.zone = _IDtoZone(contents[i].stationID);
  d.obsTime = contents[i].desc.obsTime;
  d.metarId = miutil::trimmed(contents[i].desc.metarKjennetegn);
  //Data
  //  if(contents[i].data.ddd != undef)
    d.fdata["dd"]    = (float)contents[i].data.ddd;	// grader
    //  if(contents[i].data.ff != undef)
    d.fdata["ff"]    = knots2ms(contents[i].data.ff);	// knop
  if( contents[i].data.TT>-99.5 && contents[i].data.TT<99.5 )
    d.fdata["TTT"]   = contents[i].data.TT;	// Celsius
  if( contents[i].data.TdTd>-99.5 && contents[i].data.TdTd<99.5 )
    d.fdata["TdTdTd"]= contents[i].data.TdTd;	// Celsius
  d.CAVOK = contents[i].data.CAVOK;
  if(contents[i].data.fmfm > 0 && contents[i].data.fmfm < 100 )
    d.fdata["fmfm"] = knots2ms(contents[i].data.fmfm);		// wind gust
  if(contents[i].data.dndndn > -1 && contents[i].data.dndndn < 37 )
    d.fdata["dndndn"] = (float)contents[i].data.dndndn*10;
  //limit of variable wind direction
  if(contents[i].data.dxdxdx > -1 && contents[i].data.dxdxdx < 37 )
    d.fdata["dxdxdx"] = (float)contents[i].data.dxdxdx*10;       // ditto
  for( int j=0; j<3; j++ ){
    d.REww.push_back(contents[i].data.REww[j]);
  }
  if(contents[i].data.PHPHPHPH != undef)
    d.fdata["PHPHPHPH"] = (float)contents[i].data.PHPHPHPH;	// QNH
  if(contents[i].data.VVVV > -1 && contents[i].data.VVVV < 10000 )
    d.fdata["VVVV"] = (float)contents[i].data.VVVV;		// Visibility (worst)
  if(contents[i].data.Dv != undef){
    d.fdata["Dv"] = float(contents[i].data.Dv*45);		// Direction of VVVV
  }
  if(contents[i].data.VxVxVxVx >-1 && contents[i].data.VxVxVxVx < 10000)
    d.fdata["VxVxVxVx"] = (float)contents[i].data.VxVxVxVx;	// Visibility (best)
  if(contents[i].data.Dvx != undef)
    d.fdata["Dvx"] = (float)contents[i].data.Dvx;		// Direction of VxVxVxVx
  // enter runway visibility....
  for( int j=0; j<3; j++ ){
    d.ww.push_back(contents[i].data.ww[j]);
  }
  for( int j=0; j<5; j++ )
    d.cloud.push_back(cloud(contents[i].data.cloud[j]));	// Clouds
  d.appendix = contents[i].data.appendix;  // For whatever remains


}

std::string ObsMetar::cloud(std::string cl)
{

  if(cl.empty()) return cl;

  if(miutil::contains(cl, "SCT"))      miutil::replace(cl, "SCT","S/");
  else if(miutil::contains(cl, "BKN")) miutil::replace(cl, "BKN","B/");
  else if(miutil::contains(cl, "OVC")) miutil::replace(cl, "OVC","O/");
  else if(miutil::contains(cl, "FEW")) miutil::replace(cl, "FEW","F/");

  if(miutil::contains(cl, "TCU")) miutil::replace(cl, "TCU"," TCU");
  if(miutil::contains(cl, "CB"))  miutil::replace(cl, "CB"," CB");

  return cl;

}

#endif
