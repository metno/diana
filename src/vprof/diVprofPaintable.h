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
#ifndef VPROFWIDGET_H
#define VPROFWIDGET_H

#include "diPaintable.h"

class VprofManager;

/**
   \brief The OpenGL widget for Vertical Profiles (soundings)

   Handles widget paint/redraw events.
   Receives mouse and keybord events and initiates actions.
*/
class VprofPaintable : public Paintable
{
  Q_OBJECT

public:
  VprofPaintable(VprofManager* vpm);
  ~VprofPaintable();

  VprofManager* vprofManager() { return vprofm; }

  void setCanvas(DiCanvas* c) override;
  void paintUnderlay(DiPainter* painter) override;
  void paintOverlay(DiPainter* painter) override;
  void resize(const QSize& size) override;

private:
  VprofManager* vprofm;
};

#endif
