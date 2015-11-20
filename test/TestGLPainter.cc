/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015 met.no

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

#include <diOpenGLPainter.h>
#include <diOpenGLWidget.h>

#include <diPaintable.h>
#include <diPaintGLPainter.h>
#include <diPaintGLWidget.h>

#include <diColour.h>

#include <QImage>

#include <gtest/gtest.h>

#define MILOGGER_CATEGORY "diana.test.GLPainter"
#include "miLogger/miLogging.h"

namespace {

class TestPaintable : public DiPaintable {
public:
  TestPaintable();

  void setCanvas(DiCanvas* c) { canvas = c; }
  void resize(int w, int h);
  void paintUnderlay(DiPainter* painter);
  void paintOverlay(DiPainter* painter);

  DiCanvas* canvas;
  bool text, lines;
  int width, height;
};

TestPaintable::TestPaintable()
  : canvas(0), text(false), lines(false), width(0), height(0)
{
}

void TestPaintable::resize(int w, int h)
{
  width = w;
  height = h;
}

void TestPaintable::paintUnderlay(DiPainter* painter)
{
  DiGLPainter* gl = static_cast<DiGLPainter*>(painter);

  const int SIG1SYMBOL = 248, SIG2SYMBOL = 249;
  //const QLatin1Char SIG1SYMBOL(248), SIG2SYMBOL(249);

  gl->Ortho(-1, 1, -1, 1, -1, 1);
  gl->canvas()->setVpGlSize(width, height, 2, 2);

  if (lines) {
    gl->setLineStyle(Colour(0, 0, 255), 2);
    gl->drawLine(-0.5, 0.5, 0.5, -0.5);
  }
  if (text) {
    gl->setFont("METSYMBOLFONT", 10);
    gl->drawChar(SIG1SYMBOL, -0.5, -0.5, 0);
    gl->drawChar(SIG2SYMBOL,  0.5,  0.5, 0);
  }
}

void TestPaintable::paintOverlay(DiPainter*)
{
}

} // namespace

TEST(TestOpenGL, Text)
{
  TestPaintable* tp = new TestPaintable();
  DiOpenGLWidget* w = new DiOpenGLWidget(tp);
  w->resize(1200, 900);
  w->show();
  QImage snapshot = w->grabFrameBuffer(true);
  delete w;
  delete tp;

  snapshot.save(TEST_BUILDDIR "/opengl_text.png");
}

TEST(TestPaintGL, Text)
{
  TestPaintable* tp = new TestPaintable();
  DiPaintGLWidget* w = new DiPaintGLWidget(tp, 0);
  w->resize(1200, 900);
  w->show();
  QImage snapshot(w->size(), QImage::Format_ARGB32);
  w->render(&snapshot);
  delete w;
  delete tp;

  snapshot.save(TEST_BUILDDIR "/paintgl_text.png");
}
