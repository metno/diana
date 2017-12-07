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

#ifndef PAINTGLIMAGESOURCE_H
#define PAINTGLIMAGESOURCE_H

#include "ImageSource.h"

class DiPaintGLCanvas;
class DiPaintGLPainter;
class QPaintDevice;

#include <memory>

class PaintGLImageSource : public ImageSource
{
public:
  PaintGLImageSource();
  ~PaintGLImageSource();

  void prepare(bool printing, bool single) override;
  //! length -- total, -1 if unknown
  void paint(QPainter&) override;
  void finish() override;

protected:
  virtual void switchDevice(QPainter& p);
  virtual void paintGL(DiPaintGLPainter* gl) = 0;

protected:
  QPaintDevice* paintdevice_;
  std::unique_ptr<DiPaintGLCanvas> glcanvas_;
  std::unique_ptr<DiPaintGLPainter> glpainter_;
  bool printing_;
};

#endif // PAINTGLIMAGESOURCE_H
