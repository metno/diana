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
#include <qapplication.h>
#include <qimage.h>
#include <qtGLwidget.h>
#include <diController.h>

//#include <sgiimage.h>
#include <math.h>
#include <iostream>
#include <qpixmap.h>
#include <qcursor.h>
#include <paint_cursor.xpm>


// GLwidget constructor
GLwidget::GLwidget(Controller* c,  const QGLFormat fmt,
		   QWidget* parent, const char* name)
  : QGLWidget(fmt, parent, name), contr(c), fbuffer(0),
    curcursor(keep_it)
{
  setFocusPolicy(QWidget::StrongFocus);
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
      setCursor(arrowCursor);
      break;
    case edit_move_cursor:
//    setCursor(crossCursor);
      setCursor(arrowCursor);
      break;
    case edit_value_cursor:
      setCursor(upArrowCursor);
      break;
    case draw_cursor:
      setCursor(pointingHandCursor);
      break;
    case paint_select_cursor:
      setCursor(upArrowCursor);
      break;
    case paint_move_cursor:
      setCursor(sizeAllCursor);
      break;
    case paint_draw_cursor:
      setCursor(QCursor(QPixmap(paint_cursor_xpm),0,16));
      break;
    case normal_cursor:
    default:
      setCursor(arrowCursor);
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
    if (me->button()==LeftButton)
      mev.button= leftButton;
    else if (me->button()==MidButton)
      mev.button= midButton;
    else if (me->button()==RightButton)
      mev.button= rightButton;
    else
      mev.button= noButton;
  } else {
    // if mouse-move event, button found in state-mask
    // NB: we only keep one button when moving!
    if (me->state() & LeftButton)
      mev.button= leftButton;
    else if (me->state() & MidButton)
      mev.button= midButton;
    else if (me->state() & RightButton)
      mev.button= rightButton;
    else
      mev.button= noButton;
  }

  // set modifier
  if (me->state() & ShiftButton)
    mev.modifier= key_Shift;
  else if (me->state() & AltButton)
    mev.modifier= key_Alt;
  else if (me->state() & ControlButton)
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

  if (me->state() & ShiftButton)
    kev.modifier= key_Shift;
  else if (me->state() & AltButton)
    kev.modifier= key_Alt;
  else if (me->state() & ControlButton)
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
  keymap[Key_unknown]=   key_unknown;
  keymap[Key_Escape]=   key_Escape;            // misc keys
  keymap[Key_Tab]=     key_Tab;
  keymap[Key_Backtab]=   key_Backtab;
  keymap[Key_Backspace]=   key_Backspace;
  keymap[Key_Return]=   key_Return;
  keymap[Key_Enter]=   key_Enter;
  keymap[Key_Insert]=   key_Insert;
  keymap[Key_Delete]=   key_Delete;
  keymap[Key_Pause]=   key_Pause;
  keymap[Key_Print]=   key_Print;
  keymap[Key_SysReq]=   key_SysReq;
  keymap[Key_Home]=   key_Home;              // cursor movement
  keymap[Key_End]=   key_End;
  keymap[Key_Left]=   key_Left;
  keymap[Key_Up]=   key_Up;
  keymap[Key_Right]=   key_Right;
  keymap[Key_Down]=   key_Down;
  keymap[Key_Prior]= key_Prior;
  keymap[Key_PageUp]= key_PageUp;
  keymap[Key_Next]= key_Next;
  keymap[Key_PageDown] = key_PageDown;
  keymap[Key_Shift]=   key_Shift;             // modifiers
  keymap[Key_Control]=   key_Control;
  keymap[Key_Meta]=   key_Meta;
  keymap[Key_Alt]=   key_Alt;
  keymap[Key_CapsLock]=   key_CapsLock;
  keymap[Key_NumLock]=   key_NumLock;
  keymap[Key_ScrollLock]=   key_ScrollLock;
  keymap[Key_F1]=   key_F1;                // function keys
  keymap[Key_F2]=   key_F2;
  keymap[Key_F3]=   key_F3;
  keymap[Key_F4]=   key_F4;
  keymap[Key_F5]=   key_F5;
  keymap[Key_F6]=   key_F6;
  keymap[Key_F7]=   key_F7;
  keymap[Key_F8]=   key_F8;
  keymap[Key_F9]=   key_F9;
  keymap[Key_F10]=   key_F10;
  keymap[Key_F11]=   key_F11;
  keymap[Key_F12]=   key_F12;
  keymap[Key_F13]=   key_F13;
  keymap[Key_F14]=   key_F14;
  keymap[Key_F15]=   key_F15;
  keymap[Key_F16]=   key_F16;
  keymap[Key_F17]=   key_F17;
  keymap[Key_F18]=   key_F18;
  keymap[Key_F19]=   key_F19;
  keymap[Key_F20]=   key_F20;
  keymap[Key_F21]=   key_F21;
  keymap[Key_F22]=   key_F22;
  keymap[Key_F23]=   key_F23;
  keymap[Key_F24]=   key_F24;
  keymap[Key_F25]=   key_F25;               // F25 .. F35 only on X11
  keymap[Key_F26]=   key_F26;
  keymap[Key_F27]=   key_F27;
  keymap[Key_F28]=   key_F28;
  keymap[Key_F29]=   key_F29;
  keymap[Key_F30]=   key_F30;
  keymap[Key_F31]=   key_F31;
  keymap[Key_F32]=   key_F32;
  keymap[Key_F33]=   key_F33;
  keymap[Key_F34]=   key_F34;
  keymap[Key_F35]=   key_F35;
  keymap[Key_Super_L]=   key_Super_L;           // extra keys
  keymap[Key_Super_R]=   key_Super_R;
  keymap[Key_Menu]=   key_Menu;
  keymap[Key_Hyper_L]=   key_Hyper_L;
  keymap[Key_Hyper_R]=   key_Hyper_R;
  keymap[Key_Space]=   key_Space;               // 7 bit printable ASCII
  keymap[Key_Any]=   key_Any;
  keymap[Key_Exclam]=   key_Exclam;
  keymap[Key_QuoteDbl]=   key_QuoteDbl;
  keymap[Key_NumberSign]=   key_NumberSign;
  keymap[Key_Dollar]=   key_Dollar;
  keymap[Key_Percent]=   key_Percent;
  keymap[Key_Ampersand]=   key_Ampersand;
  keymap[Key_Apostrophe]=   key_Apostrophe;
  keymap[Key_ParenLeft]=   key_ParenLeft;
  keymap[Key_ParenRight]=   key_ParenRight;
  keymap[Key_Asterisk]=   key_Asterisk;
  keymap[Key_Plus]=   key_Plus;
  keymap[Key_Comma]=   key_Comma;
  keymap[Key_Minus]=   key_Minus;
  keymap[Key_Period]=   key_Period;
  keymap[Key_Slash]=   key_Slash;
  keymap[Key_0]=   key_0;
  keymap[Key_1]=   key_1;
  keymap[Key_2]=   key_2;
  keymap[Key_3]=   key_3;
  keymap[Key_4]=   key_4;
  keymap[Key_5]=   key_5;
  keymap[Key_6]=   key_6;
  keymap[Key_7]=   key_7;
  keymap[Key_8]=   key_8;
  keymap[Key_9]=   key_9;
  keymap[Key_Colon]=   key_Colon;
  keymap[Key_Semicolon]=   key_Semicolon;
  keymap[Key_Less]=   key_Less;
  keymap[Key_Equal]=   key_Equal;
  keymap[Key_Greater]=   key_Greater;
  keymap[Key_Question]=   key_Question;
  keymap[Key_At]=   key_At;
  keymap[Key_A]=   key_A;
  keymap[Key_B]=   key_B;
  keymap[Key_C]=   key_C;
  keymap[Key_D]=   key_D;
  keymap[Key_E]=   key_E;
  keymap[Key_F]=   key_F;
  keymap[Key_G]=   key_G;
  keymap[Key_H]=   key_H;
  keymap[Key_I]=   key_I;
  keymap[Key_J]=   key_J;
  keymap[Key_K]=   key_K;
  keymap[Key_L]=   key_L;
  keymap[Key_M]=   key_M;
  keymap[Key_N]=   key_N;
  keymap[Key_O]=   key_O;
  keymap[Key_P]=   key_P;
  keymap[Key_Q]=   key_Q;
  keymap[Key_R]=   key_R;
  keymap[Key_S]=   key_S;
  keymap[Key_T]=   key_T;
  keymap[Key_U]=   key_U;
  keymap[Key_V]=   key_V;
  keymap[Key_W]=   key_W;
  keymap[Key_X]=   key_X;
  keymap[Key_Y]=   key_Y;
  keymap[Key_Z]=   key_Z;
  keymap[Key_BracketLeft]=   key_BracketLeft;
  keymap[Key_Backslash]=   key_Backslash;
  keymap[Key_BracketRight]=   key_BracketRight;
  keymap[Key_AsciiCircum]=   key_AsciiCircum;
  keymap[Key_Underscore]=   key_Underscore;
  keymap[Key_QuoteLeft]=   key_QuoteLeft;
  keymap[Key_BraceLeft]=   key_BraceLeft;
  keymap[Key_Bar]=   key_Bar;
  keymap[Key_BraceRight]=   key_BraceRight;
  keymap[Key_AsciiTilde]=   key_AsciiTilde;

// Latin 1 codes adapted from X: keysymdef.h]=   // Latin 1 codes adapted from X: keysymdef.h;v 1.21 94/08/28 16:17:06

  keymap[Key_nobreakspace]=   key_nobreakspace;
  keymap[Key_exclamdown]=   key_exclamdown;
  keymap[Key_cent]=   key_cent;
  keymap[Key_sterling]=   key_sterling;
  keymap[Key_currency]=   key_currency;
  keymap[Key_yen]=   key_yen;
  keymap[Key_brokenbar]=   key_brokenbar;
  keymap[Key_section]=   key_section;
  keymap[Key_diaeresis]=   key_diaeresis;
  keymap[Key_copyright]=   key_copyright;
  keymap[Key_ordfeminine]=   key_ordfeminine;
  keymap[Key_guillemotleft]=   key_guillemotleft;      // left angle quotation mark
  keymap[Key_notsign]=   key_notsign;
  keymap[Key_hyphen]=   key_hyphen;
  keymap[Key_registered]=   key_registered;
  keymap[Key_macron]=   key_macron;
  keymap[Key_degree]=   key_degree;
  keymap[Key_plusminus]=   key_plusminus;
  keymap[Key_twosuperior]=   key_twosuperior;
  keymap[Key_threesuperior]=   key_threesuperior;
  keymap[Key_acute]=   key_acute;
  keymap[Key_mu]=   key_mu;
  keymap[Key_paragraph]=   key_paragraph;
  keymap[Key_periodcentered]=   key_periodcentered;
  keymap[Key_cedilla]=   key_cedilla;
  keymap[Key_onesuperior]=   key_onesuperior;
  keymap[Key_masculine]=   key_masculine;
  keymap[Key_guillemotright]=   key_guillemotright;     // right angle quotation mark
  keymap[Key_onequarter]=   key_onequarter;
  keymap[Key_onehalf]=   key_onehalf;
  keymap[Key_threequarters]=   key_threequarters;
  keymap[Key_questiondown]=   key_questiondown;
  keymap[Key_Agrave]=   key_Agrave;
  keymap[Key_Aacute]=   key_Aacute;
  keymap[Key_Acircumflex]=   key_Acircumflex;
  keymap[Key_Atilde]=   key_Atilde;
  keymap[Key_Adiaeresis]=   key_Adiaeresis;
  keymap[Key_Aring]=   key_Aring;
  keymap[Key_AE]=   key_AE;
  keymap[Key_Ccedilla]=   key_Ccedilla;
  keymap[Key_Egrave]=   key_Egrave;
  keymap[Key_Eacute]=   key_Eacute;
  keymap[Key_Ecircumflex]=   key_Ecircumflex;
  keymap[Key_Ediaeresis]=   key_Ediaeresis;
  keymap[Key_Igrave]=   key_Igrave;
  keymap[Key_Iacute]=   key_Iacute;
  keymap[Key_Icircumflex]=   key_Icircumflex;
  keymap[Key_Idiaeresis]=   key_Idiaeresis;
  keymap[Key_ETH]=   key_ETH;
  keymap[Key_Ntilde]=   key_Ntilde;
  keymap[Key_Ograve]=   key_Ograve;
  keymap[Key_Oacute]=   key_Oacute;
  keymap[Key_Ocircumflex]=   key_Ocircumflex;
  keymap[Key_Otilde]=   key_Otilde;
  keymap[Key_Odiaeresis]=   key_Odiaeresis;
  keymap[Key_multiply]=   key_multiply;
  keymap[Key_Ooblique]=   key_Ooblique;
  keymap[Key_Ugrave]=   key_Ugrave;
  keymap[Key_Uacute]=   key_Uacute;
  keymap[Key_Ucircumflex]=   key_Ucircumflex;
  keymap[Key_Udiaeresis]=   key_Udiaeresis;
  keymap[Key_Yacute]=   key_Yacute;
  keymap[Key_THORN]=   key_THORN;
  keymap[Key_ssharp]=   key_ssharp;
  keymap[Key_agrave]=   key_agrave;
  keymap[Key_aacute]=   key_aacute;
  keymap[Key_acircumflex]=   key_acircumflex;
  keymap[Key_atilde]=   key_atilde;
  keymap[Key_adiaeresis]=   key_adiaeresis;
  keymap[Key_aring]=   key_aring;
  keymap[Key_ae]=   key_ae;
  keymap[Key_ccedilla]=   key_ccedilla;
  keymap[Key_egrave]=   key_egrave;
  keymap[Key_eacute]=   key_eacute;
  keymap[Key_ecircumflex]=   key_ecircumflex;
  keymap[Key_ediaeresis]=   key_ediaeresis;
  keymap[Key_igrave]=   key_igrave;
  keymap[Key_iacute]=   key_iacute;
  keymap[Key_icircumflex]=   key_icircumflex;
  keymap[Key_idiaeresis]=   key_idiaeresis;
  keymap[Key_eth]=   key_eth;
  keymap[Key_ntilde]=   key_ntilde;
  keymap[Key_ograve]=   key_ograve;
  keymap[Key_oacute]=   key_oacute;
  keymap[Key_ocircumflex]=   key_ocircumflex;
  keymap[Key_otilde]=   key_otilde;
  keymap[Key_odiaeresis]=   key_odiaeresis;
  keymap[Key_division]=   key_division;
  keymap[Key_oslash]=   key_oslash;
  keymap[Key_ugrave]=   key_ugrave;
  keymap[Key_uacute]=   key_uacute;
  keymap[Key_ucircumflex]=   key_ucircumflex;
  keymap[Key_udiaeresis]=   key_udiaeresis;
  keymap[Key_yacute]=   key_yacute;
  keymap[Key_thorn]=   key_thorn;
  keymap[Key_ydiaeresis]=   key_ydiaeresis;
}

