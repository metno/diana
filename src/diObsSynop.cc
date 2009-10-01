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

#ifdef METNOOBS

#include <diObsSynop.h>

ObsSynop::ObsSynop(const miString &file)
:synop(file){
}

void ObsSynop::init(ObsPlot *oplot){

  //It is assumed that each station has only one entry in the synop
  //file, and that fileObsTime == ObsTime

  int numStations= contents.size();

  for(int i=0; i<numStations; i++)
    if(oplot->timeOK(contents[i].desc.obsTime)){
      ObsData &d = oplot->getNextObs();
      putData(i,d);
    }

}


void ObsSynop::putData(int i, ObsData &d){

  d.dataType="synop";

  //Description
  d.id = contents[i].desc.kjennetegn;
  d.id.trim();
  d.xpos =  contents[i].desc.pos.longitude();
  d.ypos = contents[i].desc.pos.latitude();
  d.fdata["stationHeight"] = (float)contents[i].desc.hoeyde;
  d.fdata["Zone"] = d.zone = _IDtoZone(contents[i].stationID);
  d.obsTime = contents[i].desc.obsTime;

  //Data
  if(contents[i].data.ix != undef)
    d.fdata["ix"]    = (float)contents[i].data.ix;		// Eksist. værgr. ?/
  if(contents[i].data.tr != undef)
    d.fdata["tr"]   = (float)contents[i].data.tr;	// Celsius
   if(contents[i].data.ir != undef)
     d.fdata["ir"]    = (float)contents[i].data.ir;		// nedbørindikator
  if(contents[i].data.h != undef )
    d.fdata["h"]     = (float)contents[i].data.h;		// Skyhøyde
  if(contents[i].data.VV != undef)
    d.fdata["VV"]    = visibility(contents[i].data.VV);		// Synsvidde
  if(contents[i].data.N != undef)
    d.fdata["N"]     = (float)contents[i].data.N;		// Skydekke
  if(contents[i].data.dd != undef)
    d.fdata["dd"]    = (float)contents[i].data.dd;		// grader
  if(contents[i].data.ff != undef)
    d.fdata["ff"]    = knots2ms(contents[i].data.ff);		// knop
   if(contents[i].data.TTT != undef)
     d.fdata["TTT"]   = contents[i].data.TTT;	// Celsius
  if(contents[i].data.TdTdTd != undef)
    d.fdata["TdTdTd"]= contents[i].data.TdTdTd;	// Celsius
  if(contents[i].data.PPPP != undef)
    d.fdata["PPPP"]  = contents[i].data.PPPP;	// (mb)
  if(contents[i].data.PoPoPoPo != undef &&
     contents[i].data.PPPP != undef &&
     contents[i].data.TTT != undef)
    d.fdata["T_red"]  = potTemperature(contents[i].data.PoPoPoPo,
				       contents[i].data.PPPP,
				       contents[i].data.TTT);
  if(contents[i].data.hhh != undef)
    d.fdata["HHH"]   = (float)contents[i].data.hhh;       // Height(1000hPa)
  if(contents[i].data.a >= 0 && contents[i].data.a < 10)
    d.fdata["a"]     = (float)contents[i].data.a;		// Trykktendens
  if(contents[i].data.ppp != undef){
    d.fdata["ppp"]   = contents[i].data.ppp;		// Tendens
    if( d.fdata.count("a") && d.fdata["a"] > 4 )
      d.fdata["ppp"] *= -1;
  }
  if(contents[i].data.RRR != undef){
    if(contents[i].data.RRR<0.1 && contents[i].data.ir != 3)
      d.fdata["RRR"]   = 990;		// nedbør, mm
    else
      d.fdata["RRR"]   = contents[i].data.RRR;		// nedbør, mm
  }
  if(contents[i].data.Rt != undef)
    d.fdata["Rt"]   = contents[i].data.Rt;		// Tidel av målt nedbør
  if(contents[i].data.ww != undef){
    d.fdata["ww"]    = (float)contents[i].data.ww;		// Været ved obs.tid
  }
  if(contents[i].data.W1 >2 && contents[i].data.W1 <10)
    d.fdata["W1"]    = (float)contents[i].data.W1;	        // Været 1 før obs.tid
  if(contents[i].data.W2 >2 && contents[i].data.W2 <10)
    d.fdata["W2"]    = (float)contents[i].data.W2;		// Været 2 før obs.tid
  if(contents[i].data.Nh != undef)
    d.fdata["Nh"]    = (float)contents[i].data.Nh; // Amount of low (or medium) clouds
  if(contents[i].data.Cl > 0 && contents[i].data.Cl < 10)
    d.fdata["Cl"]    = (float)contents[i].data.Cl;		// Type of low clouds
  if(contents[i].data.Cm > 0 && contents[i].data.Cm < 10)
    d.fdata["Cm"]    = (float)contents[i].data.Cm;           // Type of medium clouds
  if(contents[i].data.Ch > 0 && contents[i].data.Ch < 10)
    d.fdata["Ch"]    = (float)contents[i].data.Ch;		// Type of high clouds

  // Seksjon 222
  if(contents[i].data.ds > 0 && contents[i].data.ds < 9 &&
     contents[i].data.vs > -1 && contents[i].data.vs < 10){
    d.fdata["vs"]    = (float)contents[i].data.vs;	// Skipets hastighet
    d.fdata["ds"]    = (float)contents[i].data.ds*45;	// Skipets retning
  }
  if(contents[i].data.TwTwTw != undef)
    d.fdata["TwTwTw"]= contents[i].data.TwTwTw;      	// Sjøtemperatur
  if( contents[i].data.PwaPwa != undef ||
      contents[i].data.HwaHwa != undef) {
    d.fdata["PwaPwa"] = (float)contents[i].data.PwaPwa;      	// Periode wave
    d.fdata["HwaHwa"] = (float)contents[i].data.HwaHwa/2 ;     	// Høyde wave 1/2 m
  } else if( (float)contents[i].data.PwPw != undef ||
	     contents[i].data.HwHw != undef) {
    d.fdata["PwaPwa"] = (float)contents[i].data.PwPw;		// Periode wave
    d.fdata["HwaHwa"] = (float)contents[i].data.HwHw/2;		// Høyde wave 1/2 m
  }
  if(contents[i].data.dw1dw1 > 0 && contents[i].data.dw1dw1 < 37)
    d.fdata["dw1dw1"]= (float)contents[i].data.dw1dw1*10;      // wawe direction
  if(contents[i].data.Pw1Pw1 != undef || contents[i].data.Hw1Hw1 != undef){
    d.fdata["Pw1Pw1"]= (float)contents[i].data.Pw1Pw1;
    d.fdata["Hw1Hw1"]= (float)contents[i].data.Hw1Hw1/2;
  }

  // Seksjon 333
  if( contents[i].data.TxTxTx != undef)
    d.fdata["TxTn"] = contents[i].data.TxTxTx;	// Dagens maks.temp
  else if( contents[i].data.TnTnTn != undef)
    d.fdata["TxTn"] = contents[i].data.TnTnTn;	// Nattens minimum
  if(contents[i].data.sss > 0 ){
    if( contents[i].data.sss <997)
      d.fdata["sss"]    = (float)contents[i].data.sss;	// Snødybde i cm
    else if(contents[i].data.sss <1000)
      d.fdata["sss"]    = 0;                    // Delvis snødekke
  }
  if(contents[i].data.ff_911 != undef)
    d.fdata["911ff"] = knots2ms(contents[i].data.ff_911);	// Største vindkast.

  // Seksjon 555
  if(contents[i].data.s != undef)
    d.fdata["s"]      = (float)contents[i].data.s;	// Sjøgang
  if(contents[i].data.fxfx != undef)
    d.fdata["fxfx"]   = knots2ms(contents[i].data.fxfx); // Maks. middelvind siden forige

}

float ObsSynop::visibility(int vv)
{
  if(vv<51)
    return float(vv*100);
  if(vv<81)
    return float((vv-50)*1000);
  if(vv<51)
    return float(vv*100);
  if(vv<89)
    return float((vv-80)*5000+30000);

  switch(vv) {
  case 89:
    return 75000.;
  case 90:
    return 0.;
  case 91:
    return  50.;
  case 92:
    return 200.;
  case 93:
    return 500.;
  case 94:
    return  1000.;
  case 95:
    return  2000.;
  case 96:
    return  4000.;
  case 97:
    return  10000.;
  case 98:
    return  20000.;
  case 99:
    return  55000.;
  }

  return 0;
}

float ObsSynop::potTemperature(const float& PoPoPoPo,
			      const float& PPPP,
			      const float& TTT)
{

  float a=PPPP/PoPoPoPo;
  float b=287./1004;
  return ((TTT+273.15)*pow(a,b)-273.15);

}



#endif
