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

#ifndef DIANAIMAGESOURCE_H
#define DIANAIMAGESOURCE_H

#include "diRectangle.h"
#include "export/PaintableImageSource.h"

#include <puTools/miTime.h>

#include <QSize>
#include <QTransform>

#include <vector>

class Controller;
class MainPaintable;

class DianaImageSource : public PaintableImageSource
{
public:
  DianaImageSource(MainPaintable* glw);

  void setTimes(const std::vector<miutil::miTime>& times);

  void prepare(bool printing, bool single) override;
  int count() override;
  void paintGL(DiPaintGLPainter*) override;
  bool next() override;
  void finish() override;

  void setAnnotationsOnly(bool ao) { annotationsOnly_ = ao; }
  bool isAnnotationsOnly() const { return annotationsOnly_; }
  const QRectF& annotationsCutout() const { return annotationsCutout_; }

private:
  Controller* controller();
  bool setTimeFromStep();
  void setPlotTime(const miutil::miTime& time);

private:
  bool annotationsOnly_;
  QRectF annotationsCutout_;

  miutil::miTime oldTime_;
  std::vector<miutil::miTime> times_;
  size_t timestep_;
};

#endif // DIANAIMAGESOURCE_H
