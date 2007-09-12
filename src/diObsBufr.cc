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

#ifdef BUFROBS

#include <iostream>
#include <sstream>
#include <map>
#include <miString.h>
#include <miTime.h>
#include <diObsData.h>
#include <diVprofPlot.h>
#include <diObsBufr.h>
#include <algorithm>

using namespace std;

const double bufrMissing= 1.6e+38;

bool ObsBufr::init(const miString& bufr_file, const miString& format)
{

  obsTime = miTime(); //undef
  const int ibflen=200000;
  int ibuff[ibflen/4];
  miString bufr_access= "r";
  int iunit= 20;
  int iret= 0;
  
  // open for read
  int len_bufr_file= bufr_file.length();
  int len_bufr_access= bufr_access.length();
  pbopen_(&iunit, bufr_file.cStr(), bufr_access.cStr(), &iret,
  	  len_bufr_file, len_bufr_access);
  if (iret!=0) {
    cerr<<"ObsBufr::init: PBOPEN failed for "<<bufr_file<<"   iret="<<iret<<endl;
    return false;
  }
  
  int iloop= 0;
  bool next=true;
  

  while (next) { // get the next BUFR product
    int ilen;
    int iibflen= ibflen;
    pbbufr_(&iunit, ibuff, &iibflen, &ilen, &iret);
    
    if (iret==-1) return true; // EOF

    if (iret!=0) {
      if (iret==-2) cerr<<"ObsBufr::init: ERROR: File handling problem"<<endl;
      if (iret==-3) cerr<<"ObsBufr::init: ERROR: Array too small"<<endl;
      return false;
    }

    next = BUFRdecode(ibuff, ilen, format);
  }

  return !next;
}
      
bool ObsBufr::ObsTime(const miString& bufr_file, miTime& time)
{

  const int ibflen=200000;
  int ibuff[ibflen/4];
  miString bufr_access= "r";
  int iunit= 20;
  int iret= 0;
  
  // open for read
  int len_bufr_file= bufr_file.length();
  int len_bufr_access= bufr_access.length();
  pbopen_(&iunit, bufr_file.cStr(), bufr_access.cStr(), &iret,
  	  len_bufr_file, len_bufr_access);
  if (iret!=0) {
    cerr<<"PBOPEN failed for "<<bufr_file<<"   iret="<<iret<<endl;
    return false;
  }
  
  int ilen;
  int iibflen= ibflen;
  pbbufr_(&iunit, ibuff, &iibflen, &ilen, &iret);
    
  if (iret!=0) {
    if (iret==-1) cerr<<"EOF"<<endl;
    if (iret==-2) cerr<<"ERROR: File handling problem"<<endl;
    if (iret==-3) cerr<<"ERROR: Array too small"<<endl;
    return false;
  }
  
  static int ksup[9];
  static int ksec0[3];
  static int ksec1[40];
  static int ksec2[64];
  int kerr;

  //read sec 0-2
  bus012_(&ilen, ibuff, ksup, ksec0, ksec1, ksec2,&kerr);

  //HACK assumes files from year 1971 to 2070
  int year = ksec1[8];
  year = (year>70) ? year+1900 : year+2000;
  time = miTime(year,ksec1[9],ksec1[10],ksec1[11],ksec1[12],0);

  return true;

}

bool ObsBufr::readStationInfo(const miString& bufr_file, 
			      vector<miString>& namelist,
			      vector<float>& latitudelist,
			      vector<float>& longitudelist)
{
  idmap.clear();
  init(bufr_file,"stationInfo");
  namelist      = id;
  latitudelist  = latitude;
  longitudelist = longitude;

}

VprofPlot* ObsBufr::getVprofPlot(const miString& bufr_file,
				 const miString& station, 
				 const miTime& time)
{

  index = izone = istation = 0;
  vplot = new VprofPlot;

  //if station(no)
  vector<miString> token = station.split("(");
  if(token.size()==2)
    index = atoi(token[1].cStr());

  strStation = token[0];

  if(token[0].isInt()){
    int ii = atoi(station.cStr());
    izone = ii/1000;
    istation = ii-izone*1000;
  } 
    
  init(bufr_file,"vprofplot");
  return vplot;
      
}

bool ObsBufr::BUFRdecode(int* ibuff, int ilen, const miString& format)
{
  // Decode BUFR message into fully decoded form.
  //  cerr <<"Decode BUFR message into fully decoded form."<<endl;
  
  const int kelem=4000, kvals=200000;
  
  const int len_cnames=64, len_cunits=24, len_cvals=80; 
  
  static int ksup[9];
  static int ksec0[3];
  static int ksec1[40];
  static int ksec2[64];
  static int ksec3[4];
  static int ksec4[2];

  static char cnames[kelem][len_cnames];
  static char cunits[kelem][len_cunits];
  static char cvals[kvals][len_cvals];
  static double values[kvals];
  
  static int ktdlst[kelem];
  static int ktdexp[kelem];
  int ktdlen;
  int ktdexl;
  int kerr;

  int kkelem= kelem;
  int kkvals= kvals;

  bufrex_(&ilen, ibuff, ksup, ksec0, ksec1, ksec2,
          ksec3, ksec4, &kkelem, &cnames[0][0], &cunits[0][0], &kkvals, values,
          &cvals[0][0], &kerr,
	  len_cnames, len_cunits, len_cvals);
  if (kerr>0) cerr<<"ObsBufr::init: Error in BUFREX: KERR="<<kerr<<endl;

 
  //HACK assumes files from year 1971 to 2070
  if(obsTime.undef()){
    int year = ksec1[8];
    year = (year>70) ? year+1900 : year+2000;
    obsTime = miTime(year,ksec1[9],ksec1[10],ksec1[11],ksec1[12],0);
  }

  // Return list of Data Descriptors from Section 3 of Bufr message, and 
  // total/requested list of elements. BUFREX must have been called before BUSEL.
  busel_(&ktdlen, ktdlst, &ktdexl, ktdexp, &kerr);
  if (kerr>0) cerr<<"ObsBufr::init: Error in BUSEL: KERR="<<kerr<<endl;
  
  if(format.downcase() == "obsplot"){
    ObsData &obs = oplot->getNextObs();
    if( oplot->getLevel() <-1){
      if(!get_diana_data(ktdexl, ktdexp, values, cvals, len_cvals, obs)
	 || !oplot->timeOK(obs.obsTime))
	oplot->removeObs();
    } else {
      if(!get_diana_data_level(ktdexl, ktdexp, values, cvals,len_cvals, 
			       obs, oplot->getLevel())
	 || !oplot->timeOK(obs.obsTime))
	oplot->removeObs();
    }
    return true;
  } 

  if(format.downcase() == "vprofplot"){

    return !get_data_level(ktdexl, ktdexp, values, cvals,len_cvals, obsTime);
  }

  if(format.downcase() == "stationinfo"){
    return get_station_info(ktdexl, ktdexp, values, cvals,len_cvals);
  }
  // cerr <<"returning from Bufrdecode"<<endl;
}

bool ObsBufr::get_diana_data(int ktdexl, int *ktdexp, double* values, 
			     const char cvals[][80], int len_cvals, ObsData &d)
{
  //  cerr <<"get_diana_data"<<endl;
  d.fdata.clear();

  // constants for changing to met.no units
  const double pa2hpa=0.01;
  const double t0=273.2;               // 273.15 ?????????

  int wmoBlock=0;
  int wmoStation=0;
  int year=0;
  int month=0;
  int day=0;
  int hour=0;
  int minute=0;
  bool landStation= true;
  miString cloudStr;
  int c;

  d.CAVOK = false;

  for (int i=0; i<ktdexl; i++) {

    switch (ktdexp[i]) {

      //   1001  WMO BLOCK NUMBER
    case 1001:
      wmoBlock = int(values[i]);
      d.zone = wmoBlock;
      break;

      //   1002  WMO STATION NUMBER
    case 1002:
      wmoStation = int(values[i]);
      break;

      //   1005 BUOY/PLATFORM IDENTIFIER 
    case 1005:
        d.id = miString(values[i]);
	landStation= false;
    break;

      //  1011  SHIP OR MOBILE LAND STATION IDENTIFIER, CCITTIA5 (ascii chars)
    case 1011:
      c=0;
      while (c<5 && int(cvals[i][c])!=0 && int(cvals[i][c])!=32) {
	d.id += cvals[i][c];
	c++;
      }
      landStation= false;
      break;

    case 1063:
      c=0;
      while (c<5 && int(cvals[i][c])!=0 && int(cvals[i][c])!=32) {
	d.metarId += cvals[i][c];
	c++;
      }
      break;

      //   1012  DIRECTION OF MOTION OF MOVING OBSERVING PLATFORM, DEGREE TRUE
    case 1012:
      if(values[i] > 0 && values[i]<bufrMissing)
	d.fdata["ds"] = values[i];
      break;
      
      //   1013  SPEED OF MOTION OF MOVING OBSERVING PLATFORM, M/S
    case 1013:
      if( values[i]<bufrMissing)
	d.fdata["vs"] = ms2code4451(values[i]);
      break;

      //   2001 TYPE OF STATION 
    case 2001:
      if (values[i]<bufrMissing)
	  d.fdata["auto"]=values[i];
      break;

      //   4001  YEAR
    case 4001:
      year= int(values[i]);
      break;

      //   4002  MONTH
    case 4002:
      month= int(values[i]);
      break;

      //   4003  DAY
    case 4003:
      day= int(values[i]);
      break;

      //   4004  HOUR
    case 4004:
      hour= int(values[i]);
      break;

      //   4005  MINUTE
    case 4005:
      minute= int(values[i]);
      break;

      //   5001  LATITUDE (HIGH ACCURACY),   DEGREE
      //   5002  LATITUDE (COARSE ACCURACY), DEGREE
    case 5001:
    case 5002:
      d.ypos= values[i];
      break;

      //   6001  LONGITUDE (HIGH ACCURACY),   DEGREE
      //   6002  LONGITUDE (COARSE ACCURACY), DEGREE
    case 6001:
    case 6002:
      d.xpos= values[i];
      break;

      //   7001  HEIGHT OF STATION, M
    case 7001:
      if (values[i]<bufrMissing)
	d.fdata["stationHeight"]= values[i];
      break;

      // 010003 GEOPOTENTIAL, M**2/S**2  
    case 10003:
      if (values[i]<bufrMissing){
       	d.fdata["HHH"] = values[i]/9.8;
      }
      break;

      //   10051  PRESSURE REDUCED TO MEAN SEA LEVEL, Pa->hPa
    case 10051:
      if (values[i]<bufrMissing)
	d.fdata["PPPP"] = values[i] * pa2hpa;
      break;

      //010052 ALTIMETER SETTING (QNH), Pa->hPa
    case 10052:
      if (values[i]<bufrMissing)
	d.fdata["PHPHPHPH"] = values[i] * pa2hpa;
      break;

      //   10061  3 HOUR PRESSURE CHANGE, Pa->hpa
    case 10061:
      if (values[i]<bufrMissing)
	d.fdata["ppp"] = values[i] * pa2hpa;
      break;

      //   10063  CHARACTERISTIC OF PRESSURE TENDENCY
    case 10063:
      if (values[i]<bufrMissing)
	d.fdata["a"] = values[i];
      break;

      //   11011  WIND DIRECTION AT 10 M, DEGREE TRUE
    case 11011:
      if (values[i]<bufrMissing)
	d.fdata["dd"] = values[i];
      break;

      //   11012  WIND SPEED AT 10 M, m/s 
    case 11012:
      if (values[i]<bufrMissing)
	d.fdata["ff"] = values[i];
      break;

      // 011016 EXTREME COUNTERCLOCKWISE WIND DIRECTION OF A VARIABLE WIND
    case 11016:
      if (values[i]<bufrMissing)
       	d.fdata["dndndn"] = values[i];
      break;

      // 011017 EXTREME CLOCKWISE WIND DIRECTIONOF A VARIABLE WIND       
    case 11017:
      if (values[i]<bufrMissing)
       	d.fdata["dxdxdx"] = values[i];
      break;
    
      // 011041 MAXIMUM WIND SPEED (GUSTS), m/s 
    case 11041:
      if (values[i]<bufrMissing){
	d.fdata["911ff"] = values[i];
	d.fdata["fmfm"] = values[i];  //metar
      }
      break;

      // 011042 MAXIMUM WIND SPEED (10 MIN MEAN WIND), m/s
    case 11042:
      if (values[i]<bufrMissing)
	d.fdata["fxfx"] = values[i];
      break;

      //   12004  DRY BULB TEMPERATURE AT 2M, K->Celsius
    case 12004:
      if (values[i]<bufrMissing)
	d.fdata["TTT"] = values[i] - t0;
      break;

      //   12006  DEW POINT TEMPERATURE AT 2M, K->Celsius
    case 12006:
      if (values[i]<bufrMissing)
	d.fdata["TdTdTd"] = values[i] - t0;
      break;

      //   12014  MAX TEMPERATURE AT 2M, K->Celsius
    case 12014:
      if (values[i]<bufrMissing && obsTime.hour() == 18)
	d.fdata["TxTn"] = values[i] - t0;
      break;

      //   12015  MIN TEMPERATURE AT 2M, K->Celsius
    case 12015:
      if (values[i]<bufrMissing && obsTime.hour() == 6)
	d.fdata["TxTn"] = values[i] - t0;
      break;

      //   13013  Snow depth
    case 13013:
      if (values[i]>0  && values[i]<bufrMissing)
	d.fdata["sss"] = values[i]*100;
      break;
    case 13218: //Partially snow cover (spell ??)
      if (values[i]<4){
	d.fdata["sss"] = 0.;
      }
      break;


      //Precipitation: 06 and 18: 13022, 00 and 12: 13021

      //13019 Total precipitation past 1 hour
    case 13019:
      if (obsTime.hour()%6!=0 && values[i]<bufrMissing)
	d.fdata["RRR"] = values[i];
      break;

      //13020 Total precipitation past 3 hour
    case 13020: 
      if (obsTime.hour()%6!=0 && values[i]<bufrMissing)
	d.fdata["RRR"] = values[i];
      break;

      //13021 Total precipitation past 6 hour
    case 13021:
      if (obsTime.hour()!=6 && obsTime.hour()!=18 && values[i]<bufrMissing)
	d.fdata["RRR"] = values[i];
      break;

      //13022 Total precipitation past 12 hour
    case 13022:
      if (obsTime.hour()%12!=0 && values[i]<bufrMissing)
	d.fdata["RRR"] = values[i];
      break;

      //13023 Total precipitation past 24 hour
    case 13023:
      if (obsTime.hour()%6!=0 && values[i]<bufrMissing)
	d.fdata["RRR"] = values[i];
      break;

      //   20001  HORIZONTAL VISIBILITY M
    case 20001:
      if (values[i]>0 && values[i]<bufrMissing){
	d.fdata["VV"] = values[i];
	//Metar
	if(values[i]>9999) //remove VVVV=0
	  d.fdata["VVVV"] = 9999;
	else
	  d.fdata["VVVV"] = values[i];
      }
      break;

      // 5021 BEARING OR AZIMUTH, DEGREE
    case 5021:	  //Metar
      if (values[i]<bufrMissing)
	d.fdata["Dv"] = values[i];
      break;

      //  20003  PRESENT WEATHER, CODE TABLE  20003  
    case 20003:
      if (values[i]<200) //values>200 -> w1w1, ignored here 
	d.fdata["ww"] = values[i];
      break;

      //  20004 PAST WEATHER (1),  CODE TABLE  20004
    case 20004:
      if (values[i]>2 && values[i]<20)
	d.fdata["W1"] = values[i];
      break;

      //  20005  PAST WEATHER (2),  CODE TABLE  20005
    case 20005:
      if (values[i]>2 && values[i]<20)
	d.fdata["W2"] = values[i];
      break;

      //   Clouds

    //CAVOK
    case 20009:
      if (values[i]<bufrMissing)
       	d.CAVOK = (int(values[i])==2);
      break;

      //HEIGHT OF BASE OF CLOUD (M)  
    case 20013:
      if (values[i]<bufrMissing){
	if (i<32)
	  d.fdata["h"] = height_of_clouds(values[i]);
	cloudStr += cloudHeight(int(values[i]));
      }
      break;

      //CLOUD COVER (TOTAL %) 
    case 20010: 
      if (values[i]<bufrMissing)
	d.fdata["N"] = values[i]/12.5 +0.5; //% -> oktas
      break;

      //20011  CLOUD AMOUNT  
    case 20011:
      if (values[i]<bufrMissing){
	if (i<32)	
	  d.fdata["Nh"] = values[i];
	if( cloudStr.exists()){
	  d.cloud.push_back(cloudStr);
	  cloudStr.clear();
	}
	cloudStr = cloudAmount(int(values[i]));
      }
      break;

      // 20012 CLOUD TYPE, CODE TABLE  20012   
    case 20012:
      if (values[i]<bufrMissing){
	cloud_type(d,values[i]);
	cloudStr += cloud_TCU_CB(int(values[i]));
      }
      break;

      // 020020 SIGNIFICANT RECENT WEATHER PHENOMENA, CCITTIA5   
    case 20019:
      {
	int index = int(values[i])/1000-1;
	miString ww;
	for(int j=0;j<9;j++)
	  ww += cvals[index][j];
	ww.trim();
	if(ww.exists())
	  d.ww.push_back(ww);
      }
      break;
      
      // 020020 SIGNIFICANT RECENT WEATHER PHENOMENA, CCITTIA5 
    case 20020:
      {
	int index = int(values[i])/1000-1;
        miString REww;
        for(int j=0;j<4;j++)
          REww += cvals[index][j];
	REww.trim();
	if(REww.exists())
	  d.REww.push_back(REww);
      }
      break;

      // 022011 PERIOD OF WAVES, s  
    case 22011:
      if (values[i]<bufrMissing)
	d.fdata["PwaPwa"] = values[i];
      break;

      // 022021 HEIGHT OF WAVES, m 
    case 22021:
      if (values[i]<bufrMissing)
	d.fdata["HwaHwa"] = values[i];
      break;

      // Not used in synop plot
//       // 022012 PERIOD OF WIND WAVES, s  
//     case 22012:
//       if (values[i]<bufrMissing)
// 	d.fdata["PwPw"] = values[i];
//       break;

//       // 022022 HEIGHT OF WIND WAVES, m 
//     case 22022:
//       if (values[i]<bufrMissing)
// 	d.fdata["HwHw"] = values[i];
//       break;

      // 022013 PERIOD OF SWELL WAVES, s  
    case 22013:
      if (values[i]<bufrMissing)
	d.fdata["Pw1Pw1"] = values[i];
      break;

      // 022023 HEIGHT OF SWELL WAVES, m 
    case 22023:
      if (values[i]<bufrMissing)
	d.fdata["Hw1Hw1"] = values[i];
      break;

      // 022003 DIRECTION OF SWELL WAVES, DEGREE   
    case 22003:
      if (values[i]>0 && values[i]<bufrMissing)
	d.fdata["dw1dw1"] = values[i];
      break;

      //   22042  SEA/WATER TEMPERATURE, K->Celsius
    case 22042:
      if (values[i]<bufrMissing)
	d.fdata["TwTwTw"] = values[i] - t0;
      break;

      // 022061 STATE OF THE SEA, CODE TABLE  22061
    case 22061:
      if (values[i]<bufrMissing)
	d.fdata["s"] = values[i];
      break;

    }
  }

  if (landStation) {
    ostringstream ostr;
    ostr << setw(2) << setfill('0') << wmoBlock
	 << setw(3) << setfill('0') << wmoStation;
    d.id= ostr.str();
  } else {
    d.fdata["stationHeight"]= 0.0;
    d.zone=99;
  }


  //Metar cloud
  if( cloudStr.exists()){
    d.cloud.push_back(cloudStr);
    cloudStr.clear();
  }

  //TIME
  d.obsTime = miTime(year,month,day,hour,minute,0);

  return true;
}	

bool ObsBufr::get_station_info(int ktdexl, int *ktdexp, double* values, 
			     const char cvals[][80], int len_cvals)
{

  int wmoBlock=0;
  int wmoStation=0;
  miString station;
  int year=0;
  int month=0;
  int day=0;
  int hour=0;
  int minute=0;
  bool landStation= true;
  int nn=0;
  int c;

  for (int i=0; i<ktdexl && nn<4; i++) {

    switch (ktdexp[i]) {

      //   1001  WMO BLOCK NUMBER
    case 1001:
      wmoBlock = int(values[i]);
      nn++;
      break;

      //   1002  WMO STATION NUMBER
    case 1002:
      wmoStation = int(values[i]);
      nn++;
      break;

      //  1011  SHIP OR MOBILE LAND STATION IDENTIFIER, CCITTIA5 (ascii chars)
    case 1011:
      c=0;
      while (c<5 && int(cvals[i][c])!=0 && int(cvals[i][c])!=32) {
	station += cvals[i][c];
	c++;
      }
      landStation= false;
      nn+=2;
      break;

      //   5001  LATITUDE (HIGH ACCURACY),   DEGREE
      //   5002  LATITUDE (COARSE ACCURACY), DEGREE
    case 5001:
    case 5002:
      latitude.push_back(values[i]);
      nn++;
      break;

      //   6001  LONGITUDE (HIGH ACCURACY),   DEGREE
      //   6002  LONGITUDE (COARSE ACCURACY), DEGREE
    case 6001:
    case 6002:
      longitude.push_back(values[i]);
      nn++;
      break;

    }
  }

  ostringstream ostr;
  if(landStation){
    ostr << setw(2) << setfill('0') << wmoBlock
	 << setw(3) << setfill('0') << wmoStation;
    station = ostr.str();
  } else {
    ostr <<station;
  }
 
  if(idmap.count(station)){
    ostr <<"("<<idmap[station]<<")";
    idmap[station]++;
    //    cerr <<"NEST:"<<tmpId<<endl;
  } else {
    idmap[station]=1;
    //    cerr <<"FØRST:"<<tmpId<<endl;
  }
  id.push_back(ostr.str());
  return true;

}

bool ObsBufr::get_diana_data_level(int ktdexl, int *ktdexp, double* values,
				   const char cvals[][80], int len_cvals, ObsData &d, int level)
{
  //  cerr <<"get_diana_data"<<endl;
  d.fdata.clear();

  // constants for changing to met.no units
  const double pa2hpa=0.01;
  const double t0=273.2;               // 273.15 ?????????

  int wmoBlock=0;
  int wmoStation=0;
  int year=0;
  int month=0;
  int day=0;
  int hour=0;
  int minute=0;
  bool landStation= true;
  int c;

  bool found=false;
  bool stop=false;

  for (int i=0; i<ktdexl; i++) {
    if(ktdexp[i]< 7000         // station info
       || found                // right pressure level
       || ktdexp[i]== 7004){   // next pressure level

      switch (ktdexp[i]) {

	//   1001  WMO BLOCK NUMBER
      case 1001:
	wmoBlock = int(values[i]);
	d.zone = wmoBlock;
	break;

	//   1002  WMO STATION NUMBER
      case 1002:
	wmoStation = int(values[i]);
	break;

	//  1011  SHIP OR MOBILE LAND STATION IDENTIFIER, CCITTIA5 (ascii chars)
      case 1011:
	c=0;
	while (c<5 && int(cvals[i][c])!=0 && int(cvals[i][c])!=32) {
	  d.id += cvals[i][c];
	  c++;
	}
	landStation= false;
	break;
	
	//   4001  YEAR
      case 4001:
	year= int(values[i]);
	break;
	
	//   4002  MONTH
      case 4002:
	month= int(values[i]);
	break;
	
	//   4003  DAY
      case 4003:
	day= int(values[i]);
	break;
	
	//   4004  HOUR
      case 4004:
	hour= int(values[i]);
	break;
	
	//   4005  MINUTE
      case 4005:
	minute= int(values[i]);
	break;
	
	//   5001  LATITUDE (HIGH ACCURACY),   DEGREE
	//   5002  LATITUDE (COARSE ACCURACY), DEGREE
      case 5001:
      case 5002:
	d.ypos= values[i];
      break;
      
      //   6001  LONGITUDE (HIGH ACCURACY),   DEGREE
      //   6002  LONGITUDE (COARSE ACCURACY), DEGREE
      case 6001:
      case 6002:
	d.xpos= values[i];
	break;
	
	//   7001  HEIGHT OF STATION, M
      case 7001:
	if (values[i]<bufrMissing)
	  d.fdata["stationHeight"]= values[i];
	break;
	
	//   10051  PRESSURE REDUCED TO MEAN SEA LEVEL, Pa->hPa
      case 7004:
	if(found){
	  stop=true;
	} else {
	  if (values[i]<bufrMissing){
	    if(int(values[i] * pa2hpa)==level){
	      found=true;
	      d.fdata["PPPP"] = values[i] * pa2hpa;
	    }
	  }
	}
	break;
	
	
      //   12004  DRY BULB TEMPERATURE AT 2M, K->Celsius
    case 12004:
      if (values[i]<bufrMissing)
	d.fdata["TTT"] = values[i] - t0;
      break;

      //   12006  DEW POINT TEMPERATURE AT 2M, K->Celsius
    case 12006:
      if (values[i]<bufrMissing)
	d.fdata["TdTdTd"] = values[i] - t0;
      break;


	//   11011  WIND DIRECTION AT 10 M, DEGREE TRUE
      case 11001:
	if (values[i]<bufrMissing)
	  d.fdata["dd"] = values[i];
	break;
	
	//   11012  WIND SPEED AT 10 M, m/s 
      case 11002:
	if (values[i]<bufrMissing)
	  d.fdata["ff"] = values[i];
	break;
	
	//   12004  DRY BULB TEMPERATURE AT 2M, K->Celsius
      case 12001:
	if (values[i]<bufrMissing)
	  d.fdata["TTT"] = values[i] - t0;
	break;
	
	//   12006  DEW POINT TEMPERATURE AT 2M, K->Celsius
      case 12003:
	if (values[i]<bufrMissing)
	  d.fdata["TdTdTd"] = values[i] - t0;
	break;
	
      case 10003:
	if (values[i]<bufrMissing){
	  d.fdata["HHH"] = values[i]/9.8;
	}
	break;
	
      }
    }  
  
// right pressure level found and corresponding parameters read
    if(stop) break;  
  }
  
  //right pressure level not found
  if(!found) return false; 

  if (landStation) {
    ostringstream ostr;
    ostr << setw(2) << setfill('0') << wmoBlock
	 << setw(3) << setfill('0') << wmoStation;
    d.id= ostr.str();
  } else {
    d.fdata["stationHeight"]= 0.0;
    d.zone=99;
  }

  //TIME
  d.obsTime = miTime(year,month,day,hour,minute,0);

  return true;
}	

bool ObsBufr::get_data_level(int ktdexl, int *ktdexp, double* values,
				   const char cvals[][80], int len_cvals, 
				   miTime time)
{

  // constants for changing to met.no units
  const double pa2hpa=0.01;
  const double t0=273.2;               // 273.15 ????????
  const float rad=3.141592654/180.;
  const double ms2knots=3600.0/1852.0;

  int wmoBlock=0;
  int wmoStation=0;
  miString station;
  int year=0;
  int month=0;
  int day=0;
  int hour=0;
  int minute=0;
  float p=0,tt=-30000,td=-30000;
  float fff,ddd;
  int   dd,ff,bpart;
  float lat,lon;
  bool landStation= true;
  int c;
  int   ffmax= -1, kmax= -1;

  static int ii=0;

  bool ok=false;

  for (int i=0; i<ktdexl; i++) {

    if(ktdexp[i]< 7000         // station info
       || ok                // pressure ok
       || ktdexp[i]== 7004){   // next pressure level
      
      if(  ktdexp[i]== 7004){ //new pressure level, save data
 
	if (p>0. && p<1300.) {
	  if (tt>-30000.) {
	    vplot->ptt.push_back(p);
	    vplot->tt.push_back(tt);
	    if (td>-30000.) {
	      vplot->ptd.push_back(p);
	      vplot->td.push_back(td);
	      vplot->pcom.push_back(p);
	      vplot->tcom.push_back(tt);
	      vplot->tdcom.push_back(td);
	      td=-31000.;
	    }
	    tt=-31000.;
	  }
	  if (dd>=0 && dd<=360 && ff>=0) {
	    vplot->puv.push_back(p);
	    vplot->dd.push_back(dd);
	    vplot->ff.push_back(ff);
	    // convert to east/west and north/south component
	    fff= float(ff);
	    ddd= (float(dd)+90.)*rad;
	    vplot->uu.push_back( fff*cosf(ddd));
	    vplot->vv.push_back(-fff*sinf(ddd));
 	    vplot->sigwind.push_back(bpart);
	    if (ff>ffmax) {
	      ffmax= ff;
	      kmax= vplot->sigwind.size() - 1;
	    }
	    dd=ff=-1;
	    bpart=1;
	  }
	}
      }

      switch (ktdexp[i]) {

	//   1001  WMO BLOCK NUMBER
      case 1001:
	if ( izone != int(values[i])) {
	  return false;
	}
	break;
	
	//   1002  WMO STATION NUMBER
      case 1002:
	
	if( istation != int(values[i])) {
	  return false;
	}
	if( index != ii){
	  ii++;
	  return false;
	}
	ii=0;
	break;

	// 1011  SHIP OR MOBILE LAND STATION IDENTIFIER, CCITTIA5 (ascii chars)
      case 1011:
	c=0;
	station.clear();
	while (c<5 && int(cvals[i][c])!=0 && int(cvals[i][c])!=32) {
	  station += cvals[i][c];
	  c++;
	}
	if( strStation != station ) {
	  return false;
	}
	if( index != ii){
	  ii++;
	  return false;
	}
	ii=0;
 	break;
	
	//   4001  YEAR
      case 4001:
	year= int(values[i]);
	break;
	
	//   4002  MONTH
      case 4002:
	month= int(values[i]);
	break;
	
	//   4003  DAY
      case 4003:
	day= int(values[i]);
	break;
	
	//   4004  HOUR
      case 4004:
	hour= int(values[i]);
	break;
	
	//   4005  MINUTE
      case 4005:
	minute= int(values[i]);
	break;
	
	//   5001  LATITUDE (HIGH ACCURACY),   DEGREE
	//   5002  LATITUDE (COARSE ACCURACY), DEGREE
      case 5001:
      case 5002:
	lat = values[i];
      break;
      
      //   6001  LONGITUDE (HIGH ACCURACY),   DEGREE
      //   6002  LONGITUDE (COARSE ACCURACY), DEGREE
      case 6001:
      case 6002:
	lon = values[i];
	break;
	
	//   10051  PRESSURE REDUCED TO MEAN SEA LEVEL, Pa->hPa
      case 7004:
	if (values[i]<bufrMissing){
	  p = int(values[i] * pa2hpa);
	  ok = (p>0. && p<1300.);
	}
	break;
		
	
	//   VERTICAL SOUNDING SIGNIFICANCE
      case 8001:
	if (values[i]>31. && values[i]<64)
	  bpart = 0;
	else 
	  bpart = 1;
	break;
	
	//   11011  WIND DIRECTION AT 10 M, DEGREE TRUE
      case 11001:
	if (values[i]<bufrMissing)
	  dd = int(values[i]);
	break;
	
	//   11012  WIND SPEED AT 10 M, m/s 
      case 11002:
	if (values[i]<bufrMissing)
          ff = int(values[i] * ms2knots + 0.5); //should be done elsewhere
	break;
	
	//   12001  DRY BULB TEMPERATURE 
      case 12001:
	if (values[i]<bufrMissing)
	  tt = values[i] - t0;
	break;
	
	//   12003  DEW POINT TEMPERATURE, K->Celsius
      case 12003:
	if (values[i]<bufrMissing)
	  td = values[i] - t0;
	break;
	
      }
    }  
  
// right pressure level found and corresponding parameters read
  }
  
  //right pressure level not found
  if (p>0. && p<1300.) {
    if (tt>-30000.) {
      vplot->ptt.push_back(p);
      vplot->tt.push_back(tt);
      if (td>-30000.) {
	vplot->ptd.push_back(p);
	vplot->td.push_back(td);
	vplot->pcom.push_back(p);
	vplot->tcom.push_back(tt);
	vplot->tdcom.push_back(td);
      }
    }
    if (dd>=0 && dd<=360 && ff>=0) {
      vplot->puv.push_back(p);
      vplot->dd.push_back(dd);
      vplot->ff.push_back(ff);
      // convert to east/west and north/south component
      fff= float(ff);
      ddd= (float(dd)+90.)*rad;
      vplot->uu.push_back( fff*cosf(ddd));
      vplot->vv.push_back(-fff*sinf(ddd));
      vplot->sigwind.push_back(bpart);
      if (ff>ffmax) {
	ffmax= ff;
	kmax= vplot->sigwind.size() - 1;
      }
    }
  }

  vplot->text.posName= strStation;
  vplot->text.posName.trim();
  vplot->text.prognostic= false;
  vplot->text.forecastHour= 0;
  vplot->text.validTime= miTime(year,month,day,hour,minute,0); 
  vplot->text.latitude=  lat;
  vplot->text.longitude= lon;
  vplot->text.kindexFound= false;

  if (kmax>=0) vplot->sigwind[kmax]= 3;

  vplot->prognostic= false;
  int l1= vplot->ptt.size();
  int l2= vplot->puv.size();
  vplot->maxLevels= (l1>l2) ? l1 : l2;

  return true;
}	



miString ObsBufr::cloudAmount(int i)
{
  if(i==0)  return "SKC";
  if(i==1)  return "NSC";
  if(i==8)  return "O/";
  if(i==11) return "S/";
  if(i==12) return "B/";
  if(i==13) return "F/";
  return "";
}

miString ObsBufr::cloudHeight(int i)
{
  ostringstream cl;
  i/=30;
  cl << setw(3) << setfill('0')<<i;
  return miString(cl.str());
}

miString ObsBufr::cloud_TCU_CB(int i)
{
  if(i==3) return " TCU";
  if(i==9) return " CB";
  return "";
}

float ObsBufr::height_of_clouds(double height)
{
  if(height > 2500) return 9.0;
  if(height > 2000) return 8.0;
  if(height > 1500) return 7.0;
  if(height > 1000) return 6.0;
  if(height >  600) return 5.0;
  if(height >  300) return 4.0;
  if(height >  200) return 3.0;
  if(height >  100) return 2.0;
  if(height >   50) return 1.0;
  return 9.0; // when there are no clouds -> height = 0
}

void ObsBufr::cloud_type(ObsData& d, double v)
{
  int type = int(v)/10;
  float value = float(int(v)%10);
  
  if(value < 1) return;

  if(type == 1) d.fdata["Ch"] = value;
  if(type == 2) d.fdata["Cm"] = value;
  if(type == 3) d.fdata["Cl"] = value;

}

float ObsBufr::ms2code4451(float v)
{
  if(v<   1852.0/3600.0) return 0.0;  
  if(v< 5*1852.0/3600.0) return 1.0;
  if(v<10*1852.0/3600.0) return 2.0;
  if(v<15*1852.0/3600.0) return 3.0;
  if(v<20*1852.0/3600.0) return 4.0;
  if(v<25*1852.0/3600.0) return 5.0;
  if(v<30*1852.0/3600.0) return 6.0;
  if(v<35*1852.0/3600.0) return 7.0;
  if(v<40*1852.0/3600.0) return 8.0;
  if(v<25*1852.0/3600.0) return 9.0;
  return 9.0;
}

#endif








