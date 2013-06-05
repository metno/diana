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

#include <qapplication.h>
#include <QFrame>
#include <qimage.h>
#include <QKeyEvent>

#define MILOGGER_CATEGORY "diana.SpectrumWidget"
#include <miLogger/miLogging.h>

#include "qtSpectrumWidget.h"
#include "diSpectrumManager.h"

#if defined(USE_PAINTGL)
#include "GL/gl.h"
#endif

#if !defined(USE_PAINTGL)
SpectrumWidget::SpectrumWidget(SpectrumManager *spm, const QGLFormat fmt,
                        QWidget* parent)
    : QGLWidget( fmt, parent), spectrumm(spm)
#else
SpectrumWidget::SpectrumWidget(SpectrumManager *spm, QWidget* parent)
    : QGLWidget(parent, true), spectrumm(spm)
#endif
{

  if ( !isValid() ) {
    qFatal("Failed to create OpenGL rendering context on this display");
  }

  setFocusPolicy(Qt::StrongFocus);
}


//  Set up the OpenGL rendering state
void SpectrumWidget::initializeGL()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("SpectrumWidget::initializeGL");
#endif

  glShadeModel( GL_FLAT );
  setAutoBufferSwap(false);
  glDrawBuffer(GL_BACK);
}


void SpectrumWidget::paintGL()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("SpectrumWidget::paintGL");
#endif
#ifdef DEBUGREDRAW
  METLIBS_LOG_DEBUG("SpectrumWidget::paintGL");
#endif

  if (!spectrumm) return;

  spectrumm->plot();

  swapBuffers();
}


//  Set up the OpenGL view port, matrix mode, etc.
void SpectrumWidget::resizeGL( int w, int h )
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("SpectrumWidget::resizeGL  w=" << w << " h=" << h);
#endif
  if (spectrumm) spectrumm->setPlotWindow(w,h);

  glViewport( 0, 0, (GLint)w, (GLint)h );
  //plotw= w;
  //ploth= h;
  updateGL();

  setFocus();
}

// ---------------------- event callbacks -----------------

void SpectrumWidget::keyPressEvent(QKeyEvent *me)
{
  if (me->key()==Qt::Key_Left  ||
      me->key()==Qt::Key_Right ||
      me->key()==Qt::Key_Down  ||
      me->key()==Qt::Key_Up) {

    if (me->key()==Qt::Key_Left){
      spectrumm->setTime(-1);
      emit timeChanged(-1);
    } else if (me->key()==Qt::Key_Right){
      spectrumm->setTime(+1);
      emit timeChanged(+1);
    }else if (me->key()==Qt::Key_Down){
      spectrumm->setStation(-1);
      emit stationChanged(-1);
    }else if (me->key()==Qt::Key_Up){
      spectrumm->setStation(+1);
      emit stationChanged(+1);
    }
    updateGL();
  }
}


bool SpectrumWidget::saveRasterImage(const miutil::miString fname,
			          const miutil::miString format,
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
