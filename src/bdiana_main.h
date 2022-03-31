/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2022 met.no

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

#ifndef BDIANA_GLOBE_H
#define BDIANA_GLOBE_H

#include "bdiana_source.h"
#include "diController.h"

#include <QTransform>

#include <memory>
#include <string>

class MainPaintable;

struct BdianaMain : public BdianaSource
{
  BdianaMain();
  ~BdianaMain();

  bool MAKE_CONTROLLER();
  PlotCommand_cpv makeCommands(std::vector<std::string>& pcom);
  void commands(std::vector<std::string>& pcom);
  ImageSource* imageSource() override;

  void setAnnotationsOnly(bool ao);
  bool hasCutout() override;
  QRectF cutout() override;

  miutil::miTime getReferenceTime() override;
  plottimes_t getTimes() override;
  void setTime(const miutil::miTime& time) override;

  bool keeparea;

  std::vector<std::string> extra_field_lines;

  bool plot_trajectory = false;
  bool trajectory_started = false;
  std::string trajectory_options;

  std::unique_ptr<Controller> controller;

private:
  std::unique_ptr<MainPaintable> paintable_;
  std::unique_ptr<ImageSource> imageSource_;
};

#endif // BDIANA_GLOBE_H
