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

#ifndef DIOBSDATAUNION_H
#define DIOBSDATAUNION_H

#include "diObsDataContainer.h"

/*! ObsDataContainer implementation joining one or more ObsDataContainer instances.
 *
 * The implementation assumes that ObsDataContainer instances do not change after being added.
 *
 * Used in ObsManager to support reading observations for a single plot from multiple "readers",
 * where each reader returns a read-only ObsDataContainer instances.
 */
class ObsDataUnion : public ObsDataContainer
{
public:
  ObsDataUnion();

  void add(ObsDataContainer_cp c);

  //! if this wraps a single container, return that one; else return nullptr
  ObsDataContainer_cp single() const;

  size_t size() const override;

  //! obtain an ObsDataRef pointing directly to the container with data for the given index
  ObsDataRef at(size_t idx) const override;

  const ObsDataBasic& basic(size_t i) const override;
  const ObsDataMetar& metar(size_t i) const override;

  std::vector<std::string> get_keys() const override;
  const float* get_float(size_t i, const std::string& key) const override;
  const std::string* get_string(size_t i, const std::string& key) const override;

private:
  std::pair<ObsDataContainer_cp, size_t> find(size_t index) const;

private:
  //! list of joined containers
  std::vector<ObsDataContainer_cp> containers_;

  /*! list of accumulated sizes of joined containers'
   *
   * For three containers with sizes 3, 6, and 1, the list would contain 3, 3+6, 3+6+1.
   */
  std::vector<size_t> sizes_;
};

typedef std::shared_ptr<ObsDataUnion> ObsDataUnion_p;

#endif // DIOBSDATAUNION_H
