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
//#define DEBUGPRINT
//#define DEBUGREDRAW

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <QImage>
#include <QKeyEvent>
#include <QApplication>

#define MILOGGER_CATEGORY "diana.VprofWidget"
#include <miLogger/miLogging.h>

#include "qtVprofWidget.h"
#include "diVprofManager.h"


#if !defined(USE_PAINTGL)
VprofWidget::VprofWidget(VprofManager *vpm, const QGLFormat fmt,
                        QWidget* parent)
    : QGLWidget( fmt, parent ), vprofm(vpm)
#else
VprofWidget::VprofWidget(VprofManager *vpm, QWidget* parent)
    : PaintGLWidget(parent, true), vprofm(vpm)
#endif
{

  if ( !isValid() ) {
    qFatal("Failed to create OpenGL rendering context on this display");
  }

  setFocusPolicy(Qt::StrongFocus);
}


//  Set up the OpenGL rendering state
void VprofWidget::initializeGL()
{
  METLIBS_LOG_SCOPE();

  glShadeModel( GL_FLAT );
  setAutoBufferSwap(false);
  glDrawBuffer(GL_BACK);
}


void VprofWidget::paintGL()
{
  METLIBS_LOG_SCOPE();

  if (!vprofm) return;

  QApplication::setOverrideCursor( Qt::WaitCursor );
  vprofm->plot();
  QApplication::restoreOverrideCursor();

  swapBuffers();
}


//  Set up the OpenGL view port, matrix mode, etc.
void VprofWidget::resizeGL( int w, int h )
{
  METLIBS_LOG_DEBUG("VprofWidget::resizeGL  w=" << w << " h=" << h);
  if (vprofm) vprofm->setPlotWindow(w,h);

  glViewport( 0, 0, (GLint)w, (GLint)h );
  //plotw= w;
  //ploth= h;
  updateGL();

  setFocus();
}


bool VprofWidget::saveRasterImage(const std::string fname,
			          const std::string format,
			          const int quality)
{

  updateGL();
  makeCurrent();
  glFlush();

  // test of new grabFrameBuffer command
  QImage image= grabFrameBuffer(true); // withAlpha=TRUE
  image.save(fname.c_str(), format.c_str(), quality );

  return true;
}
