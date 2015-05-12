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
#ifndef VCROSSQTWIDGET_H
#define VCROSSQTWIDGET_H

#include "diColour.h"
#include "vcross_v2/VcrossQtManager.h"

//#define VCROSS_GLWIDGET 1
#ifdef VCROSS_GLWIDGET
#include <QGLWidget>
#define VCROSS_GL(gl,nogl) gl
#else
#include <QWidget>
#define VCROSS_GL(gl,nogl) nogl
#endif

class QKeyEvent;
class QMouseEvent;
class QPrinter;

namespace vcross {

/**
   \brief The widget for displaying vertical crossections.

   Handles widget paint/redraw events.
   Receives mouse and keybord events and initiates actions.
*/
#ifdef VCROSS_GLWIDGET // cannot use VCROSS_GL here because moc-qt4 does not understand
class QtWidget : public QGLWidget
#else
class QtWidget : public QWidget
#endif
{
  Q_OBJECT;

public:
  QtWidget(QWidget* parent = 0);
  ~QtWidget();

  void setVcrossManager(QtManager_p vcm);

  void print(QPrinter& printer);
  bool saveRasterImage(const QString& fname);

protected:
  virtual void paintEvent(QPaintEvent* event);
  virtual void resizeEvent(QResizeEvent* event);

  virtual void keyPressEvent(QKeyEvent *me);
  virtual void mousePressEvent(QMouseEvent* me);
  virtual void mouseMoveEvent(QMouseEvent* me);
  virtual void mouseReleaseEvent(QMouseEvent* me);

Q_SIGNALS:
  void stepTime(int direction);
  void stepCrossection(int direction);
  void mouseOverText(const QString& text);

private Q_SLOTS:
  void switchedTimeGraph(bool on);

private:
  QtManager_p vcrossm;

  bool dorubberband;   // while zooming
  bool dopanning;

  int arrowKeyDirection;
  int firstx, firsty, mousex, mousey;
};

} // namespace vcross

#endif // VCROSSQTWIDGET_H
