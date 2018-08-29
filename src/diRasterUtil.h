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

#ifndef diRasterUtil_h
#define diRasterUtil_h 1

#include "diColour.h"
#include "diGlUtilities.h"

#include <vector>

class PlotOptions;
class DiPainter;

namespace diutil {

class is_undef
{
public:
  is_undef(const float* fd, int w)
      : fielddata(fd)
      , width(w)
  {
  }
  inline bool operator()(int ix, int iy) const { return diutil::is_undefined(fielddata[diutil::index(width, ix, iy)]); }

private:
  const float* fielddata;
  int width;
};

class ColourLimits
{
public:
  ColourLimits()
      : colours(0)
      , limits(0)
      , ncl(0)
  {
  }

  void initialize(const PlotOptions& poptions, float mini, float maxi);
  void setColour(DiPainter* gl, float c) const;

  operator bool() const { return ncl > 0; }

private:
  const std::vector<Colour>* colours;
  const std::vector<float>* limits;
  std::vector<Colour> colours_default;
  std::vector<float> limits_default;
  size_t ncl;
};

bool mini_maxi(const float* data, size_t n, float UNDEF, float& mini, float& maxi);

void mincut_maxcut(const float* image, size_t size, float UNDEF, float cut, float& mincut, float& maxcut);

} // namespace diutil

#endif // diRasterUtil_h
