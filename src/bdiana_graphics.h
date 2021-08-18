/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2021 met.no

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

#ifndef BDIANA_OUTPUT_H
#define BDIANA_OUTPUT_H

#include "bdiana_source.h"
#include "diPaintGLPainter.h"
#include "diPrintOptions.h"
#include "export/ImageSink.h"
#include "export/ImageSource.h"
#include "export/MovieMaker.h"

#include <puTools/miTime.h>

#include <QPainter>
#include <QPicture>
#include <QtSvg>

#include <memory>

class BdianaGraphics
{
public:
  BdianaGraphics();
  ~BdianaGraphics();

  bool setBufferSize(int width, int height);
  bool enableMultiPlot(int rows, int columns, float spacing, float margin);
  void disableMultiPlot();
  bool setPlotCell(int row, int col);
  void setOutputFile(const std::string& filename);
  bool render(BdianaSource& src);
  void endOutput();

private:
  bool isPrinting() const;
  bool isSvg() const;
  bool isRaster() const;
  bool isAnimation() const;
  bool render(ImageSource* is, ImageSink* sink);
  void createSink(const QSize& size);
  void beginMultiplePage();
  void endMultiplePage();

  std::string outputfilename_;
  std::string movieFormat;

  bool antialias;
  QSize buffersize;        // total pixmap size
  bool multiple_plots;     // multiple plots per page
  bool multiple_beginpage; // start new page for multiple plots
  bool multiple_endpage;   // need to end page for multiple plots
  int numcols, numrows;    // for multiple plots
  int plotcol, plotrow;    // current plotcell for multiple plots
  QSize cellsize;          // width and height of plotcells
  int margin, spacing;     // margin and spacing for multiple plots

  std::vector<std::string> lines_;

  std::unique_ptr<ImageSink> sink;
};

#endif // BDIANA_OUTPUT_H
