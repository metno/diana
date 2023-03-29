/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2018 met.no

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

#include "diVprofDiagram.h"

#include "diColour.h"
#include "diUtilities.h"
#include "diVprofAxesPT.h"
#include "diVprofAxesStandard.h"
#include "diVprofModelSettings.h"
#include "diVprofOptions.h"
#include "diVprofPainter.h"
#include "diVprofUtils.h"
#include "diField/VcrossUtil.h"
#include "diField/diPoint.h"
#include "util/misc_util.h"

#include <mi_fieldcalc/math_util.h>

#include <iomanip>
#include <sstream>

using diutil::PointF;

#define MILOGGER_CATEGORY "diana.VprofDiagram"
#include <miLogger/miLogging.h>

static const std::string pre_z = "z";
const std::string VprofDiagram::key_z_unit = pre_z + ".unit";
const std::string VprofDiagram::key_z_quantity = pre_z + ".quantity";
const std::string VprofDiagram::key_z_type = pre_z + ".type";
const std::string VprofDiagram::key_z_min = pre_z + ".min";
const std::string VprofDiagram::key_z_max = pre_z + ".max";
static const std::string pre_area = "area";
const std::string VprofDiagram::key_area_y1 = pre_area + ".y1";
const std::string VprofDiagram::key_area_y2 = pre_area + ".y2";
const std::string VprofDiagram::key_background = "background";
const std::string VprofDiagram::key_text = "text";
const std::string VprofDiagram::key_geotext = "geotext";
const std::string VprofDiagram::key_kindex = "kindex";
const std::string VprofDiagram::key_frame = "frame";

VprofDiagram::VprofDiagram()
    : plotw(0)
    , ploth(0)
    , numprog(0)
    , layoutChanged_(true)
{
  METLIBS_LOG_SCOPE();
}

VprofDiagram::~VprofDiagram()
{
  METLIBS_LOG_SCOPE();
}

void VprofDiagram::changeNumber(int nprog)
{
  METLIBS_LOG_SCOPE(LOGVAL(nprog));

  numprog = nprog;
  layoutChanged_ = true;
}

void VprofDiagram::setPlotWindow(const QSize& size)
{
  plotw = size.width();
  ploth = size.height();
}

void VprofDiagram::plot(VprofPainter* painter)
{
  METLIBS_LOG_SCOPE();

  if (plotw < 5 || ploth < 5)
    return;

  if (layoutChanged_) {
    updateLayout();
    layoutChanged_ = false;
  }

  Rectangle box_diagram = box_total;
  const float margin = 0.02;
  const bool fixed_aspect_ratio = true;
  if (margin > 0)
    diutil::adjustRectangle(box_diagram, margin * box_diagram.width(), margin * box_diagram.height());
  if (fixed_aspect_ratio)
    diutil::fixAspectRatio(box_diagram, plotw / float(ploth), true);

  painter->init(backgroundColour, box_diagram, PointF(plotw, ploth));

  plotDiagram(painter);
  painter->fpDrawStr();
}

void VprofDiagram::addBox(VprofBox_p box)
{
  METLIBS_LOG_SCOPE();
  if (box) {
    boxes.push_back(box);
    box->setVerticalAxis(zaxis);
    METLIBS_LOG_DEBUG(LOGVAL(box->id()) << LOGVAL(boxes.size()));
    layoutChanged_ = true;
  }
}

void VprofDiagram::setVerticalArea(float y1, float y2)
{
  area_y1_ = y1;
  area_y2_ = y2;
}

// FIXME copied from VprofUtils
namespace {
const std::string z_types[4] = {"amble", "exner", "linear", "log"};
const int nz_types = sizeof(z_types) / sizeof(z_types[0]);
} // namespace

void VprofDiagram::setZQuantity(const std::string& q)
{
  zaxis->setQuantity(q);
}

void VprofDiagram::setZUnit(const std::string& u)
{
  zaxis->setLabel(u);
}

void VprofDiagram::setZType(const std::string& t)
{
  int diagramtype = 0;
  for (const auto& zt : z_types) {
    if (t == zt)
      break;
    diagramtype += 1;
  }
  if (diagramtype > nz_types)
    diagramtype = 0;
  switch (diagramtype) {
  case 0:
    zaxis->setType(vcross::detail::Axis::AMBLE);
    break;
  case 1:
    zaxis->setType(vcross::detail::Axis::EXNER);
    break;
  case 3:
    zaxis->setType(vcross::detail::Axis::LOGARITHMIC);
    break;
  case 2:
  default:
    zaxis->setType(vcross::detail::Axis::LINEAR);
    break;
  }
}
void VprofDiagram::setZValueRange(float zmin, float zmax)
{
  zaxis->setValueRange(zmax, zmin);
}

void VprofDiagram::configureDiagram(const miutil::KeyValue_v& config)
{
  METLIBS_LOG_SCOPE();
  vptext.clear();
  boxes.clear();
  box_total = box_text = Rectangle();
  layoutChanged_ = true;

  pframe = true;
  Linestyle frameLS;
  backgroundColour = Colour("white");
  float zmin = 100, zmax = 1050;
  float area_y1 = 0, area_y2 = 25;

  zaxis = std::make_shared<vcross::detail::Axis>(false);

  for (const auto& kv : config) {
    if (kv.key() == key_z_quantity) {
      setZQuantity(kv.value());
    } else if (kv.key() == key_z_unit) {
      setZUnit(kv.value());
    } else if (kv.key() == key_z_type) {
      setZType(kv.value());
    } else if (kv.key() == key_z_min)
      zmin = kv.toFloat();
    else if (kv.key() == key_z_max)
      zmax = kv.toFloat();
    else if (kv.key() == key_area_y1)
      area_y1 = kv.toFloat();
    else if (kv.key() == key_area_y2)
      area_y2 = kv.toFloat();
    else if (kv.key() == key_background + vprof::kv_linestyle_colour)
      setBackgroundColour(Colour(kv.value()));
    else if (kv.key() == key_text)
      setShowText(kv.toBool());
    else if (kv.key() == key_geotext)
      setShowGeoText(kv.toBool());
    else if (kv.key() == key_kindex)
      setShowKIndex(kv.toBool());
    else if (kv.key() == key_frame)
      setFrame(kv.toBool());
    else if (vprof::kvLinestyle(frameLS, key_frame, kv))
      ; // nothing
  }

  chxtxt = vprof::chxbas; // * vpopt->rstext;
  chytxt = vprof::chybas; // * vpopt->rstext;

  setVerticalArea(area_y1, area_y2);
  setZValueRange(zmin, zmax);
  setFrameStyle(frameLS);
}

void VprofDiagram::updateLayout()
{
  using miutil::maximize;

  for (auto b : boxes)
    b->updateLayout();

  // adjust size of all boxes, making top/bottom of size equal
  if (!boxes.empty()) {
    const int n = std::max(numprog, 1);
    float ps_x2 = 0; // previous box size x2
    for (const VprofBox_p b : boxes) {
      const float area_x1 = ps_x2 + b->margin.x1;
      const float area_x2 = area_x1 + b->width() * (b->separate() ? n : 1);
      b->setArea(Rectangle(area_x1, area_y1_, area_x2, area_y2_));
      ps_x2 += b->size().width();
    }

    // assumes all areas are already aligned
    float y1max = boxes.front()->margin.y1;
    float y2max = boxes.front()->margin.y2;
    for (const VprofBox_p b : boxes) {
      maximize(y1max, b->margin.y1);
      maximize(y2max, b->margin.y2);
    }
    for (VprofBox_p b : boxes) {
      b->margin.y1 = y1max;
      b->margin.y2 = y2max;
    }

    // space for text
    float text_height = 0;
    if (ptext) {
      const int n = std::max(1, numprog);
      text_height = (chytxt * (1.5 * n + 0.5));
    }
    const Rectangle size0 = boxes.front()->size();
    box_text.x1 = size0.x1;
    box_text.x2 = boxes.back()->size().x2;
    box_text.y2 = size0.y1;
    box_text.y1 = box_text.y2 - text_height;

    box_total = box_text;
    box_total.y2 = size0.y2;
  } else {
    box_total = box_text = Rectangle();
  }
}

VprofBox_p VprofDiagram::box(const std::string& id)
{
  for (auto b : boxes) {
    if (b->id() == id)
      return b;
  }
  return VprofBox_p();
}

vcross::Z_AXIS_TYPE VprofDiagram::verticalType() const
{
  if (zaxis->quantity() == vcross::detail::Axis::ALTITUDE)
    return vcross::Z_TYPE_ALTITUDE;
  else if (zaxis->quantity() == vcross::detail::Axis::DEPTH)
    return vcross::Z_TYPE_DEPTH;
  else
    return vcross::Z_TYPE_PRESSURE;
}

std::set<std::string> VprofDiagram::variables() const
{
  std::set<std::string> vn;
  for (const VprofBox_p b : boxes) {
    for (size_t g = 0; g < b->graphCount(); ++g) {
      for (size_t c = 0; c < b->graphComponentCount(g); ++c)
        vn.insert(b->graphComponentVarName(g, c));
    }
  }
  return vn;
}

void VprofDiagram::plotDiagram(VprofPainter* painter)
{
  METLIBS_LOG_SCOPE();

  for (const VprofBox_p b : boxes) {
    b->plotDiagram(painter);
  }

  plotDiagramFrame(painter);
}

void VprofDiagram::plotDiagramFrame(VprofPainter* painter)
{
  METLIBS_LOG_SCOPE();

  if (boxes.empty())
    return;

  painter->setLineStyle(frameLineStyle);

  // outer frame
  if (pframe)
    painter->drawRect(false, box_total);

  // lines between boxes, i.e. at right of each box except last
  for (int i = 0; i < (int)boxes.size() - 1; ++i) {
    const VprofBox& box = *boxes[i];
    const Rectangle& r = box.size();
    painter->drawLine(r.x2, r.y1, r.x2, r.y2);
  }

  // frame line for each model/obs
  for (const VprofBox_p& box : boxes) {
    if (box->separate()) {
      const Rectangle& r = box->size();
      const float dx1 = r.width() / numprog;
      for (int j = 1; j < numprog; ++j) {
        const float x = r.x1 + dx1 * j;
        painter->drawLine(x, r.y1, x, r.y2);
      }
    }
    box->plotDiagramFrame(painter);
  }

  // line between boxes and text
  if (box_text.height() > 0) {
    painter->drawLine(box_total.x1, box_text.y2, box_total.x2, box_text.y2);
  }

  painter->enableLineStipple(false);
}

void VprofDiagram::plotText(VprofPainter* painter)
{
  METLIBS_LOG_SCOPE();
  using miutil::maximize;

  if (ptext) {
    const int n = vptext.size();

    painter->setFontsize(chytxt);
    const float wspace = painter->getTextWidth("oo");

    std::vector<std::string> fctext(n);
    std::vector<std::string> geotext(n);
    std::vector<std::string> kitext(n);
    std::vector<std::string> rtext(n);
    float wmod = 0, wpos = 0, wfc = 0, wgeo = 0, wkindex = 0, wrealization = 0;

    for (int i = 0; i < n; i++) {
      maximize(wmod, painter->getTextWidth(vptext[i].modelName));
      maximize(wpos, painter->getTextWidth(vptext[i].posName));
    }
    for (int i = 0; i < n; i++) {
      if (vptext[i].prognostic) {
        std::ostringstream ostr;
        ostr << "(" << std::setiosflags(std::ios::showpos) << vptext[i].forecastHour << ")";
        fctext[i] = ostr.str();
        maximize(wfc, painter->getTextWidth(fctext[i]));

        if (vptext[i].realization >= 0) {
          std::ostringstream ostr;
          ostr << "member " << vptext[i].realization;
          rtext[i] = ostr.str();
          maximize(wrealization, painter->getTextWidth(rtext[i]));
        }
      }
    }
    if (pgeotext) {
      for (int i = 0; i < n; i++) {
        std::ostringstream ostr;
        ostr << "   (" << fabsf(vptext[i].latitude);
        if (vptext[i].latitude >= 0.0)
          ostr << "N";
        else
          ostr << "S";
        ostr << "  " << fabsf(vptext[i].longitude);
        if (vptext[i].longitude >= 0.0)
          ostr << "E)";
        else
          ostr << "W)";
        geotext[i] = ostr.str();
        maximize(wgeo, painter->getTextWidth(geotext[i]));
      }
    }
    if (pkindex) {
      for (int i = 0; i < n; i++) {
        if (vptext[i].kindexFound) {
          int k = std::lround(vptext[i].kindexValue);
          std::ostringstream ostr;
          ostr << "K= " << std::setiosflags(std::ios::showpos) << k;
          kitext[i] = ostr.str();
          maximize(wkindex, painter->getTextWidth(kitext[i]));
        }
      }
    }

    const float wtime = painter->getTextWidth("2222-22-22 23:59 UTC");

    float xmod = box_text.x1 + chxtxt * 0.5;
    float xnext = xmod + wmod + wspace;
    float xpos = xnext;
    xnext += wpos + wspace;
    float xfc = xnext;
    if (wfc > 0)
      xnext += wfc + wspace * 0.6;
    float xtime = xnext;
    xnext += wtime + wspace;
    float xgeo = xnext;
    if (pgeotext)
      xnext += wgeo + wspace;
    float xkindex = xnext;
    if (pkindex && wkindex > 0)
      xnext += wkindex + wspace;
    float xrealization = xnext;

    for (int i = 0; i < n; i++) {
      const VprofText& vpt = vptext[i];
      painter->setColour(vpt.colour);
      const float y = box_text.y2 - chytxt * 1.5 * (vpt.index + 1);
      painter->drawText(vpt.modelName, xmod, y);
      painter->drawText(vpt.posName, xpos, y);
      if (vpt.prognostic)
        painter->drawText(fctext[i], xfc, y, 0.0);
      std::string tstr = vpt.validTime.format("$date %H:%M UTC", "", true);
      painter->drawText(tstr, xtime, y);
      if (pgeotext)
        painter->drawText(geotext[i], xgeo, y);
      if (pkindex && vpt.kindexFound)
        painter->drawText(kitext[i], xkindex, y);
      if (vpt.realization >= 0)
        painter->drawText(rtext[i], xrealization, y, 0.0);
    }
  }

  vptext.clear();
}

void VprofDiagram::plotValues(VprofPainter* painter, VprofValues_cp values, const VprofModelSettings& ms)
{
  METLIBS_LOG_SCOPE(LOGVAL(ms.nplot) << LOGVAL(ms.isSelectedRealization));
  METLIBS_LOG_DEBUG("start plotting '" << values->text.posName << "'");

  for (const VprofBox_p& b : boxes) {
    b->plotValues(painter, values, ms);
  }

  if (ms.isSelectedRealization) {
    // text from data values
    vptext.push_back(values->text);
    vptext.back().index = ms.nplot;
    vptext.back().colour = ms.colour;
  }
}
