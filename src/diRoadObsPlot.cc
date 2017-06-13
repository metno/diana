/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2016 met.no

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

#include "diRoadObsPlot.h"

#include "diObsPositions.h"
#include "diImageGallery.h"
#include "diGlUtilities.h"
#include "diLocalSetupParser.h"
#include "diUtilities.h"
#include "miSetupParser.h"
#include "util/qstring_util.h"

#include <puCtools/stat.h>
#include <puTools/miStringFunctions.h>

#include <QString>
#include <QTextCodec>

#include <fstream>
#include <iomanip>
#include <sstream>

#define MILOGGER_CATEGORY "diana.RoadObsPlot"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

#ifdef ROADOBS
static const float undef = -32767.0; //should be defined elsewhere
#endif

RoadObsPlot::RoadObsPlot(const miutil::KeyValue_v& pin, ObsPlotType plottype)
  : ObsPlot(pin, plottype)
{
}

long RoadObsPlot::findModificationTime(const std::string& fname)
{
  if (miutil::contains(fname, "ROAD")) {
    return time(0);
  }
  return ObsPlot::findModificationTime(fname);
}

bool RoadObsPlot::isFileUpdated(const std::string& fname, long now, long mod_time)
{
  if (miutil::contains(fname, "ROAD")) {
    return ((allObs && now - mod_time > 30) || (!allObs && now != mod_time));
  }
  return ObsPlot::isFileUpdated(fname, now, mod_time);
}

void RoadObsPlot::weather(DiGLPainter* gl, short int ww, float TTT, bool show_time_id,
    QPointF xypos, float scale, bool align_right)
{
  
#ifdef ROADOBS
  if (ww == 508)
    ww = 0;
  // Check if new BUFR code, not supported yet!
  if (ww > 199)
    return;
#endif
  METLIBS_LOG_DEBUG("WW: " << ww);
  return ObsPlot::weather(gl, ww, TTT, show_time_id, xypos, scale, align_right);
}

void RoadObsPlot::plotIndex(DiGLPainter* gl, int index)
{
  if (plottype() == OPT_ROADOBS) {
    plotRoadobs(gl, index);
  } else {
    ObsPlot::plotIndex(gl, index);
  }
}

#ifdef ROADOBS
using namespace road;

void RoadObsPlot::plotDBMetar(DiGLPainter* gl,int index)
{
  METLIBS_LOG_SCOPE("index: " << index);

  // NOTE: We must use the new data structures....
  ObsData &dta = obsp[index];

  std::string icao_value = "X";
  std::string station_type = dta.stringdata["data_type"];
  int automationcode = dta.fdata["auto"];
  bool isData = dta.fdata["isdata"];
  // Don't plot stations with no data
  if (!isData) return;

  float N_value = undef;
  float ww_value = undef;
  float GWI_value = undef;
  float TTT_value = undef;
  float TdTdTd_value = undef;
  float PHPHPHPH_value = undef;
  float ppp_value = undef;
  float a_value = undef;
  float Nh_value = undef;
  float h_value = undef;
  float Ch_value = undef;
  float Cm_value = undef;
  float Cl_value = undef;
  float W1_value = undef;
  float W2_value = undef;
  float TxTx_value = undef;
  float TnTn_value = undef;
  float sss_value = undef;
  float VV_value = undef;
  float dxdxdx_value = undef;
  float dndndn_value = undef;
  float fmfm_value = undef;
  float fxfx_value = undef;
  // Cloud layer 1-4 from automat stations
  float NS_A1_value = undef;
  float HS_A1_value = undef;
  float NS_A2_value = undef;
  float HS_A2_value = undef;
  float NS_A3_value = undef;
  float HS_A3_value = undef;
  float NS_A4_value = undef;
  float HS_A4_value = undef;

  // Cloud layer 1-4 from manual stations
  float NS1_value = undef;
  float HS1_value = undef;
  float NS2_value = undef;
  float HS2_value = undef;
  float NS3_value = undef;
  float HS3_value = undef;
  float NS4_value = undef;
  float HS4_value = undef;

  // Decode the string from database

  if (pFlag.count("name") && dta.stringdata.count("Name"))
    icao_value = dta.stringdata["Name"];
  if (pFlag.count("dxdxdx") && dta.stringdata.count("dxdxdx"))
    dxdxdx_value = atof(dta.stringdata["dxdxdx"].c_str());
  if (pFlag.count("dndndn") && dta.stringdata.count("dndndn"))
    dndndn_value = atof(dta.stringdata["dndndn"].c_str());
  if (pFlag.count("fmfmk") && dta.stringdata.count("fmfmk"))
    fmfm_value = atof(dta.stringdata["fmfmk"].c_str());
  if (pFlag.count("fxfx") && dta.stringdata.count("fxfx"))
    fxfx_value = atof(dta.stringdata["fxfx"].c_str());
  if (pFlag.count("sss") && dta.stringdata.count("sss"))
    sss_value = atof(dta.stringdata["sss"].c_str());
  if (pFlag.count("vv") && dta.stringdata.count("VV"))
    VV_value = atof(dta.stringdata["VV"].c_str());
  if (pFlag.count("n") && dta.stringdata.count("N"))
    N_value = atof(dta.stringdata["N"].c_str());
  if (pFlag.count("ww") && dta.stringdata.count("ww"))
    ww_value = atof(dta.stringdata["ww"].c_str());
  if (pFlag.count("gwi") && dta.stringdata.count("GWI"))
    GWI_value = atof(dta.stringdata["GWI"].c_str());
  if (pFlag.count("a") && dta.stringdata.count("a"))
    a_value = atof(dta.stringdata["a"].c_str());
  if (pFlag.count("ttt") && dta.stringdata.count("TTT"))
    TTT_value = atof(dta.stringdata["TTT"].c_str());
  if (pFlag.count("tdtdtd") && dta.stringdata.count("TdTdTd"))
    TdTdTd_value = atof(dta.stringdata["TdTdTd"].c_str());
  if (pFlag.count("phphphph") && dta.stringdata.count("PHPHPHPH"))
    PHPHPHPH_value = atof(dta.stringdata["PHPHPHPH"].c_str());
  if (pFlag.count("ppp") && dta.stringdata.count("ppp"))
    ppp_value = atof(dta.stringdata["ppp"].c_str());
  if (pFlag.count("nh") && dta.stringdata.count("Nh"))
    Nh_value = atof(dta.stringdata["Nh"].c_str());
  if (pFlag.count("h") && dta.stringdata.count("h"))
    h_value = atof(dta.stringdata["h"].c_str());
  if (pFlag.count("ch") && dta.stringdata.count("Ch"))
    Ch_value = atof(dta.stringdata["Ch"].c_str());
  if (pFlag.count("cm") && dta.stringdata.count("Cm"))
    Cm_value = atof(dta.stringdata["Cm"].c_str());
  if (pFlag.count("cl") && dta.stringdata.count("Cl"))
    Cl_value = atof(dta.stringdata["Cl"].c_str());
  if (pFlag.count("w1") && dta.stringdata.count("W1"))
    W1_value = atof(dta.stringdata["W1"].c_str());
  if (pFlag.count("w2") && dta.stringdata.count("W2"))
    W2_value = atof(dta.stringdata["W2"].c_str());
  // FIXME: Is the 24 and 12 hour values reported at the same time?
  if (pFlag.count("txtn") && dta.stringdata.count("TxTxTx"))
    TxTx_value = atof(dta.stringdata["TxTxTx"].c_str());
  // FIXME: Is the 24 and 12 hour values reported at the same time?
  if (pFlag.count("txtn") && dta.stringdata.count("TnTnTn"))
    TnTn_value = atof(dta.stringdata["TnTnTn"].c_str());
  // Cload layer 1-4 from automat stations
  if (pFlag.count("ns_a1") && dta.stringdata.count("NS_A1"))
    NS_A1_value = atof(dta.stringdata["NS_A1"].c_str());
  if (pFlag.count("hs_a1") && dta.stringdata.count("HS_A1"))
    HS_A1_value = atof(dta.stringdata["HS_A1"].c_str());
  if (pFlag.count("ns_a2") && dta.stringdata.count("NS_A2"))
    NS_A2_value = atof(dta.stringdata["NS_A2"].c_str());
  if (pFlag.count("hs_a2") && dta.stringdata.count("HS_A2"))
    HS_A2_value = atof(dta.stringdata["HS_A2"].c_str());
  if (pFlag.count("ns_a3") && dta.stringdata.count("NS_A3"))
    NS_A3_value = atof(dta.stringdata["NS_A3"].c_str());
  if (pFlag.count("hs_a3") && dta.stringdata.count("HS_A3"))
    HS_A3_value = atof(dta.stringdata["HS_A3"].c_str());
  if (pFlag.count("ns_a4") && dta.stringdata.count("NS_A4"))
    NS_A4_value = atof(dta.stringdata["NS_A4"].c_str());
  if (pFlag.count("hs_a4") && dta.stringdata.count("HS_A4"))
    HS_A4_value = atof(dta.stringdata["HS_A4"].c_str());
  // Cload layer 1-4 from manual stations
  if (pFlag.count("ns1") && dta.stringdata.count("NS1"))
    NS1_value = atof(dta.stringdata["NS1"].c_str());
  if (pFlag.count("hs1") && dta.stringdata.count("HS1"))
    HS1_value = atof(dta.stringdata["HS1"].c_str());
  if (pFlag.count("ns2") && dta.stringdata.count("NS2"))
    NS2_value = atof(dta.stringdata["NS2"].c_str());
  if (pFlag.count("hs2") && dta.stringdata.count("HS2"))
    HS2_value = atof(dta.stringdata["HS2"].c_str());
  if (pFlag.count("ns3") && dta.stringdata.count("NS3"))
    NS3_value = atof(dta.stringdata["NS3"].c_str());
  if (pFlag.count("hs3") && dta.stringdata.count("HS3"))
    HS3_value = atof(dta.stringdata["HS3"].c_str());
  if (pFlag.count("ns4") && dta.stringdata.count("NS4"))
    NS4_value = atof(dta.stringdata["NS4"].c_str());
  if (pFlag.count("hs4") && dta.stringdata.count("HS4"))
    HS4_value = atof(dta.stringdata["HS4"].c_str());

  DiGLPainter::GLfloat radius = 7.0;
  int lpos = vtab(1) + 10;
  /*const map<std::string, float>::iterator fend = dta.fdata.end();
   map<std::string, float>::iterator f2_p;
   map<std::string, float>::iterator f_p;*/

  //reset colour
  gl->setColour(origcolour);
  colour = origcolour;

  checkTotalColourCriteria(gl, index);

  diutil::GlMatrixPushPop pushpop(gl);
  gl->Translatef(x[index], y[index], 0.0);

  //Circle
  diutil::GlMatrixPushPop pushpop2(gl);
  gl->Scalef(scale, scale, 0.0);
  drawCircle(gl);
  pushpop2.PopMatrix();

  //wind
  if (pFlag.count("wind") && dta.fdata.count("dd") && dta.fdata.count("ff")) {
    checkColourCriteria(gl, "dd", dta.fdata["dd"]);
    checkColourCriteria(gl, "ff", dta.fdata["ff"]);
    metarWind(gl,(int) dta.fdata["dd_adjusted"], diutil::ms2knots(dta.fdata["ff"]), radius, lpos);
  }
  //limit of variable wind direction
  int dndx = 16;
  if (dndndn_value!= undef && dxdxdx_value != undef) {
    QString cs = QString("%1V%2")
        .arg(dndndn_value / 10)
        .arg(dxdxdx_value / 10);
    printString(gl, cs, xytab(lpos + 2, lpos + 3) + QPointF(2, 2));
    dndx = 2;
  }

  //Wind gust
  QPointF xyid = xytab(lpos + 4);
  if (fmfm_value != undef) {
    checkColourCriteria(gl, "fmfmk", fmfm_value);
    printNumber(gl, diutil::float2int(fmfm_value), xyid + QPointF(2, 2-dndx), "left", true);
    //understrekes
    xyid += QPointF(20 + 15, -dndx + 8);
  } else {
    xyid += QPointF(2 + 15, 2 - dndx + 8);
  }

  //Temperature
  if (TTT_value != undef) {
    checkColourCriteria(gl, "TTT", TTT_value);
    //    if( dta.TT>-99.5 && dta.TT<99.5 ) //right align_righted
    //printNumber(TTT_value, iptab[lpos + 12] + 23, iptab[lpos + 13] + 16, "temp");
    printNumber(gl,TTT_value, xytab(lpos+10,lpos+11)+QPointF(2,2),"temp");
  }

  //Dewpoint temperature
  if (TdTdTd_value != undef) {
    checkColourCriteria(gl, "TdTdTd", TdTdTd_value);
    //    if( dta.TdTd>-99.5 && dta.TdTd<99.5 )  //right align_righted and underlined
    printNumber(gl,TdTdTd_value, xytab(lpos+16,lpos+17)+QPointF(2,2),"temp");
  }

  QPointF VVxpos = xytab(lpos+14) + QPointF(22,0);
  //CAVOK, GWI
  if (GWI_value != undef) {
    checkColourCriteria(gl, "GWI", 0);

    if (GWI_value == 2) {
      printString(gl, "OK", xytab(lpos+12,lpos+13) + QPointF(-8,0));
    } else if (GWI_value == 1) { //Clouds
      printString(gl, "NSC", xytab(lpos+12,lpos+13) + QPointF(-8,0));
    } else if (GWI_value == 3) { //Clouds
      printString(gl, "SKC", xytab(lpos+12,lpos+13) + QPointF(-8,0));
    } else if (GWI_value == 1) { //Clouds
      printString(gl, "NSW", xytab(lpos+12,lpos+13) + QPointF(-8,0));
    }
    VVxpos = xytab(lpos+12) - QPointF(-28,0);

  }
  //int zone = 1;
  //if( ww_value != undef &&
  //    ww_value>3) {//1-3 skal ikke plottes
  //  checkColourCriteria(gl, "ww",ww_value);
  //  weather((short int)(int)ww_value,TTT_value,zone,
  //      iptab[lpos+12],iptab[lpos+13]);

  diutil::GlMatrixPushPop pushpop3(gl);
  gl->Scalef(scale, scale, 0.0);
  gl->Scalef(0.8, 0.8, 0.0);

  //Significant weather
  // Two string parameters ?!
  //int wwshift = 0; //idxm
  //if (ww_value != undef) {
  // checkColourCriteria(gl, "ww", 0);
  // metarSymbol(ww_value, iptab[lpos + 8], iptab[lpos + 9], wwshift);
  // //if (dta.ww.size() > 0 && dta.ww[0].exists()) {
  // // metarSymbol(dta.ww[0], iptab[lpos + 8], iptab[lpos + 9], wwshift);
  // //}
  // //if (dta.ww.size() > 1 && dta.ww[1].exists()) {
  // // metarSymbol(dta.ww[1], iptab[lpos + 10], iptab[lpos + 11], wwshift);
  // //}
  //}

  //Recent weather
  /*if (pFlag.count("reww")) {
   checkColourCriteria(gl, "REww", 0);
   if (dta.REww.size() > 0 && dta.REww[0].exists()) {
   int intREww[5];
   metarString2int(dta.REww[0], intREww);
   if (intREww[0] >= 0 && intREww[0] < 100) {
   symbol(itab[40 + intREww[0]], iptab[lpos + 30], iptab[lpos + 31] + 2);
   }
   }
   if (dta.REww.size() > 1 && dta.REww[1].exists()) {
   int intREww[5];
   metarString2int(dta.REww[1], intREww);
   if (intREww[0] >= 0 && intREww[0] < 100) {
   symbol(itab[40 + intREww[0]], iptab[lpos + 30] + 15, iptab[lpos + 31]
   + 2);
   }
   }
   }*/

  pushpop3.PopMatrix();
  bool ClFlag = false;

  if (NS1_value != undef || HS1_value != undef || NS2_value != undef || HS2_value != undef
      || NS3_value != undef || HS3_value != undef || NS4_value != undef || HS4_value != undef)
  {
    //convert to hfoot
    if (HS1_value != undef)
      HS1_value = (HS1_value*3.2808399)/100.0;
    if (HS2_value != undef)
      HS2_value = (HS2_value*3.2808399)/100.0;
    if (HS3_value != undef)
      HS3_value = (HS3_value*3.2808399)/100.0;
    if (HS4_value != undef)
      HS4_value = (HS4_value*3.2808399)/100.0;
    if( ClFlag ) {
      amountOfClouds_1_4(gl,
          (short int)(int)NS1_value, (short int)(int)HS1_value,
          (short int)(int)NS2_value, (short int)(int)HS2_value,
          (short int)(int)NS3_value, (short int)(int)HS3_value,
          (short int)(int)NS4_value, (short int)(int)HS4_value,
          xytab(lpos+24,lpos+25)+QPointF(2,2),true);
    } else {
      amountOfClouds_1_4(gl,
          (short int)(int)NS1_value, (short int)(int)HS1_value,
          (short int)(int)NS2_value, (short int)(int)HS2_value,
          (short int)(int)NS3_value, (short int)(int)HS3_value,
          (short int)(int)NS4_value, (short int)(int)HS4_value,
          xytab(lpos+24,lpos+25) + QPointF(2,12),true);
    }
  }
  else
  {
    // Clouds
    //METLIBS_LOG_DEBUG("Clouds: Nh = " << Nh_value << " h = " << h_value);
    if(Nh_value != undef || h_value != undef) {
      float Nh,h;
      Nh = Nh_value;

      /* NOTE, the height should be converted to hektfoot */
      if (h_value != undef)
      {
        h_value = (h_value*3.2808399)/100.0;
      }
      h = h_value;
      if(Nh!=undef) checkColourCriteria(gl, "Nh",Nh);
      if(h!=undef) checkColourCriteria(gl, "h",h);
      if( ClFlag ) {
        amountOfClouds_1(gl, (short int)(int)Nh, (short int)(int)h,xytab(lpos+24,lpos+25)+QPointF(2,2), true);
      } else {
        amountOfClouds_1(gl, (short int)(int)Nh, (short int)(int)h,xytab(lpos+24,lpos+25)+QPointF(2,12),true);
      }
    }
  }

  if( VV_value != undef ) {
    checkColourCriteria(gl, "VV",VV_value);
    // dont print in synop code, print in km #515, redmine
    QPointF VVxp(VVxpos.x(),xytab(lpos+15).x());
    if (VV_value < 5000.0)
      printNumber(gl,VV_value/1000.0,VVxp,"float_1");
    else
      printNumber(gl,VV_value/1000.0,VVxp,"fill_1");

  }

  //QNH ??
  // Sort, hPa ?
  if (PHPHPHPH_value != undef) {
    checkColourCriteria(gl, "PHPHPHPH", PHPHPHPH_value);
    int pp = (int) PHPHPHPH_value;
    pp -= (pp / 100) * 100;

    printNumber(gl,pp, xytab(lpos+44,lpos+45)+QPointF(2,2), "fill_2");
    printString(gl, "x",xytab(lpos+44,lpos+45) + QPointF(18,2));
  }

  //Id
  if (icao_value != "X") {
    checkColourCriteria(gl, "Name", 0);
    printString(gl, decodeText(icao_value), xytab(lpos+46,lpos+47)+QPointF(2,2));
  }
}

/*
 * We must replace ObsData with the correct data types and structures
 * in order to plot observations from road in synop format.
 * We can either use the ascci data representation or something else?
 * What about the parameter names ?
 *
 */

void RoadObsPlot::plotRoadobs(DiGLPainter* gl, int index)
{
  METLIBS_LOG_SCOPE("index: " << index);
  // Just to be safe...
  if (index > obsp.size() - 1) return;
  if (index < 0) return;
  ObsData & dta = obsp[index];
 
  // Does this work for ship ?!
  if (dta.stringdata["data_type"] == road::diStation::WMO || dta.stringdata["data_type"] == road::diStation::SHIP)
    plotDBSynop(gl,index);
  else if (dta.stringdata["data_type"] == road::diStation::ICAO)
    plotDBMetar(gl,index);
  // Unknown type of station...
  else return;
}

void RoadObsPlot::plotDBSynop(DiGLPainter* gl, int index)
{
  METLIBS_LOG_SCOPE("index: " << index);


  // NOTE: We must use the new data structures....

  ObsData &dta = obsp[index];

  std::string station_type = dta.stringdata["data_type"];
  std::string call_sign;
  int automationcode = dta.fdata["auto"];
  bool isData = dta.fdata["isdata"];
  // Do not plot stations with no data
  if (!isData) return;

  // loop all the parameters and then plot them
  // check for the TTT value etc
  float wmono_value = undef;

  float N_value = undef;
  float ww_value = undef;
  float TTT_value = undef;
  float TdTdTd_value = undef;
  float PPPP_value = undef;
  float ppp_value = undef;
  float a_value = undef;
  float Nh_value = undef;
  float h_value = undef;
  float Ch_value = undef;
  float Cm_value = undef;
  float Cl_value = undef;
  float W1_value = undef;
  float W2_value = undef;
  // Direction and speed of ship
  float DS_value = undef;
  float VS_value = undef;
  float TxTx_value = undef;
  float TnTn_value = undef;
  float rrr_24_value = undef;
  float rrr_12_value = undef;
  float rrr_6_value = undef;
  float rrr_3_value = undef;
  float rrr_1_value = undef;
  float sss_value = undef;
  float VV_value = undef;
  float f911ff_value = undef;
  float fxfx_value = undef;
  // Cloud layer 1-4 from automat stations
  float NS_A1_value = undef;
  float HS_A1_value = undef;
  float NS_A2_value = undef;
  float HS_A2_value = undef;
  float NS_A3_value = undef;
  float HS_A3_value = undef;
  float NS_A4_value = undef;
  float HS_A4_value = undef;

  // Cloud layer 1-4 from manual stations
  float NS1_value = undef;
  float HS1_value = undef;
  float NS2_value = undef;
  float HS2_value = undef;
  float NS3_value = undef;
  float HS3_value = undef;
  float NS4_value = undef;
  float HS4_value = undef;

  // Decode the string from database

  if (pFlag.count("name") && dta.stringdata.count("Name")) {
    std::string str = dta.stringdata["Name"];
    if (station_type == road::diStation::WMO)
      wmono_value = atof(str.c_str());
    else if (station_type == road::diStation::SHIP)
      call_sign = str;
  }
  if (pFlag.count("911ff") && dta.stringdata.count("911ff"))
    f911ff_value = atof(dta.stringdata["911ff"].c_str());
  if (pFlag.count("fxfx") && dta.stringdata.count("fxfx"))
    fxfx_value = atof(dta.stringdata["fxfx"].c_str());
  if (pFlag.count("sss") && dta.stringdata.count("sss"))
    sss_value = atof(dta.stringdata["sss"].c_str());
  if (pFlag.count("vv") && dta.stringdata.count("VV"))
    VV_value = atof(dta.stringdata["VV"].c_str());
  if (pFlag.count("n") && dta.stringdata.count("N"))
    N_value = atof(dta.stringdata["N"].c_str());
  if (pFlag.count("ww") && dta.stringdata.count("ww"))
    ww_value = atof(dta.stringdata["ww"].c_str());
  if (pFlag.count("vv") && dta.stringdata.count("VV"))
    VV_value = atof(dta.stringdata["VV"].c_str());
  if (pFlag.count("a") && dta.stringdata.count("a"))
    a_value = atof(dta.stringdata["a"].c_str());
  if (pFlag.count("ttt") && dta.stringdata.count("TTT"))
    TTT_value = atof(dta.stringdata["TTT"].c_str());
  if (pFlag.count("tdtdtd") && dta.stringdata.count("TdTdTd"))
    TdTdTd_value = atof(dta.stringdata["TdTdTd"].c_str());
  if (pFlag.count("pppp") && dta.stringdata.count("PPPP"))
    PPPP_value = atof(dta.stringdata["PPPP"].c_str());
  if (pFlag.count("ppp") && dta.stringdata.count("ppp"))
    ppp_value = atof(dta.stringdata["ppp"].c_str());
  if (pFlag.count("nh") && dta.stringdata.count("Nh"))
    Nh_value = atof(dta.stringdata["Nh"].c_str());
  if (pFlag.count("h") && dta.stringdata.count("h"))
    h_value = atof(dta.stringdata["h"].c_str());
  if (pFlag.count("ch") && dta.stringdata.count("Ch"))
    Ch_value = atof(dta.stringdata["Ch"].c_str());
  if (pFlag.count("cm") && dta.stringdata.count("Cm"))
    Cm_value = atof(dta.stringdata["Cm"].c_str());
  if (pFlag.count("cl") && dta.stringdata.count("Cl"))
    Cl_value = atof(dta.stringdata["Cl"].c_str());
  if (pFlag.count("w1") && dta.stringdata.count("W1"))
    W1_value = atof(dta.stringdata["W1"].c_str());
  if (pFlag.count("w2") && dta.stringdata.count("W2"))
    W2_value = atof(dta.stringdata["W2"].c_str());
  if (pFlag.count("vs") && dta.stringdata.count("vs"))
    VS_value = atof(dta.stringdata["vs"].c_str());
  if (pFlag.count("ds") && dta.stringdata.count("ds"))
    DS_value = atof(dta.stringdata["ds"].c_str());
  // FIXME: Is the 24 and 12 hour values reported at the same time?
  if (pFlag.count("txtn") && dta.stringdata.count("TxTxTx"))
    TxTx_value = atof(dta.stringdata["TxTxTx"].c_str());
  // FIXME: Is the 24 and 12 hour values reported at the same time?
  if (pFlag.count("txtn") && dta.stringdata.count("TnTnTn"))
    TnTn_value = atof(dta.stringdata["TnTnTn"].c_str());
  // Cload layer 1-4 from automat stations
  if (pFlag.count("ns_a1") && dta.stringdata.count("NS_A1"))
    NS_A1_value = atof(dta.stringdata["NS_A1"].c_str());
  if (pFlag.count("hs_a1") && dta.stringdata.count("HS_A1"))
    HS_A1_value = atof(dta.stringdata["HS_A1"].c_str());
  if (pFlag.count("ns_a2") && dta.stringdata.count("NS_A2"))
    NS_A2_value = atof(dta.stringdata["NS_A2"].c_str());
  if (pFlag.count("hs_a2") && dta.stringdata.count("HS_A2"))
    HS_A2_value = atof(dta.stringdata["HS_A2"].c_str());
  if (pFlag.count("ns_a3") && dta.stringdata.count("NS_A3"))
    NS_A3_value = atof(dta.stringdata["NS_A3"].c_str());
  if (pFlag.count("hs_a3") && dta.stringdata.count("HS_A3"))
    HS_A3_value = atof(dta.stringdata["HS_A3"].c_str());
  if (pFlag.count("ns_a4") && dta.stringdata.count("NS_A4"))
    NS_A4_value = atof(dta.stringdata["NS_A4"].c_str());
  if (pFlag.count("hs_a4") && dta.stringdata.count("HS_A4"))
    HS_A4_value = atof(dta.stringdata["HS_A4"].c_str());
  // Cload layer 1-4 from manual stations
  if (pFlag.count("ns1") && dta.stringdata.count("NS1"))
    NS1_value = atof(dta.stringdata["NS1"].c_str());
  if (pFlag.count("hs1") && dta.stringdata.count("HS1"))
    HS1_value = atof(dta.stringdata["HS1"].c_str());
  if (pFlag.count("ns2") && dta.stringdata.count("NS2"))
    NS2_value = atof(dta.stringdata["NS2"].c_str());
  if (pFlag.count("hs2") && dta.stringdata.count("HS2"))
    HS2_value = atof(dta.stringdata["HS2"].c_str());
  if (pFlag.count("ns3") && dta.stringdata.count("NS3"))
    NS3_value = atof(dta.stringdata["NS3"].c_str());
  if (pFlag.count("hs3") && dta.stringdata.count("HS3"))
    HS3_value = atof(dta.stringdata["HS3"].c_str());
  if (pFlag.count("ns4") && dta.stringdata.count("NS4"))
    NS4_value = atof(dta.stringdata["NS4"].c_str());
  if (pFlag.count("hs4") && dta.stringdata.count("HS4"))
    HS4_value = atof(dta.stringdata["HS4"].c_str());

  // another special case, the RRR
  if (pFlag.count("rrr_24") && dta.stringdata.count("RRR_24"))
    rrr_24_value = atof(dta.stringdata["RRR_24"].c_str());
  if (pFlag.count("rrr_12") && dta.stringdata.count("RRR_12"))
    rrr_12_value = atof(dta.stringdata["RRR_12"].c_str());
  if (pFlag.count("rrr_6") && dta.stringdata.count("RRR_6"))
    rrr_6_value = atof(dta.stringdata["RRR_6"].c_str());
  if (pFlag.count("rrr_3") && dta.stringdata.count("RRR_3"))
    rrr_3_value = atof(dta.stringdata["RRR_3"].c_str());
  if (pFlag.count("rrr_1") && dta.stringdata.count("RRR_1"))
    rrr_1_value = atof(dta.stringdata["RRR_1"].c_str());

  DiGLPainter::GLfloat radius=7.0, x1,x2,x3,y1,y2,y3;
  int lpos;

  //reset colour
  gl->setColour(origcolour);
  colour = origcolour;

  checkTotalColourCriteria(gl, index);

  diutil::GlMatrixPushPop pushpop(gl);
  gl->Translatef(x[index],y[index],0.0);

  diutil::GlMatrixPushPop pushpop2(gl);
  gl->Scalef(scale,scale,0.0);
  // No circle if auto obs
  if (automationcode != 0)
    if (N_value != undef)
      drawCircle(gl);

  // manned / automated station - ix
  // This is not a special parameter in road, it is returned
  // in every row from RDK.
  // it should be set on a station level ?
  /*
   if( (dta.fdata.count("ix") && dta.fdata["ix"] > 3)
   || ( dta.fdata.count("auto") && dta.fdata["auto"] == 0)){
   */
  /* 0 = automat, 1 = manuell, 2 = hybrid */
  DiGLPainter::GLfloat tmp_radius = 0.6 * radius;
  if(automationcode == 0) {
    if (N_value != undef)
    {
      y1 = y2 = -1.1*tmp_radius;
      x1 = y1*sqrtf(3.0);
      x2 = -1*x1;
      x3 = 0;
      y3 = tmp_radius*2.2;
      gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_LINE);
      gl->Begin(DiGLPainter::gl_POLYGON);
      gl->Vertex2f(x1,y1);
      gl->Vertex2f(x2,y2);
      gl->Vertex2f(x3,y3);
      gl->End();
    }
  }

  //wind - dd,ff
  if (pFlag.count("wind") && dta.fdata.count("dd") && dta.fdata.count("ff")
      && dta.fdata.count("dd_adjusted") && dta.fdata["dd"] != undef) {
    bool ddvar = false;
    int dd = (int) dta.fdata["dd"];
    int dd_adjusted = (int) dta.fdata["dd_adjusted"];
    if (dd == 990 || dd == 510) {
      ddvar = true;
      dd_adjusted = 270;
    }
    if (diutil::ms2knots(dta.fdata["ff"]) < 1.)
      dd = 0;
    lpos = vtab((dd / 10 + 3) / 2) + 10;
    checkColourCriteria(gl, "dd", dd);
    checkColourCriteria(gl, "ff", dta.fdata["ff"]);
    plotWind(gl,dd_adjusted, dta.fdata["ff"], ddvar, radius);
  } else
    lpos = vtab(1) + 10;

  int zone = 0;
  // No snow depth from ship.
  if (station_type == road::diStation::SHIP)
    zone = 99;
  else if(station_type == road::diStation::WMO)
    zone = wmono_value/1000;

  /*
   bool ClFlag = (pFlag.count("cl") && dta.fdata.count("Cl") ||
   (pFlag.count("st.type") && dta.dataType.exists()));
   bool TxTnFlag = (pFlag.count("txtn") && dta.fdata.find("TxTn")!=fend);
   bool timeFlag = (pFlag.count("time") && dta.zone==99);
   bool precip   = (dta.fdata.count("ix") && dta.fdata["ix"] == -1);
   */
  bool TxTnFlag = ((TxTx_value != undef)||(TnTn_value != undef));
  bool ClFlag = Cl_value != undef;
  bool precip = automationcode; // Not correct!

  //Total cloud cover - N
  METLIBS_LOG_DEBUG("Total cloud cover - N: value " << N_value);
  if(N_value != undef) {
    checkColourCriteria(gl, "N",N_value);
    if (automationcode != 0)
      cloudCover(gl, N_value,radius);
    else
      cloudCoverAuto(gl, N_value,radius);
  } /*else if( !precip ) {
   gl->setColour(colour);
   cloudCover(gl, undef,radius);
   }*/

  //Weather - WW
  METLIBS_LOG_DEBUG("Weather - WW: value " << ww_value);
  QPointF VVxpos = xytab(lpos+14) + QPointF(22,0);
  if( ww_value != undef &&
      ww_value>3) {  //1-3 skal ikke plottes
    checkColourCriteria(gl, "ww",ww_value);
    weather(gl,(short int)(int)ww_value,TTT_value,zone,
        xytab(lpos+12,lpos+13));
    VVxpos = xytab(lpos+12) - QPointF(18,0);
  }

  //characteristics of pressure tendency - a
  METLIBS_LOG_DEBUG("characteristics of pressure tendency - a: value " << a_value);
  if( a_value != undef ) {
    checkColourCriteria(gl, "a",a_value);
    if(ppp_value != undef && ppp_value> 9 )
      symbol(gl,vtab(201+(int)a_value), xytab(lpos+42, lpos+43) + QPointF(10,0),0.8);
    else
      symbol(gl,vtab(201+(int)a_value), xytab(lpos+42, lpos+43),0.8);
  }

  // High cloud type - Ch
  METLIBS_LOG_DEBUG("High cloud type - Ch, value: " << Ch_value);
  if(Ch_value != undef)
  {
    checkColourCriteria(gl, "Ch",Ch_value);
    symbol(gl,vtab(190+(int)Ch_value), xytab(lpos+4, lpos+5),0.8);
  }

  // Middle cloud type - Cm
  METLIBS_LOG_DEBUG("Middle cloud type - Cm, value: " << Cm_value);
  if(Cm_value != undef)
  {
      checkColourCriteria(gl, "Cm",Cm_value);
      symbol(gl,vtab(180+(int)Cm_value), xytab(lpos+2, lpos+3),0.8);
  }

  // Low cloud type - Cl
  METLIBS_LOG_DEBUG("Low cloud type - Cl, value: " << Cl_value);
  if(Cl_value != undef)
  {
      checkColourCriteria(gl, "Cl",Cl_value);
      symbol(gl,vtab(170+(int)Cl_value), xytab(lpos+22, lpos+23),0.8);
  }

  // Past weather - W1
  METLIBS_LOG_DEBUG("Past weather - W1: value " << W1_value);
  if( W1_value != undef) {
    checkColourCriteria(gl, "W1",W1_value);
    pastWeather(gl,int(W1_value), xytab(lpos+34, lpos+35),0.8);
  }

  // Past weather - W2
  METLIBS_LOG_DEBUG("Past weather - W2: value " << W2_value);
  if( W2_value != undef) {
    checkColourCriteria(gl, "W2",W2_value);
    pastWeather(gl,(int)W2_value, xytab(lpos+36, lpos+37),0.8);
  }
  // Direction and speed of ship movement - ds/vs
  if (DS_value != undef && VS_value != undef)
  {
    checkColourCriteria(gl, "ds",DS_value);
    checkColourCriteria(gl, "vs", VS_value);
    printNumber(gl, VS_value, xytab(lpos + 36, lpos + 39));
    arrow(gl,DS_value, xytab(lpos+32, lpos+33));
  }
  /* Currently not used
   // Direction of swell waves - dw1dw1
   if(  pFlag.count("dw1dw1")
   && (f_p=dta.fdata.find("dw1dw1")) != fend ){
   checkColourCriteria(gl, "dw1dw1",f_p->second);
   zigzagArrow(f_p->second, iptab[lpos+30], iptab[lpos+31]);
   }
   */
  // Change of coordinate system
  pushpop2.PopMatrix();

  METLIBS_LOG_DEBUG("Pressure - PPPP: value " << PPPP_value);
  if( PPPP_value != undef ) {
    checkColourCriteria(gl, "PPPP",PPPP_value);
    printNumber(gl,PPPP_value,xytab(lpos+44,lpos+45)+QPointF(2,2),"PPPP");
  }

  // Pressure tendency over 3 hours - ppp
  METLIBS_LOG_DEBUG("Pressure tendency over 3 hours - ppp: value " << ppp_value);
  if( ppp_value != undef ) {
    checkColourCriteria(gl, "ppp",ppp_value);
    printNumber(gl,ppp_value,xytab(lpos+40,lpos+41)+QPointF(2,2),"ppp");
  }

  if(automationcode == 0) {
    if (NS_A1_value != undef || HS_A1_value != undef || NS_A2_value != undef || HS_A2_value != undef
        || NS_A3_value != undef || HS_A3_value != undef || NS_A4_value != undef || HS_A4_value != undef)
    {
      //convert to hfoot
      if (HS_A1_value != undef)
        HS_A1_value = (HS_A1_value*3.2808399)/100.0;
      if (HS_A2_value != undef)
        HS_A2_value = (HS_A2_value*3.2808399)/100.0;
      if (HS_A3_value != undef)
        HS_A3_value = (HS_A3_value*3.2808399)/100.0;
      if (HS_A4_value != undef)
        HS_A4_value = (HS_A4_value*3.2808399)/100.0;
      if( ClFlag ) {
        amountOfClouds_1_4(gl,
            (short int)(int)NS_A1_value, (short int)(int)HS_A1_value,
            (short int)(int)NS_A2_value, (short int)(int)HS_A2_value,
            (short int)(int)NS_A3_value, (short int)(int)HS_A3_value,
            (short int)(int)NS_A4_value, (short int)(int)HS_A4_value,
            xytab(lpos+24,lpos+25)+QPointF(2,2));
      } else {
        amountOfClouds_1_4(gl,
            (short int)(int)NS_A1_value, (short int)(int)HS_A1_value,
            (short int)(int)NS_A2_value, (short int)(int)HS_A2_value,
            (short int)(int)NS_A3_value, (short int)(int)HS_A3_value,
            (short int)(int)NS_A4_value, (short int)(int)HS_A4_value,
            xytab(lpos+24,lpos+25)+QPointF(2,12));
      }
    }
    else
    {
      // Clouds
      METLIBS_LOG_DEBUG("Clouds: Nh = " << Nh_value << " h = " << h_value);
      if(Nh_value != undef || h_value != undef) {
        float Nh,h;
        Nh = Nh_value;

        /* NOTE, the height is already converted to hektfoot */
        h = h_value;
        if(Nh!=undef) checkColourCriteria(gl, "Nh",Nh);
        if(h!=undef) checkColourCriteria(gl, "h",h);
        if( ClFlag ) {
          amountOfClouds_1(gl, (short int)(int)Nh, (short int)(int)h,xytab(lpos+24,lpos+25)+QPointF(2,2));
        } else {
          amountOfClouds_1(gl, (short int)(int)Nh, (short int)(int)h,xytab(lpos+24,lpos+25)+QPointF(2,12));
        }
      }
    }
  }
  else
  {
    if (NS1_value != undef || HS1_value != undef || NS2_value != undef || HS2_value != undef
        || NS3_value != undef || HS3_value != undef || NS4_value != undef || HS4_value != undef)
    {
      //convert to hfoot
      if (HS1_value != undef)
        HS1_value = (HS1_value*3.2808399)/100.0;
      if (HS2_value != undef)
        HS2_value = (HS2_value*3.2808399)/100.0;
      if (HS3_value != undef)
        HS3_value = (HS3_value*3.2808399)/100.0;
      if (HS4_value != undef)
        HS4_value = (HS4_value*3.2808399)/100.0;
      if( ClFlag ) {
        amountOfClouds_1_4(gl,
            (short int)(int)NS1_value, (short int)(int)HS1_value,
            (short int)(int)NS2_value, (short int)(int)HS2_value,
            (short int)(int)NS3_value, (short int)(int)HS3_value,
            (short int)(int)NS4_value, (short int)(int)HS4_value,
            xytab(lpos+24,lpos+25)+QPointF(2,2));
      } else {
        amountOfClouds_1_4(gl,
            (short int)(int)NS1_value, (short int)(int)HS1_value,
            (short int)(int)NS2_value, (short int)(int)HS2_value,
            (short int)(int)NS3_value, (short int)(int)HS3_value,
            (short int)(int)NS4_value, (short int)(int)HS4_value,
            xytab(lpos+24,lpos+25)+QPointF(2,12));
      }
    }
    else
    {
      // Clouds
      METLIBS_LOG_DEBUG("Clouds: Nh = " << Nh_value << " h = " << h_value);
      if(Nh_value != undef || h_value != undef) {
        float Nh,h;
        Nh = Nh_value;

        /* NOTE, the height is already converted to hektofoot */
        h = h_value;
        if(Nh!=undef) checkColourCriteria(gl, "Nh",Nh);
        if(h!=undef) checkColourCriteria(gl, "h",h);
        if( ClFlag ) {
          amountOfClouds_1(gl, (short int)(int)Nh, (short int)(int)h,xytab(lpos+24,lpos+25)+QPointF(2,2));
        } else {
          amountOfClouds_1(gl, (short int)(int)Nh, (short int)(int)h,xytab(lpos+24,lpos+25)+QPointF(2,12));
        }
      }
    }
  }

  //Precipitation - RRR, select 1,3,6,12,24 hour accumulation time.
  METLIBS_LOG_DEBUG("Precipitation - RRR, select 1,3,6,12,24 hour accumulation time.");
  float rrr_plot_value = undef;
  if( rrr_24_value != undef)
    rrr_plot_value = rrr_24_value;
  else if( rrr_12_value != undef)
    rrr_plot_value = rrr_12_value;
  else if( rrr_6_value != undef)
    rrr_plot_value = rrr_6_value;
  else if( rrr_3_value != undef)
    rrr_plot_value = rrr_3_value;
  else if( rrr_1_value != undef)
    rrr_plot_value = rrr_1_value;

  METLIBS_LOG_DEBUG("Value to plot: value " << rrr_plot_value);
  if (rrr_plot_value != undef)
  {
    checkColourCriteria(gl, "RRR",rrr_plot_value);
    if( rrr_plot_value < 0.1) //No precipitation (0.)
      printString(gl, "0.",xytab(lpos+32,lpos+33)+QPointF(2,2));
    else if( rrr_plot_value> 989)//Precipitation, but less than 0.1 mm (0.0)
      printString(gl, "0.0",xytab(lpos+32,lpos+33)+QPointF(2,2));
    else
      printNumber(gl,rrr_plot_value,xytab(lpos+32,lpos+33)+QPointF(2,2),"RRR");
  }

  // Horizontal visibility - VV
  METLIBS_LOG_DEBUG("Horizontal visibility - VV: value " << VV_value);
  if( VV_value != undef ) {
    checkColourCriteria(gl, "VV",VV_value);
    // dont print in synop code, print in km #515, redmine
    //printNumber(visibility(VV_value,zone == 99),VVxpos,iptab[lpos+15],"fill_2");
    QPointF VVxp(VVxpos.x(),xytab(lpos+15).x());
    if (VV_value < 5000.0)
      printNumber(gl,VV_value/1000.0,VVxp,"float_1");
    else
      printNumber(gl,VV_value/1000.0,VVxp,"fill_1");

  }
  // Temperature - TTT
  //METLIBS_LOG_DEBUG("Temperature - TTT: value " << TTT_value);
  if( TTT_value != undef ) {
    checkColourCriteria(gl, "TTT",TTT_value);
    printNumber(gl,TTT_value,xytab(lpos+10,lpos+11)+QPointF(2,2),"temp");
  }
  // Dewpoint temperature - TdTdTd
  //METLIBS_LOG_DEBUG("Dewpoint temperature - TdTdTd: value " << TdTdTd_value);
  if( TdTdTd_value != undef ) {
    checkColourCriteria(gl, "TdTdTd",TdTdTd_value);
    printNumber(gl,TdTdTd_value,xytab(lpos+16,lpos+17)+QPointF(2,2),"temp");
  }

  // Max/min temperature - TxTxTx/TnTnTn
  METLIBS_LOG_DEBUG("Max/min temperature - TxTxTx/TnTnTn");
  if( TxTnFlag ) {
    // The days maximum should be plotted at 18Z
    // The nights minimum should be plotted at 06Z
    float TxTn_value = undef;
    if (Time.hour() == 6)
    {
      TxTn_value = TnTn_value;
    }
    else if (Time.hour() == 18)
    {
      TxTn_value = TxTx_value;
    }
    METLIBS_LOG_DEBUG("TxTn: " << TxTn_value);
    if (TxTn_value != undef)
    {
      checkColourCriteria(gl, "TxTn",TxTn_value);
      printNumber(gl,TxTn_value,xytab(lpos+8,lpos+9)+QPointF(2,2),"temp");
    }
  }

  // Snow depth - sss
  METLIBS_LOG_DEBUG("Snow depth - sss: value " << sss_value);
  if( sss_value != undef && zone!=99 ) {
    checkColourCriteria(gl, "sss",sss_value);
    printNumber(gl,sss_value,xytab(lpos+46,lpos+47)+QPointF(2,2));
  }

  // Maximum wind speed (gusts) - 911ff
  METLIBS_LOG_DEBUG("Maximum wind speed (gusts) - 911ff: value " << f911ff_value);
  if( f911ff_value != undef ) {
    checkColourCriteria(gl, "911ff",f911ff_value);
    printNumber(gl,diutil::ms2knots(f911ff_value),
        xytab(lpos+38,lpos+39)+QPointF(2,2),"fill_2",true);
  }

  /* Not currently used
   // State of the sea - s
   if( pFlag.count("s") && (f_p=dta.fdata.find("s")) != fend ){
   checkColourCriteria(gl, "s",f_p->second);
   if(TxTnFlag)
   printNumber(f_p->second,iptab[lpos+6]+2,iptab[lpos+7]+2);
   else
   printNumber(f_p->second,iptab[lpos+6]+2,iptab[lpos+7]-14);
   }

   */

  // Maximum wind speed
  METLIBS_LOG_DEBUG("Maximum wind speed: value " << fxfx_value);
  if( fxfx_value != undef)
  {
    checkColourCriteria(gl, "fxfx",fxfx_value);
    if(TxTnFlag)
      printNumber(gl,diutil::ms2knots(fxfx_value),
          xytab(lpos+6,lpos+7)+QPointF(12,2),"fill_2",true);
    else
      printNumber(gl,diutil::ms2knots(fxfx_value),
          xytab(lpos+6,lpos+7)+QPointF(12,-14),"fill_2",true);
  }
  // WMO station id
  
  if (wmono_value != undef || !call_sign.empty())
  {
    checkColourCriteria(gl, "Name",0);
    int wmo = (int)wmono_value;
    QString buf;
    if (station_type == road::diStation::WMO)
      buf.setNum(wmo);
    else if (station_type == road::diStation::SHIP)
      buf = decodeText(call_sign);
    METLIBS_LOG_DEBUG("WMO station id or callsign: " << buf.toStdString());
    if( sss_value != undef) //if snow
      printString(gl, buf,xytab(lpos+46,lpos+47)+QPointF(2,15));
    else
      printString(gl, buf,xytab(lpos+46,lpos+47)+QPointF(2,2));
  }
}

// we must keep this for performance reasons

/* this metod compute wich stations to plot
 and fills a vector with index to the internal stationlist that should get data
 from road */
bool RoadObsPlot::preparePlot()
{
  METLIBS_LOG_SCOPE();

  if (!isEnabled()) {
    // make sure plot-densities etc are recalc. next time
    if (getStaticPlot()->getDirty())
      beendisabled= true;
    return false;
  }

  int numObs = numPositions();
  if (not numObs)
    return false;

  scale= textSize*getStaticPlot()->getPhysToMapScaleX()*0.7;

  int num = calcNum();

  float xdist,ydist;
  // I think we should plot roadobs like synop here
  // OBS!******************************************
  if (isSynopMetarRoad()) {
    xdist = 100*scale/density;
    ydist = 90*scale/density;
  } else if (plottype() == OPT_LIST || plottype() == OPT_ASCII) {
    if (num>0) {
      if(vertical_orientation) {
        xdist = 58*scale/density;
        ydist = 18*(num+0.2)*scale/density;
      } else {
        xdist = 50*num*scale/density;
        ydist = 10*scale/density;
      }
    } else {
      xdist = 14*scale/density;
      ydist = 14*scale/density;
    }
  }

  //**********************************************************************
  //Which stations to plot

  bool testpos = true; // positionFree or areaFree must be tested
  vector<int> ptmp;
  vector<int>::iterator p, pbegin, pend;

  if (getStaticPlot()->getDirty() || firstplot || beendisabled) { //new area

    //init of areaFreeSetup
    // I think we should plot roadobs like synop here
    // OBS!******************************************

    thisObs = false;

    // new area, find stations inside current area
    all_this_area.clear();
    int nn = all_stations.size();
    for (int j = 0; j < nn; j++) {
      int i = all_stations[j];
      if (getStaticPlot()->getMapSize().isinside(x[i], y[i])) {
        all_this_area.push_back(i);
      }
    }

    //    METLIBS_LOG_DEBUG("all this area:"<<all_this_area.size());
    // plot the observations from last plot if possible,
    // then the rest if possible

    if (!firstplot) {
      vector<int> a, b;
      int n = list_plotnr.size();
      if (n == numObs) {
        int psize = all_this_area.size();
        for (int j = 0; j < psize; j++) {
          int i = all_this_area[j];
          if (list_plotnr[i] == plotnr)
            a.push_back(i);
          else
            b.push_back(i);
        }
        if (a.size() > 0) {
          all_this_area.clear();
          all_this_area.insert(all_this_area.end(), a.begin(), a.end());
          all_this_area.insert(all_this_area.end(), b.begin(), b.end());
        }
      }
    }

    //reset
    list_plotnr.clear();
    list_plotnr.insert(list_plotnr.begin(), numObs, -1);
    maxnr = plotnr = 0;

    pbegin = all_this_area.begin();
    pend = all_this_area.end();

  } else if (thisObs) {
    //    METLIBS_LOG_DEBUG("thisobs");
    // plot the station pointed at and those plotted last time if possible,
    // then the rest if possible.
    ptmp = nextplot;
    ptmp.insert(ptmp.end(), notplot.begin(), notplot.end());
    pbegin = ptmp.begin();
    pend = ptmp.end();

  } else if (plotnr > maxnr) {
    //    METLIBS_LOG_DEBUG("plotnr:"<<plotnr);
    // plot as many observations as possible which have not been plotted before
    maxnr++;
    plotnr = maxnr;

    int psize = all_this_area.size();
    for (int j = 0; j < psize; j++) {
      int i = all_this_area[j];
      if (list_plotnr[i] == -1)
        ptmp.push_back(i);
    }
    pbegin = ptmp.begin();
    pend = ptmp.end();

  } else if (previous && plotnr < 0) {
    //    METLIBS_LOG_DEBUG("plotnr:"<<plotnr);
    // should return to the initial priority as often as possible...

    if (!fromFile) { //if priority from last plot has been used so far
      all_this_area.clear();
      for (int j = 0; j < numObs; j++) {
        int i = all_from_file[j];
        if (getStaticPlot()->getMapSize().isinside(x[i], y[i]))
          all_this_area.push_back(i);
      }
      all_stations = all_from_file;
      fromFile = true;
    }
    //clear
    plotnr = maxnr = 0;
    list_plotnr.clear();
    list_plotnr.insert(list_plotnr.begin(), numObs, -1);

    pbegin = all_this_area.begin();
    pend = all_this_area.end();

  } else if (previous || next) {
    //    METLIBS_LOG_DEBUG("plotnr:"<<plotnr);
    //    plot observations from plotnr
    int psize = all_this_area.size();
    notplot.clear();
    nextplot.clear();
    for (int j = 0; j < psize; j++) {
      int i = all_this_area[j];
      if (list_plotnr[i] == plotnr) {
        nextplot.push_back(i);
      } else if (list_plotnr[i] > plotnr || list_plotnr[i] == -1) {
        notplot.push_back(i);
      }
    }
    testpos = false; //no need to test positionFree or areaFree

  } else {
    //nothing has changed
    testpos = false; //no need to test positionFree or areaFree
  }
  //######################################################
  //  int ubsize1= usedBox.size();
  //######################################################

  if (testpos) { //test of positionFree or areaFree
    notplot.clear();
    nextplot.clear();
    // I think we should plot roadobs like synop here
    // OBS!******************************************
    if (plottype() == OPT_LIST || plottype() == OPT_ASCII) {
      for (p = pbegin; p != pend; p++) {
        int i = *p;
        if (allObs || areaFree(i)) {
          //Select parameter with correct accumulation/max value interval
          if (pFlag.count("911ff")) {
            checkGustTime(obsp[i]);
          }
          if (pFlag.count("rrr")) {
            checkAccumulationTime(obsp[i]);
          }
          if (pFlag.count("fxfx")) {
            checkMaxWindTime(obsp[i]);
          }
          if (checkPlotCriteria(i)) {
            nextplot.push_back(i);
            list_plotnr[i] = plotnr;
          } else {
            list_plotnr[i] = -2;
            collider_->areaPop();
          }
        } else {
          notplot.push_back(i);
        }
      }
    } else {
      for (p = pbegin; p != pend; p++) {
        int i = *p;
        if (allObs || collider_->positionFree(x[i], y[i], xdist, ydist)) {
          //Select parameter with correct accumulation/max value interval
          if (plottype() != OPT_ROADOBS) {
            if (pFlag.count("911ff")) {
              checkGustTime(obsp[i]);
            }
            if (pFlag.count("rrr")) {
              checkAccumulationTime(obsp[i]);
            }
            if (pFlag.count("fxfx")) {
              checkMaxWindTime(obsp[i]);
            }
          }
          if (checkPlotCriteria(i)) {
            nextplot.push_back(i);
            list_plotnr[i] = plotnr;
          } else {
            list_plotnr[i] = -2;
            if (!allObs)
              collider_->positionPop();
          }
        } else {
          notplot.push_back(i);
        }
      }
    }
    if (thisObs) {
      int n = notplot.size();
      for (int i = 0; i < n; i++)
        if (list_plotnr[notplot[i]] == plotnr)
          list_plotnr[notplot[i]] = -1;
    }
  }


  // BEE CAREFULL! This code assumes that the number of entries in
  // stationlist are the same as in the roadobsp map.
  // reset stations_to_plot
  stations_to_plot.clear();
  // use nextplot info to fill the stations_to_plot.
  METLIBS_LOG_DEBUG("nextplot.size() " << nextplot.size());
  for (size_t i=0; i<nextplot.size(); i++) {
    stations_to_plot.push_back(nextplot[i]);
  }
  //reset

  next = false;
  previous = false;
  thisObs = false;
  if (nextplot.empty())
    plotnr=-1;
  //firstplot = false;
  beendisabled = false;

  //clearPos();
  //clear();

  return true;
}
#else // !ROADOBS
void RoadObsPlot::plotDBMetar(DiGLPainter* gl,int index)
{
}
void RoadObsPlot::plotRoadobs(DiGLPainter* gl, int index)
{
}
void RoadObsPlot::plotDBSynop(DiGLPainter* gl, int index)
{
}
bool RoadObsPlot::preparePlot()
{
  return false;
}
#endif // !ROADOBS
