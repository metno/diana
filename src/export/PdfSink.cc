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

#include "PdfSink.h"

#include "diPrintOptions.h"
#include "util/subprocess.h"

#include <QPageSize>
#include <QPdfWriter>
#include <QPicture>

#define MILOGGER_CATEGORY "diana.bdiana" // FIXME
#include <miLogger/miLogging.h>

namespace {
QSize initialize_qt_default_dpi()
{
  QPicture picture;
  QPainter painter(&picture);
  QPaintDevice* d = painter.device();
  return QSize(d->logicalDpiX(), d->logicalDpiY());
}
const QSize& qt_default_dpi()
{
  static const QSize qdd = initialize_qt_default_dpi();
  return qdd;
}
int qt_default_dpi_x()
{
  return qt_default_dpi().width();
}
} // namespace

// ########################################################################

PdfSink::PdfSink(const QSize& xysize, const QString& filename)
    : pageSize_(xysize)
    , resultfilename_(filename)
    , firstpage_(true)
{
  METLIBS_LOG_SCOPE(LOGVAL(resultfilename_.toStdString()));
  if (isPS() || isEPS()) {
    tmp.create();
    pdffilename_ = tmp.filePath("diana.pdf");
  } else {
    pdffilename_ = resultfilename_;
  }

  const int dpi = qt_default_dpi_x();

  pdfwriter_.reset(new QPdfWriter(pdffilename_));
  pdfwriter_->setPageSize(QPageSize(pageSize_ * (72 / qreal(dpi)), QPageSize::Point, "Custom"));
  pdfwriter_->setPageMargins(QMarginsF());
  pdfwriter_->setResolution(dpi);

  painter_.begin(pdfwriter_.get());
  painter_.setClipRect(QRectF(QPointF(0, 0), pageSize_));
}

PdfSink::~PdfSink()
{
}

bool PdfSink::isPrinting()
{
  return true;
}

bool PdfSink::isPS() const
{
  return resultfilename_.endsWith(".ps");
}

bool PdfSink::isEPS() const
{
  return resultfilename_.endsWith(".eps");
}

bool PdfSink::beginPage()
{
  if (!firstpage_) {
    if (isEPS())
      return false;
    if (!pdfwriter_->newPage())
      return false;
  }
  firstpage_ = false;
  return true;
}

QPainter& PdfSink::paintPage()
{
  return painter_;
}

bool PdfSink::endPage()
{
  return true;
}

bool PdfSink::finish()
{
  painter_.end();
  pdfwriter_.reset(0);
  const bool ps = isPS(), eps = isEPS();
  if (ps || eps) {
    QStringList args;
    if (eps)
      args << "-eps";
    args << pdffilename_ << resultfilename_;
    return (diutil::execute("pdftops", args) == 0);
  }
  return true;
}
