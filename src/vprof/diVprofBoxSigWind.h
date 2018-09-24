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

#ifndef VPROFBOXSIGWIND_H
#define VPROFBOXSIGWIND_H

#include "diVprofBoxZ.h"

class VprofBoxSigWind : public VprofBoxZ
{
public:
  VprofBoxSigWind();

  bool separate() const override;

  void updateLayout() override;
  void plotDiagram(VprofPainter* p) override;
  void plotDiagramFrame(VprofPainter* p) override;
  void plotValues(VprofPainter* p, VprofValues_cp values, const VprofModelSettings& ms) override;

  size_t graphCount() override;
  size_t graphComponentCount(size_t graph) override;
  std::string graphComponentVarName(size_t graph, size_t component) override;

  static const std::string& boxType();

protected:
  void configureDefaults() override;
};
typedef std::shared_ptr<VprofBoxSigWind> VprofBoxSigWind_p;

#endif // VPROFDIAGRAM_H
