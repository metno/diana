/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2018 met.no

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

#include "diana_config.h"

#include "bdiana_graphics.h"

#include "export/ExportUtil.h"
#include "export/PdfSink.h"
#include "export/QPictureSink.h"
#include "export/RasterFileSink.h"
#include "export/SvgFileSink.h"
#include "util/string_util.h"

#include <cmath>

#define MILOGGER_CATEGORY "diana.bdiana" // FIXME
#include <miLogger/miLogging.h>

BdianaGraphics::BdianaGraphics()
    : image_type(image_auto)
    , antialias(false)
    , buffersize(1696, 1200)
    , multiple_plots(false)
    , multiple_beginpage(false)
    , multiple_endpage(false)
{
}

BdianaGraphics::~BdianaGraphics()
{
  endOutput();
}

bool BdianaGraphics::render(BdianaSource& src)
{
  METLIBS_LOG_SCOPE(LOGVAL(multiple_plots) << LOGVAL(cellsize.width()) << LOGVAL(cellsize.height()));
  bool data_ok = true;

  if (src.hasCutout()) {
    QPictureSink pic(buffersize, isPrinting());
    data_ok = render(src.imageSource(), &pic);

    const QRectF& ac = src.cutout();
    createSink(ac.size().toSize());
    if (!sink)
      return false;
    sink->beginPage();
    sink->paintPage().drawPicture(ac.topLeft(), pic.picture());
    sink->endPage();
  } else {
    if (!sink)
      createSink(buffersize);
    if (!sink)
      return false;

    if (!multiple_plots) {
      data_ok = render(src.imageSource(), sink.get());
      if (!isAnimation())
        endOutput();
    } else {
      QPictureSink pic(cellsize, isPrinting());
      data_ok = render(src.imageSource(), &pic);

      const QPoint pos(margin + plotcol * (cellsize.width() + spacing), buffersize.height() - (margin + (plotrow + 1) * (cellsize.height() + spacing)));
      beginMultiplePage();
      sink->paintPage().drawPicture(pos, pic.picture());
    }
  }
  return data_ok;
}

bool BdianaGraphics::render(ImageSource* is, ImageSink* sink)
{
  const bool data_ok = true; // FIXME
  diutil::renderSingle(*is, *sink);
  return data_ok;
}

bool BdianaGraphics::isPrinting() const
{
  if (image_type == image_auto) {
    return (diutil::endswith(outputfilename_, ".pdf") || diutil::endswith(outputfilename_, ".ps") || diutil::endswith(outputfilename_, ".eps"));
  } else {
    return (image_type == image_pdf || image_type == image_ps || image_type == image_eps);
  }
}

bool BdianaGraphics::isSvg() const
{
  if (image_type == image_auto) {
    return (diutil::endswith(outputfilename_, ".svg"));
  } else {
    return image_type == image_svg;
  }
}

bool BdianaGraphics::isRaster() const
{
  if (image_type == image_auto) {
    return (diutil::endswith(outputfilename_, ".png") || diutil::endswith(outputfilename_, ".jpeg") || diutil::endswith(outputfilename_, ".jpg") ||
            diutil::endswith(outputfilename_, ".bmp"));
  } else {
    return image_type == image_raster;
  }
}

bool BdianaGraphics::isAnimation() const
{
  if (image_type == image_auto) {
    return (diutil::endswith(outputfilename_, ".mp4") || diutil::endswith(outputfilename_, ".mpg") || diutil::endswith(outputfilename_, ".avi") ||
            diutil::endswith(outputfilename_, ".gif"));
  } else {
    return image_type == image_movie;
  }
}

void BdianaGraphics::createSink(const QSize& size)
{
  endOutput();

  QString filename = QString::fromStdString(outputfilename_);
  // FIXME almost same as start of ExportImageDialog::saveSingle
  if (isPrinting()) {
    sink.reset(new PdfSink(size, filename));
  } else if (isSvg()) {
    sink.reset(new SvgFileSink(size, filename));
  } else if (isRaster()) {
    sink.reset(new RasterFileSink(size, filename));
  } else if (isAnimation()) {
    QString fmt = "AVI";
    if (image_type == image_auto) {
      if (diutil::endswith(outputfilename_, ".mp4"))
        fmt = "MP4";
      else if (diutil::endswith(outputfilename_, ".mpg"))
        fmt = "MPG";
      else if (diutil::endswith(outputfilename_, ".gif"))
        fmt = MovieMaker::format_animated;
    } else if (image_type == image_movie) {
      fmt = QString::fromStdString(movieFormat);
    }
    const float framerate = 0.2f;
    sink.reset(new MovieMaker(filename, fmt, framerate, size));
  } else {
    METLIBS_LOG_ERROR("could not create sink for '" << outputfilename_ << "'");
  }
}

bool BdianaGraphics::setBufferSize(int width, int height)
{
  if (width < 1 || height < 1)
    return false;

  if (!buffersize.isValid() || width != buffersize.width() || height != buffersize.height()) {
    endOutput();
    buffersize = QSize(width, height);
  }

  return true;
}

bool BdianaGraphics::enableMultiPlot(int rows, int columns, float fspacing, float fmargin)
{
  METLIBS_LOG_SCOPE(LOGVAL(rows) << LOGVAL(columns));
  if (rows < 1 || columns < 1) {
    disableMultiPlot();
    return false;
  }
  if (!buffersize.isValid()) {
    METLIBS_LOG_WARN("need buffer size before enabling multiplot");
    return false;
  }
  if (fspacing >= 100 || fspacing < 0) {
    METLIBS_LOG_WARN("illegal value " << fspacing << " for multiplot spacing");
    fspacing = 0;
  }
  if (fmargin >= 100 || fmargin < 0) {
    METLIBS_LOG_WARN("illegal value " << fmargin << " for multiplot margin");
    fmargin = 0;
  }

  endMultiplePage();

  numrows = rows;
  numcols = columns;
  margin = int(buffersize.width() * fmargin / 100.0);
  spacing = int(buffersize.width() * fspacing / 100.0);
  cellsize =
      QSize((buffersize.width() - 2 * margin - (numcols - 1) * spacing) / numcols, (buffersize.height() - 2 * margin - (numrows - 1) * spacing) / numrows);
  multiple_plots = true;
  multiple_beginpage = true;
  plotcol = plotrow = 0;

  return true;
}

void BdianaGraphics::disableMultiPlot()
{
  endMultiplePage();

  multiple_plots = false;
  multiple_beginpage = false;
  multiple_endpage = false;
}

bool BdianaGraphics::setPlotCell(int row, int col)
{
  if (!multiple_plots)
    return 1;
  if (row < 0 || row >= numrows || col < 0 || col >= numcols)
    return false;

  // row 0 should be on top of page
  plotrow = numrows - 1 - row;
  plotcol = col;
  return true;
}

void BdianaGraphics::setImageType(image_type_t it)
{
  if (it != image_type) {
    endOutput();
    image_type = it;
  }
}

void BdianaGraphics::setOutputFile(const std::string& filename)
{
  if (filename != outputfilename_) {
    endOutput();
    outputfilename_ = filename;
  }
}

void BdianaGraphics::setMovieFormat(const std::string& fmt)
{
  if (image_type != image_movie || movieFormat != fmt) {
    endOutput();
    image_type = image_movie;
    movieFormat = fmt;
  }
}

void BdianaGraphics::beginMultiplePage()
{
  METLIBS_LOG_SCOPE(LOGVAL(multiple_beginpage));
  if (sink && multiple_beginpage) {
    multiple_beginpage = false;
    multiple_endpage = true;
    sink->beginPage();
  }
}

void BdianaGraphics::endMultiplePage()
{
  METLIBS_LOG_SCOPE(LOGVAL(multiple_endpage));
  if (sink && multiple_plots && multiple_endpage) {
    multiple_endpage = false;
    sink->endPage();
  }
}

void BdianaGraphics::endOutput()
{
  METLIBS_LOG_SCOPE();
  endMultiplePage();
  if (sink) {
    sink->finish();
    sink.reset(0);
  }
}
