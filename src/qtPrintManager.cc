/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2016 met.no

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
/* Created at Fri Nov  2 10:22:02 2001 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtPrintManager.h"

#include <QPrinter>

#define MILOGGER_CATEGORY "diana.PrintManager"
#include <qUtilities/miLoggingQt.h>

namespace {

// conversion from Qt-printing types to diana-types
d_print::Orientation getOrientation(QPrinter::Orientation ori)
{
  if (ori == QPrinter::Landscape)
    return d_print::ori_landscape;
  else if (ori==QPrinter::Portrait)
    return d_print::ori_portrait;
  else
    return d_print::ori_automatic;
}

d_print::ColourOption getColourMode(QPrinter::ColorMode cm)
{
  if (cm==QPrinter::GrayScale)
    return d_print::greyscale;
  else
    return d_print::incolour;
}

d_print::PageSize getPageSize(QPrinter::PageSize ps)
{
  if (ps==QPrinter::A4)
    return d_print::A4;
  else if (ps==QPrinter::A3)
    return d_print::A3;
  else if (ps==QPrinter::A2)
    return d_print::A2;
  else if (ps==QPrinter::A1)
    return d_print::A1;
  else if (ps==QPrinter::A0)
    return d_print::A0;
  else
    return d_print::A4;
}

// conversion from diana-types to Qt-printing types
QPrinter::Orientation getQPOrientation(d_print::Orientation ori)
{
  if (ori==d_print::ori_landscape)
    return QPrinter::Landscape;
  else
    return QPrinter::Portrait;
}

QPrinter::ColorMode getQPColourMode(d_print::ColourOption cm)
{
  if (cm==d_print::greyscale)
    return QPrinter::GrayScale;
  else
    return QPrinter::Color;
}

QPrinter::PageSize getQPPageSize(d_print::PageSize ps)
{
  if (ps==d_print::A4)
    return QPrinter::A4;
  else if (ps==d_print::A3)
    return QPrinter::A3;
  else if (ps==d_print::A2)
    return QPrinter::A2;
  else if (ps==d_print::A1)
    return QPrinter::A1;
  else if (ps==d_print::A0)
    return QPrinter::A0;
  else
    return QPrinter::A4;
}

std::ostream& operator<<(std::ostream& out, const printOptions& priop)
{
  out << "[printOptions printer='" << priop.printer << "'"
      << ' ' << (priop.colop == d_print::incolour? "color" : "grayscale")
      << ' ' << (priop.orientation==d_print::ori_portrait ? "portrait" : "landscape")
      << ']';
  return out;
}

} // anonymous namespace

namespace d_print {

// fill printOptions from QPrinter-selections
void toPrintOption(const QPrinter& qp, printOptions& priop)
{
  METLIBS_LOG_SCOPE();
  priop.printer = qp.printerName().toStdString();
  priop.colop = getColourMode(qp.colorMode());

  priop.orientation= getOrientation(qp.orientation());
  priop.pagesize= getPageSize(qp.pageSize());
  METLIBS_LOG_DEBUG(priop);
}

// set QPrinter-selections from printOptions
void fromPrintOption(QPrinter& qp, printOptions& priop)
{
  METLIBS_LOG_SCOPE(priop);
  if (not priop.printer.empty())
    qp.setPrinterName(QString::fromStdString(priop.printer));
  qp.setColorMode(getQPColourMode(priop.colop));
  qp.setOrientation(getQPOrientation(priop.orientation));
  qp.setPageSize(getQPPageSize(priop.pagesize));
}

} // namespace d_print
