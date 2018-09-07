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

#include "diana_config.h"

#include "diVprofPaintable.h"

#include "diGLPainter.h"
#include "diVprofManager.h"

#define MILOGGER_CATEGORY "diana.VprofPaintable"
#include <miLogger/miLogging.h>

VprofPaintable::VprofPaintable(VprofManager* vpm)
    : vprofm(vpm)
{
}

VprofPaintable::~VprofPaintable()
{
}

void VprofPaintable::setCanvas(DiCanvas* c)
{
  Paintable::setCanvas(c);
  vprofm->setCanvas(c);
  requestBackgroundBufferUpdate();
}

void VprofPaintable::paintUnderlay(DiPainter*)
{
}

void VprofPaintable::paintOverlay(DiPainter* painter)
{
  METLIBS_LOG_SCOPE();

  DiGLPainter* gl = dynamic_cast<DiGLPainter*>(painter);
  if (gl && vprofm) {
    vprofm->plot(gl);
  }
}

void VprofPaintable::resize(const QSize& size)
{
  METLIBS_LOG_SCOPE();
  if (vprofm)
    vprofm->setPlotWindow(size);
  Paintable::resize(size);
}
