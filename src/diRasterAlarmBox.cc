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

#include "diRasterAlarmBox.h"

#include "diColour.h"
#include "diColourShading.h"
#include "diField/VcrossUtil.h" // minimize + maximize
#include "diGLPainter.h"
#include "diPlotOptions.h"
#include "util/misc_util.h"
#include "util/plotoptions_util.h"
#include "util/string_util.h"

#include <mi_fieldcalc/math_util.h>

#include <puTools/miStringFunctions.h>

#include <QPainter>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#define MILOGGER_CATEGORY "diana.RasterAlarmBox"
#include <miLogger/miLogging.h>

RasterAlarmBox::RasterAlarmBox(const PlotArea& pa, Field_cp f, const PlotOptions& po)
    : pa_(pa)
    , field(f)
    , poptions(po)
    , valid(false)
{
  vmin = -fieldUndef;
  vmax = fieldUndef;

  const std::vector<int>& fcl = poptions.forecastLength;

  const size_t sf = fcl.size(), s1 = poptions.forecastValueMin.size(), s2 = poptions.forecastValueMax.size();
  METLIBS_LOG_DEBUG(LOGVAL(sf) << LOGVAL(s1) << LOGVAL(s2));
  if (sf > 1) {
    const int fc = field->forecastHour;
    std::vector<int>::const_iterator it = std::lower_bound(fcl.begin(), fcl.end(), fc);
    if (it == fcl.begin() || it == fcl.end())
      return;
    const int i = (it - fcl.begin());
    const float r1 = float(fcl[i] - fc) / float(fcl[i] - fcl[i - 1]);
    const float r2 = 1 - r1;
    METLIBS_LOG_DEBUG(LOGVAL(i) << LOGVAL(r1) << LOGVAL(r2));

    if (sf == s1)
      vmin = r1 * poptions.forecastValueMin[i - 1] + r2 * poptions.forecastValueMin[i];
    if (sf == s2)
      vmax = r1 * poptions.forecastValueMax[i - 1] + r2 * poptions.forecastValueMax[i];
  } else if (sf == 0 && s1 == 0 && s2 == 0) {
    vmin = poptions.minvalue;
    vmax = poptions.maxvalue;
    METLIBS_LOG_DEBUG("no fc values" << LOGVAL(vmin) << LOGVAL(vmax));
  } else {
    METLIBS_LOG_ERROR("ERROR in setup!");
    return;
  }

  const Colour& ca = poptions.linecolour;
  c_alarm = qRgba(ca.R(), ca.G(), ca.B(), 255);

  valid = true;
}

void RasterAlarmBox::pixelQuad(const diutil::PointI& s, const diutil::PointD& pxy00, const diutil::PointD& pxy10, const diutil::PointD& pxy01,
                               const diutil::PointD& pxy11, int n)
{
  // analyse unscaled data!
  // FIXME box is too large

  QRgb* pixels = RasterPlot::pixels(s);
  const int nx = field->area.nx, ny = field->area.ny;
  const QRgb c_ok = 0;

  const diutil::PointD pdxy0 = (pxy10 - pxy00) / n;
  const diutil::PointD pdxy1 = (pxy11 - pxy01) / n;

  const diutil::PointD fxy0(field->area.R().x1, field->area.R().y1);
  const diutil::PointD resi = 1.0 / diutil::PointD(field->area.resolutionX, field->area.resolutionY);
  for (int i = 0; i < n; ++i) {
    const diutil::PointD c00 = (pxy00 + pdxy0 * i - fxy0) * resi;
    const diutil::PointD c10 = (pxy00 + pdxy0 * (i + 1) - fxy0) * resi;
    const diutil::PointD c01 = (pxy01 + pdxy1 * i - fxy0) * resi;
    const diutil::PointD c11 = (pxy01 + pdxy1 * (i + 1) - fxy0) * resi;

    const float SAFETY = 0.1f;
    int ix_min = int(c00.x() - SAFETY);
    miutil::minimize(ix_min, int(c10.x() - SAFETY));
    miutil::minimize(ix_min, int(c01.x() - SAFETY));
    miutil::minimize(ix_min, int(c11.x() - SAFETY));
    int ix_max = int(c00.x() + SAFETY);
    miutil::maximize(ix_max, int(c10.x() + SAFETY));
    miutil::maximize(ix_max, int(c01.x() + SAFETY));
    miutil::maximize(ix_max, int(c11.x() + SAFETY));

    int iy_min = int(c00.y() - SAFETY);
    miutil::minimize(iy_min, int(c10.y() - SAFETY));
    miutil::minimize(iy_min, int(c01.y() - SAFETY));
    miutil::minimize(iy_min, int(c11.y() - SAFETY));
    int iy_max = int(c00.y() + SAFETY);
    miutil::maximize(iy_max, int(c10.y() + SAFETY));
    miutil::maximize(iy_max, int(c01.y() + SAFETY));
    miutil::maximize(iy_max, int(c11.y() + SAFETY));

    miutil::maximize(ix_min, 0);
    miutil::minimize(ix_max, nx - 1);
    miutil::maximize(iy_min, 0);
    miutil::minimize(iy_max, ny - 1);

    bool is_alarm = false;
    // this box is a square in the field projection, but we need only the points
    // inside pxy00..pxy11
    for (int iyy = iy_min; !is_alarm && iyy <= iy_max; ++iyy) {
      for (int ixx = ix_min; !is_alarm && ixx <= ix_max; ++ixx) {
        const float v = field->data[ixx + iyy * nx];
        if (v != fieldUndef && v >= vmin && v <= vmax)
          is_alarm = true;
      }
    }
    pixels[i] = is_alarm ? c_alarm : c_ok;
  }
}
