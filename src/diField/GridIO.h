// -*- c++ -*-
/*
 Copyright (C) 2006-2022 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 email: diana@met.no

 This is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*
 * GridIO.h
 *
 *  Created on: Mar 11, 2010
 *      Author: audunc
 */

#ifndef DIANA_DIFIELD_GRIDIO_H_
#define DIANA_DIFIELD_GRIDIO_H_

#include "GridIOBase.h"

/**
 * Base class for all grid sources with content.
 */
class GridIO : public GridIOBase
{
protected:
  gridinventory::Inventory inventory; ///< The inventory for this grid source

  std::string limit_min; ///< only include reference times greater than this
  std::string limit_max; ///< only include reference times less than this

public:
  GridIO();
  ~GridIO();

  const gridinventory::Inventory& getInventory() const override { return inventory; }

  const gridinventory::ReftimeInventory& getReftimeInventory(const std::string reftime) const override;

  void setReferencetimeLimits(const std::string& min, const std::string& max) override
  {
    limit_min = min;
    limit_max = max;
  }

  std::set<std::string> getReferenceTimes() const override;

  //! get the grid from reftime with name = grid
  const gridinventory::Grid& getGrid(const std::string& reftime, const std::string& grid) override;

  //! get the Taxis from reftime with name = taxis
  const gridinventory::Taxis& getTaxis(const std::string& reftime, const std::string& taxis) override;

  //! get the Zaxis from reftime with name = zaxis
  const gridinventory::Zaxis& getZaxis(const std::string& reftime, const std::string& zaxis) override;

  //! get the extraaxis from reftime with name = extraaxis
  const gridinventory::ExtraAxis& getExtraAxis(const std::string& reftime, const std::string& extraaxis) override;

  //! make and initialize Field
  Field_p initializeField(const std::string& modelname, const std::string& reftime, const gridinventory::GridParameter& param, const std::string& level,
                          const miutil::miTime& time, const std::string& elevel) override;

private:
  const gridinventory::ReftimeInventory& findModelAndReftime(const std::string& reftime) const;
};

#endif /* DIANA_DIFIELD_GRIDIO_H_ */
