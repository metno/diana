/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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
#ifndef SPECTRUMWIDGET_H
#define SPECTRUMWIDGET_H

#include "diPaintable.h"
#include <QObject>

class SpectrumManager;

/**
   \brief The OpenGL widget for Wave Spectrum

   Handles widget paint/redraw events.
   Receives keybord events and initiates actions.
*/
class SpectrumWidget : public QObject, public DiPaintable
{
  Q_OBJECT;

public:
  SpectrumWidget(SpectrumManager *spm);

  void setCanvas(DiCanvas* c)
    { mCanvas = c; }
  DiCanvas* canvas() const
    { return mCanvas; }
  void paint(DiPainter* painter);
  void resize(int width, int height);

  bool handleKeyEvents(QKeyEvent*);

Q_SIGNALS:
  void timeChanged(int);
  void stationChanged(int);

private:
  SpectrumManager *spectrumm;
  DiCanvas* mCanvas;
};

#endif
