/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "diVprofOptions.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.VprofOptions"
#include <miLogger/miLogging.h>

VprofOptions::VprofOptions()
{
  setDefaults();
}

VprofOptions::~VprofOptions()
{
}

void VprofOptions::setDefaults()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  ptttt = true;     // t
  ptdtd = true;     // td
  pwind = true;     // wind
  pvwind = false;   // vertical wind
  prelhum = false;  // relative humidty
  pducting = false; // ducting
  pkindex = true;   // Kindex
  pslwind = false;  // numbers at significant wind levels

  dataColour.clear();
  dataColour.push_back("blue");
  dataColour.push_back("red");
  dataColour.push_back("darkGreen");
  dataColour.push_back("black");
  dataColour.push_back("magenta");
  dataColour.push_back("darkGray");
  dataColour.push_back("darkRed");
  dataColour.push_back("darkCyan");
  dataColour.push_back("orange");
  dataColour.push_back("darkMagenta");

  dataLinewidth.clear();
  windLinewidth.clear();
  for (unsigned int i = 0; i < dataColour.size(); i++) {
    dataLinewidth.push_back(3.0);
    windLinewidth.push_back(1.0);
  }

  backgroundColour = "white";
  diagramtype = 0; // 0=Amble  1=Exner(pi)  2=Pressure  3=ln(P)
  tangle = 45.;    // angle between the vertical and temperatur lines,
  // 0. - 80. degrees, 45 degrees on Amble diagram
  pminDiagram = 100;  // min pressure
  pmaxDiagram = 1050; // max pressure
  tminDiagram = -30;  // min temperature (C) at 1000 hPa
  tmaxDiagram = +30;  // max temperature (C) at 1000 hPa

  pplines = true;    // pressure lines
  pplinesfl = false; // false=at pressure levels  true=at flight levels
  pColour = "black";
  pLinetype = "solid";
  pLinewidth1 = 1.; // thin lines
  pLinewidth2 = 2;

  for (int p = 1050; p > 0; p -= 50)
    plevels.push_back(p);

  // flight levels, unit 100 feet
  // Warning: pressure at flight levels found by looking at an Amble diagram!
  const int mflvl = 45;
  int iflvl[mflvl] = {0,   10,  20,  30,  40,  50,  60,  70,  80,  90,  100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 210, 220,
                      230, 240, 250, 260, 270, 280, 290, 300, 310, 320, 330, 340, 350, 360, 370, 380, 390, 400, 450, 500, 550, 600};

  for (int i = 0; i < mflvl; i++)
    flightlevels.push_back(iflvl[i]);

  ptlines = true; // temperature lines
  tStep = 5;      // temp. step
  tColour = "black";
  tLinetype = "solid";
  tLinewidth1 = 1.; // thin lines
  tLinewidth2 = 2;

  pdryadiabat = true;  // dry adiabats
  dryadiabatStep = 10; // temperature step (C at 1000hPa)
  dryadiabatColour = "black";
  dryadiabatLinetype = "solid";
  dryadiabatLinewidth = 1.;

  pwetadiabat = true; // dry adiabats
  wetadiabatStep = 5; // temperature step (C at 1000hPa)
  wetadiabatColour = "darkRed";
  wetadiabatLinetype = "solid";
  wetadiabatLinewidth = 1.;
  wetadiabatPmin = 300;
  wetadiabatTmin = -50;

  pmixingratio = true; // mixing ratio
  mixingratioSet = 1;  // line set no. (0,1,2,3 available)
  mixingratioColour = "magenta";
  mixingratioLinetype = "longdash";
  mixingratioLinewidth = 1.;
  mixingratioPmin = 300;
  mixingratioTmin = -50;

  // mixing ratio line sets  (q in unit g/kg)
  const int nq0 = 9, nq1 = 11, nq2 = 15, nq3 = 28;
  float q0[nq0] = {.1, .2, .4, 1., 5., 10., 20., 30., 50.};
  float q1[nq1] = {.1, .2, .4, 1., 2., 5., 10., 20., 30., 40., 50.};
  float q2[nq2] = {.1, .2, .4, .6, 1., 2., 3., 5., 7., 10., 14., 20., 30., 40., 50.};
  float q3[nq3] = {.1, .2, .4, .6, .8, 1., 1.5, 2., 2.5, 3., 4., 5., 6., 7., 8., 9., 10., 12., 14., 16., 18., 20., 25., 30., 35., 40., 45., 50.};
  qtable.clear();
  std::vector<float> q;
  q.clear();
  for (int i = 0; i < nq0; i++)
    q.push_back(q0[i]);
  qtable.push_back(q);
  q.clear();
  for (int i = 0; i < nq1; i++)
    q.push_back(q1[i]);
  qtable.push_back(q);
  q.clear();
  for (int i = 0; i < nq2; i++)
    q.push_back(q2[i]);
  qtable.push_back(q);
  q.clear();
  for (int i = 0; i < nq3; i++)
    q.push_back(q3[i]);
  qtable.push_back(q);

  plabelp = true; // p labels (numbers)
  plabelt = true; // t labels (numbers)
  plabelq = true; // mixing ratio labels (numbers)
  pframe = true;  // frame
  ptext = true;   // text (station name etc.)
  frameColour = "black";
  frameLinetype = "solid";
  frameLinewidth = 2.;
  textColour = "black";

  pflevels = true; // flight levels (numbers/marks on axis only)
  flevelsColour = "black";
  flevelsLinetype = "solid";
  flevelsLinewidth1 = 1.;
  plabelflevels = true; // labels (numbers)

  rsvaxis = 1.;        // relative size vertical axis
  rstext = 1.;         // relative size text
  rslabels = 1.;       // relative size labels (p and t numbers)
  rswind = 1.;         // relative size wind   (width of column)
  rsvwind = 1.;        // relative size vertical wind (width of column)
  rsrelhum = 1.;       // relative size relative humidity (width of column)
  rsducting = 1.;      // relative size ducting (width of column)
  windseparate = true; // separate wind columns when multiple data
  rvwind = 0.01;       // range of vertical wind in unit hPa/s (-range to +range)
  ductingMin = -200.;  // min ducting in diagram
  ductingMax = +200.;  // max ducting in diagram

  rangeLinetype = "solid"; // for vertical wind, rel.hum. and ducting
  rangeLinewidth = 1.;

  pcotrails = true; // condensation trail lines
  // (linjer for vurdering av mulighet for
  //  kondensstriper fra fly)
  cotrailsColour = "cyan";
  cotrailsLinetype = "solid";
  cotrailsLinewidth = 3.;
  cotrailsPmin = 100;
  cotrailsPmax = 700;

  pgeotext = true; // geographic position in text
}

void VprofOptions::checkValues()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofOptions::checkValues");
#endif

  if (diagramtype < 0 || diagramtype > 3)
    diagramtype = 0;
  if (tangle < 1.)
    tangle = 0.;
  if (tangle > 80.)
    tangle = 80.;

  if (tmaxDiagram - tminDiagram < 5)
    tminDiagram = tmaxDiagram - 5;

  if (rsvaxis < 0.01)
    rsvaxis = 0.01;
  if (rsvaxis > 100.)
    rsvaxis = 100.;
  if (rstext < 0.01)
    rstext = 0.01;
  if (rstext > 100.)
    rstext = 100.;
  if (rslabels < 0.01)
    rslabels = 0.01;
  if (rslabels > 100.)
    rslabels = 100.;
  if (rswind < 0.01)
    rswind = 0.01;
  if (rswind > 100.)
    rswind = 100.;
  if (rsvwind < 0.01)
    rsvwind = 0.01;
  if (rsvwind > 100.)
    rsvwind = 100.;
  if (rsrelhum < 0.01)
    rsrelhum = 0.01;
  if (rsrelhum > 100.)
    rsrelhum = 100.;
  if (rsducting < 0.01)
    rsducting = 0.01;
  if (rsducting > 100.)
    rsducting = 100.;

  const int dp = 10, np = 261;
  if (pmaxDiagram - pminDiagram < dp * 2)
    pminDiagram = pmaxDiagram - dp * 2;
  if (pminDiagram < 0)
    pminDiagram = 0;
  if (pmaxDiagram - pminDiagram < dp * 2)
    pmaxDiagram = pminDiagram + dp * 2;
  if (pmaxDiagram > dp * (np - 1))
    pmaxDiagram = dp * (np - 1);
}

std::vector<std::string> VprofOptions::writeOptions()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofOptions::writeOptions");
#endif

  std::vector<std::string> vstr;
  std::string str;

  str = "tttt=" + std::string(ptttt ? "on" : "off");
  vstr.push_back(str);
  str = "tdtd=" + std::string(ptdtd ? "on" : "off");
  vstr.push_back(str);
  str = "wind=" + std::string(pwind ? "on" : "off");
  vstr.push_back(str);
  str = "vwind=" + std::string(pvwind ? "on" : "off");
  vstr.push_back(str);
  str = "relhum=" + std::string(prelhum ? "on" : "off");
  vstr.push_back(str);
  str = "ducting=" + std::string(pducting ? "on" : "off");
  vstr.push_back(str);
  str = "kindex=" + std::string(pkindex ? "on" : "off");
  vstr.push_back(str);
  str = "slwind=" + std::string(pslwind ? "on" : "off");
  vstr.push_back(str);

  if (dataColour.size()) {
    str = "dataColour=";
    for (unsigned int i = 0; i < dataColour.size(); i++) {
      if (i > 0)
        str += ",";
      str += dataColour[i];
    }
    vstr.push_back(str);
  }
  if (dataLinewidth.size()) {
    str = "dataLinewidth=";
    for (unsigned int i = 1; i < dataLinewidth.size(); i++) {
      if (i > 0)
        str += ",";
      str += miutil::from_number(dataLinewidth[i]);
    }
    vstr.push_back(str);
  }
  if (windLinewidth.size()) {
    str = "windLinewidth=";
    for (unsigned int i = 1; i < windLinewidth.size(); i++) {
      if (i > 0)
        str += ",";
      str += miutil::from_number(windLinewidth[i]);
    }
    vstr.push_back(str);
  }

  str = "windseparate=" + std::string(windseparate ? "on" : "off");
  str += " text=" + std::string(ptext ? "on" : "off");
  str += " geotext=" + std::string(pgeotext ? "on" : "off");
  vstr.push_back(str);

  str = "rvwind=" + miutil::from_number(rvwind);
  str += (" ductingMin=" + miutil::from_number(ductingMin));
  str += (" ductingMax=" + miutil::from_number(ductingMax));
  vstr.push_back(str);

  str = "backgroundColour=" + backgroundColour;
  vstr.push_back(str);

  str = "diagramtype=" + miutil::from_number(diagramtype);
  str += (" tangle=" + miutil::from_number(tangle));
  vstr.push_back(str);

  str = "pminDiagram=" + miutil::from_number(pminDiagram) + " pmaxDiagram=" + miutil::from_number(pmaxDiagram);
  vstr.push_back(str);
  str = "tminDiagram=" + miutil::from_number(tminDiagram) + " tmaxDiagram=" + miutil::from_number(tmaxDiagram);
  vstr.push_back(str);

  str = "plinesfl=" + std::string(pplinesfl ? "on" : "off");
  vstr.push_back(str);

  str = "plines=" + std::string(pplines ? "on" : "off");
  str += (" pColour=" + pColour);
  str += (" pLinetype=" + pLinetype);
  str += (" pLinewidth1=" + miutil::from_number(pLinewidth1));
  str += (" pLinewidth2=" + miutil::from_number(pLinewidth2));
  vstr.push_back(str);

  str = "tlines=" + std::string(ptlines ? "on" : "off");
  str += (" tStep=" + miutil::from_number(tStep));
  str += (" tColour=" + tColour);
  str += (" tLinetype=" + tLinetype);
  str += (" tLinewidth1=" + miutil::from_number(tLinewidth1));
  str += (" tLinewidth2=" + miutil::from_number(tLinewidth2));
  vstr.push_back(str);

  str = "dryadiabat=" + std::string(pdryadiabat ? "on" : "off");
  str += (" dryadiabatStep=" + miutil::from_number(dryadiabatStep));
  str += (" dryadiabatColour=" + dryadiabatColour);
  str += (" dryadiabatLinetype=" + dryadiabatLinetype);
  str += (" dryadiabatLinewidth=" + miutil::from_number(dryadiabatLinewidth));
  vstr.push_back(str);

  str = "wetadiabat=" + std::string(pwetadiabat ? "on" : "off");
  str += (" wetadiabatStep=" + miutil::from_number(wetadiabatStep));
  str += (" wetadiabatColour=" + wetadiabatColour);
  str += (" wetadiabatLinetype=" + wetadiabatLinetype);
  str += (" wetadiabatLinewidth=" + miutil::from_number(wetadiabatLinewidth));
  str += (" wetadiabatPmin=" + miutil::from_number(wetadiabatPmin));
  str += (" wetadiabatTmin=" + miutil::from_number(wetadiabatTmin));
  vstr.push_back(str);

  str = "mixingratio=" + std::string(pmixingratio ? "on" : "off");
  str += (" mixingratioSet=" + miutil::from_number(mixingratioSet));
  str += (" mixingratioColour=" + mixingratioColour);
  str += (" mixingratioLinetype=" + mixingratioLinetype);
  str += (" mixingratioLinewidth=" + miutil::from_number(mixingratioLinewidth));
  str += (" mixingratioPmin=" + miutil::from_number(mixingratioPmin));
  str += (" mixingratioTmin=" + miutil::from_number(mixingratioTmin));
  vstr.push_back(str);

  str = "labelp=" + std::string(plabelp ? "on" : "off");
  str += " labelt=" + std::string(plabelt ? "on" : "off");
  str += " labelq=" + std::string(plabelq ? "on" : "off");
  vstr.push_back(str);

  str = "frame=" + std::string(pframe ? "on" : "off");
  str += (" frameColour=" + frameColour);
  str += (" frameLinetype=" + frameLinetype);
  str += (" frameLinewidth=" + miutil::from_number(frameLinewidth));
  vstr.push_back(str);

  str = ("textColour=" + textColour);
  vstr.push_back(str);

  str = "flevels=" + std::string(pflevels ? "on" : "off");
  str += (" flevelsColour=" + flevelsColour);
  str += (" flevelsLinetype=" + flevelsLinetype);
  str += (" flevelsLinewidth1=" + miutil::from_number(flevelsLinewidth1));
  vstr.push_back(str);

  str = "labelflevels=" + std::string(plabelflevels ? "on" : "off");
  vstr.push_back(str);

  str = "rsvaxis=" + miutil::from_number(rsvaxis);
  str += " rstext=" + miutil::from_number(rstext);
  str += " rslabels=" + miutil::from_number(rslabels);
  str += " rswind=" + miutil::from_number(rswind);
  str += " rsvwind=" + miutil::from_number(rsvwind);
  str += " rsrelhum=" + miutil::from_number(rsrelhum);
  str += " rsducting=" + miutil::from_number(rsducting);
  vstr.push_back(str);

  str = "rangeLinetype=" + rangeLinetype;
  str += (" rangeLinewidth=" + miutil::from_number(rangeLinewidth));
  vstr.push_back(str);

  str = "cotrails=" + std::string(pcotrails ? "on" : "off");
  str += (" cotrailsColour=" + cotrailsColour);
  str += (" cotrailsLinetype=" + cotrailsLinetype);
  str += (" cotrailsLinewidth=" + miutil::from_number(cotrailsLinewidth));
  str += (" cotrailsPmin=" + miutil::from_number(cotrailsPmin));
  str += (" cotrailsPmax=" + miutil::from_number(cotrailsPmax));
  vstr.push_back(str);

  return vstr;
}

void VprofOptions::readOptions(const miutil::KeyValue_v& kvs)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  for (const miutil::KeyValue& kv : kvs) {
    if (kv.hasValue()) {
      const std::string& key = kv.key();
      const std::string& value = kv.value();

      if (key == "tttt")
        ptttt = kv.toBool();
      else if (key == "tdtd")
        ptdtd = kv.toBool();
      else if (key == "wind")
        pwind = kv.toBool();
      else if (key == "vwind")
        pvwind = kv.toBool();
      else if (key == "relhum")
        prelhum = kv.toBool();
      else if (key == "ducting")
        pducting = kv.toBool();
      else if (key == "kindex")
        pkindex = kv.toBool();
      else if (key == "slwind")
        pslwind = kv.toBool();

      else if (key == "windseparate")
        windseparate = kv.toBool();
      else if (key == "text")
        ptext = kv.toBool();
      else if (key == "geotext")
        pgeotext = kv.toBool();

      else if (key == "rvwind")
        rvwind = kv.toFloat();
      else if (key == "ductingmin")
        ductingMin = kv.toFloat();
      else if (key == "ductingmax")
        ductingMax = kv.toFloat();

      else if (key == "backgroundcolour")
        backgroundColour = value;

      else if (key == "diagramtype")
        diagramtype = kv.toInt();
      else if (key == "tangle")
        tangle = kv.toFloat();

      else if (key == "pmindiagram")
        pminDiagram = kv.toInt();
      else if (key == "pmaxdiagram")
        pmaxDiagram = kv.toInt();
      else if (key == "tmindiagram")
        tminDiagram = kv.toInt();
      else if (key == "tmaxdiagram")
        tmaxDiagram = kv.toInt();

      else if (key == "plinesfl")
        pplinesfl = kv.toBool();

      else if (key == "plines")
        pplines = kv.toBool();
      else if (key == "pcolour")
        pColour = value;
      else if (key == "plinetype")
        pLinetype = value;
      else if (key == "plinewidth1")
        pLinewidth1 = kv.toInt();
      else if (key == "plinewidth2")
        pLinewidth2 = kv.toInt();

      else if (key == "tlines")
        ptlines = kv.toBool();
      else if (key == "tstep")
        tStep = kv.toInt();
      else if (key == "tcolour")
        tColour = value;
      else if (key == "tlinetype")
        tLinetype = value;

      else if (key == "dryadiabat")
        pdryadiabat = kv.toBool();
      else if (key == "dryadiabatstep")
        dryadiabatStep = kv.toInt();
      else if (key == "dryadiabatcolour")
        dryadiabatColour = value;
      else if (key == "dryadiabatlinetype")
        dryadiabatLinetype = value;
      else if (key == "dryadiabatlinewidth")
        dryadiabatLinewidth = kv.toFloat();

      else if (key == "wetadiabat")
        pwetadiabat = kv.toBool();
      else if (key == "wetadiabatstep")
        wetadiabatStep = kv.toInt();
      else if (key == "wetadiabatcolour")
        wetadiabatColour = value;
      else if (key == "wetadiabatlinetype")
        wetadiabatLinetype = value;
      else if (key == "wetadiabatlinewidth")
        wetadiabatLinewidth = kv.toFloat();
      else if (key == "wetadiabatpmin")
        wetadiabatPmin = kv.toInt();
      else if (key == "wetadiabattmin")
        wetadiabatTmin = kv.toInt();

      else if (key == "mixingratio")
        pmixingratio = kv.toBool();
      else if (key == "mixingratioset")
        mixingratioSet = kv.toInt();
      else if (key == "mixingratiocolour")
        mixingratioColour = value;
      else if (key == "mixingratiolinetype")
        mixingratioLinetype = value;
      else if (key == "mixingratiolinewidth")
        mixingratioLinewidth = kv.toFloat();
      else if (key == "mixingratiopmin")
        mixingratioPmin = kv.toInt();
      else if (key == "mixingratiotmin")
        mixingratioTmin = kv.toInt();

      else if (key == "labelp")
        plabelp = kv.toBool();
      else if (key == "labelt")
        plabelt = kv.toBool();
      else if (key == "labelq")
        plabelq = kv.toBool();

      else if (key == "frame")
        pframe = kv.toBool();
      else if (key == "framecolour")
        frameColour = value;
      else if (key == "framelinetype")
        frameLinetype = value;
      else if (key == "framelinewidth")
        frameLinewidth = kv.toFloat();

      else if (key == "textcolour")
        textColour = value;

      else if (key == "flevels")
        pflevels = kv.toBool();
      else if (key == "flevelscolour")
        flevelsColour = value;
      else if (key == "flevelslinetype")
        flevelsLinetype = value;
      else if (key == "flevelslinewidth1")
        flevelsLinewidth1 = kv.toFloat();

      else if (key == "labelflevels")
        plabelflevels = kv.toBool();

      else if (key == "rsvaxis")
        rsvaxis = kv.toFloat();
      else if (key == "rstext")
        rstext = kv.toFloat();
      else if (key == "rslabels")
        rslabels = kv.toFloat();
      else if (key == "rswind")
        rswind = kv.toFloat();
      else if (key == "rsvwind")
        rsvwind = kv.toFloat();
      else if (key == "rsrelhum")
        rsrelhum = kv.toFloat();
      else if (key == "rsducting")
        rsducting = kv.toFloat();

      else if (key == "rangelinetype")
        rangeLinetype = value;
      else if (key == "rangelinewidth")
        rangeLinewidth = kv.toFloat();

      else if (key == "cotrails")
        pcotrails = kv.toBool();
      else if (key == "cotrailscolour")
        cotrailsColour = value;
      else if (key == "cotrailslinetype")
        cotrailsLinetype = value;
      else if (key == "cotrailslinewidth")
        cotrailsLinewidth = kv.toFloat();
      else if (key == "cotrailspmin")
        cotrailsPmin = kv.toInt();
      else if (key == "cotrailspmax")
        cotrailsPmax = kv.toInt();

      if (key == "datacolour" || key == "datalinewidth" || key == "windlinewidth") {
        const std::vector<std::string> vs = miutil::split(value, 0, ",");
        const int nv = vs.size();
        if (nv > 0) {
          if (key == "datacolour") {
            if (nv > int(dataColour.size()))
              dataColour.resize(nv);
            for (int k = 0; k < nv; k++)
              dataColour[k] = vs[k];
          } else if (key == "datalinewidth") {
            if (nv > int(dataLinewidth.size()))
              dataLinewidth.resize(nv);
            for (int k = 0; k < nv; k++)
              dataLinewidth[k] = miutil::to_float(vs[k]);
          } else if (key == "windlinewidth") {
            if (nv > int(windLinewidth.size()))
              windLinewidth.resize(nv);
            for (int k = 0; k < nv; k++)
              windLinewidth[k] = miutil::to_float(vs[k]);
          }
        }
      }
    }
  }
}
