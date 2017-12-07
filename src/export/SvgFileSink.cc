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

#include "SvgFileSink.h"

#include <QPrinter>
#include <QSvgGenerator>

#include "diana_config.h"

SvgFileSink::SvgFileSink(const QSize& size, const QString& filename)
    : generator(new QSvgGenerator())
    , started_(false)
{
  generator->setFileName(filename);
  generator->setSize(size);
  generator->setViewBox(QRect(QPoint(0, 0), size));
  generator->setTitle("diana image");
  generator->setDescription(QString("Created by diana %1.").arg(PVERSION));

  // For some reason, QPrinter can determine the correct resolution to use, but
  // QSvgGenerator cannot manage that on its own, so we take the resolution from
  // a QPrinter instance which we do not otherwise use.
  QPrinter sprinter;
  generator->setResolution(sprinter.resolution());
}

bool SvgFileSink::isPrinting()
{
  return false;
}

bool SvgFileSink::beginPage()
{
  if (started_)
    return false;
  started_ = true;
  painter_.begin(generator.get());
  return true;
}

QPainter& SvgFileSink::paintPage()
{
  return painter_;
}

bool SvgFileSink::endPage()
{
  if (!started_)
    return false;
  painter_.end();
  return true;
}
