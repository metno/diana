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

#include <qgl.h>

#include <QMouseEvent>
#include <QKeyEvent>

#include <vector>
#include <puTools/miString.h>
#include <map>

#include <diField/diArea.h>
#include <diMapMode.h>
#include <diPrintOptions.h>

using namespace std;

class Controller;

/**
   \brief the map OpenGL widget

   the map OpenGL widget supporting
   - simple underlay
   - keyboard/mouse event translation to Diana types

*/

class GLwidget : public QGLWidget {

  Q_OBJECT

public:
  GLwidget(Controller*, const QGLFormat,
	   QWidget*);
  ~GLwidget();

  /// save contents of widget as raster image
  bool saveRasterImage(const miString fname,
		       const miString format,
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
  void mouseGridPos(const mouseEvent mev);
  /// single click signal (right mouse button)
  void mouseRightPos(const mouseEvent mev);
  /// mouse movement, no buttons
  void mouseMovePos(const mouseEvent mev, bool quick);
  /// profet grid objects changed
  void gridAreaChanged();
  void objectsChanged();
  void fieldsChanged();
  /// key press
  void keyPress(const keyboardEvent kev);

protected:

  void initializeGL();
  void paintGL();
  void editPaint(bool drawb= true);
  void resizeGL(int w, int h);

  void buildKeyMap();
  void fillMouseEvent(const QMouseEvent*,mouseEvent&);
  void changeCursor(const cursortype);

  void handleMouseEvents(QMouseEvent*,const mouseEventType);
  void handleKeyEvents(QKeyEvent*,const keyboardEventType);

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
  map<int,KeyType> keymap; // keymap's for keyboardevents
};

#endif







