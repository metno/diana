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
#ifndef VCROSSWIDGET_H
#define VCROSSWIDGET_H

#include <qglobal.h>

#if !defined(USE_PAINTGL)
#include <qgl.h>
#else
#include "PaintGL/paintgl.h"
#endif

#include <diColour.h>

class printOptions;
class VcrossManager;
class QKeyEvent;
class QMouseEvent;

/**
   \brief The OpenGL widget for Vertical Crossections

   Handles widget paint/redraw events.
   Receives mouse and keybord events and initiates actions.
*/
class VcrossWidget : public
#if !defined(USE_PAINTGL)
QGLWidget
#else
PaintGLWidget
#endif // USE_PAINTGL
{
  Q_OBJECT;

public:
  VcrossWidget(VcrossManager *vcm, QWidget* parent = 0);
  ~VcrossWidget();

  void enableTimeGraph(bool on);

  bool saveRasterImage(const std::string& fname, const std::string& format, const int quality = -1);

  /** print using either given QPrinter (if USE_PAINTGL) or using the given printOptions */
  void print(QPrinter* qprt, const printOptions& priop);

  /** make hardcopy using the given printOptions */
  void print(const printOptions& priop);

protected:
  virtual void initializeGL();
  virtual void paintGL();
  virtual void resizeGL( int w, int h );

  virtual void keyPressEvent(QKeyEvent *me);
  virtual void mousePressEvent(QMouseEvent* me);
  virtual void mouseMoveEvent(QMouseEvent* me);
  virtual void mouseReleaseEvent(QMouseEvent* me);

private:
  void startHardcopy(const printOptions& po);
  void endHardcopy();

private:
  VcrossManager *vcrossm;

  int plotw, ploth;   // size of widget (pixels)
  GLuint *fbuffer;    // fake overlay buffer (only while zooming)
  float glx1,gly1,glx2,gly2; // overlay buffer area
  Colour rubberbandColour;   // contrast to background

  bool savebackground; // while zooming
  bool dorubberband;   // while zooming
  bool dopanning;

  int arrowKeyDirection;
  int firstx, firsty, mousex, mousey;

  bool timeGraph;
  bool startTimeGraph;

signals:
  void timeChanged(int);
  void crossectionChanged(int);
};

#endif
