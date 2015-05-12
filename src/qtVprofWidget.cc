/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2014 met.no

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
//#define DEBUGPRINT
//#define DEBUGREDRAW

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtVprofWidget.h"
#include "diVprofManager.h"
#include "diGLPainter.h"

#include <QImage>
#include <QKeyEvent>

#include "qtUtility.h"
#include "qtVprofWidget.h"
#include "diVprofManager.h"

#define MILOGGER_CATEGORY "diana.VprofWidget"
#include <miLogger/miLogging.h>

VprofWidget::VprofWidget(VprofManager *vpm)
  : vprofm(vpm)
{
}

void VprofWidget::setCanvas(DiCanvas* c)
{
  vprofm->setCanvas(c);
}

DiCanvas* VprofWidget::canvas() const
{
  return vprofm->canvas();
}

void VprofWidget::paint(DiPainter* painter)
{
  METLIBS_LOG_SCOPE();

  DiGLPainter* gl = dynamic_cast<DiGLPainter*>(painter);
  if (gl && vprofm) {
    diutil::OverrideCursor waitCursor;
    vprofm->plot(gl);
  }
}

void VprofWidget::resize(int w, int h)
{
  METLIBS_LOG_SCOPE("w=" << w << " h=" << h);
  if (vprofm)
    vprofm->setPlotWindow(w,h);
}

// ---------------------- event callbacks -----------------

bool VprofWidget::handleKeyEvents(QKeyEvent *ke)
{
  if (ke->type() != QKeyEvent::KeyPress)
    return false;

  if (ke->key()==Qt::Key_Left){
    Q_EMIT timeChanged(-1);
  } else if (ke->key()==Qt::Key_Right) {
    Q_EMIT timeChanged(+1);
  } else if (ke->key()==Qt::Key_Down) {
    vprofm->setStation(-1);
    Q_EMIT stationChanged(-1);
  } else if (ke->key()==Qt::Key_Up) {
    vprofm->setStation(+1);
    Q_EMIT stationChanged(+1);
  } else {
    return false;
  }

  return true;
}
