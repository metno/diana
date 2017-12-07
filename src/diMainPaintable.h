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
#ifndef _diMainPaintable_h
#define _diMainPaintable_h

#include "diPaintable.h"

class Controller;

/**
   \brief the Diana map widget

   the Diana map widget supporting
   - simple underlay
   - keyboard/mouse event translation to Diana types

   The class name is misleading, it does not have much to do with OpenGL any longer.
*/
class MainPaintable : public Paintable
{
  Q_OBJECT

public:
  MainPaintable(Controller*);
  ~MainPaintable();

  Controller* controller() { return contr; }

  void paintUnderlay(DiPainter* gl) override;
  void paintOverlay(DiPainter* gl) override;

public:
  void setCanvas(DiCanvas* canvas) override;
  void resize(const QSize& size) override;

private:
  Controller* contr;
};

#endif
