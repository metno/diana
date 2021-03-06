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

#ifndef VPROFPLOTCOMMAND_H
#define VPROFPLOTCOMMAND_H

#include "diKVListPlotCommand.h"

class VprofPlotCommand;

typedef std::shared_ptr<VprofPlotCommand> VprofPlotCommand_p;
typedef std::shared_ptr<const VprofPlotCommand> VprofPlotCommand_cp;

typedef std::vector<VprofPlotCommand_p> VprofPlotCommand_pv;
typedef std::vector<VprofPlotCommand_cp> VprofPlotCommand_cpv;

class VprofPlotCommand : public KVListPlotCommand
{
public:
  enum Type {
    OPTIONS, //!< old vprof options
    STATION, //!< station selection
    MODELS,  //!< old vprof model/obs selection
    DIAGRAM, //!< diagram options
    BOX,     //!< add box
    GRAPH,   //!< add graph to box
    DATA,    //!< select model/observation
  };

  VprofPlotCommand(Type type);

  VprofPlotCommand(Type type, const std::string& command);

  std::string toString() const override;
  static VprofPlotCommand_cp fromString(const std::string& text);

  Type type() const { return type_; }

  const std::vector<std::string>& items() const { return items_; }

  void setItems(const std::vector<std::string>& items) { items_ = items; }

private:
  Type type_;
  std::vector<std::string> items_; //! models or stations
};

#endif // VPROFPLOTCOMMAND_H
