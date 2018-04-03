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
#ifndef _qtMainPaintableUiEventHandler_h
#define _qtMainPaintableUiEventHandler_h

#include "diEventResult.h"
#include "qtUiEventHandler.h"

#include <map>
#include <string>
#include <vector>

class MainPaintable;

class MainUiEventHandler : public UiEventHandler
{
  Q_OBJECT

public:
  MainUiEventHandler(MainPaintable*);

  bool handleKeyEvents(QKeyEvent*) override;
  bool handleMouseEvents(QMouseEvent*) override;
  bool handleWheelEvents(QWheelEvent* we) override;

  bool useScrollwheelZoom() const { return scrollwheelZoom; }

  void setUseScrollwheelZoom(bool use) { scrollwheelZoom = use; }

Q_SIGNALS:
  /// single click signal
  void mouseGridPos(QMouseEvent* me);
  /// single click signal (right mouse button)
  void mouseRightPos(QMouseEvent* me);
  /// mouse movement, no buttons
  void mouseMovePos(QMouseEvent* me, bool quick);
  void objectsChanged();
  void fieldsChanged();

  void keyPress(QKeyEvent* ke);

  void mouseDoubleClick(QMouseEvent* me);

  void changeCursor(cursortype);

private:
  void setFlagsFromEventResult(const EventResult& res);

private:
  MainPaintable* p;
  bool scrollwheelZoom;
};

#endif
