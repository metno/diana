/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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

#include "diPaintable.h"
#include "diMapMode.h"
#include "diPrintOptions.h"

#include <diField/diArea.h>

#include <QMouseEvent>
#include <QKeyEvent>

#include <vector>
#include <string>
#include <map>

class Controller;
class DiGLPainter;

/**
   \brief the map OpenGL widget

   the map OpenGL widget supporting
   - simple underlay
   - keyboard/mouse event translation to Diana types
*/
class GLwidget : public QObject, public DiPaintable
{
  Q_OBJECT

public:
  GLwidget(Controller*);
  ~GLwidget();

  /// toggles use of underlay
  void forceUnderlay(bool b)
    {savebackground= b;}

Q_SIGNALS:
  /// single click signal
  void mouseGridPos(QMouseEvent* me);
  /// single click signal (right mouse button)
  void mouseRightPos(QMouseEvent* me);
  /// mouse movement, no buttons
  void mouseMovePos(QMouseEvent* me, bool quick);
  void objectsChanged();
  void fieldsChanged();
  /// key press
  void keyPress(QKeyEvent* ke);
  /// mouse double click
  void mouseDoubleClick(QMouseEvent* me);

  void changeCursor(cursortype);

public:
  void setCanvas(DiCanvas* canvas);
  void paint(DiPainter* gl);
  void resize(int width, int height);

  bool handleKeyEvents(QKeyEvent*);
  bool handleMouseEvents(QMouseEvent*);
  bool handleWheelEvents(QWheelEvent *we);

private:
  void drawUnderlay(DiGLPainter* gl);
  void drawOverlay(DiGLPainter* gl);

private:
  Controller* contr;       // gate to main system
  int plotw, ploth;        // size of widget (pixels)

  bool savebackground;     // use fake overlay
  bool useSavedUnderlay;
  typedef unsigned int    GLuint;     /* 4-byte unsigned */
  GLuint *fbuffer;         // fake overlay buffer
  std::map<int,KeyType> keymap; // keymap's for keyboardevents
};

#endif
