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

#include "diVprofBoxSigWind.h"

#include "diColour.h"
#include "diLinetype.h"
#include "diUtilities.h"
#include "diVprofAxesStandard.h"
#include "diVprofCalculations.h"
#include "diVprofModelSettings.h"
#include "diVprofPainter.h"
#include "diVprofUtils.h"

#include <iomanip>
#include <sstream>

using diutil::PointF;

#define MILOGGER_CATEGORY "diana.VprofBoxSigWind"
#include <miLogger/miLogging.h>

VprofBoxSigWind::VprofBoxSigWind()
{
}

bool VprofBoxSigWind::separate() const
{
  return true;
}

void VprofBoxSigWind::configure(const miutil::KeyValue_v& options)
{
  setWidth(vprof::chxbas * 6.5);
  VprofBoxZ::configure(options);
}

void VprofBoxSigWind::updateLayout()
{
  VprofBoxZ::updateLayout();
  margin.y1 = vprof::chybas * 2;
  margin.y2 = vprof::chybas * 0.85;
}

void VprofBoxSigWind::plotDiagram(VprofPainter*)
{
}

void VprofBoxSigWind::plotDiagramFrame(VprofPainter*)
{
}

void VprofBoxSigWind::plotValues(VprofPainter* p, VprofValues_cp values, const VprofModelSettings& ms)
{
  METLIBS_LOG_SCOPE();

  if (!ms.isSelectedRealization)
    return;

  VprofGraphData_cp dd = values->series(graphComponentVarName(0, 0));
  VprofGraphData_cp ff = values->series(graphComponentVarName(0, 1));
  VprofGraphData_cp wsig = values->series(graphComponentVarName(0, 2));
  if (!vprof::valid_content(dd) || !vprof::valid_content(ff) || !vprof::valid_content(wsig)) {
    METLIBS_LOG_WARN("invalid dd/ff/wsig");
    return;
  }
  if (!vprof::check_same_z(dd, ff) || !vprof::check_same_z(dd, wsig)) {
    METLIBS_LOG_WARN("z mismatch dd -- ff/wsig");
    return;
  }

  p->setLineStyle(ms.colour, ms.dataLinewidth);

  // significant wind levels, wind as numbers (temp and prog) (other levels also if space)
  const Rectangle area = this->area();
  const float dchy = vprof::chybas * 1.3;
  const float ylim1 = area.y1 + dchy * 0.5;
  const float ylim2 = area.y2 - dchy * 0.5;
  int k1 = -1;
  int k2 = -1;
  for (unsigned int k = 0; k < dd->length(); k++) {
    const float yy = axis_z->value2paint(dd->z(k), false);
    if (yy > ylim1 && yy < ylim2) {
      if (k1 == -1)
        k1 = k;
      k2 = k;
    }
  }

  if (k1 < 0)
    return;

  std::vector<float> used;
  const float dx = width();
  const float x = area.x1 + dx * ms.nplot + vprof::chxbas * 0.5;
  p->setFontsize(vprof::chybas);

  const int nsig = wsig->length();
  for (int sig = 3; sig >= 0; sig--) {
    for (int k = k1; k <= k2; k++) {
      if (k < nsig && wsig->x(k) == sig) {
        const float yy = axis_z->value2paint(dd->z(k), false);
        const float yylim1 = yy - dchy;
        const float yylim2 = yy + dchy;
        size_t i = 0;
        while (i < used.size() && (used[i] < yylim1 || used[i] > yylim2))
          i++;
        if (i == used.size()) {
          used.push_back(yy);
          int idd = int(dd->x(k) + 5) / 10;
          const float iff = diutil::float2int(diutil::ms2knots(ff->x(k)));
          if (idd == 0 && iff > 0)
            idd = 36;
          std::ostringstream ostr;
          ostr << std::setw(2) << std::setfill('0') << idd << "-" << std::setw(3) << std::setfill('0') << iff;
          std::string str = ostr.str();
          const float y = yy - vprof::chybas * 0.5;
          p->drawText(str, x, y, 0.0);
        }
      }
    }
  }
}

size_t VprofBoxSigWind::graphCount()
{
  return 1;
}

size_t VprofBoxSigWind::graphComponentCount(size_t graph)
{
  if (graph == 0)
    return 3;
  return VprofBox::graphComponentCount(graph);
}

std::string VprofBoxSigWind::graphComponentVarName(size_t graph, size_t component)
{
  if (graph == 0) {
    if (component == 0)
      return vprof::VP_WIND_DD;
    else if (component == 1)
      return vprof::VP_WIND_FF;
    else if (component == 2)
      return vprof::VP_WIND_SIG;
  }
  return VprofBox::graphComponentVarName(graph, component);
}

// static
const std::string& VprofBoxSigWind::boxType()
{
  static const std::string bt = "significant_wind";
  return bt;
}
