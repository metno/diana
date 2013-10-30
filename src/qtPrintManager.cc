/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2006 met.no

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


// conversion from Qt-printing types to diana-types
d_print::Orientation getOrientation(QPrinter::Orientation ori)
{
  d_print::Orientation dori= d_print::ori_automatic;

  if (ori==QPrinter::Landscape)
    dori= d_print::ori_landscape;
  else if (ori==QPrinter::Portrait)
    dori= d_print::ori_portrait;

  return dori;
}

d_print::ColourOption getColourMode(QPrinter::ColorMode cm)
{
  d_print::ColourOption dcol= d_print::incolour;

  if (cm==QPrinter::Color)
    dcol= d_print::incolour;
  else if (cm==QPrinter::GrayScale){
    dcol= d_print::greyscale;
  }

  return dcol;
}

d_print::PageSize getPageSize(QPrinter::PageSize ps)
{
  d_print::PageSize dps= d_print::A4;

  if (ps==QPrinter::A4){
    dps= d_print::A4;
  } else if (ps==QPrinter::A3){
    dps= d_print::A3;
  } else if (ps==QPrinter::A2){
    dps= d_print::A2;
  } else if (ps==QPrinter::A1){
    dps= d_print::A1;
  } else if (ps==QPrinter::A0){
    dps= d_print::A0;
  }

  return dps;
}

// conversion from diana-types to Qt-printing types
QPrinter::Orientation getQPOrientation(d_print::Orientation ori)
{
  QPrinter::Orientation dori = QPrinter::Portrait;

  if (ori==d_print::ori_landscape)
    dori= QPrinter::Landscape;
  else if (ori==d_print::ori_portrait)
    dori= QPrinter::Portrait;

  return dori;
}

QPrinter::ColorMode getQPColourMode(d_print::ColourOption cm)
{

  QPrinter::ColorMode dcol = QPrinter::Color;

  if (cm==d_print::incolour)
    dcol= QPrinter::Color;
  else if (cm==d_print::greyscale){
    dcol= QPrinter::GrayScale;
  }

  return dcol;
}

QPrinter::PageSize getQPPageSize(d_print::PageSize ps)
{
  QPrinter::PageSize dps= QPrinter::A4;

  if (ps==d_print::A4){
    dps= QPrinter::A4;
  } else if (ps==d_print::A3){
    dps= QPrinter::A3;
  } else if (ps==d_print::A2){
    dps= QPrinter::A2;
  } else if (ps==d_print::A1){
    dps= QPrinter::A1;
  } else if (ps==d_print::A0){
    dps= QPrinter::A0;
  }

  return dps;
}


// fill printOptions from QPrinter-selections
void toPrintOption(const QPrinter& qp, printOptions& priop)
{
  priop.printer= (qp.outputFileName().isNull()?"":qp.printerName().toStdString());
  priop.colop= getColourMode(qp.colorMode());
  if (priop.colop==d_print::greyscale) priop.drawbackground= false;

  priop.orientation= getOrientation(qp.orientation());
  priop.pagesize= getPageSize(qp.pageSize());
}

// set QPrinter-selections from printOptions
void fromPrintOption(QPrinter& qp, printOptions& priop)
{
  if (not priop.printer.empty())
    qp.setPrinterName(QString::fromStdString(priop.printer));
  qp.setColorMode(getQPColourMode(priop.colop));
  qp.setOrientation(getQPOrientation(priop.orientation));
  qp.setPageSize(getQPPageSize(priop.pagesize));
}

