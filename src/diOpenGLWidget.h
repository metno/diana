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

#ifndef DIOPENGLWIDGET_H
#define DIOPENGLWIDGET_H 1

#include "diOpenGLPainter.h"
#include <QGLWidget>
#include <memory>

class UiEventHandler;
class Paintable;

class DiOpenGLWidget : public QGLWidget
{
public:
  DiOpenGLWidget(Paintable* p, UiEventHandler* i, QWidget* parent = 0);
  ~DiOpenGLWidget();

  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int w, int h) override;

  void keyPressEvent(QKeyEvent *ke) override;
  void keyReleaseEvent(QKeyEvent *ke) override;
  void mousePressEvent(QMouseEvent* me) override;
  void mouseMoveEvent(QMouseEvent* me) override;
  void mouseReleaseEvent(QMouseEvent* me) override;
  void mouseDoubleClickEvent(QMouseEvent* me) override;
  void wheelEvent(QWheelEvent *we) override;

private:;
  void paintUnderlay();
  void paintOverlay();
  void dropBackgroundBuffer();

private:
  std::unique_ptr<DiOpenGLCanvas> glcanvas;
  std::unique_ptr<DiOpenGLPainter> glpainter;
  Paintable* paintable;
  UiEventHandler* interactive;
  DiGLPainter::GLuint *buffer_data;
};

#endif // DIOPENGLWIDGET_H
