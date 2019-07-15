/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2019 met.no

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

#ifndef DIOBJECTSPLOTCOMMAND_H
#define DIOBJECTSPLOTCOMMAND_H

#include "diPlotCommand.h"

#include "util/diKeyValue.h"

#include <puTools/miTime.h>

class ObjectsPlotCommand : public PlotCommand
{
public:
  ObjectsPlotCommand();

  const std::string& commandKey() const override;
  std::string toString() const override;

  miutil::KeyValue_v toKV() const;

  std::string objectname;
  std::vector<std::string> objecttypes;
  std::string file;
  miutil::miTime time;
  int timeDiff;
  int alpha; // 0..255
  int newfrontlinewidth;
  int fixedsymbolsize;
  std::vector<std::string> symbolfilter;

  static std::shared_ptr<const ObjectsPlotCommand> fromString(const std::string& line);
};

typedef std::shared_ptr<ObjectsPlotCommand> ObjectsPlotCommand_p;
typedef std::shared_ptr<const ObjectsPlotCommand> ObjectsPlotCommand_cp;

#endif // DIOBJECTSPLOTCOMMAND_H
