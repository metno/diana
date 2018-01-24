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

#include "bdiana_vprof.h"

#include "diPlotCommandFactory.h"
#include "diVprofOptions.h"
#include "diVprofPaintable.h"
#include "export/PaintableImageSource.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.bdiana"
#include <miLogger/miLogging.h>

BdianaVprof::BdianaVprof()
{
}

BdianaVprof::~BdianaVprof()
{
}

void BdianaVprof::MAKE_VPROF()
{
  if (!manager) {
    manager.reset(new VprofManager());
    manager->init();
  }
}

void BdianaVprof::set_options(const std::vector<std::string>& opts)
{
  const PlotCommand_cpv cmds = makeCommands(opts, PLOTCOMMANDS_VPROF);
  vprof_options.insert(vprof_options.end(), cmds.begin(), cmds.end());
}

void BdianaVprof::commands(const std::vector<std::string>& pcom)
{
  manager->parseQuickMenuStrings(vprof_options);
  manager->parseQuickMenuStrings(makeCommands(pcom, PLOTCOMMANDS_VPROF));
  manager->setModel();
}

ImageSource* BdianaVprof::imageSource()
{
  if (!imageSource_) {
    paintable_.reset(new VprofPaintable(manager.get()));
    imageSource_.reset(new PaintableImageSource(paintable_.get()));
  }
  return imageSource_.get();
}

miutil::miTime BdianaVprof::getTime()
{
  return manager->getTime();
}

void BdianaVprof::setTime(const miutil::miTime& time)
{
  manager->setTime(time);
}
