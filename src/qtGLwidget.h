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
#ifndef _qtGLwidget_h
#define _qtGLwidget_h

#include <qglobal.h>

#if !defined(USE_PAINTGL)
#include <qgl.h>
#else
#include <QWidget>
#endif

#include <QMouseEvent>
#include <QKeyEvent>

#include <vector>
#include <string>
#include <map>

#include <diField/diArea.h>
#include <diMapMode.h>
#include <diPrintOptions.h>

#if defined(USE_PAINTGL)
#include <GL/gl.h>
#include "PaintGL/paintgl.h"
#endif

class Controller;
#if defined(USE_PAINTGL)
class PaintGLContext;
class QPrinter;
#define QGLWidget PaintGLWidget
#endif

/**
   \brief the map OpenGL widget

   the map OpenGL widget supporting
   - simple underlay
   - keyboard/mouse event translation to Diana types

*/
class GLwidget : public QGLWidget {

  Q_OBJECT

public:
#if !defined(USE_PAINTGL)
  GLwidget(Controller*, const QGLFormat,
           QWidget*);
#else
  GLwidget(Controller*, QWidget*);
#endif
  ~GLwidget();

  /// save contents of widget as raster image
  bool saveRasterImage(const std::string fname,
		       const std::string format,
		       const int quality = -1);

  /// toggles use of underlay
  void forceUnderlay(bool b)
  {savebackground= b;}

  /// start hardcopy plot
  void startHardcopy(const printOptions& po);
  /// end hardcopy plot
  void endHardcopy();

signals:
  /// single click signal
  void mouseGridPos(QMouseEvent* me);
  /// single click signal (right mouse button)
  void mouseRightPos(QMouseEvent* me);
  /// mouse movement, no buttons
  void mouseMovePos(QMouseEvent* me, bool quick);
  /// profet grid objects changed
  void gridAreaChanged();
  void objectsChanged();
  void fieldsChanged();
  /// key press
  void keyPress(QKeyEvent* ke);
  /// mouse double click
  void mouseDoubleClick(QMouseEvent* me);

protected:

  void initializeGL();
  void paintGL();
  void editPaint(bool drawb= true);
  void resizeGL(int width, int height);

  void changeCursor(const cursortype);

  void handleMouseEvents(QMouseEvent*);
  void handleKeyEvents(QKeyEvent*);

  void wheelEvent(QWheelEvent*);
  void keyPressEvent(QKeyEvent*);
  void keyReleaseEvent(QKeyEvent*);
  void mousePressEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void mouseDoubleClickEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);

private:
  cursortype curcursor;    // current cursor
  bool savebackground;     // use fake overlay
  Controller* contr;       // gate to main system
  int plotw, ploth;        // size of widget (pixels)
  GLuint *fbuffer;         // fake overlay buffer
  std::map<int,KeyType> keymap; // keymap's for keyboardevents
};

#endif
