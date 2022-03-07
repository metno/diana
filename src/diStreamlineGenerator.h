/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2022 met.no

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

#ifndef DIANA_STREAMLINEGENERATOR_H
#define DIANA_STREAMLINEGENERATOR_H 1

#include "diField/diFieldFwd.h"
#include "diPoint.h"

#include <vector>

class StreamlineGenerator
{
public:
  StreamlineGenerator(Field_cp u, Field_cp v);

  struct SLP
  {
    diutil::PointF pos;
    float speed;
  };
  typedef std::vector<SLP> SLP_v;

  /*! generate streamline segment
   * \param from start position in grid coordinates
   * \param count max number of positions to generate
   * \return streamline segment
   */
  SLP_v generate(const diutil::PointF& from, const size_t count);

private:
  diutil::PointF interpolate(const diutil::PointF& pos) const;

private:
  Field_cp u_;
  Field_cp v_;
};

typedef std::shared_ptr<StreamlineGenerator> StreamlineGenerator_p;

#endif // DIANA_STREAMLINEGENERATOR_H
