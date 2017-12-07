/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017 met.no

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

#ifndef BDIANA_SOURCE_H
#define BDIANA_SOURCE_H

#include <QRectF>
#include <puTools/miTime.h>

class ImageSource;

class BdianaSource
{
public:
  virtual ~BdianaSource();

  virtual ImageSource* imageSource() = 0;
  virtual bool hasCutout();
  virtual QRectF cutout();

  virtual miutil::miTime getTime() = 0;
  virtual void setTime(const miutil::miTime& time) = 0;
};

#endif // BDIANA_SOURCE_H
