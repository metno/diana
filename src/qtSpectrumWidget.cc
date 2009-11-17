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

#include <qapplication.h>
#include <QFrame>
#include <qimage.h>

#include <qtSpectrumWidget.h>
//Added by qt3to4:
#include <QKeyEvent>
#include <diSpectrumManager.h>


SpectrumWidget::SpectrumWidget(SpectrumManager *spm, const QGLFormat fmt,
			 QWidget* parent, const char* name )
    : QGLWidget( fmt, parent, name ), spectrumm(spm)
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
  cerr << "SpectrumWidget::initializeGL" << endl;
#endif

  glShadeModel( GL_FLAT );
  setAutoBufferSwap(false);
  glDrawBuffer(GL_BACK);
}


void SpectrumWidget::paintGL()
{
#ifdef DEBUGPRINT
  cerr << "SpectrumWidget::paintGL" << endl;
#endif
#ifdef DEBUGREDRAW
  cerr << "SpectrumWidget::paintGL" << endl;
#endif

  if (!spectrumm) return;

  spectrumm->plot();

  swapBuffers();
}


//  Set up the OpenGL view port, matrix mode, etc.
void SpectrumWidget::resizeGL( int w, int h )
{
#ifdef DEBUGPRINT
  cerr << "SpectrumWidget::resizeGL  w=" << w << " h=" << h << endl;
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
  // problems on SGI - make sure plot is flushed properly
  updateGL();
  makeCurrent();
  glFlush();

#ifndef linux
  QApplication::flushX();

  updateGL();
  makeCurrent();
  glFlush();
#endif

  // test of new grabFrameBuffer command
  QImage image= grabFrameBuffer(true); // withAlpha=TRUE
  image.save(fname.cStr(), format.cStr(), quality );

  return true;
}
