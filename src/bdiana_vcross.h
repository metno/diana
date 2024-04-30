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

#ifndef BDIANA_VCROSS_H
#define BDIANA_VCROSS_H

#include "bdiana_source.h"
#include "vcross_v2/VcrossQtManager.h"

class VcrossImageSource;

struct DianaVcross : public BdianaSource
{
  DianaVcross();
  ~DianaVcross();

  void MAKE_VCROSS();
  void commands(const std::vector<std::string>& pcom);
  ImageSource* imageSource() override;

  miutil::miTime getReferenceTime() override;
  plottimes_t getTimes() override;
  bool setTime(const miutil::miTime& time) override;

  vcross::QtManager_p manager;
  std::unique_ptr<VcrossImageSource> imageSource_;
};

#endif // BDIANA_VCROSS_H
