/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017 met.no

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

#ifndef DIPAINTGLWIDGET_H
#define DIPAINTGLWIDGET_H 1

#include "diPaintGLPainter.h"
#include <QWidget>
#include <memory>

class UiEventHandler;
class Paintable;
class QImage;

class DiPaintGLWidget : public QWidget
{
  Q_OBJECT;

public:
  DiPaintGLWidget(Paintable* paintable, UiEventHandler* interactive, QWidget* parent, bool antialiasing = false);
  ~DiPaintGLWidget();

protected:
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void keyPressEvent(QKeyEvent *ke) override;
  void keyReleaseEvent(QKeyEvent *ke) override;
  void mousePressEvent(QMouseEvent* me) override;
  void mouseMoveEvent(QMouseEvent* me) override;
  void mouseReleaseEvent(QMouseEvent* me) override;
  void mouseDoubleClickEvent(QMouseEvent* me) override;
  void wheelEvent(QWheelEvent *we) override;

private:
  void paint(QPainter& painter);
  void dropBackgroundBuffer();

protected:
  std::unique_ptr<DiPaintGLCanvas> glcanvas;
  std::unique_ptr<DiPaintGLPainter> glpainter;
  Paintable* paintable;
  UiEventHandler* interactive;
  QImage* background_buffer;
  bool antialiasing;
};

#endif // DIPAINTGLWIDGET_H
