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
#ifndef _diDrawingManager_h
#define _diDrawingManager_h

#include <vector>
#include <puTools/miString.h>
#include <diCommonTypes.h>
#include <diDrawingTypes.h>
#include <diField/diGridConverter.h>
#include <diMapMode.h>

using namespace std;

class EditItemManager;
class PlotModule;
class ObjectManager;

class QKeyEvent;
class QMouseEvent;

/**
  \brief Manager for drawing areas and annotations.
*/

class DrawingManager
{
public:
  DrawingManager(PlotModule*, ObjectManager*);
  ~DrawingManager();

  /// parse DRAWING section of setup file (defines Drawing products)
  bool parseSetup();

  /// handle mouse event
  void sendMouseEvent(QMouseEvent* me, EventResult& res);
  /// handle keyboard event
  void sendKeyboardEvent(QKeyEvent* me, EventResult& res);

  bool changeProjection(const Area& newArea);
  void plot(bool under, bool over);

  bool drawingModeEnabled;

  EditItemManager *getEditItemManager() { return editItemManager; }

private:
  PlotModule* plotm;
  ObjectManager* objm;
  EditItemManager *editItemManager;     // fronts,symbols,areas

  GridConverter gc;   // gridconverter class

  Rectangle plotRect;
  Rectangle editRect;
  float first_x, first_y;
  bool moved;
};

#endif
