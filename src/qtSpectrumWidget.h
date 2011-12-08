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
#ifndef SPECTRUMWIDGET_H
#define SPECTRUMWIDGET_H

#include <puTools/miString.h>
#include <map>

#include <QKeyEvent>
#include <QWidget>

#include "GL/paintgl.h"

using namespace std;

class SpectrumManager;

/**
   \brief The OpenGL widget for Wave Spectrum

   Handles widget paint/redraw events.
   Receives keybord events and initiates actions.
*/
class SpectrumWidget : public PaintGLWidget
{
  Q_OBJECT

public:
  SpectrumWidget(SpectrumManager *spm, QWidget* parent = 0);

  bool saveRasterImage(const miutil::miString fname,
  		       const miutil::miString format,
		       const int quality = -1);

protected:

  void initializeGL();
  void paintGL();
  void resizeGL( int w, int h );

private:
  SpectrumManager *spectrumm;

  void keyPressEvent(QKeyEvent *me);

signals:
  void timeChanged(int);
  void stationChanged(int);

};


#endif
