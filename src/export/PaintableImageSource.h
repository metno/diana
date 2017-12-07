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

#ifndef VPROFIMAGESOURCE_H
#define VPROFIMAGESOURCE_H

#include "PaintGLImageSource.h"

#include <QSize>

class DiCanvas;
class Paintable;

class PaintableImageSource : public PaintGLImageSource
{

public:
  PaintableImageSource(Paintable* p);

  void prepare(bool printing, bool single) override;
  void finish() override;
  QSize size() override;

protected:
  void switchDevice(QPainter& p) override;
  void paintGL(DiPaintGLPainter* gl) override;

  Paintable* paintable() const { return p_; }

private Q_SLOTS:
  void paintableResized(const QSize& size);

private:
  Paintable* p_;
  DiCanvas* oldCanvas_;
  QSize oldSize_;
};

#endif // VPROFIMAGESOURCE_H
