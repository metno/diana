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
#ifndef VprofUiEventHandler_h
#define VprofUiEventHandler_h

#include "qtUiEventHandler.h"

class VprofPaintable;

class VprofUiEventHandler : public UiEventHandler
{
  Q_OBJECT

public:
  VprofUiEventHandler(VprofPaintable*);

  bool handleKeyEvents(QKeyEvent* ke) override;

Q_SIGNALS:
  void timeChanged(int);
  void stationChanged(int);

private:
  VprofPaintable* p;
};

#endif // VprofUiEventHandler_h
