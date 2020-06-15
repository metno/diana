/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2020 met.no

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

//#define DEBUGPRINT

#include "diRasterFillCell.h"

#include "diColour.h"
#include "diColourShading.h"
#include "diField/diField.h"
#include "diGLPainter.h"
#include "diGlUtilities.h"
#include "diPaintGLPainter.h"
#include "diPlotOptions.h"
#include "diRasterPlot.h"
#include "diUtilities.h"
#include "util/misc_util.h"
#include "util/plotoptions_util.h"
#include "util/string_util.h"

#include <mi_fieldcalc/math_util.h>

#include <QPainter>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#define MILOGGER_CATEGORY "diana.RasterFillCell"
#include <miLogger/miLogging.h>

RasterFillCell::RasterFillCell(const PlotArea& pa, Field_cp f, PlotOptions& po)
    : RasterGrid(pa, f)
    , poptions(po)
{
  if (poptions.palettecolours.empty())
    poptions.palettecolours = ColourShading::getColourShading("standard");
}

void RasterFillCell::colorizePixel(QRgb& pixel, const diutil::PointI& i)
{
  const int nx = field->area.nx;
  const float value = field->data[diutil::index(nx, i.x(), i.y())];
  const Colour* c = 0;
  if (value != fieldUndef)
    c = colourForValue(value);
  if (c != 0)
    pixel = qRgba(c->R(), c->G(), c->B(), poptions.alpha);
}

const Colour* RasterFillCell::colourForValue(float value) const
{
  if (value < poptions.minvalue || value > poptions.maxvalue)
    return 0;

  const int npalettecolours = poptions.palettecolours.size(), npalettecolours_cold = poptions.palettecolours_cold.size();

  if (poptions.linevalues.empty()) {
    if (poptions.lineinterval <= 0)
      return 0;
    int index = int((value - poptions.base) / poptions.lineinterval);
    if (value > poptions.base) {
      if (npalettecolours == 0)
        return 0;
      index = diutil::find_index(poptions.repeat, npalettecolours, index);
      return &poptions.palettecolours[index];
    } else if (npalettecolours_cold != 0) {
      index = diutil::find_index(poptions.repeat, npalettecolours_cold, -index);
      return &poptions.palettecolours_cold[index];
    } else {
      if (npalettecolours == 0)
        return 0;
      // Use index 0...
      return &poptions.palettecolours[0];
    }
  } else {
    if (npalettecolours == 0)
      return 0;
    std::vector<float>::const_iterator it = std::lower_bound(poptions.linevalues.begin(), poptions.linevalues.end(), value);
    if (it == poptions.linevalues.begin())
      return 0;
    int index = std::min(int(it - poptions.linevalues.begin()), npalettecolours) - 1;
    return &poptions.palettecolours[index];
  }
  return 0;
}
