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

#ifndef DIFieldPlotCommand_H
#define DIFieldPlotCommand_H

#include "diPlotCommand.h"
#include "util/diKeyValue.h"

class FieldPlotCommand;
typedef std::shared_ptr<FieldPlotCommand> FieldPlotCommand_p;
typedef std::shared_ptr<const FieldPlotCommand> FieldPlotCommand_cp;

class FieldPlotCommand : public PlotCommand
{
public:
  struct FieldSpec
  {
    std::string model;
    std::string reftime;
    std::string plot; // "plot" is used if requesting a predefined plot, else "parameters" is/are used
    std::vector<std::string> parameters;
    bool isPredefinedPlot() const { return parameters.empty(); }

    std::string vcoord;
    std::string vlevel;

    std::string ecoord;
    std::string elevel;

    std::string units;

    int hourOffset;
    int hourDiff;

    FieldSpec();
    miutil::KeyValue_v toKV() const;
    const std::string& name() const;
  };

public:
  FieldPlotCommand(bool edit);

  const std::string& commandKey() const override;
  std::string toString() const override;

  void addOptions(const miutil::KeyValue_v& opts);
  void clearOptions();
  const miutil::KeyValue_v& options() const { return options_; }

  miutil::KeyValue_v toKV() const;
  bool hasMinusField() const { return !minus.model.empty(); }

  static FieldPlotCommand_cp fromKV(const miutil::KeyValue_v& kvs, bool edit);
  static FieldPlotCommand_cp fromString(const std::string& text, bool edit);

public:
  bool isEdit;
  FieldSpec field;
  FieldSpec minus;
  bool allTimeSteps;
  std::string time;

private:
  miutil::KeyValue_v options_;
};

#endif // DIFieldPlotCommand_H
