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

#ifndef DISATPLOTCOMMAND_H
#define DISATPLOTCOMMAND_H

#include "diKVListPlotCommand.h"
#include "diSatDialogInfo.h"

#include <puTools/miTime.h>

#include <map>

//static values that should be set from SatManager
namespace {
const float defaultCut = -1;
const int defaultAlphacut = 0;
const int defaultAlpha = 255;
const int defaultTimediff = 60;
const bool defaultClasstable = false;
} // namespace

class SatPlotCommand : public KVListPlotCommand
{
public:
  SatPlotCommand();

  std::string toString() const override;

  SatImageAndSubType sist;
  const std::string& image_name() const { return sist.image_name; }
  const std::string& subtype_name() const { return sist.subtype_name; }
  std::string plotChannels; ///< channelname for annotation
  std::string filename;     ///< filename
  miutil::miTime filetime;  ///< time

  bool mosaic;             ///<plot mosaic of pictures
  int timediff;            ///< timediff in minutes

  float cut;      ///< image cut/stretch factor
  float alphacut; ///< alpha-blending cutoff value
  float alpha;    ///< alpha-blending value

  bool classtable; ///< show colour table in legend
  std::map<int, char> coloursToHideInLegend;

  bool hasFileName() const { return !filename.empty(); }
  bool hasFileTime() const { return !filetime.undef(); }
  bool isAuto() const { return !hasFileName() && !hasFileTime(); }

  static std::shared_ptr<const SatPlotCommand> fromString(const std::string& line);
};

typedef std::shared_ptr<SatPlotCommand> SatPlotCommand_p;
typedef std::shared_ptr<const SatPlotCommand> SatPlotCommand_cp;

#endif // DISATPLOTCOMMAND_H
