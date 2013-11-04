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
#ifndef SPECTRUMOPTIONS_H
#define SPECTRUMOPTIONS_H

#include <string>
#include <vector>

/**
  \brief Wave Spectrum diagram settings

   Contains diagram layout settings and defaults.
*/
class SpectrumOptions
{
public:
  SpectrumOptions();
  ~SpectrumOptions();
  void setDefaults();

  // log and setup
  std::vector<std::string> writeOptions();
  void readOptions(const std::vector<std::string>& vstr);

private:
  friend class SpectrumSetupDialog;
  friend class SpectrumPlot;
  friend class SpectrumManager;

  bool changed;

  bool     pText;
  std::string textColour;

  bool     pFixedText;
  std::string fixedTextColour;

  bool     pFrame;
  std::string frameColour;
  float    frameLinewidth;

  bool     pSpectrumLines;
  std::string spectrumLineColour;
  float    spectrumLinewidth;

  bool     pSpectrumColoured;

  bool     pEnergyLine;
  std::string energyLineColour;
  float    energyLinewidth;

  bool     pEnergyColoured;
  std::string energyFillColour;

  bool     pWind;
  std::string windColour;
  float    windLinewidth;

  bool     pPeakDirection;
  std::string peakDirectionColour;
  float    peakDirectionLinewidth;

  float    freqMax;

  std::string backgroundColour;
};

#endif
