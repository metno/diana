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

#ifndef VPROFAXES_H
#define VPROFAXES_H

#include "diField/diPoint.h"
#include "diRectangle.h"

#include <memory>

class VprofAxes
{
public:
  virtual ~VprofAxes();
  virtual void setPaintRange(const Rectangle& rect) = 0;
  virtual Rectangle paintRange() const = 0;

  virtual diutil::PointF value2paint(const diutil::PointF& value) const = 0;
  virtual diutil::PointF paint2value(const diutil::PointF& canvas) const = 0;
};
typedef std::shared_ptr<VprofAxes> VprofAxes_p;
typedef std::shared_ptr<const VprofAxes> VprofAxes_cp;

#endif // VPROFAXES_H
