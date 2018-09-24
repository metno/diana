/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2018 met.no

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

#include "diVprofBoxVerticalWind.h"

#include "diColour.h"
#include "diField/VcrossUtil.h"
#include "diField/diPoint.h"
#include "diLinetype.h"
#include "diUtilities.h"
#include "diVprofOptions.h"
#include "diVprofPainter.h"
#include "diVprofUtils.h"

#include <sstream>

using diutil::PointF;

#define MILOGGER_CATEGORY "diana.VprofBoxVerticalWind"
#include <miLogger/miLogging.h>

VprofBoxVerticalWind::VprofBoxVerticalWind()
{
}

void VprofBoxVerticalWind::configureDefaults()
{
  VprofBoxLine::configureDefaults();
  setXUnitLabel("hPa/s"); // default unit hPa/s
  setWidth(vprof::chxbas * 6.5);
  //  vwindColour = vprof::alternateColour(Colour(vpopt->frameColour));
  //  vwindLinewidth = vpopt->frameLinewidth;
}

void VprofBoxVerticalWind::plotXAxisLabels(VprofPainter* p)
{
  METLIBS_LOG_SCOPE();

  p->setLineStyle(labelStyle.colour, labelStyle.linewidth * 2);

  const Rectangle area = axes->paintRange();

  const float dy = std::min(margin.y2 * 0.5, vprof::chybas * 0.9);
  const float xc = (area.x1 + area.x2) * 0.5;
  const float yc = area.y2 + margin.y2 * 0.5;

  // down arrow (sinking motion, omega>0 !)
  const float xdown = (area.x1 + xc) * 0.5;
  const PointF from(xdown, yc + dy), to(xdown, yc - dy);
  p->drawArrow(from, to);

  // up arrow (raising motion, omega<0 !)
  const float xup = (area.x2 + xc) * 0.5;
  const PointF fromD(xup, yc - dy), toD(xup, yc + dy);
  p->drawArrow(fromD, toD);

  // range of vertical wind (-range to +range)
  const float rvwind = axes->x->getValueMin() - axes->x->getValueMax();

  // number showing the x-axis range of omega (+ hpa/s -)
  const Colour& c = labelStyle.colour;
  std::ostringstream ostr;
  ostr << rvwind;
  std::string str = ostr.str();
  const PointF chs = vprof::scaledTextSize(str.length() + 4, area.width());
  const float ylbl1 = area.y1 - 1.2 * chs.y();
  p->fpInitStr("+", area.x1 + chs.x() * 0.3, ylbl1, 0., chs.y(), c);
  p->fpInitStr("-", area.x2 - chs.x() * 1.3, ylbl1, 0., chs.y(), c);
  p->fpInitStr(str, xc, ylbl1, 0., chs.y(), c, VprofPainter::ALIGN_CENTER);

  plotXAxisUnits(p);
}

// static
const std::string& VprofBoxVerticalWind::boxType()
{
  static const std::string bt = "vertical_wind";
  return bt;
}
