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

#ifndef VPROFBOXLINE_H
#define VPROFBOXLINE_H

#include "diVprofBox.h"
#include "diVprofPainter.h"

class VprofBoxLine : public VprofBox
{
protected:
  struct Graph
  {
    enum Type { T_LINE, T_FILL };
    enum MarkerType { M_OFF, M_DOT, M_SQUARE, M_BAR };
    enum LineType { L_SOLID, L_OFF, L_STIPPLED };

    Graph();

    std::vector<std::string> components_;
    Type type_;

    LineType linetype_;
    float linewidth_;

    MarkerType markertype_;
    float markersize_;
  };

  typedef std::vector<Graph> Graph_v;

public:
  VprofBoxLine();

  void setArea(const Rectangle& area) override;
  Rectangle area() const override;

  void setVerticalAxis(vcross::detail::AxisPtr zaxis) override;

  void setFrame(bool on) { frame_ = on; }
  void setTitle(const std::string& title) { title_ = title; }
  void setZGrid(bool on) { z_grid_ = on; }
  void setZGridStyleMinor(const Linestyle& s) { z_grid_linestyle_minor_ = s; }
  void setZGridStyleMajor(const Linestyle& s) { z_grid_linestyle_major_ = s; }
  void setZTicksShowText(bool on) { z_ticks_showtext_ = on; }
  virtual void setXValueRange(float xmin, float xmax);
  void setXGrid(bool on) { x_grid_ = on; }
  void setXGridStyleMinor(const Linestyle& s) { x_grid_linestyle_minor_ = s; }
  void setXGridStyleMajor(const Linestyle& s) { x_grid_linestyle_major_ = s; }
  void setXTicksShowText(bool on) { x_ticks_showtext_ = on; }
  void setXLimitsInCorners(bool on) { x_limits_in_corners_ = on; }
  void setXUnitLabel(const std::string& x_unit_label);

  void updateLayout() override;

  void plotDiagram(VprofPainter* p) override;
  void plotDiagramFrame(VprofPainter* p) override;
  void plotValues(VprofPainter* p, VprofValues_cp values, const VprofModelSettings& ms) override;

  bool addGraph(const miutil::KeyValue_v& options) override;
  size_t graphCount() override;
  size_t graphComponentCount(size_t graph) override;
  std::string graphComponentVarName(size_t graph, size_t component) override;

  static const std::string& boxType();

  static const std::string key_frame;
  static const std::string key_title;
  static const std::string key_z_grid;
  static const std::string key_z_grid_linewidth2;
  static const std::string key_z_ticks_text;
  static const std::string key_x_grid;
  static const std::string key_x_grid_linewidth2;
  static const std::string key_x_ticks_text;
  static const std::string key_x_min;
  static const std::string key_x_max;
  static const std::string key_x_limits_in_corners;
  static const std::string key_x_unit_label;

  static const std::string key_graph_box;
  static const std::string key_graph_type;
  static const std::string key_graph_style;
  static const std::string key_graph_style_marker_type;
  static const std::string key_graph_style_marker_size;
  static const std::string COMPONENTS;

protected:
  void configureDefaults() override;
  void configureOptions(const miutil::KeyValue_v& options) override;

  virtual void configureZAxisLabelSpace();
  virtual void configureXAxisLabelSpace();

  virtual void plotZAxisGrid(VprofPainter* p);
  virtual void plotZAxisLabels(VprofPainter* p);
  virtual void plotXAxisGrid(VprofPainter* p);
  virtual void plotXAxisLabels(VprofPainter* p);
  virtual void plotXAxisUnits(VprofPainter* p);

  void plotLine(VprofPainter* p, const std::vector<diutil::PointF>& values);
  bool isVerticalPressure() const;

private:
  void plotSeries(VprofPainter* p, VprofGraphData_cp values);

protected:
  VprofAxesStandard_p axes;
  Graph_v graphs_;

  Linestyle labelStyle;
  Linestyle z_grid_linestyle_minor_;
  Linestyle z_grid_linestyle_major_;
  Linestyle x_grid_linestyle_minor_;
  Linestyle x_grid_linestyle_major_;

  bool frame_;

  bool z_grid_;
  bool x_grid_;
  bool z_ticks_showtext_;
  bool x_ticks_showtext_;
  bool x_limits_in_corners_; // true iff x axis min/max text is to be placed in corners, as in old vprof
  std::string title_;
};

typedef std::shared_ptr<VprofBoxLine> VprofBoxLine_p;

#endif // VPROFBOXLINE_H
