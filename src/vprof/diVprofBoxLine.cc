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

#include "diana_config.h"

#include "diVprofBoxLine.h"

#include "diField/VcrossUtil.h"
#include "diField/diPoint.h"
#include "diUtilities.h"
#include "diVprofAxesStandard.h"
#include "diVprofCalculations.h"
#include "diVprofModelSettings.h"
#include "diVprofOptions.h"
#include "diVprofPainter.h"
#include "diVprofUtils.h"
#include "diVprofValues.h"
#include "vcross_v2/VcrossVerticalTicks.h"

#include <mi_fieldcalc/MetConstants.h>
#include <mi_fieldcalc/math_util.h>

#include <puTools/miStringFunctions.h>

#include <cmath>
#include <iomanip>
#include <sstream>

using namespace ::miutil;
using diutil::PointF;

#define MILOGGER_CATEGORY "diana.VprofBoxLine"
#include <miLogger/miLogging.h>

namespace {
const float xlimits_width = vprof::chxbas * 2; // FIXME

const std::string dot_grid = ".grid", dot_ticks_text = ".ticks.text", dot_linewidth2 = ".linewidth2";
const std::string pre_z = "z";
const std::string pre_x = "x";
} // namespace

const std::string VprofBoxLine::key_frame = "frame";
const std::string VprofBoxLine::key_title = "title";
const std::string VprofBoxLine::key_z_grid = pre_z + dot_grid;
const std::string VprofBoxLine::key_z_grid_linewidth2 = VprofBoxLine::key_z_grid + dot_linewidth2;
const std::string VprofBoxLine::key_z_ticks_text = pre_z + dot_ticks_text;
const std::string VprofBoxLine::key_x_grid = pre_x + dot_grid;
const std::string VprofBoxLine::key_x_grid_linewidth2 = VprofBoxLine::key_x_grid + dot_linewidth2;
const std::string VprofBoxLine::key_x_ticks_text = pre_x + dot_ticks_text;
const std::string VprofBoxLine::key_x_min = pre_x + ".min";
const std::string VprofBoxLine::key_x_max = pre_x + ".max";
const std::string VprofBoxLine::key_x_limits_in_corners = pre_x + ".limits_in_corners";
const std::string VprofBoxLine::key_x_unit_label = pre_x + ".unit.label";

const std::string VprofBoxLine::key_graph_box = "box";
const std::string VprofBoxLine::key_graph_type = "type";
const std::string VprofBoxLine::key_graph_style = "style";
namespace {
const std::string pre_graph_style_marker = VprofBoxLine::key_graph_style + ".marker";
} // namespace
const std::string VprofBoxLine::key_graph_style_marker_type = pre_graph_style_marker + ".type";
const std::string VprofBoxLine::key_graph_style_marker_size = pre_graph_style_marker + ".size";

VprofBoxLine::Graph::Graph()
    : type_(T_LINE)
    , linetype_(L_SOLID)
    , linewidth_(1)
    , markertype_(M_OFF)
    , markersize_(1)
{
}

// static
const std::string VprofBoxLine::COMPONENTS = "components";

VprofBoxLine::VprofBoxLine()
    : axes(std::make_shared<VprofAxesStandard>())
{
}

void VprofBoxLine::setArea(const Rectangle& area)
{
  axes->setPaintRange(area);
}

Rectangle VprofBoxLine::area() const
{
  return axes->paintRange();
}

void VprofBoxLine::setVerticalAxis(vcross::detail::AxisPtr zaxis)
{
  axes->z = zaxis;
}

void VprofBoxLine::setXValueRange(float xmin, float xmax)
{
  axes->x->setValueRange(xmin, xmax);
}

void VprofBoxLine::setXUnitLabel(const std::string& x_unit_label)
{
  axes->x->setLabel(x_unit_label);
}

void VprofBoxLine::configureDefaults()
{
  VprofBox::configureDefaults();

  setWidth(vprof::chxbas * 7);
  labelStyle = Linestyle("red", 1, "solid");

  frame_ = true;

  z_grid_ = false;
  z_ticks_showtext_ = false;

  x_grid_ = true;
  x_ticks_showtext_ = true;
  x_limits_in_corners_ = false;
}

void VprofBoxLine::configureOptions(const miutil::KeyValue_v& options)
{
  VprofBox::configureOptions(options);

  Linestyle zGridStyle("black", 1, "solid");
  float zGridLinewidth2 = 2;
  Linestyle xGridStyle = zGridStyle;
  float xGridLinewidth2 = zGridLinewidth2;
  float x_min = 0, x_max = 1;
  for (const auto& kv : options) {
    if (kv.key() == key_frame)
      setFrame(kv.toBool());
    else if (kv.key() == key_title)
      setTitle(kv.value());

    else if (kv.key() == key_z_grid)
      setZGrid(kv.toBool());
    else if (kv.key() == key_z_ticks_text)
      setZTicksShowText(kv.toBool());
    else if (vprof::kvLinestyle(zGridStyle, key_z_grid, kv))
      ; // nothing
    else if (kv.key() == key_z_grid_linewidth2)
      zGridLinewidth2 = kv.toFloat();

    else if (kv.key() == key_x_grid)
      setXGrid(kv.toBool());
    else if (kv.key() == key_x_ticks_text)
      setXTicksShowText(kv.toBool());
    else if (vprof::kvLinestyle(xGridStyle, key_x_grid, kv))
      ; // nothing
    else if (kv.key() == key_x_grid_linewidth2)
      xGridLinewidth2 = kv.toFloat();

    if (kv.key() == key_x_min)
      x_min = kv.toFloat();
    else if (kv.key() == key_x_max)
      x_max = kv.toFloat();
    else if (kv.key() == key_x_limits_in_corners)
      setXLimitsInCorners(kv.toBool());
    else if (kv.key() == key_x_unit_label)
      setXUnitLabel(kv.value());

    else if (vprof::kvLinestyle(labelStyle, "label.", kv))
      ; // nothing
  }
  setXValueRange(x_min, x_max);

  setZGridStyleMinor(zGridStyle);
  zGridStyle.linewidth = zGridLinewidth2;
  setZGridStyleMajor(zGridStyle);

  setXGridStyleMinor(xGridStyle);
  xGridStyle.linewidth = xGridLinewidth2;
  setXGridStyleMajor(xGridStyle);
}

void VprofBoxLine::updateLayout()
{
  VprofBox::updateLayout();
  margin.y1 = vprof::chybas * 1.5;
  margin.y2 = vprof::chybas * 1.5;

  configureZAxisLabelSpace();
  configureXAxisLabelSpace();
}

void VprofBoxLine::configureZAxisLabelSpace()
{
  using miutil::maximize;
  if (z_grid_ & z_ticks_showtext_)
    maximize(margin.x1, vprof::chxbas * 5.5); // space for zlabels
  if (z_ticks_showtext_)
    maximize(margin.y1, vprof::chybas * 2);
}

void VprofBoxLine::configureXAxisLabelSpace()
{
  using miutil::maximize;
  if (x_ticks_showtext_) {
    const float yspace = vprof::chybas * 2.5;
    maximize(margin.y1, yspace);
    maximize(margin.y2, yspace);
    if (!x_limits_in_corners_) {
      maximize(margin.x1, xlimits_width);
      maximize(margin.x2, xlimits_width);
    }
  }
}

void VprofBoxLine::plotDiagram(VprofPainter* p)
{
  METLIBS_LOG_SCOPE(LOGVAL(id()) << LOGVAL(z_grid_) << LOGVAL(z_ticks_showtext_) << LOGVAL(x_grid_) << LOGVAL(x_ticks_showtext_));

  if (z_grid_)
    plotZAxisGrid(p);
  if (x_grid_)
    plotXAxisGrid(p);
  if (z_ticks_showtext_)
    plotZAxisLabels(p);
  if (x_ticks_showtext_)
    plotXAxisLabels(p);
}

void VprofBoxLine::plotDiagramFrame(VprofPainter* p)
{
  const Rectangle area = axes->paintRange();
  if (frame_ && (margin.x1 > 0 || margin.x2 > 0 || margin.y1 > 0 || margin.y2 > 0)) {
    p->drawRect(false, axes->paintRange());
  }
  p->drawLine(area.x1, area.y1, area.x2, area.y1);
  p->drawLine(area.x1, area.y2, area.x2, area.y2);
}

void VprofBoxLine::plotZAxisGrid(VprofPainter* p)
{
  METLIBS_LOG_SCOPE(LOGVAL(z_grid_) << LOGVAL(z_ticks_showtext_));
  const Rectangle area = axes->paintRange();
  vcross::detail::AxisCPtr zAxis = axes->z;

  vcross::ticks_t tickValues;
  std::vector<bool> tickIsMajor;
  vcross::tick_to_axis_f tta = vcross::identity;
  if (zAxis->quantity() == vcross::detail::Axis::PRESSURE) {
    if (zAxis->label() == "hPa") {
      for (int p = 1050; p > 0; p -= 50) {
        tickValues.push_back(p);
        tickIsMajor.push_back(p == 1000 || p == 500);
      }
    } else if (zAxis->label() == "FL") {
      // transform from int to float
      std::copy(vprof::default_flightlevels.begin(), vprof::default_flightlevels.end(), std::back_inserter(tickValues));
      std::transform(vprof::default_flightlevels.begin(), vprof::default_flightlevels.end(), std::back_inserter(tickIsMajor),
                     [](int fl) { return ((fl % 50) == 0); });
      tta = vcross::util::FL_to_hPa;
    }
  }
  if (tickValues.empty()) {
    vcross::generateVerticalTicks(zAxis, tickValues, tta);
    bool major = true;
    for (vcross::ticks_t::const_iterator it = tickValues.begin(); it != tickValues.end(); ++it) {
      tickIsMajor.push_back(major);
      major = !major;
    }
  }
  METLIBS_LOG_DEBUG(LOGVAL(tickValues.size()));

  const float dy = vprof::chybas * 1.2;
  const float x1 = area.x1 - vprof::chxbas * 0.75;
  vprof::TextSpacing ts(area.y1 + dy * 0.5, area.y2 - dy * 0.5, dy);
  if (tickValues.size() > 1) {
    const float yy0 = zAxis->value2paint(tta(tickValues.front()), false);
    const float yy1 = zAxis->value2paint(tta(tickValues.back()), false);
    if (yy0 > yy1)
      ts.flip();
  }

  const Colour& c = x_grid_linestyle_minor_.colour;
  for (size_t i = 0; i < tickValues.size(); ++i) {
    const bool major = tickIsMajor[i];
    const float axisValue = tta(tickValues[i]);
    const float zPos = zAxis->value2paint(axisValue);
    if (zAxis->legalPaint(zPos)) {
      p->setLineStyle(major ? z_grid_linestyle_major_ : z_grid_linestyle_minor_);
      p->drawLine(area.x1, zPos, area.x2, zPos);
    }
    if (z_ticks_showtext_ && ts.accept(zPos)) {
      std::ostringstream ostr;
      ostr << tickValues[i];
      p->fpInitStr(ostr.str(), x1, zPos - vprof::chybas * 0.5, 0.0, vprof::chybas, c, VprofPainter::ALIGN_RIGHT);
    }
  }
  if (z_ticks_showtext_)
    p->fpInitStr(zAxis->label(), x1, area.y1 - vprof::chybas * 2.25, 0.0, vprof::chybas, c, VprofPainter::ALIGN_RIGHT);
}

void VprofBoxLine::plotZAxisLabels(VprofPainter*)
{
  METLIBS_LOG_SCOPE(LOGVAL(z_ticks_showtext_));
  // done in plotZAxisGrid
}

void VprofBoxLine::plotXAxisGrid(VprofPainter* p)
{
  METLIBS_LOG_SCOPE();
  const Rectangle area = this->area();

  const float v0 = axes->x->getValueMin(), v1 = axes->x->getValueMax(), vmin = std::min(v0, v1), vmax = std::max(v0, v1);
  const float gdv = (vmax - vmin) / 5;
  if (gdv > 0) {
    p->setLineStyle(x_grid_linestyle_minor_);
    const float g0 = (std::round(vmin / gdv) - 1) * gdv;
    for (float gv = g0; gv <= vmax; gv += gdv) {
      const float gx = axes->x->value2paint(gv, true);
      if (axes->x->legalPaint(gx)) {
        if (gv == 0)
          p->setLineStyle(x_grid_linestyle_major_);
        p->drawLine(gx, area.y1, gx, area.y2);
        if (gv == 0)
          p->setLineStyle(x_grid_linestyle_minor_);
      }
    }
  }
}

void VprofBoxLine::plotXAxisLabels(VprofPainter* p)
{
  METLIBS_LOG_SCOPE(LOGVAL(x_ticks_showtext_));
  const Colour& c = labelStyle.colour;

  const Rectangle area = this->area();
  const float dx = area.width();

  if (!title_.empty()) {
    const PointF chs = vprof::scaledTextSize(title_.length(), dx);
    const float x = (area.x1 + area.x2) * 0.5;
    p->fpInitStr(title_, x, area.y2 + 0.25 * chs.y(), 0.0, chs.y(), c, VprofPainter::ALIGN_CENTER);
  }

  {
    // plot x axis min and max value
    const std::string lx0 = miutil::from_number(axes->x->getValueMin());
    const std::string lx1 = miutil::from_number(axes->x->getValueMax());
    if (x_limits_in_corners_) {
      // x limits aligned left/right to area
      const PointF chs = vprof::scaledTextSize(lx0.length() + lx1.length() + 2, dx);
      const float ldx = chs.x() * 0.25, ly = area.y1 - 1.25 * chs.y();
      p->fpInitStr(lx0, area.x1 + ldx, ly, 0.0, chs.y(), c, VprofPainter::ALIGN_LEFT);
      p->fpInitStr(lx1, area.x2 - ldx, ly, 0.0, chs.y(), c, VprofPainter::ALIGN_RIGHT);
    } else {
      // x limits centered on area limits
      const PointF chs = vprof::scaledTextSize(std::max(lx0.length(), lx1.length()), 2 * xlimits_width);
      const float ly = area.y1 - 1.25 * chs.y();
      p->fpInitStr(lx0, area.x1, ly, 0.0, chs.y(), c, VprofPainter::ALIGN_CENTER);
      p->fpInitStr(lx1, area.x2, ly, 0.0, chs.y(), c, VprofPainter::ALIGN_CENTER);
    }
  }

  plotXAxisUnits(p);
}

void VprofBoxLine::plotXAxisUnits(VprofPainter* p)
{
  vcross::detail::AxisCPtr x = axes->x;
  const std::string& xUnit = x->label();
  if (xUnit.empty())
    return;

  const Rectangle area = axes->paintRange();
  const float ch = vprof::scaledTextSize(xUnit.length() + 4, area.width()).y();
  const float tx = (area.x1 + area.x2) * 0.5;
  const float ty = area.y1 - 2.25 * ch;
  p->fpInitStr(xUnit, tx, ty, 0., ch, labelStyle.colour, VprofPainter::ALIGN_CENTER);
}

void VprofBoxLine::plotSeries(VprofPainter* p, VprofGraphData_cp values)
{
  vcross::detail::AxisCPtr axis_x = std::static_pointer_cast<const VprofAxesStandard>(axes)->x;
  const float value_diff = axis_x->getValueMax() - axis_x->getValueMin();
  if (std::abs(value_diff) < 1e-8)
    return;
  if (!vprof::valid_content(values))
    return;

  const size_t nlevel = values->length();
  std::vector<PointF> vals;
  vals.reserve(nlevel);
  for (unsigned int k = 0; k < nlevel; k++) {
    vals.push_back(PointF(values->x(k), values->z(k)));
  }
  plotLine(p, vals);
}

void VprofBoxLine::plotLine(VprofPainter* p, const std::vector<PointF>& values)
{
  const size_t nlevel = values.size();
  std::vector<PointF> canvas;
  canvas.reserve(nlevel);
  for (auto v : values) {
    canvas.push_back(axes->value2paint(v));
  }
  p->drawClipped(canvas, axes->paintRange());
}

void VprofBoxLine::plotValues(VprofPainter* p, VprofValues_cp values, const VprofModelSettings& ms)
{
  METLIBS_LOG_SCOPE();

  p->setLineStyle(ms.colour, ms.dataLinewidth);

  const short unsigned int stipples[] = {0xFFFF, 0xFFC0, 0xF0F0};
  for (size_t i = 0; i < graphCount(); ++i) {
    if (graphs_[i].type_ == Graph::T_LINE) {
      if (VprofGraphData_cp data = values->series(graphComponentVarName(i, 0))) {
        METLIBS_LOG_DEBUG(LOGVAL(data->z(0)) << LOGVAL(data->z(data->length() - 1)));
        const size_t stipple_index = i % (sizeof(stipples) / sizeof(stipples[0]));
        p->setLineStipple(stipples[stipple_index]);
        plotSeries(p, data);
      }
    } else {
      METLIBS_LOG_WARN("only line plot is implemented");
    }
  }
  p->enableLineStipple(false);
}

bool VprofBoxLine::addGraph(const miutil::KeyValue_v& options)
{
  METLIBS_LOG_SCOPE(LOGVAL(options));
  Graph g;
  for (const auto& o : options) {
    if (o.key() == key_graph_type) {
      if (o.value() == "fill")
        g.type_ = Graph::T_FILL;
      else
        g.type_ = Graph::T_LINE;
    }
  }
  for (const auto& o : options) {
    if (o.key() == key_graph_type) {
      // handled above
    } else if (o.key() == key_graph_style + vprof::kv_linestyle_linetype) {
      if (o.value() == "stippled")
        g.linetype_ = Graph::L_STIPPLED;
      else if (o.value() == "off")
        g.linetype_ = Graph::L_OFF;
      else
        g.linetype_ = Graph::L_SOLID;
    } else if (o.key() == key_graph_style + vprof::kv_linestyle_linewidth) {
      g.linewidth_ = std::max(o.toFloat(), 0.1f);
    } else if (o.key() == key_graph_style_marker_type) {
      if (o.value() == "dot")
        g.markertype_ = Graph::M_DOT;
      if (o.value() == "square")
        g.markertype_ = Graph::M_SQUARE;
      else if (o.value() == "bar")
        g.markertype_ = Graph::M_BAR;
      else
        g.markertype_ = Graph::M_OFF;
    } else if (o.key() == key_graph_style_marker_size) {
      g.markersize_ = std::max(o.toFloat(), 0.01f);
    } else if (o.key() == COMPONENTS) {
      g.components_ = miutil::split(o.value(), ",");
      if ((g.type_ == Graph::T_LINE && g.components_.size() != 1) || (g.type_ == Graph::T_FILL && g.components_.size() != 2)) {
        g.components_.clear();
      }
    }
  }
  graphs_.push_back(g);
  return true;
}

size_t VprofBoxLine::graphCount()
{
  return graphs_.size();
}

size_t VprofBoxLine::graphComponentCount(size_t g)
{
  if (g < graphs_.size())
    return graphs_[g].components_.size();
  return VprofBox::graphComponentCount(g);
}

std::string VprofBoxLine::graphComponentVarName(size_t g, size_t c)
{
  if (g < graphs_.size()) {
    if (c < graphs_[g].components_.size())
      return graphs_[g].components_[c];
  }
  return VprofBox::graphComponentVarName(g, c);
}

bool VprofBoxLine::isVerticalPressure() const
{
  return (axes->z->quantity() == vcross::detail::Axis::PRESSURE);
}

// static
const std::string& VprofBoxLine::boxType()
{
  static const std::string bt = "line";
  return bt;
}
