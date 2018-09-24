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

#include "diVprofBoxFL.h"

#include "diColour.h"
#include "diField/VcrossUtil.h"
#include "diVprofAxesStandard.h"
#include "diVprofPainter.h"
#include "diVprofUtils.h"

#include <puTools/miStringFunctions.h>

#include <cmath>
#include <iomanip>
#include <sstream>

using diutil::PointF;

#define MILOGGER_CATEGORY "diana.VprofBoxFL"
#include <miLogger/miLogging.h>

const std::string VprofBoxFL::key_text = "text";
const std::string VprofBoxFL::key_levels = "levels";
const std::string VprofBoxFL::key_style = "style";

VprofBoxFL::VprofBoxFL()
{
}

void VprofBoxFL::configureOptions(const miutil::KeyValue_v& options)
{
  VprofBoxZ::configureOptions(options);

  Linestyle ls("black", 1, "solid");
  plabelflevels = true;
  flightlevels = {0,   10,  20,  30,  40,  50,  60,  70,  80,  90,  100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 210, 220,
                  230, 240, 250, 260, 270, 280, 290, 300, 310, 320, 330, 340, 350, 360, 370, 380, 390, 400, 450, 500, 550, 600};

  for (const auto& kv : options) {
    if (kv.key() == key_text)
      setLabels(kv.toBool());
    else if (kv.key() == key_levels)
      setLevels(kv.toInts());
    else if (vprof::kvLinestyle(ls, key_style, kv))
      ; // nothing
  }
  setStyle(ls);
}

void VprofBoxFL::updateLayout()
{
  VprofBoxZ::updateLayout();
  if (plabelflevels) {
    setWidth(vprof::chxbas * 5.5);
    margin.y1 = vprof::chybas * 2;
  } else {
    setWidth(vprof::chxbas * 1.5);
  }
}

void VprofBoxFL::plotDiagram(VprofPainter* p)
{
  METLIBS_LOG_SCOPE();
  if (axis_z->quantity() != vcross::detail::Axis::PRESSURE)
    return;

  const float chx = vprof::chxbas;
  const float chy = vprof::chybas;
  const Rectangle area = this->area();
  vprof::TextSpacing ts(area.y1 + chy * 0.6, area.y2 - chy * 0.6, chy * 1.2);
  const float x1l = area.x1 + chx * 0.5;

  Linestyle style2 = style;
  style2.linewidth *= 2;

  const size_t kk = flightlevels.size();
  for (size_t k = 0; k < kk; k++) {
    const int fl = flightlevels[k];
    const float fl_p = vcross::util::FL_to_hPa(fl);
    const bool level50 = ((fl % 50) == 0);
    const float y = axis_z->value2paint(fl_p, false);
    if (y > area.y1 && y < area.y2) {
      p->setLineStyle(level50 ? style2 : style);
      p->drawLine(area.x2 - chx * (!level50 ? 0.85 : 1.5), y, area.x2, y);
    }
    if (plabelflevels && level50) {
      if (ts.accept(y)) {
        std::ostringstream ostr;
        ostr << std::setw(3) << std::setfill('0') << fl;
        p->fpInitStr(ostr.str(), x1l, y - chy * 0.5, 0., chy, style.colour);
      }
    }
  }
  if (plabelflevels)
    p->fpInitStr("FL", x1l, area.y1 - chy * 1.25, 0., chy, style.colour);
}

void VprofBoxFL::plotDiagramFrame(VprofPainter*)
{
}

void VprofBoxFL::plotValues(VprofPainter*, VprofValues_cp, const VprofModelSettings&)
{
}

size_t VprofBoxFL::graphCount()
{
  return 0;
}

size_t VprofBoxFL::graphComponentCount(size_t graph)
{
  return VprofBox::graphComponentCount(graph);
}

std::string VprofBoxFL::graphComponentVarName(size_t graph, size_t component)
{
  return VprofBox::graphComponentVarName(graph, component);
}

// static
const std::string& VprofBoxFL::boxType()
{
  static const std::string bt = "fl";
  return bt;
}
