/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2006 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>
#include <iostream>
#define MILOGGER_CATEGORY "diana.DrawingManager"
#include <miLogger/miLogging.h>

#include <diDrawingManager.h>
#include <diPlotModule.h>
#include <diObjectManager.h>
#include <puTools/miDirtools.h>
#include <diAnnotationPlot.h>
#include <puCtools/puCglob.h>
#include <puCtools/glob_cache.h>
#include <puTools/miSetupParser.h>

#include <iomanip>
#include <set>
#include <cmath>

#include <QKeyEvent>
#include <QMouseEvent>

//#define DEBUGPRINT
using namespace miutil;

DrawingManager::DrawingManager(PlotModule* pm, ObjectManager* om)
  : plotm(pm), objm(om)
{
  if (plotm==0 || objm==0){
    METLIBS_LOG_WARN("Catastrophic error: plotm or objm == 0");
  }
}

DrawingManager::~DrawingManager()
{
}

bool DrawingManager::parseSetup()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ DrawingManager::parseSetup");
#endif

  miString section="DRAWING";
  vector<miString> vstr;

  return true;
}

void DrawingManager::sendMouseEvent(QMouseEvent* me, EventResult& res)
{
}

void DrawingManager::sendKeyboardEvent(QKeyEvent* me, EventResult& res)
{
}
