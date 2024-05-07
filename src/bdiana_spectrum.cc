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

#include "bdiana_spectrum.h"

#include "diSpectrumOptions.h"
#include "diSpectrumPaintable.h"
#include "export/PaintableImageSource.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.bdiana"
#include <miLogger/miLogging.h>

BdianaSpectrum::BdianaSpectrum()
{
}

BdianaSpectrum::~BdianaSpectrum()
{
}

void BdianaSpectrum::MAKE_SPECTRUM()
{
  if (!manager) {
    manager.reset(new SpectrumManager);
  }
}

void BdianaSpectrum::set_options(const std::vector<std::string>& opts)
{
  int n = opts.size();
  for (int i = 0; i < n; i++) {
    std::string line = opts[i];
    miutil::trim(line);
    if (line.empty())
      continue;
    std::string upline = miutil::to_upper(line);

    if (miutil::contains(upline, "MODELS=") || miutil::contains(upline, "MODEL=") || miutil::contains(upline, "STATION=")) {
      std::vector<std::string> vs = miutil::split(line, "=");
      if (vs.size() > 1) {
        std::string key = miutil::to_upper(vs[0]);
        std::string value = vs[1];
        if (key == "STATION") {
          if (miutil::contains(value, "\""))
            miutil::remove(value, '\"');
          station = value;
        } else if (key == "MODELS" || key == "MODEL") {
          spectrum_models = miutil::split(value, 0, ",");
        }
      }
    } else {
      // assume plot-options
      spectrum_options.push_back(line);
      spectrum_optionschanged = true;
    }
  }
}

void BdianaSpectrum::set_spectrum(const std::vector<std::string>& pcom)
{
  // extract options for plot
  set_options(pcom);

  //  if (verbose)
  //    METLIBS_LOG_INFO("- sending plotCommands");
  if (spectrum_optionschanged)
    manager->getOptions()->readOptions(spectrum_options);
  spectrum_optionschanged = false;
  manager->setSelectedModels(spectrum_models);
  manager->setModel();
}

void BdianaSpectrum::set_station()
{
  //  if (verbose)
  //    METLIBS_LOG_INFO("- setting station:" << wavespec.station);
  if (!station.empty())
    manager->setStation(station);
}

ImageSource* BdianaSpectrum::imageSource()
{
  if (!imageSource_) {
    paintable_.reset(new SpectrumPaintable(manager.get()));
    imageSource_.reset(new PaintableImageSource(paintable_.get()));
  }
  return imageSource_.get();
}

miutil::miTime BdianaSpectrum::getReferenceTime()
{
  const std::vector<SpectrumManager::SelectedModel>& sms = manager->getSelectedModels();
  for (const auto& sm : sms) {
    if (!sm.reftime.empty())
      return miutil::miTime(sm.reftime);
  }
  return miutil::miTime();
}

plottimes_t BdianaSpectrum::getTimes()
{
  return manager->getTimeList();
}

bool BdianaSpectrum::setTime(const miutil::miTime& time)
{
  if (hasTime(time)) {
    manager->setTime(time);
    return true;
  }
  return false;
}
