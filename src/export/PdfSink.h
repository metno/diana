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

#ifndef EXPORT_PDFSINK_H
#define EXPORT_PDFSINK_H

#include "ImageSink.h"

#include "qtTempDir.h"

#include <QPainter>
#include <QSize>

#include <memory>

class printOptions;
class QPdfWriter;

class PdfSink : public ImageSink
{
public:
  // PrinterSink(const QSize& xysize, const printOptions& priop);
  PdfSink(const QSize& xysize, const QString& filename);
  ~PdfSink();

  bool isPrinting() override;
  bool beginPage() override;
  QPainter& paintPage() override;
  bool endPage() override;

  bool finish() override;

private:
  void createPrinter();
  bool isPS() const;
  bool isEPS() const;

private:
  QSize pageSize_;
  TempDir tmp;
  QString resultfilename_;
  QString pdffilename_;
  std::unique_ptr<QPdfWriter> pdfwriter_;
  QPainter painter_;
  bool firstpage_;
};

#endif // EXPORT_PDFSINK_H
