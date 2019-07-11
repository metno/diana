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

#include "DianaImageSource.h"

#include "diController.h"
#include "diMainPaintable.h"
#include "diPaintGLPainter.h"

#include <QPicture>

#define MILOGGER_CATEGORY "diana.DianaImageSource"
#include <miLogger/miLogging.h>

DianaImageSource::DianaImageSource(MainPaintable* glw)
    : PaintableImageSource(glw)
    , annotationsOnly_(false)
    , timestep_(0)
{
}

void DianaImageSource::setTimes(const std::vector<miutil::miTime>& times)
{
  times_ = times;
}

void DianaImageSource::prepare(bool printing, bool single)
{
  PaintableImageSource::prepare(printing, single);
  oldTime_ = controller()->getPlotTime();
  if (single || times_.empty()) {
    times_.clear();
    times_.push_back(oldTime_);
  }
  timestep_ = 0;
  setTimeFromStep();
}

int DianaImageSource::count()
{
  return times_.size();
}

void DianaImageSource::paintGL(DiPaintGLPainter* gl)
{
  if (annotationsOnly_) {
    const std::vector<Rectangle> annotationRectangles_ = controller()->plotAnnotations(gl);
    const QTransform annotationTransform_ = gl->transform;

    QRectF cutout;
    int i = 0, number = -1;
    for (const Rectangle& ar : annotationRectangles_) {
      if (i == number || number == -1) {
        QRectF r = annotationTransform_.mapRect(QRectF(ar.x1, ar.y1, ar.width(), ar.height()));
        if (cutout.isNull())
          cutout = r;
        else if (!r.isNull())
          cutout = cutout.united(r);
      }
      i += 1;
    }
    if (!cutout.isNull()) {
      annotationsCutout_ = QRectF(-cutout.x(), -cutout.y(), cutout.width(), cutout.height());
    } else {
      annotationsCutout_ = QRectF();
    }
  } else {
    PaintableImageSource::paintGL(gl);
  }
}

bool DianaImageSource::next()
{
  timestep_ += 1;
  return setTimeFromStep();
}

bool DianaImageSource::setTimeFromStep()
{
  if (timestep_ >= times_.size())
    return false;
  setPlotTime(times_[timestep_]);
  return true;
}

void DianaImageSource::setPlotTime(const miutil::miTime& time)
{
  controller()->setPlotTime(time);
  controller()->updatePlots();
}

void DianaImageSource::finish()
{
  PaintableImageSource::finish();
  setPlotTime(oldTime_);
}

Controller* DianaImageSource::controller()
{
  return static_cast<MainPaintable*>(paintable())->controller();
}
