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

#include "bdiana_vcross.h"

#include "diPlotCommandFactory.h"
#include "export/VcrossImageSource.h"
#include "vcross_v2/VcrossQuickmenues.h"

DianaVcross::DianaVcross()
{
}

DianaVcross::~DianaVcross()
{
}

void DianaVcross::MAKE_VCROSS()
{
  if (!manager)
    manager = std::make_shared<vcross::QtManager>();
}

void DianaVcross::commands(const std::vector<std::string>& pcom)
{
  vcross::VcrossQuickmenues::parse(manager, makeCommands(pcom, PLOTCOMMANDS_VCROSS));
}

ImageSource* DianaVcross::imageSource()
{
  if (!imageSource_)
    imageSource_.reset(new VcrossImageSource(manager));
  return imageSource_.get();
}

miutil::miTime DianaVcross::getReferenceTime()
{
  if (manager->getFieldCount() > 0)
    return manager->getReftimeAt(0);
  else
    return miutil::miTime();
}

plottimes_t DianaVcross::getTimes()
{
  plottimes_t times;
  const int count = manager->getTimeCount();
  for (int i = 0; i < count; ++i)
    times.insert(manager->getTimeValue(i));
  return times;
}

void DianaVcross::setTime(const miutil::miTime& time)
{
  manager->setTimeToBestMatch(time);
}
