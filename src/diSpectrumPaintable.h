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
#ifndef DISPECTRUMWIDGET_H
#define DISPECTRUMWIDGET_H

#include "diPaintable.h"

class SpectrumManager;

/**
   \brief The OpenGL widget for Wave Spectrum

   Handles widget paint/redraw events.
   Receives keybord events and initiates actions.
*/
class SpectrumPaintable : public Paintable
{
public:
  SpectrumPaintable(SpectrumManager* spm);

  SpectrumManager* spectrumManager() { return spectrumm; }

  void setCanvas(DiCanvas* c) override;
  void paintUnderlay(DiPainter* painter) override;
  void paintOverlay(DiPainter* painter) override;
  void resize(const QSize& size) override;

private:
  SpectrumManager* spectrumm;
};

#endif
