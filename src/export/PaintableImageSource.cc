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

#include "PaintableImageSource.h"

#include "diPaintGLPainter.h"
#include "diPaintable.h"

PaintableImageSource::PaintableImageSource(Paintable* p)
    : p_(p)
    , oldCanvas_(0)
{
  connect(p_, &Paintable::resized, this, &PaintableImageSource::paintableResized);
}

void PaintableImageSource::prepare(bool printing, bool single)
{
  oldCanvas_ = p_->canvas();
  oldSize_ = p_->size();
  PaintGLImageSource::prepare(printing, single);
}

void PaintableImageSource::finish()
{
  p_->setCanvas(oldCanvas_);
  p_->resize(oldSize_);
  PaintGLImageSource::finish();
}

QSize PaintableImageSource::size()
{
  return p_->size();
}

void PaintableImageSource::paintableResized(const QSize& size)
{
  Q_EMIT resized(size);
}

void PaintableImageSource::switchDevice(QPainter& p)
{
  p_->setCanvas(0);
  PaintGLImageSource::switchDevice(p);
  p_->setCanvas(glcanvas_.get());

  QPaintDevice* d = p.device();
  p_->resize(QSize(d->width(), d->height()));
}

void PaintableImageSource::paintGL(DiPaintGLPainter* gl)
{
  p_->paintUnderlay(gl);
  p_->paintOverlay(gl);
}
