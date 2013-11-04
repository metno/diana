/*
  Diana - A Free Meteorological Visualisation Tool

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

#include "qtVcrossWidget.h"

#ifdef USE_VCROSS_V2
#include "vcross_v2/diVcrossManager.h"
#else
#include "vcross_v1/diVcross1Manager.h"
#endif

#include <QImage>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>

#define MILOGGER_CATEGORY "diana.VcrossWidget"
#include <miLogger/miLogging.h>

namespace /* anonymous */ {
#if !defined(USE_PAINTGL)
QGLFormat glfmt()
{
  QGLFormat fmt;
  fmt.setOverlay(false);
  fmt.setDoubleBuffer(true);
  fmt.setDirectRendering(false);
  return fmt;
}
#endif // !USE_PAINTGL
} // namespace anonymous

VcrossWidget::VcrossWidget(VcrossManager *vcm, QWidget* parent)
  :
#if !defined(USE_PAINTGL)
  QGLWidget(glfmt(), parent),
#else
  PaintGLWidget(parent, true),
#endif
  vcrossm(vcm), fbuffer(0), arrowKeyDirection(1),
  timeGraph(false), startTimeGraph(false)
{
  if ( !isValid() ) {
    qFatal("Failed to create OpenGL rendering context on this display");
  }

  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(false);

  savebackground= false;
  dorubberband= false;
  dopanning= false;
}


//  Release allocated resources
VcrossWidget::~VcrossWidget()
{
  delete[] fbuffer;
}


//  Set up the OpenGL rendering state
void VcrossWidget::initializeGL()
{
  METLIBS_LOG_SCOPE();

  glShadeModel( GL_FLAT );
  setAutoBufferSwap(false);
  glDrawBuffer(GL_BACK);
}


void VcrossWidget::paintGL()
{
  METLIBS_LOG_SCOPE();

  if (!vcrossm)
    return;

  if (fbuffer && !savebackground) {
    delete[] fbuffer;
    fbuffer= 0;
  }

  if (!fbuffer) {

#ifdef DEBUGREDRAW
    METLIBS_LOG_DEBUG("... vcrossm->plot");
#endif
    QApplication::setOverrideCursor( Qt::WaitCursor );
    vcrossm->plot();
    QApplication::restoreOverrideCursor();

    if (savebackground) {
#ifdef DEBUGREDRAW
    METLIBS_LOG_DEBUG("...... savebackground");
#endif

    vcrossm->getPlotSize(glx1,gly1,glx2,gly2,rubberbandColour);

      fbuffer= new GLuint[4*plotw*ploth];

      glPixelZoom(1,1);
      glPixelStorei(GL_PACK_SKIP_ROWS,0);
      glPixelStorei(GL_PACK_SKIP_PIXELS,0);
      glPixelStorei(GL_PACK_ROW_LENGTH,plotw);
      glPixelStorei(GL_PACK_ALIGNMENT,4);

      glReadPixels(0,0,plotw,ploth,
		   GL_RGBA,GL_UNSIGNED_BYTE,
		   fbuffer);
      glPixelStorei(GL_PACK_ROW_LENGTH,0);
    }

  } else {
#ifdef DEBUGREDRAW
    METLIBS_LOG_DEBUG("...... drawbackground");
#endif

    makeCurrent();

    glLoadIdentity();
    glOrtho(glx1,glx2,gly1,gly2,-1.,1.);

    glPixelZoom(1,1);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,plotw);
    glPixelStorei(GL_UNPACK_ALIGNMENT,4);

    float glx= glx1+(glx2-glx1)*0.0001f;
    float gly= gly1+(gly2-gly1)*0.0001f;
    glRasterPos2f(glx,gly);

    glDrawPixels(plotw,ploth,
		 GL_RGBA,GL_UNSIGNED_BYTE,
		 fbuffer);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);

    if (dorubberband) {
      float x1= glx1 + (glx2-glx1)*float(firstx)/float(plotw);
      float y1= gly1 + (gly2-gly1)*float(firsty)/float(ploth);
      float x2= glx1 + (glx2-glx1)*float(mousex)/float(plotw);
      float y2= gly1 + (gly2-gly1)*float(mousey)/float(ploth);
      glColor4ubv(rubberbandColour.RGBA());
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glLineWidth(2.0);
      glRectf(x1,y1,x2,y2);
      //glBegin(GL_LINE_LOOP); // Mesa problems ?
      //glVertex2f(x1,y1);
      //glVertex2f(x2,y1);
      //glVertex2f(x2,y2);
      //glVertex2f(x1,y2);
      //glEnd();
    }

  }

  swapBuffers();
}


//  Set up the OpenGL view port, matrix mode, etc.
void VcrossWidget::resizeGL(int w, int h)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG("w=" << w << " h=" << h);

  vcrossm->setPlotWindow(w,h);

  glViewport( 0, 0, (GLint)w, (GLint)h );
  plotw= w;
  ploth= h;
  updateGL();

  setFocus();
}

// ---------------------- event callbacks -----------------

void VcrossWidget::keyPressEvent(QKeyEvent *me)
{
  if (dorubberband || dopanning)
    return;

  bool change= true;

  if (me->modifiers() & Qt::ControlModifier) {

    if (me->key()==Qt::Key_Left && timeGraph){
      vcrossm->setTimeGraphPos(-1);
    } else if (me->key()==Qt::Key_Right && timeGraph){
      vcrossm->setTimeGraphPos(+1);
    } else if (me->key()==Qt::Key_Left){
      vcrossm->setTime(-1);
      emit timeChanged(-1);
    } else if (me->key()==Qt::Key_Right){
      vcrossm->setTime(+1);
      emit timeChanged(+1);
    } else if (me->key()==Qt::Key_Down){
      vcrossm->setCrossection(-1);
      emit crossectionChanged(-1);
    } else if (me->key()==Qt::Key_Up){
      vcrossm->setCrossection(+1);
      emit crossectionChanged(+1);
    } else {
      change= false;
    }

  } else if (me->key()==Qt::Key_Left){
    vcrossm->movePart(-arrowKeyDirection*plotw/8, 0);
  } else if (me->key()==Qt::Key_Right){
    vcrossm->movePart(arrowKeyDirection*plotw/8, 0);
  } else if (me->key()==Qt::Key_Down){
    vcrossm->movePart(0, -arrowKeyDirection*ploth/8);
  } else if (me->key()==Qt::Key_Up){
    vcrossm->movePart(0, arrowKeyDirection*ploth/8);
  } else if (me->key()==Qt::Key_X) {
    vcrossm->increasePart();
  } else if (me->key()==Qt::Key_Z && me->modifiers() & Qt::ShiftModifier) {
    vcrossm->increasePart();
  } else if (me->key()==Qt::Key_Z) {
    int dw= plotw - int(plotw/1.3);
    int dh= ploth - int(ploth/1.3);
    vcrossm->decreasePart(dw,dh,plotw-dw,ploth-dh);
  } else if (me->key()==Qt::Key_Home) {
    vcrossm->standardPart();
  } else if (me->key()==Qt::Key_R) {
    if (arrowKeyDirection>0) arrowKeyDirection= -1;
    else                     arrowKeyDirection=  1;
    change= false;
  } else {
    change= false;
  }

  if (change) updateGL();
}


void VcrossWidget::mousePressEvent(QMouseEvent* me)
{
  mousex= me->x();
  mousey= height() - me->y();

  if (me->button()==Qt::LeftButton) {
    if (startTimeGraph) {
      vcrossm->setTimeGraphPos(mousex,mousey);
      startTimeGraph= false;
      timeGraph= true;
    } else {
      dorubberband= true;
      savebackground= true;
      firstx= mousex;
      firsty= mousey;
    }
  } else if (me->button()==Qt::MidButton) {
    dopanning= true;
    firstx= mousex;
    firsty= mousey;
  } else if (me->button()==Qt::RightButton) {
    vcrossm->increasePart();
  }

  updateGL();
}


void VcrossWidget::mouseMoveEvent(QMouseEvent* me)
{
  if (dorubberband) {
    mousex= me->x();
    mousey= height() - me->y();
    updateGL();
  } else if (dopanning) {
    mousex= me->x();
    mousey= height() - me->y();
    vcrossm->movePart(firstx-mousex, firsty-mousey);
    firstx= mousex;
    firsty= mousey;
    updateGL();
  }
}


void VcrossWidget::mouseReleaseEvent(QMouseEvent* me)
{
  const int rubberlimit= 15;

  if (dorubberband) {
    mousex= me->x();
    mousey= height() - me->y();
    if (abs(firstx-mousex)>rubberlimit ||
	abs(firsty-mousey)>rubberlimit)
      vcrossm->decreasePart(firstx,firsty,mousex,mousey);
    dorubberband= false;
    savebackground= false;
    updateGL();
  } else if (dopanning) {
//    mousex= me->x();
//    mousey= height() - me->y();
//    vcrossm->movePart(firstx-mousex, firsty-mousey);
    dopanning= false;
    updateGL();
  }
}


void VcrossWidget::enableTimeGraph(bool on)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG("on=" << on);

  timeGraph= false;
  startTimeGraph= on;
}


bool VcrossWidget::saveRasterImage(const std::string& fname,
			           const std::string& format,
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

void VcrossWidget::print(QPrinter *qprt, const printOptions& priop)
{
#if defined(USE_PAINTGL)
  print(qprt, priop);
#else
  print(priop);
#endif
}

void VcrossWidget::print(const printOptions& priop)
{
  startHardcopy(priop);
  updateGL();
  endHardcopy();
  updateGL();
}

// start hardcopy plot
void VcrossWidget::startHardcopy(const printOptions& po){
  makeCurrent();
  vcrossm->startHardcopy(po);
}


// end hardcopy plot
void VcrossWidget::endHardcopy(){
  makeCurrent();
  vcrossm->endHardcopy();
}
