/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2018 met.no

 Contact information:
 Norwegian Meteorological Institute
 BoxFactory 43 Blindern
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

#include "diVprofBoxFactory.h"

#include "diVprofBoxFL.h"
#include "diVprofBoxLine.h"
#include "diVprofBoxPT.h"
#include "diVprofBoxSigWind.h"
#include "diVprofBoxVerticalWind.h"
#include "diVprofBoxWind.h"

namespace vprof {

template <class B>
void createBoxForType(VprofBox_p& box, const std::string& type)
{
  if (!box && type == B::boxType())
    box = std::make_shared<B>();
}

VprofBox_p createBox(const miutil::KeyValue_v& config)
{
  size_t i_type = miutil::rfind(config, "type");
  if (i_type == size_t(-1))
    return VprofBox_p();
  size_t i_id = miutil::rfind(config, "id");
  if (i_id == size_t(-1))
    return VprofBox_p();

  const std::string& type = config.at(i_type).value();
  const std::string& id = config.at(i_id).value();

  VprofBox_p box;
  createBoxForType<VprofBoxSigWind>(box, type);
  createBoxForType<VprofBoxFL>(box, type);
  createBoxForType<VprofBoxPT>(box, type);
  createBoxForType<VprofBoxWind>(box, type);
  createBoxForType<VprofBoxVerticalWind>(box, type);
  createBoxForType<VprofBoxLine>(box, type);

  if (box) {
    box->setId(id);
    box->configure(config);
  }

  return box;
}

} // namespace vprof
