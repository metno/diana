/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2017 met.no

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
//#define DEBUGREDRAW

#include "diana_config.h"

#include "diController.h"
#include "diGLPainter.h"
#include "diMainPaintable.h"

#define MILOGGER_CATEGORY "diana.MainPaintable"
#include <miLogger/miLogging.h>

MainPaintable::MainPaintable(Controller* c)
    : contr(c)
{
}

MainPaintable::~MainPaintable()
{
}

void MainPaintable::setCanvas(DiCanvas* canvas)
{
  Paintable::setCanvas(canvas);
  contr->setCanvas(canvas);
  requestBackgroundBufferUpdate();
}

void MainPaintable::paintUnderlay(DiPainter* painter)
{
  if (!contr)
    return;

  DiGLPainter* gl = dynamic_cast<DiGLPainter*>(painter);
  if (!gl)
    return;

  contr->plot(gl, true, false); // draw underlay
}

void MainPaintable::paintOverlay(DiPainter* painter)
{
  if (!contr)
    return;

  DiGLPainter* gl = dynamic_cast<DiGLPainter*>(painter);
  if (!gl)
    return;

  contr->plot(gl, false, true); // draw overlay
}

//  Set up the OpenGL view port, matrix mode, etc.
void MainPaintable::resize(const QSize& size)
{
  Paintable::resize(size);
  if (contr)
    contr->setPlotWindow(size);
}
