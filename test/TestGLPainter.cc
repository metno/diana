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

#include <QCoreApplication>
#include <QImage>
#include <QTimer>

#include <gtest/gtest.h>

#define MILOGGER_CATEGORY "diana.test.GLPainter"
#include "miLogger/miLogging.h"

#define TEST_FONTDIR TEST_SRCDIR "/../share/diana/fonts/"

namespace {

void wait(int ms)
{
  QTimer timer;
  timer.start(ms + 10);
  QCoreApplication::processEvents(QEventLoop::AllEvents, ms);
}

class TestPaintable : public DiPaintable {
public:
  TestPaintable();

  virtual void setCanvas(DiCanvas* c);
  void resize(int w, int h);
  void paintUnderlay(DiPainter* painter);
  void paintOverlay(DiPainter* painter);

  void drawTextInBox(DiGLPainter* gl, const QString& text, float tx, float ty);

  DiCanvas* canvas;
  bool text, lines;
  int width, height;
};

TestPaintable::TestPaintable()
  : canvas(0), text(true), lines(true), width(0), height(0)
{
}

void TestPaintable::setCanvas(DiCanvas* c)
{
  DiPaintable::setCanvas(c);
  c->defineFont("BITMAPFONT",    TEST_FONTDIR "Vera.ttf",       "NORMAL", true);
  c->defineFont("SCALEFONT" ,    TEST_FONTDIR "Vera.ttf",       "NORMAL", false);
  c->defineFont("METSYMBOLFONT", TEST_FONTDIR "metsymbols.ttf", "NORMAL", false);
}

void TestPaintable::resize(int w, int h)
{
  width = w;
  height = h;
}

void TestPaintable::paintUnderlay(DiPainter* painter)
{
  DiGLPainter* gl = static_cast<DiGLPainter*>(painter);

  const QString SIG1SYMBOL(QLatin1Char(248)), SIG2SYMBOL(QLatin1Char(249));

  const float C = -10;
  gl->LoadIdentity();
  gl->Ortho(-C, C, -C, C, -1, 1);
  gl->canvas()->setVpGlSize(width, height, 2*C, 2*C);

  gl->ClearColor(1, 1, 1, 1);
  gl->Clear(DiGLPainter::gl_COLOR_BUFFER_BIT | DiGLPainter::gl_DEPTH_BUFFER_BIT | DiGLPainter::gl_STENCIL_BUFFER_BIT);

  if (lines) {
    gl->setLineStyle(Colour(0, 0, 255), 2);
    gl->drawLine(-C/2, C/2, C/2, -C/2);
  }
  if (text) {
    gl->setFont("METSYMBOLFONT", 200);
    drawTextInBox(gl, SIG1SYMBOL, -C/2, -C/2);
    drawTextInBox(gl, SIG2SYMBOL,  C/2,  C/2);

    gl->setFont("BITMAPFONT", 100);
    drawTextInBox(gl, "Sky?", -C/2,  C/2);
    drawTextInBox(gl, "air",   C/2, -C/2);
  }
}

void TestPaintable::drawTextInBox(DiGLPainter* gl, const QString& text, float tx, float ty)
{
  float x, y, w, h;
  gl->canvas()->getTextRect(text, x, y, w, h);
  x += tx;
  y += ty;
  gl->setLineStyle(Colour(0, 255, 0), 1);
  gl->drawRect(false, x, y, x+w, y+h);
  gl->drawText(text, tx, ty, 0);

  gl->setLineStyle(Colour(255, 0, 0), 1);
  gl->drawCross(tx, ty, 0.2, true);
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
  wait(200);
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
