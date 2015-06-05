/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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

/* Created at Thu Jul 18 14:14:53 2002 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diObjectPoint.h"

#define MILOGGER_CATEGORY "diana.ObjectPoint"
#include <miLogger/miLogging.h>

ObjectPoint::ObjectPoint()
  : mXY(0, 0)
  , mMarked(false)
  , mJoined(false)
{
  METLIBS_LOG_SCOPE();
}

ObjectPoint::ObjectPoint(float xin, float yin)
  : mXY(xin, yin)
  , mMarked(false)
  , mJoined(false)
{
  METLIBS_LOG_SCOPE();
}

bool ObjectPoint::operator==(const ObjectPoint &rhs) const
{
  return mXY == rhs.mXY;
}

float ObjectPoint::distSquared(float xm, float ym) const
{
  const float dx = mXY.x() - xm, dy = mXY.y() - ym;
  return dx*dx + dy*dy;
}

bool ObjectPoint::isInRectangle(float xm, float ym, float fdeltaw) const
{
  const Rectangle myRect(mXY.x() - fdeltaw, mXY.y() - fdeltaw, mXY.x() + fdeltaw, mXY.y() + fdeltaw);
  return myRect.isinside(xm,ym);
}
