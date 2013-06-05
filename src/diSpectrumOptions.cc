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

#define MILOGGER_CATEGORY "diana.SpectrumOptions"
#include <miLogger/miLogging.h>

#include <diSpectrumOptions.h>
#include <iostream>

using namespace::miutil;

// default constructor
SpectrumOptions::SpectrumOptions()
{
  setDefaults();
}


// destructor
SpectrumOptions::~SpectrumOptions()
{
}


void SpectrumOptions::setDefaults()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("SpectrumOptions::setDefaults");
#endif

  pText= true;
  textColour= "black";

  pFixedText= true;
  fixedTextColour= "blue";

  pFrame= true;
  frameColour= "blue";
  frameLinewidth= 1.;

  pSpectrumLines= true;
  spectrumLineColour= "black";
  spectrumLinewidth= 1.;

  pSpectrumColoured= true;

  pEnergyLine= true;
  energyLineColour= "black";
  energyLinewidth= 3.;

  pEnergyColoured= true;
  energyFillColour= "red";

  pWind= true;
  windColour= "black";
  windLinewidth= 2.;

  pPeakDirection= true;
  peakDirectionColour= "red";
  peakDirectionLinewidth= 3.;

  freqMax= 0.3;

  backgroundColour= "white";

  changed= true;
}


vector<miString> SpectrumOptions::writeOptions()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("SpectrumOptions::writeOptions");
#endif

  vector<miString> vstr;
  miString str;

  str=  "text=" + miString(pText ? "on" : "off");
  str+= " textColour=" + textColour;
  vstr.push_back(str);

  str=  "fixedText=" + miString(pFixedText ? "on" : "off");
  str+= " fixedTextColour=" + fixedTextColour;
  vstr.push_back(str);

  str=  "frame=" + miString(pFrame ? "on" : "off");
  str+= " frameColour=" + frameColour;
  str+= " frameLinewidth=" + miString(frameLinewidth);
  vstr.push_back(str);

  str=  "spectrumLines=" + miString(pSpectrumLines ? "on" : "off");
  str+= " spectrumLineColour=" + spectrumLineColour;
  str+= " spectrumLinewidth=" + miString(spectrumLinewidth);
  vstr.push_back(str);

  str=  "spectrumColoured=" + miString(pSpectrumColoured ? "on" : "off");
  vstr.push_back(str);

  str=  "energyLine=" + miString(pEnergyLine ? "on" : "off");
  str+= " energyLineColour=" + energyLineColour;
  str+= " energyLinewidth=" + miString(energyLinewidth);
  vstr.push_back(str);

  str=  "energyColoured=" + miString(pEnergyColoured ? "on" : "off");
  str+= " energyFillColour=" + energyFillColour;
  vstr.push_back(str);

  str=  "wind=" + miString(pWind ? "on" : "off");
  str+= " windColour=" + windColour;
  str+= " windLinewidth=" + miString(windLinewidth);
  vstr.push_back(str);

  str=  "peakDirection=" + miString(pPeakDirection ? "on" : "off");
  str+= " peakDirectionColour=" + peakDirectionColour;
  str+= " peakDirectionLinewidth=" + miString(peakDirectionLinewidth);
  vstr.push_back(str);

  str= "freqMax=" + miString(freqMax);
  vstr.push_back(str);

  str= "backgroundColour=" + backgroundColour;
  vstr.push_back(str);

  return vstr;
}


void SpectrumOptions::readOptions(const vector<miString>& vstr)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("SpectrumOptions::readOptions");
#endif

  vector<miString> vs,tokens;
  miString key,value;

  int n= vstr.size();

  for (int i=0; i<n; i++) {

    vs= vstr[i].split(' ');

    int m= vs.size();

    for (int j=0; j<m; j++) {

      tokens= vs[j].split('=');

      if (tokens.size()==2) {

//	key=   tokens[0].downcase();
	key=   tokens[0];
	value= tokens[1];

	if      (key=="text")       pText= (value.downcase()=="on");
	else if (key=="textColour") textColour= value;

	else if (key=="fixedText")       pFixedText= (value.downcase()=="on");
	else if (key=="fixedTextColour") fixedTextColour= value;

	else if (key=="frame")          pFrame= (value.downcase()=="on");
	else if (key=="frameColour")    frameColour= value;
	else if (key=="frameLinewidth") frameLinewidth= atof(value.c_str());

	else if (key=="spectrumLines")      pSpectrumLines= (value.downcase()=="on");
	else if (key=="spectrumLineColour") spectrumLineColour= value;
	else if (key=="spectrumLinewidth")  spectrumLinewidth= atof(value.c_str());

	else if (key=="spectrumColoured") pSpectrumColoured= (value.downcase()=="on");

	else if (key=="energyLine")       pEnergyLine= (value.downcase()=="on");
	else if (key=="energyLineColour") energyLineColour= value;
	else if (key=="energyLinewidth")  energyLinewidth= atof(value.c_str());

	else if (key=="energyColoured")   pEnergyColoured= (value.downcase()=="on");
	else if (key=="energyFillColour") energyFillColour= value;

	else if (key=="wind")          pWind= (value.downcase()=="on");
	else if (key=="windColour")    windColour= value;
	else if (key=="windLinewidth") windLinewidth= atof(value.c_str());

	else if (key=="peakDirection")          pPeakDirection= (value.downcase()=="on");
	else if (key=="peakDirectionColour")    peakDirectionColour= value;
	else if (key=="peakDirectionLinewidth") peakDirectionLinewidth= atof(value.c_str());

	else if (key=="freqMax")        freqMax= atof(value.c_str());

	else if (key=="backgroundColour") backgroundColour= value;
//####################################################################
//      else METLIBS_LOG_DEBUG(">>>>>>>>ERROR KEY: "<<vs[j]);
//####################################################################

      }
//####################################################################
//    else METLIBS_LOG_DEBUG(">>>>>>>>ERROR LINE: "<<vs[j]);
//####################################################################
    }
  }
}
