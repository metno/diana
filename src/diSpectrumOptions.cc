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

#include <diSpectrumOptions.h>

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.SpectrumOptions"
#include <miLogger/miLogging.h>

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

  METLIBS_LOG_DEBUG("SpectrumOptions::setDefaults");


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


std::vector<std::string> SpectrumOptions::writeOptions()
{

  METLIBS_LOG_SCOPE();


  std::vector<std::string> vstr;
  std::string str;

  str=  "text=" + std::string(pText ? "on" : "off");
  str+= " textColour=" + textColour;
  vstr.push_back(str);

  str=  "fixedText=" + std::string(pFixedText ? "on" : "off");
  str+= " fixedTextColour=" + fixedTextColour;
  vstr.push_back(str);

  str=  "frame=" + std::string(pFrame ? "on" : "off");
  str+= " frameColour=" + frameColour;
  str+= " frameLinewidth=" + miutil::from_number(frameLinewidth);
  vstr.push_back(str);

  str=  "spectrumLines=" + std::string(pSpectrumLines ? "on" : "off");
  str+= " spectrumLineColour=" + spectrumLineColour;
  str+= " spectrumLinewidth=" + miutil::from_number(spectrumLinewidth);
  vstr.push_back(str);

  str=  "spectrumColoured=" + std::string(pSpectrumColoured ? "on" : "off");
  vstr.push_back(str);

  str=  "energyLine=" + std::string(pEnergyLine ? "on" : "off");
  str+= " energyLineColour=" + energyLineColour;
  str+= " energyLinewidth=" + miutil::from_number(energyLinewidth);
  vstr.push_back(str);

  str=  "energyColoured=" + std::string(pEnergyColoured ? "on" : "off");
  str+= " energyFillColour=" + energyFillColour;
  vstr.push_back(str);

  str=  "wind=" + std::string(pWind ? "on" : "off");
  str+= " windColour=" + windColour;
  str+= " windLinewidth=" + miutil::from_number(windLinewidth);
  vstr.push_back(str);

  str=  "peakDirection=" + std::string(pPeakDirection ? "on" : "off");
  str+= " peakDirectionColour=" + peakDirectionColour;
  str+= " peakDirectionLinewidth=" + miutil::from_number(peakDirectionLinewidth);
  vstr.push_back(str);

  str= "freqMax=" + miutil::from_number(freqMax);
  vstr.push_back(str);

  str= "backgroundColour=" + backgroundColour;
  vstr.push_back(str);

  return vstr;
}


void SpectrumOptions::readOptions(const std::vector<std::string>& vstr)
{

  METLIBS_LOG_SCOPE();


  std::vector<std::string> vs,tokens;
  std::string key,value;

  int n= vstr.size();

  for (int i=0; i<n; i++) {

    vs= miutil::split(vstr[i], 0, " ");

    int m= vs.size();

    for (int j=0; j<m; j++) {

      tokens= miutil::split(vs[j], 0, "=");

      if (tokens.size()==2) {

//	key=   miutil::to_lower(tokens[0]);
	key=   tokens[0];
	value= tokens[1];

	if      (key=="text")       pText= (miutil::to_lower(value)=="on");
	else if (key=="textColour") textColour= value;

	else if (key=="fixedText")       pFixedText= (miutil::to_lower(value)=="on");
	else if (key=="fixedTextColour") fixedTextColour= value;

	else if (key=="frame")          pFrame= (miutil::to_lower(value)=="on");
	else if (key=="frameColour")    frameColour= value;
	else if (key=="frameLinewidth") frameLinewidth= atof(value.c_str());

	else if (key=="spectrumLines")      pSpectrumLines= (miutil::to_lower(value)=="on");
	else if (key=="spectrumLineColour") spectrumLineColour= value;
	else if (key=="spectrumLinewidth")  spectrumLinewidth= atof(value.c_str());

	else if (key=="spectrumColoured") pSpectrumColoured= (miutil::to_lower(value)=="on");

	else if (key=="energyLine")       pEnergyLine= (miutil::to_lower(value)=="on");
	else if (key=="energyLineColour") energyLineColour= value;
	else if (key=="energyLinewidth")  energyLinewidth= atof(value.c_str());

	else if (key=="energyColoured")   pEnergyColoured= (miutil::to_lower(value)=="on");
	else if (key=="energyFillColour") energyFillColour= value;

	else if (key=="wind")          pWind= (miutil::to_lower(value)=="on");
	else if (key=="windColour")    windColour= value;
	else if (key=="windLinewidth") windLinewidth= atof(value.c_str());

	else if (key=="peakDirection")          pPeakDirection= (miutil::to_lower(value)=="on");
	else if (key=="peakDirectionColour")    peakDirectionColour= value;
	else if (key=="peakDirectionLinewidth") peakDirectionLinewidth= atof(value.c_str());

	else if (key=="freqMax") {      freqMax= atof(value.c_str()); if (freqMax <= 1e-5) freqMax = 0.3; }

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
