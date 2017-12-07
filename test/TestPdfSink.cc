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

#include <export/PdfSink.h>
#include <export/QPictureSink.h>

#include <QPainter>
#include <QPdfWriter>
#include <QPicture>

#include <gtest/gtest.h>

#define MILOGGER_CATEGORY "diana.test.PdfSink"
#include "miLogger/miLogging.h"

namespace {
const int w = 800, h = 600;
const QSize wh(w, h);
}

TEST(TestPdfSink, MultiplePages)
{
  const QString filename = TEST_BUILDDIR "/testpdfsink_multipage.pdf";

  PdfSink pdf(wh, filename);

  EXPECT_TRUE(pdf.beginPage());
  pdf.paintPage().drawText(100, 100, "page 1, line from bottom-left to top-right");
  pdf.paintPage().drawLine(w, 0, 0, h);
  EXPECT_TRUE(pdf.endPage());

  EXPECT_TRUE(pdf.beginPage());
  pdf.paintPage().drawText(200, 200, "page 2, line from top-left to bottom-right");
  pdf.paintPage().drawLine(0, 0, w, h);
  EXPECT_TRUE(pdf.endPage());

  EXPECT_TRUE(pdf.finish());

  EXPECT_TRUE(QFile(filename).exists());
}

TEST(TestPdfSink, Picture)
{
  const QString filename = TEST_BUILDDIR "/testpdfsink_picture.pdf";

  QPictureSink pic(wh, true);
  pic.beginPage();
  pic.paintPage().setBrush(QColor(255, 0, 0, 128));
  pic.paintPage().drawRect(QRect(QPoint(), wh));
  pic.endPage();
  pic.finish();
  EXPECT_EQ(w, pic.picture().boundingRect().width());

  PdfSink pdf(wh, filename);
  EXPECT_TRUE(pdf.beginPage());
  pdf.paintPage().drawPicture(0, 0, pic.picture());

  pdf.paintPage().setPen(QPen(QColor(0, 0, 255, 128), 8));
  pdf.paintPage().setBrush(Qt::NoBrush);
  pdf.paintPage().drawRect(QRect(QPoint(), wh));
  EXPECT_TRUE(pdf.endPage());

  EXPECT_TRUE(pdf.finish());

  EXPECT_TRUE(QFile(filename).exists());
}

TEST(TestPdfSink, PostScript)
{
  const QString filename = TEST_BUILDDIR "/testpdfsink_postscript.ps";

  PdfSink pdf(wh, filename);

  EXPECT_TRUE(pdf.beginPage());
  pdf.paintPage().drawText(100, 100, "this is postscript");
  pdf.paintPage().drawLine(w, 0, 0, h);
  EXPECT_TRUE(pdf.endPage());

  EXPECT_TRUE(pdf.finish());

  EXPECT_TRUE(QFile(filename).exists()) << "this test requires pdftops";
}

TEST(TestPdfSink, EncapsulatedPostScript)
{
  const QString filename = TEST_BUILDDIR "/testpdfsink_postscript.eps";

  PdfSink pdf(wh, filename);

  EXPECT_TRUE(pdf.beginPage());
  pdf.paintPage().drawText(100, 100, "this is encapsulated postscript");
  pdf.paintPage().drawLine(w, 0, 0, h);
  EXPECT_TRUE(pdf.endPage());

  EXPECT_TRUE(pdf.finish());

  EXPECT_TRUE(QFile(filename).exists()) << "this test requires pdftops";
}
