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

#include "PaintGLImageSource.h"

#include "diPaintGLPainter.h"

PaintGLImageSource::PaintGLImageSource()
    : paintdevice_(0)
    , printing_(false)
{
}

PaintGLImageSource::~PaintGLImageSource()
{
}

void PaintGLImageSource::prepare(bool printing, bool single)
{
  printing_ = printing;
  ImageSource::prepare(printing, single);
}

void PaintGLImageSource::paint(QPainter& p)
{
  if (!paintdevice_ || p.device() != paintdevice_ || p.device()->width() != paintdevice_->width() || p.device()->height() != paintdevice_->height()) {
    switchDevice(p);
  }
  glpainter_->begin(&p);
  paintGL(glpainter_.get());
  glpainter_->Flush();
  glpainter_->end();
}

void PaintGLImageSource::finish()
{
  paintdevice_ = 0;
  glpainter_.reset(0);
  glcanvas_.reset(0);
  ImageSource::finish();
}

void PaintGLImageSource::switchDevice(QPainter& p)
{
  if (glpainter_)
    glpainter_->Flush();
  paintdevice_ = p.device();
  glcanvas_.reset(new DiPaintGLCanvas(paintdevice_));
  glcanvas_->parseFontSetup();
  glcanvas_->setPrinting(printing_);
  glpainter_.reset(new DiPaintGLPainter(glcanvas_.get()));
  glpainter_->ShadeModel(DiGLPainter::gl_FLAT);
  // bad: conflict with bdiana multiplot
  glpainter_->Viewport(0, 0, paintdevice_->width(), paintdevice_->height());
}
