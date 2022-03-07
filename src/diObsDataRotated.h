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

#ifndef DIOBSDATAROTATED_H
#define DIOBSDATAROTATED_H

#include "diObsDataContainer.h"

/*! Special observation data container which supports storage of rotated values for use in ObsPlot.
 *
 * When changing projections, ObsPlot needs to rotate some data (e.g., wind vectors).
 * This class allows access to rotated and unrotated observation values.
 *
 * It implements ObsDataContainer for efficiency: less code and if there are no rotated data,
 * ObsDataRef may point directly to the underlying container.
 */
class ObsDataRotated : public ObsDataContainer
{
public:
  ObsDataRotated(ObsDataContainer_cp c = ObsDataContainer_cp());

  bool empty() const override { return !container_ || container_->empty(); }
  size_t size() const override { return container_ ? container_->size() : 0; }
  ObsDataRef at(size_t idx) const override;

  const ObsDataBasic& basic(size_t i) const override { return container_->basic(i); }
  const ObsDataMetar& metar(size_t i) const override { return container_->metar(i); }

  std::vector<std::string> get_keys() const override;
  const float* get_float(size_t i, const std::string& key) const override;
  const std::string* get_string(size_t i, const std::string& key) const override;

  const float* get_unrotated_float(size_t i, const std::string& key) const;
  void put_rotated_float(size_t i, const std::string& key, float value);
  void put_float(size_t i, const std::string& key, float value);
  void put_string(size_t i, const std::string& key, const std::string& value);

private:
  ObsDataContainer_cp container_;
  std::vector<ObsData::stringdata_t> stringdata_plot_;
  std::vector<ObsData::fdata_t> fdata_plot_;
  std::vector<ObsData::fdata_t> fdata_rotated_;
};

#endif // DIOBSDATAROTATED_H
