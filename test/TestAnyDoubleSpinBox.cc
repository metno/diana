/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018-2022 met.no

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

#include "util/qtAnyDoubleSpinBox.h"

#include <cmath>

#include <gtest/gtest.h>

using diutil::AnyDoubleSpinBox;

TEST(AnyDoubleSpinBox, SetValue)
{
  std::unique_ptr<AnyDoubleSpinBox> adsb(new AnyDoubleSpinBox(0));
  const double values[] = {1, 1e8, 1.2234e-12};
  for (auto value : values) {
    adsb->setValue(value);
    EXPECT_FLOAT_EQ(adsb->value(), value);
  }
}

TEST(AnyDoubleSpinBox, SetValueWithRange)
{
  std::unique_ptr<AnyDoubleSpinBox> adsb(new AnyDoubleSpinBox(0));
  const double min = 0, max = 100;
  adsb->setRange(0, 100);
  const double values[] = {-1, 12, 1.2234e-12, 120};
  for (auto value : values) {
    adsb->setValue(value);
    const auto expect = std::min(max, std::max(min, value));
    EXPECT_FLOAT_EQ(adsb->value(), expect);
  }
}

TEST(AnyDoubleSpinBox, StepBy)
{
  std::unique_ptr<AnyDoubleSpinBox> adsb(new AnyDoubleSpinBox(0));
  adsb->setValue(1);

  adsb->stepBy(1);
  EXPECT_FLOAT_EQ(1.1, adsb->value());
  adsb->stepBy(-1);
  EXPECT_FLOAT_EQ(1, adsb->value());

  adsb->stepBy(10);
  EXPECT_FLOAT_EQ(2, adsb->value());
  adsb->stepBy(-10);
  EXPECT_FLOAT_EQ(1, adsb->value());
}

TEST(AnyDoubleSpinBox, StepByWithRange)
{
  std::unique_ptr<AnyDoubleSpinBox> adsb(new AnyDoubleSpinBox(0));
  adsb->setRange(0.95, 1.05);

  adsb->setValue(1);
  adsb->stepBy(1);
  EXPECT_FLOAT_EQ(1.05, adsb->value());

  adsb->setValue(1);
  adsb->stepBy(-1);
  EXPECT_FLOAT_EQ(0.95, adsb->value());
}

TEST(AnyDoubleSpinBox, Off)
{
  std::unique_ptr<AnyDoubleSpinBox> adsb(new AnyDoubleSpinBox(0));
  adsb->setValue(std::nan(""));
  EXPECT_EQ(0, adsb->value());

  adsb->setSpecialValueText("off");
  adsb->setValue(std::nan(""));
  EXPECT_TRUE(adsb->isOff());

  adsb->stepBy(1);
  EXPECT_FALSE(adsb->isOff());
  EXPECT_FLOAT_EQ(0.1, adsb->value());
}
