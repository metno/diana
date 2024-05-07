/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017 met.no

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

#ifndef BDIANA_WAVESPEC_H
#define BDIANA_WAVESPEC_H

#include "bdiana_source.h"
#include "diSpectrumManager.h"

#include <string>
#include <vector>

class SpectrumPaintable;

struct BdianaSpectrum : public BdianaSource
{
  BdianaSpectrum();
  ~BdianaSpectrum();

  void MAKE_SPECTRUM();
  void set_options(const std::vector<std::string>& opts);
  void set_spectrum(const std::vector<std::string>& pcom);
  void set_station();
  ImageSource* imageSource() override;

  miutil::miTime getReferenceTime() override;
  plottimes_t getTimes() override;
  bool setTime(const miutil::miTime& time) override;

  std::string station;
  std::vector<std::string> spectrum_models, spectrum_options;
  bool spectrum_optionschanged;

  std::unique_ptr<SpectrumManager> manager;
  std::unique_ptr<SpectrumPaintable> paintable_;
  std::unique_ptr<ImageSource> imageSource_;
};

#endif // BDIANA_WAVESPEC_H
