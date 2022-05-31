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

#include "qtDoubleStepAdapter.h"

#include <cmath>

namespace diutil {

inline double mag10(double v)
{
  return std::floor(std::log10(v));
}

double adaptedStep(double value, int sb_decimals)
{
  const double valua = std::abs(value);
  const double factor = std::pow(10, 1 - mag10(valua));
  const double valur = std::round(valua * factor) / factor;
  return std::pow(10, std::max(-1.0 * sb_decimals, mag10(valur) - 1));
}

DoubleStepAdapter::DoubleStepAdapter(QDoubleSpinBox* parent)
    : QObject(parent)
{
  connect(parent, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &DoubleStepAdapter::adaptValue);
}

DoubleStepAdapter::~DoubleStepAdapter() {}

void DoubleStepAdapter::adaptValue(double value)
{
  QDoubleSpinBox* sb = static_cast<QDoubleSpinBox*>(parent());
  sb->setSingleStep(adaptedStep(value, sb->decimals()));
}

} // namespace diutil
