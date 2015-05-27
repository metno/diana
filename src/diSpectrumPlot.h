/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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
#ifndef diSpectrumPlot_h
#define diSpectrumPlot_h

#include "diSpectrumOptions.h"

#include <puTools/miTime.h>

#include <vector>

class DiGLPainter;

/**
   \brief Plot wave spectrum

   Data preparation and plotting.
   Uses the standard (field) Contouring.
   An object holds data for one wave spectrum.
*/
class SpectrumPlot
{
  friend class SpectrumFile;
  friend class SpectrumData;

public:
  SpectrumPlot();
  ~SpectrumPlot();

  static void startPlot(int numplots, int w, int h,
      SpectrumOptions *spopt, DiGLPainter* gl);
  static void plotDiagram(SpectrumOptions *spopt, DiGLPainter* gl);

  bool plot(SpectrumOptions *spopt, DiGLPainter* gl);

private:
  static int plotw;
  static int ploth;
  static int numplot;
  static int nplot;

  static bool firstPlot;

  static float freqMax;
  static float specRadius;
  static float dytext1;
  static float dytext2;
  static float xplot1;
  static float xplot2;
  static float yplot1;
  static float yplot2;
  static float rwind;
  static float xwind;
  static float ywind;
  static float xefdiag;
  static float yefdiag;
  static float dxefdiag;
  static float dyefdiag;
  static float vxefdiag;
  static float vyefdiag;
  static int   numyefdiag;
  static int   incyefdiag;
  static float xhmo;
  static float yhmo;
  static float dxhmo;
  static float dyhmo;
  static float vyhmo;
  static float fontSize1;
  static float fontSize2;
  static float fontSizeLabel;

  static float eTotalMax;

  bool prognostic;

  std::string modelName;
  std::string posName;
  miutil::miTime   validTime;
  int      forecastHour;

  int  numDirec;
  int  numFreq;

  std::vector<float> directions;
  std::vector<float> frequences;
  std::vector<float> eTotal;

  float northRotation;
  float wspeed;
  float wdir;
  float hmo;
  float tPeak;
  float ddPeak;
  float specMax;

  float *sdata;
  float *xdata;
  float *ydata;
};

#endif
