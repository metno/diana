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
//#define DEBUGREDRAW

#include <fstream>

#include <QApplication>
#include <QImage>
#include <qtGLwidget.h>
#include <QMouseEvent>
#include <QKeyEvent>
#include <diController.h>

#include <math.h>
#include <iostream>
#include <qpixmap.h>
#include <qcursor.h>
#include <paint_cursor.xpm>
#include <paint_add_crusor.xpm>
#include <paint_remove_crusor.xpm>
#include <paint_forbidden_crusor.xpm>


// GLwidget constructor
GLwidget::GLwidget(Controller* c,  const QGLFormat fmt,
		   QWidget* parent)
  : QGLWidget(fmt, parent), contr(c), fbuffer(0),
    curcursor(keep_it)
{
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);

  savebackground= false;
  
  buildKeyMap();

  // sets default cursor in widget
  changeCursor(normal_cursor);
}


//  Release allocated resources
GLwidget::~GLwidget()
{
  delete[] fbuffer;
}


void GLwidget::paintGL(){

#ifdef DEBUGPRINT
  cerr << "paintGL()" << endl;
#endif

#ifdef DEBUGREDRAW
  cerr<<"GLwidget::paintGL ... plot under"<<endl;
#endif
  if (contr) contr->plot(true,false); // draw underlay

  if (savebackground){
#ifdef DEBUGREDRAW
    cerr<<"GLwidget::paintGL ... savebackground"<<endl;
#endif
    if(!fbuffer) fbuffer= new GLuint[4*plotw*ploth];

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

  editPaint(false);
}


void GLwidget::editPaint(bool drawb){
  makeCurrent();

  if (drawb && fbuffer) {
#ifdef DEBUGREDRAW
    cerr<<"GLwidget::editPaint ... drawbackground"<<endl;
#endif
    float glx1,gly1,glx2,gly2,delta;
    contr->getPlotSize(glx1,gly1,glx2,gly2);
    delta= (fabs(glx1-glx2)*0.1/plotw);
    if (delta < 0.0005) delta= 0.0005;

    glPixelZoom(1,1);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,plotw);
    glPixelStorei(GL_UNPACK_ALIGNMENT,4);
    // AC, 27.01.2004: for large glx1 and gly1, this small addition 
    // did not seem to work....
    //     glRasterPos2f(glx1+0.0001,gly1+0.0001);
    glRasterPos2f(glx1+delta,gly1+delta);
    
    glDrawPixels(plotw,ploth,
		 GL_RGBA,GL_UNSIGNED_BYTE,
		 fbuffer);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);

  }

#ifdef DEBUGREDRAW
  cerr<<"GLwidget::editPaint ... plot over"<<endl;
#endif
  if (contr) contr->plot(false,true); // draw overlay

  swapBuffers();
}


//  Set up the OpenGL rendering state
void GLwidget::initializeGL()
{
  glShadeModel( GL_FLAT );
  setAutoBufferSwap(false);
}


//  Set up the OpenGL view port, matrix mode, etc.
void GLwidget::resizeGL( int w, int h )
{
#ifdef DEBUGPRINT
  cerr << "resizeGL" << endl;
#endif
  if (contr) contr->setPlotWindow(w,h);
  
  glViewport( 0, 0, (GLint)w, (GLint)h );
  plotw= w;
  ploth= h;
  
  // make fake overlay buffer
  if (fbuffer) delete[] fbuffer;
//fbuffer= new GLuint[4*w*h];
  fbuffer=0;
}




// change mousepointer.
// See diMapMode.h for types
void GLwidget::changeCursor(const cursortype c){
  if ((c != keep_it) && (c != curcursor)){
#ifdef DEBUGPRINT
    cerr << "About to change cursor to: " << c << endl;
#endif
    switch (c){
    case edit_cursor:
//    setCursor(crossCursor);
      setCursor(Qt::ArrowCursor);
      break;
    case edit_move_cursor:
//    setCursor(crossCursor);
      setCursor(Qt::ArrowCursor);
      break;
    case edit_value_cursor:
      setCursor(Qt::UpArrowCursor);
      break;
    case draw_cursor:
      setCursor(Qt::PointingHandCursor);
      break;
    case paint_select_cursor:
      setCursor(Qt::UpArrowCursor);
      break;
    case paint_move_cursor:
      setCursor(Qt::SizeAllCursor);
      break;
    case paint_draw_cursor:
      setCursor(QCursor(QPixmap(paint_cursor_xpm),0,16));
      break;
    case paint_add_crusor:
      setCursor(QCursor(QPixmap(paint_add_crusor_xpm),7,1));
      break;
    case paint_remove_crusor:
      setCursor(QCursor(QPixmap(paint_remove_crusor_xpm),7,1));
      break;
    case paint_forbidden_crusor:
      setCursor(QCursor(QPixmap(paint_forbidden_crusor_xpm),7,1));
      break;
    case normal_cursor:
    default:
      setCursor(Qt::ArrowCursor);
      break;
    }
    curcursor= c;
  }
}


// fill mouseEvent struct (diMapMode.h) with 
// information from QMouseEvent
void GLwidget::fillMouseEvent(const QMouseEvent* me,
			      mouseEvent& mev){
  // set button
  if (mev.type != mousemove){
    // for mouse-click and release event
    if (me->button()==Qt::LeftButton)
      mev.button= leftButton;
    else if (me->button()==Qt::MidButton)
      mev.button= midButton;
    else if (me->button()==Qt::RightButton)
      mev.button= rightButton;
    else
      mev.button= noButton;
  } else {
    // if mouse-move event, button found in state-mask
    // NB: we only keep one button when moving!
    if (me->buttons() & Qt::LeftButton)
      mev.button= leftButton;
    else if (me->buttons() & Qt::MidButton)
      mev.button= midButton;
    else if (me->buttons() & Qt::RightButton)
      mev.button= rightButton;
    else
      mev.button= noButton;
  }

  // set modifier
  if (me->modifiers() & Qt::ShiftModifier)
    mev.modifier= key_Shift;
  else if (me->modifiers() & Qt::AltModifier)
    mev.modifier= key_Alt;
  else if (me->modifiers() & Qt::ControlModifier)
    mev.modifier= key_Control;
  else mev.modifier= key_unknown;

  // set position
  mev.x= me->x();
  mev.y= height() - me->y();
  mev.globalX= me->globalX();
  mev.globalY= me->globalY();
}


// Translates all Qmouseevents into mouseEvent structs (diMapMode.h)
// and sends it off to controller. Return values are checked,
// and any GUI-action taken
void GLwidget::handleMouseEvents(QMouseEvent* me,const mouseEventType met){
  mouseEvent mev;
  EventResult res;

  mev.type= met;
  fillMouseEvent(me, mev);
  
  // send event to controller
  contr->sendMouseEvent(mev, res);
  
  // check return values, and take appropriate action
  changeCursor(res.newcursor);
  savebackground= res.savebackground;

  // check if any specific GUI-action requested
  if (res.action != no_action){
    switch (res.action){
    case browsing:
      //      emit mouseMovePos(mev.x,mev.y,false);
      emit mouseMovePos(mev,false);
      break;
    case quick_browsing:
      //      emit mouseMovePos(mev.x,mev.y,true);
      emit mouseMovePos(mev,true);
      break;
    case pointclick:
      emit mouseGridPos(mev);
      break;
    case rightclick:
      emit mouseRightPos(mev);
      break;
    case objects_changed:
      emit objectsChanged();
      break;
    case fields_changed:
      emit fieldsChanged();
      break;
    case grid_area_changed:
      emit gridAreaChanged();
      break;
    }
  }

  // check if repaint requested
  if (res.repaint){
    if (res.background)
      updateGL();  // full paint 
    else
      editPaint(); // only editPaint
  }
  
}

// Translates all QKeyEvents into keyboardEvent structs (diMapMode.h)
// and sends it off to controller. Return values are checked,
// and any GUI-action taken
void GLwidget::handleKeyEvents(QKeyEvent* me,const keyboardEventType ket){
  keyboardEvent kev;
  EventResult res;

  kev.type= ket;
  kev.key= keymap[me->key()];

  // set modifier

  if (me->modifiers() & Qt::ShiftModifier)
    kev.modifier= key_Shift;
  else if (me->modifiers() & Qt::AltModifier)
    kev.modifier= key_Alt;
  else if (me->modifiers() & Qt::ControlModifier)
    kev.modifier= key_Control;
  else kev.modifier= key_unknown;

  // send event to controller
  contr->sendKeyboardEvent(kev, res);
  
  // check return values, and take appropriate action
  changeCursor(res.newcursor);
  savebackground= res.savebackground;

  // check if any specific GUI-action requested
  if (res.action != no_action){
    switch (res.action){
    case objects_changed:
      emit objectsChanged();
      break;
    case keypressed:
      emit keyPress(kev);
      break;
    default:
      break;
    }
  }
  
  // check if repaint requested
  if (res.repaint){
    if (res.background)
      updateGL();  // full paint
    else  
      editPaint(); // ..only editPaint
  }
}

// ---------------------- event callbacks -----------------

void GLwidget::wheelEvent(QWheelEvent *we)
{
  int numDegrees = we->delta() / 8;
  int numSteps = numDegrees / 15;

  if (we->orientation() == Qt::Vertical) {
    if (numSteps > 0) {
      float x1, y1, x2, y2;
      float xmap, ymap;
      
      contr->getPlotSize(x1, y1, x2, y2);
      /// (why -(y-height())? I have no idea ...)
      contr->PhysToMap(we->x(), -(we->y()-height()), xmap, ymap);
      
      int wd = static_cast<int> ((x2 - x1) / 3.);
      int hd = static_cast<int> ((y2 - y1) / 3.);

      Rectangle r(xmap - wd, ymap - hd, xmap + wd, ymap + hd);
      contr->zoomTo(r);
      updateGL();
    } else {
      contr->zoomOut();
      updateGL();
    }
  }
}

void GLwidget::keyPressEvent(QKeyEvent *me)
{
  handleKeyEvents(me,keypress);
}


void GLwidget::keyReleaseEvent(QKeyEvent *me)
{
  handleKeyEvents(me,keyrelease);
}


void GLwidget::mousePressEvent(QMouseEvent* me)
{
  handleMouseEvents(me,mousepress);
}


void GLwidget::mouseMoveEvent(QMouseEvent* me)
{
  handleMouseEvents(me,mousemove);
}


void GLwidget::mouseReleaseEvent(QMouseEvent* me)
{
  handleMouseEvents(me,mouserelease);
}


void GLwidget::mouseDoubleClickEvent(QMouseEvent* me)
{
  handleMouseEvents(me,mousedoubleclick);
}


// start hardcopy plot
void GLwidget::startHardcopy(const printOptions& po){
  makeCurrent();
  contr->startHardcopy(po);
}


// end hardcopy plot
void GLwidget::endHardcopy(){
  makeCurrent();
  contr->endHardcopy();
}


bool GLwidget::saveRasterImage(const miString fname,
			       const miString format,
			       const int quality){

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

//   int npixels, nchannels;
//   unsigned char *data;
//   unsigned long *ptr;
//   unsigned long *lbuf, pixelv;
//   GLint viewport[4];

//   updateGL();
//   makeCurrent();
//   glFlush();

//   QApplication::flushX();
// /////   QApplication::syncX();

//   updateGL();
//   makeCurrent();
//   glFlush();

//   glGetIntegerv(GL_VIEWPORT, viewport);
//   npixels = viewport[2]*viewport[3];
//   nchannels = 4;
//   data = new unsigned char[npixels*nchannels];
//   glReadPixels(0,0,viewport[2],viewport[3],
// 	       GL_RGBA, GL_UNSIGNED_BYTE,
// 	       data);
//   lbuf= new unsigned long[npixels];
//   ptr= lbuf;
//   for (int i=0; i<npixels*nchannels; i+=nchannels) {
// #ifdef linux
//     pixelv= data[i+3]<<24 | data[i+2]<<16 |
//       data[i+1]<<8 | data[i];
// #else
//     pixelv= data[i]<<24 | data[i+1]<<16 |
//       data[i+2]<<8 | data[i+3];
// #endif
//     *(ptr++)= pixelv;
//   }
  
//   cout << "Lagrer til fil:" << fname;
//   cout.flush();
//   int result= longstoimage(lbuf, viewport[2],viewport[3], nchannels,
// 			   const_cast<char*>(fname.c_str()));
//   cerr << " .." << miString(result?"Ok":" **FEILET!**") << endl;
//   delete[] lbuf;
//   delete[] data;
//   return bool(result);
}

void GLwidget::buildKeyMap(){
  keymap[Qt::Key_unknown]=   key_unknown;
  keymap[Qt::Key_Escape]=   key_Escape;            // misc keys
  keymap[Qt::Key_Tab]=     key_Tab;
  keymap[Qt::Key_Backtab]=   key_Backtab;
  keymap[Qt::Key_Backspace]=   key_Backspace;
  keymap[Qt::Key_Return]=   key_Return;
  keymap[Qt::Key_Enter]=   key_Enter;
  keymap[Qt::Key_Insert]=   key_Insert;
  keymap[Qt::Key_Delete]=   key_Delete;
  keymap[Qt::Key_Pause]=   key_Pause;
  keymap[Qt::Key_Print]=   key_Print;
  keymap[Qt::Key_SysReq]=   key_SysReq;
  keymap[Qt::Key_Home]=   key_Home;              // cursor movement
  keymap[Qt::Key_End]=   key_End;
  keymap[Qt::Key_Left]=   key_Left;
  keymap[Qt::Key_Up]=   key_Up;
  keymap[Qt::Key_Right]=   key_Right;
  keymap[Qt::Key_Down]=   key_Down;
  keymap[Qt::Key_PageUp]= key_Prior;
  keymap[Qt::Key_PageUp]= key_PageUp;
  keymap[Qt::Key_PageDown]= key_Next;
  keymap[Qt::Key_PageDown] = key_PageDown;
  keymap[Qt::Key_Shift]=   key_Shift;             // modifiers
  keymap[Qt::Key_Control]=   key_Control;
  keymap[Qt::Key_Meta]=   key_Meta;
  keymap[Qt::Key_Alt]=   key_Alt;
  keymap[Qt::Key_CapsLock]=   key_CapsLock;
  keymap[Qt::Key_NumLock]=   key_NumLock;
  keymap[Qt::Key_ScrollLock]=   key_ScrollLock;
  keymap[Qt::Key_F1]=   key_F1;                // function keys
  keymap[Qt::Key_F2]=   key_F2;
  keymap[Qt::Key_F3]=   key_F3;
  keymap[Qt::Key_F4]=   key_F4;
  keymap[Qt::Key_F5]=   key_F5;
  keymap[Qt::Key_F6]=   key_F6;
  keymap[Qt::Key_F7]=   key_F7;
  keymap[Qt::Key_F8]=   key_F8;
  keymap[Qt::Key_F9]=   key_F9;
  keymap[Qt::Key_F10]=   key_F10;
  keymap[Qt::Key_F11]=   key_F11;
  keymap[Qt::Key_F12]=   key_F12;
  keymap[Qt::Key_F13]=   key_F13;
  keymap[Qt::Key_F14]=   key_F14;
  keymap[Qt::Key_F15]=   key_F15;
  keymap[Qt::Key_F16]=   key_F16;
  keymap[Qt::Key_F17]=   key_F17;
  keymap[Qt::Key_F18]=   key_F18;
  keymap[Qt::Key_F19]=   key_F19;
  keymap[Qt::Key_F20]=   key_F20;
  keymap[Qt::Key_F21]=   key_F21;
  keymap[Qt::Key_F22]=   key_F22;
  keymap[Qt::Key_F23]=   key_F23;
  keymap[Qt::Key_F24]=   key_F24;
  keymap[Qt::Key_F25]=   key_F25;               // F25 .. F35 only on X11
  keymap[Qt::Key_F26]=   key_F26;
  keymap[Qt::Key_F27]=   key_F27;
  keymap[Qt::Key_F28]=   key_F28;
  keymap[Qt::Key_F29]=   key_F29;
  keymap[Qt::Key_F30]=   key_F30;
  keymap[Qt::Key_F31]=   key_F31;
  keymap[Qt::Key_F32]=   key_F32;
  keymap[Qt::Key_F33]=   key_F33;
  keymap[Qt::Key_F34]=   key_F34;
  keymap[Qt::Key_F35]=   key_F35;
  keymap[Qt::Key_Super_L]=   key_Super_L;           // extra keys
  keymap[Qt::Key_Super_R]=   key_Super_R;
  keymap[Qt::Key_Menu]=   key_Menu;
  keymap[Qt::Key_Hyper_L]=   key_Hyper_L;
  keymap[Qt::Key_Hyper_R]=   key_Hyper_R;
  keymap[Qt::Key_Space]=   key_Space;               // 7 bit printable ASCII
  keymap[Qt::Key_Any]=   key_Any;
  keymap[Qt::Key_Exclam]=   key_Exclam;
  keymap[Qt::Key_QuoteDbl]=   key_QuoteDbl;
  keymap[Qt::Key_NumberSign]=   key_NumberSign;
  keymap[Qt::Key_Dollar]=   key_Dollar;
  keymap[Qt::Key_Percent]=   key_Percent;
  keymap[Qt::Key_Ampersand]=   key_Ampersand;
  keymap[Qt::Key_Apostrophe]=   key_Apostrophe;
  keymap[Qt::Key_ParenLeft]=   key_ParenLeft;
  keymap[Qt::Key_ParenRight]=   key_ParenRight;
  keymap[Qt::Key_Asterisk]=   key_Asterisk;
  keymap[Qt::Key_Plus]=   key_Plus;
  keymap[Qt::Key_Comma]=   key_Comma;
  keymap[Qt::Key_Minus]=   key_Minus;
  keymap[Qt::Key_Period]=   key_Period;
  keymap[Qt::Key_Slash]=   key_Slash;
  keymap[Qt::Key_0]=   key_0;
  keymap[Qt::Key_1]=   key_1;
  keymap[Qt::Key_2]=   key_2;
  keymap[Qt::Key_3]=   key_3;
  keymap[Qt::Key_4]=   key_4;
  keymap[Qt::Key_5]=   key_5;
  keymap[Qt::Key_6]=   key_6;
  keymap[Qt::Key_7]=   key_7;
  keymap[Qt::Key_8]=   key_8;
  keymap[Qt::Key_9]=   key_9;
  keymap[Qt::Key_Colon]=   key_Colon;
  keymap[Qt::Key_Semicolon]=   key_Semicolon;
  keymap[Qt::Key_Less]=   key_Less;
  keymap[Qt::Key_Equal]=   key_Equal;
  keymap[Qt::Key_Greater]=   key_Greater;
  keymap[Qt::Key_Question]=   key_Question;
  keymap[Qt::Key_At]=   key_At;
  keymap[Qt::Key_A]=   key_A;
  keymap[Qt::Key_B]=   key_B;
  keymap[Qt::Key_C]=   key_C;
  keymap[Qt::Key_D]=   key_D;
  keymap[Qt::Key_E]=   key_E;
  keymap[Qt::Key_F]=   key_F;
  keymap[Qt::Key_G]=   key_G;
  keymap[Qt::Key_H]=   key_H;
  keymap[Qt::Key_I]=   key_I;
  keymap[Qt::Key_J]=   key_J;
  keymap[Qt::Key_K]=   key_K;
  keymap[Qt::Key_L]=   key_L;
  keymap[Qt::Key_M]=   key_M;
  keymap[Qt::Key_N]=   key_N;
  keymap[Qt::Key_O]=   key_O;
  keymap[Qt::Key_P]=   key_P;
  keymap[Qt::Key_Q]=   key_Q;
  keymap[Qt::Key_R]=   key_R;
  keymap[Qt::Key_S]=   key_S;
  keymap[Qt::Key_T]=   key_T;
  keymap[Qt::Key_U]=   key_U;
  keymap[Qt::Key_V]=   key_V;
  keymap[Qt::Key_W]=   key_W;
  keymap[Qt::Key_X]=   key_X;
  keymap[Qt::Key_Y]=   key_Y;
  keymap[Qt::Key_Z]=   key_Z;
  keymap[Qt::Key_BracketLeft]=   key_BracketLeft;
  keymap[Qt::Key_Backslash]=   key_Backslash;
  keymap[Qt::Key_BracketRight]=   key_BracketRight;
  keymap[Qt::Key_AsciiCircum]=   key_AsciiCircum;
  keymap[Qt::Key_Underscore]=   key_Underscore;
  keymap[Qt::Key_QuoteLeft]=   key_QuoteLeft;
  keymap[Qt::Key_BraceLeft]=   key_BraceLeft;
  keymap[Qt::Key_Bar]=   key_Bar;
  keymap[Qt::Key_BraceRight]=   key_BraceRight;
  keymap[Qt::Key_AsciiTilde]=   key_AsciiTilde;

// Latin 1 codes adapted from X: keysymdef.h]=   // Latin 1 codes adapted from X: keysymdef.h;v 1.21 94/08/28 16:17:06

  keymap[Qt::Key_nobreakspace]=   key_nobreakspace;
  keymap[Qt::Key_exclamdown]=   key_exclamdown;
  keymap[Qt::Key_cent]=   key_cent;
  keymap[Qt::Key_sterling]=   key_sterling;
  keymap[Qt::Key_currency]=   key_currency;
  keymap[Qt::Key_yen]=   key_yen;
  keymap[Qt::Key_brokenbar]=   key_brokenbar;
  keymap[Qt::Key_section]=   key_section;
  keymap[Qt::Key_diaeresis]=   key_diaeresis;
  keymap[Qt::Key_copyright]=   key_copyright;
  keymap[Qt::Key_ordfeminine]=   key_ordfeminine;
  keymap[Qt::Key_guillemotleft]=   key_guillemotleft;      // left angle quotation mark
  keymap[Qt::Key_notsign]=   key_notsign;
  keymap[Qt::Key_hyphen]=   key_hyphen;
  keymap[Qt::Key_registered]=   key_registered;
  keymap[Qt::Key_macron]=   key_macron;
  keymap[Qt::Key_degree]=   key_degree;
  keymap[Qt::Key_plusminus]=   key_plusminus;
  keymap[Qt::Key_twosuperior]=   key_twosuperior;
  keymap[Qt::Key_threesuperior]=   key_threesuperior;
  keymap[Qt::Key_acute]=   key_acute;
  keymap[Qt::Key_mu]=   key_mu;
  keymap[Qt::Key_paragraph]=   key_paragraph;
  keymap[Qt::Key_periodcentered]=   key_periodcentered;
  keymap[Qt::Key_cedilla]=   key_cedilla;
  keymap[Qt::Key_onesuperior]=   key_onesuperior;
  keymap[Qt::Key_masculine]=   key_masculine;
  keymap[Qt::Key_guillemotright]=   key_guillemotright;     // right angle quotation mark
  keymap[Qt::Key_onequarter]=   key_onequarter;
  keymap[Qt::Key_onehalf]=   key_onehalf;
  keymap[Qt::Key_threequarters]=   key_threequarters;
  keymap[Qt::Key_questiondown]=   key_questiondown;
  keymap[Qt::Key_Agrave]=   key_Agrave;
  keymap[Qt::Key_Aacute]=   key_Aacute;
  keymap[Qt::Key_Acircumflex]=   key_Acircumflex;
  keymap[Qt::Key_Atilde]=   key_Atilde;
  keymap[Qt::Key_Adiaeresis]=   key_Adiaeresis;
  keymap[Qt::Key_Aring]=   key_Aring;
  keymap[Qt::Key_AE]=   key_AE;
  keymap[Qt::Key_Ccedilla]=   key_Ccedilla;
  keymap[Qt::Key_Egrave]=   key_Egrave;
  keymap[Qt::Key_Eacute]=   key_Eacute;
  keymap[Qt::Key_Ecircumflex]=   key_Ecircumflex;
  keymap[Qt::Key_Ediaeresis]=   key_Ediaeresis;
  keymap[Qt::Key_Igrave]=   key_Igrave;
  keymap[Qt::Key_Iacute]=   key_Iacute;
  keymap[Qt::Key_Icircumflex]=   key_Icircumflex;
  keymap[Qt::Key_Idiaeresis]=   key_Idiaeresis;
  keymap[Qt::Key_ETH]=   key_ETH;
  keymap[Qt::Key_Ntilde]=   key_Ntilde;
  keymap[Qt::Key_Ograve]=   key_Ograve;
  keymap[Qt::Key_Oacute]=   key_Oacute;
  keymap[Qt::Key_Ocircumflex]=   key_Ocircumflex;
  keymap[Qt::Key_Otilde]=   key_Otilde;
  keymap[Qt::Key_Odiaeresis]=   key_Odiaeresis;
  keymap[Qt::Key_multiply]=   key_multiply;
  keymap[Qt::Key_Ooblique]=   key_Ooblique;
  keymap[Qt::Key_Ugrave]=   key_Ugrave;
  keymap[Qt::Key_Uacute]=   key_Uacute;
  keymap[Qt::Key_Ucircumflex]=   key_Ucircumflex;
  keymap[Qt::Key_Udiaeresis]=   key_Udiaeresis;
  keymap[Qt::Key_Yacute]=   key_Yacute;
  keymap[Qt::Key_THORN]=   key_THORN;
  keymap[Qt::Key_ssharp]=   key_ssharp;
  keymap[Qt::Key_Agrave]=   key_agrave;
  keymap[Qt::Key_Aacute]=   key_aacute;
  keymap[Qt::Key_Acircumflex]=   key_acircumflex;
  keymap[Qt::Key_Atilde]=   key_atilde;
  keymap[Qt::Key_Adiaeresis]=   key_adiaeresis;
  keymap[Qt::Key_Aring]=   key_aring;
  keymap[Qt::Key_AE]=   key_ae;
  keymap[Qt::Key_Ccedilla]=   key_ccedilla;
  keymap[Qt::Key_Egrave]=   key_egrave;
  keymap[Qt::Key_Eacute]=   key_eacute;
  keymap[Qt::Key_Ecircumflex]=   key_ecircumflex;
  keymap[Qt::Key_Ediaeresis]=   key_ediaeresis;
  keymap[Qt::Key_Igrave]=   key_igrave;
  keymap[Qt::Key_Iacute]=   key_iacute;
  keymap[Qt::Key_Icircumflex]=   key_icircumflex;
  keymap[Qt::Key_Idiaeresis]=   key_idiaeresis;
  keymap[Qt::Key_ETH]=   key_eth;
  keymap[Qt::Key_Ntilde]=   key_ntilde;
  keymap[Qt::Key_Ograve]=   key_ograve;
  keymap[Qt::Key_Oacute]=   key_oacute;
  keymap[Qt::Key_Ocircumflex]=   key_ocircumflex;
  keymap[Qt::Key_Otilde]=   key_otilde;
  keymap[Qt::Key_Odiaeresis]=   key_odiaeresis;
  keymap[Qt::Key_division]=   key_division;
  keymap[Qt::Key_Ooblique]=   key_oslash;
  keymap[Qt::Key_Ugrave]=   key_ugrave;
  keymap[Qt::Key_Uacute]=   key_uacute;
  keymap[Qt::Key_Ucircumflex]=   key_ucircumflex;
  keymap[Qt::Key_Udiaeresis]=   key_udiaeresis;
  keymap[Qt::Key_Yacute]=   key_yacute;
  keymap[Qt::Key_THORN]=   key_thorn;
  keymap[Qt::Key_ydiaeresis]=   key_ydiaeresis;
}

