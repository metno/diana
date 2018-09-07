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

#ifndef VPROFDIAGRAM_H
#define VPROFDIAGRAM_H

#include "diLinestyle.h"
#include "diVprofBox.h"
#include "diVprofValues.h"

#include "diField/VcrossData.h"

#include <QSize>

class VprofPainter;

/**
   \brief Plots the Vertical Profile diagram background without data
*/
class VprofDiagram
{
public:
  VprofDiagram();
  ~VprofDiagram();

  void changeNumber(int nprog);
  void setPlotWindow(const QSize& size);

  void setVerticalArea(float y1, float y2);
  void setZQuantity(const std::string& q);
  void setZUnit(const std::string& u);
  void setZType(const std::string& t);
  void setZValueRange(float zmin, float zmax);
  void setBackgroundColour(const Colour& c) { backgroundColour = c; }
  void setShowText(bool on) { ptext = on; }
  void setShowGeoText(bool on) { pgeotext = on; }
  void setShowKIndex(bool on) { pkindex = on; }
  void setFrame(bool on) { pframe = on; }
  void setFrameStyle(const Linestyle& s) { frameLineStyle = s; }

  void configureDiagram(const miutil::KeyValue_v& config);
  void addBox(VprofBox_p box);

  VprofBox_p box(const std::string& id);

  vcross::Z_AXIS_TYPE verticalType() const;
  std::set<std::string> variables() const;

  void plot(VprofPainter* painter);
  void plotValues(VprofPainter* painter, VprofValues_cp values, const VprofModelSettings& ms);
  void plotText(VprofPainter* painter);

  static const std::string key_z_unit;
  static const std::string key_z_quantity;
  static const std::string key_z_type;
  static const std::string key_z_min;
  static const std::string key_z_max;
  static const std::string key_area_y1;
  static const std::string key_area_y2;
  static const std::string key_background;
  static const std::string key_text;
  static const std::string key_geotext;
  static const std::string key_kindex;
  static const std::string key_frame;

private:
  void updateLayout();
  void plotDiagram(VprofPainter* painter);
  void plotDiagramFrame(VprofPainter* painter);

private:
  int plotw, ploth;
  int numprog;
  bool layoutChanged_;

  vcross::detail::AxisPtr zaxis;
  Rectangle box_total; // bounding box of entire diagram
  Rectangle box_text;  // bounding box for text
  std::vector<VprofBox_p> boxes;

  float area_y1_;
  float area_y2_;
  Colour backgroundColour;

  bool pframe;
  Linestyle frameLineStyle;

  bool ptext, pgeotext, pkindex;
  float chxtxt;
  float chytxt;

  std::vector<VprofText> vptext; // info for text plotting
};

#endif
