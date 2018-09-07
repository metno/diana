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

#ifndef VPROFBOXFL_H
#define VPROFBOXFL_H

#include "diLinestyle.h"
#include "diVprofBoxZ.h"

class VprofBoxFL : public VprofBoxZ
{
public:
  VprofBoxFL();

  void setLabels(bool on) { plabelflevels = on; }
  void setLevels(const std::vector<int>& table) { flightlevels = table; }
  void setStyle(const Linestyle& s) { style = s; }

  void configure(const miutil::KeyValue_v& options) override;
  void updateLayout() override;
  void plotDiagram(VprofPainter* p) override;
  void plotDiagramFrame(VprofPainter* p) override;
  void plotValues(VprofPainter* p, VprofValues_cp values, const VprofModelSettings& ms) override;

  size_t graphCount() override;
  size_t graphComponentCount(size_t graph) override;
  std::string graphComponentVarName(size_t graph, size_t component) override;

  static const std::string& boxType();

  static const std::string key_text;
  static const std::string key_levels;
  static const std::string key_style;

private:
  Linestyle style;
  bool plabelflevels;
  std::vector<int> flightlevels; //!< flight levels, unit 100 feet
};
typedef std::shared_ptr<VprofBoxFL> VprofBoxFL_p;

#endif // VPROFBOXFL_H
