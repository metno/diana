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

#include "diPaintable.h"
#include "export/PaintableImageSource.h"

#define MILOGGER_CATEGORY "diana.Paintable"
#include <miLogger/miLogging.h>

Paintable::Paintable()
    : enable_background_buffer(false)
    , update_background_buffer(false)
    , mCanvas(0)
    , mSize(1, 1)
{
}

Paintable::~Paintable()
{
}

void Paintable::setCanvas(DiCanvas* canvas)
{
  mCanvas = canvas;
}

void Paintable::resize(const QSize& size)
{
  if (size != mSize) {
    mSize = size;
    Q_EMIT resized(mSize);
  }
}

ImageSource* Paintable::imageSource()
{
  METLIBS_LOG_SCOPE();
  if (!imageSource_)
    imageSource_.reset(new PaintableImageSource(this));
  return imageSource_.get();
}
