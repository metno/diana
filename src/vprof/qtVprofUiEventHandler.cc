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

#include "qtVprofUiEventHandler.h"

#include "diVprofManager.h"
#include "diVprofPaintable.h"

#include <QKeyEvent>

#define MILOGGER_CATEGORY "diana.VprofPaintable"
#include <miLogger/miLogging.h>

VprofUiEventHandler::VprofUiEventHandler(VprofPaintable* w)
    : p(w)
{
}

bool VprofUiEventHandler::handleKeyEvents(QKeyEvent* ke)
{
  if (ke->type() != QKeyEvent::KeyPress)
    return false;

  if (ke->key() == Qt::Key_Left) {
    Q_EMIT timeChanged(-1);
  } else if (ke->key() == Qt::Key_Right) {
    Q_EMIT timeChanged(+1);
  } else if (ke->key() == Qt::Key_Down) {
    p->vprofManager()->stepStation(-1);
    Q_EMIT stationChanged(-1);
  } else if (ke->key() == Qt::Key_Up) {
    p->vprofManager()->stepStation(+1);
    Q_EMIT stationChanged(+1);
  } else {
    return false;
  }

  return true;
}
