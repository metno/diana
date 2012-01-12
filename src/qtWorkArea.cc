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

#include <QVBoxLayout>

#include "qtGLwidget.h"
#include "qtWorkArea.h"
#include "diController.h"


WorkArea::WorkArea(Controller *co,  QWidget* parent)
    : QWidget( parent), contr(co)
{
  QVBoxLayout* vlayout = new QVBoxLayout(this);
#ifndef Q_WS_QWS
  // Create an openGL widget
  QGLFormat fmt, ofmt;
  ofmt= QGLFormat::defaultOverlayFormat();
  ofmt.setDoubleBuffer(true);
  QGLFormat::setDefaultOverlayFormat(ofmt);
  fmt.setOverlay(false);
  fmt.setDoubleBuffer(true);
  fmt.setDirectRendering(false);
  glw = new GLwidget(contr, fmt, this);
#else
  glw = new GLwidget(contr, this);
#endif
  
  if ( !glw->isValid() ) {
#ifndef Q_WS_QWS
    // Try without double-buffering
    fmt.setDoubleBuffer(false);
    glw->setFormat( fmt );
    if ( !glw->isValid() ){
      qCritical("Failed to create OpenGL rendering context on this display");
    }
#else
    qCritical("Failed to create OpenGL rendering context on this display");
#endif
  }
  glw->setMinimumSize( 300, 200 );
  
  // Start the geometry management
  vlayout->addWidget(glw,1);
  vlayout->activate();
}


void WorkArea::updateGL(){
  glw->updateGL();
}
