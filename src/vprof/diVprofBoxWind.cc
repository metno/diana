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

#include "diVprofBoxWind.h"

#include "diField/diMetConstants.h"
#include "diVprofAxesStandard.h"
#include "diVprofCalculations.h"
#include "diVprofModelSettings.h"
#include "diVprofPainter.h"
#include "diVprofUtils.h"

#include <algorithm>

using diutil::PointF;

#define MILOGGER_CATEGORY "diana.VprofBoxWind"
#include <miLogger/miLogging.h>

VprofBoxWind::VprofBoxWind()
{
}

bool VprofBoxWind::separate() const
{
  return separate_;
}

void VprofBoxWind::configureDefaults()
{
  VprofBoxZ::configureDefaults();
  separate_ = true;
  setWidth(vprof::chxbas * 8);
}

void VprofBoxWind::configureOptions(const miutil::KeyValue_v& options)
{
  VprofBoxZ::configureOptions(options);

  const size_t i_id = miutil::rfind(options, "separate");
  if (i_id != size_t(-1))
    setSeparate(options.at(i_id).toBool());
}

void VprofBoxWind::updateLayout()
{
  VprofBoxZ::updateLayout();
  margin.y1 = std::max(vprof::chybas * 2.0f, width() * 0.5f);
  margin.y2 = width() * 0.5;
}

void VprofBoxWind::plotDiagram(VprofPainter*)
{
}

void VprofBoxWind::plotDiagramFrame(VprofPainter*)
{
}

void VprofBoxWind::plotValues(VprofPainter* p, VprofValues_cp values, const VprofModelSettings& ms)
{
  METLIBS_LOG_SCOPE();

  if (!ms.isSelectedRealization)
    return;

  // wind (u(e/w) and v(n/s)
  VprofGraphData_cp u = values->series(graphComponentVarName(0, 0));
  VprofGraphData_cp v = values->series(graphComponentVarName(0, 1));
  if (!vprof::valid_content(u) || !vprof::valid_content(v))
    return;
  if (!vprof::check_same_z(u, v))
    return;

  const int n = separate_ ? ms.nplot : 1;
  const Rectangle area = this->area();
  float x0 = area.x1 + width() * (0.5 + n);
  float flagl = width() * 0.5 * 0.85;

  p->setLineStyle(ms.colour, ms.windLinewidth);

  vprof::TextSpacing spc(area.y1, area.y2, vprof::chybas / 2);
  if (u->length() > 1) {
    const float yy0 = axis_z->value2paint(u->z(0), false);
    const float yy1 = axis_z->value2paint(u->z(u->length() - 1), false);
    if (yy0 > yy1)
      spc.flip();
  }
  for (unsigned int k = 0; k < u->length(); k++) {
    const float yy = axis_z->value2paint(u->z(k), false);
    if (spc.accept(yy)) {
      p->drawWindArrow(PointF(u->x(k), v->x(k)) * MetNo::Constants::ms2knots, PointF(x0, yy), flagl, false);
    }
  }
}

size_t VprofBoxWind::graphCount()
{
  return 1;
}

size_t VprofBoxWind::graphComponentCount(size_t graph)
{
  if (graph == 0)
    return 2;
  return VprofBox::graphComponentCount(graph);
}

std::string VprofBoxWind::graphComponentVarName(size_t graph, size_t component)
{
  if (graph == 0) {
    if (component == 0)
      return vprof::VP_WIND_X;
    else if (component == 1)
      return vprof::VP_WIND_Y;
  }
  return VprofBox::graphComponentVarName(graph, component);
}

// static
const std::string& VprofBoxWind::boxType()
{
  static const std::string bt = "wind";
  return bt;
}
