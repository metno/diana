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

#include "PrinterDialog.h"

#include "export/ImageSource.h"
#include "qtPrintManager.h"
#include "qtUtility.h"

#include <QPainter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>

PrinterDialog::PrinterDialog(QWidget* parent, ImageSource* is, printOptions* priop)
    : parent_(parent)
    , imageSource_(is)
    , priop_(priop)
    , printer_(new QPrinter)

{
  if (priop_)
    d_print::fromPrintOption(*printer_, *priop);
}

PrinterDialog::~PrinterDialog()
{
  if (priop_)
    d_print::toPrintOption(*printer_, *priop_);
}

void PrinterDialog::print()
{
  QPrintDialog printerDialog(printer_, parent_);
  if (printerDialog.exec() == QDialog::Accepted && printer_->isValid()) {
    diutil::OverrideCursor waitCursor;
    paintOnPrinter(printer_);
  }
}

void PrinterDialog::preview()
{
  QPrintPreviewDialog previewDialog(printer_, parent_);
  connect(&previewDialog, &QPrintPreviewDialog::paintRequested, this, &PrinterDialog::paintOnPrinter);
  previewDialog.exec();
}

void PrinterDialog::paintOnPrinter(QPrinter* p)
{
  QPainter painter(p);
  imageSource_->prepare(true, true);
  imageSource_->paint(painter);
  painter.end();
  imageSource_->finish();
}
