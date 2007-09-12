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
#ifndef _qtPrintManager_h
#define _qtPrintManager_h


#include <qprinter.h>
#include <diPrintOptions.h>

using namespace std; 


// conversion from Qt-printing types to diana-types
d_print::Orientation getOrientation(QPrinter::Orientation ori);
d_print::ColourOption getColourMode(QPrinter::ColorMode cm);
d_print::PageSize getPageSize(QPrinter::PageSize ps);

// fill printOptions from QPrinter-selections
void fillPrintOption(const QPrinter* qp, printOptions& priop);

#endif
