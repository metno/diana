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

#ifndef VPROFAXESSTANDARD_H
#define VPROFAXESSTANDARD_H

#include "diVprofAxes.h"
#include "vcross_v2/VcrossQtAxis.h"

class VprofAxesStandard : public VprofAxes
{
public:
  VprofAxesStandard();
  vcross::detail::AxisPtr z;
  vcross::detail::AxisPtr x;

  void setPaintRange(const Rectangle& rect) override;
  Rectangle paintRange() const override;
  diutil::PointF value2paint(const diutil::PointF& value) const override;
  diutil::PointF paint2value(const diutil::PointF& paint) const override;
};
typedef std::shared_ptr<VprofAxesStandard> VprofAxesStandard_p;
typedef std::shared_ptr<const VprofAxesStandard> VprofAxesStandard_cp;

#endif // VPROFAXESSTANDARD_H
