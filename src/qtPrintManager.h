/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2014 met.no

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
#ifndef _qtPrintManager_h
#define _qtPrintManager_h

#include "diPrintOptions.h"

#include <qprinter.h>

// conversion from Qt-printing types to diana-types
d_print::Orientation getOrientation(QPrinter::Orientation ori);
d_print::ColourOption getColourMode(QPrinter::ColorMode cm);
d_print::PageSize getPageSize(QPrinter::PageSize ps);

// conversion from diana printing-types to Qt types
QPrinter::Orientation getQPOrientation(d_print::Orientation ori);
QPrinter::ColorMode getQPColourMode(d_print::ColourOption cm);
QPrinter::PageSize getQPPageSize(d_print::PageSize ps);

// fill printOptions from QPrinter-selections
void toPrintOption(const QPrinter& qp, printOptions& priop);

// set QPrinter-selections from printOptions
void fromPrintOption(QPrinter& qp, printOptions& priop);

#endif
