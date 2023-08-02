// -*- c++ -*-
/*
 Copyright (C) 2020-2022 met.no

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

#ifndef DIANA_DIFIELD_CACHEDGRIDIO_H_
#define DIANA_DIFIELD_CACHEDGRIDIO_H_

#include "GridIOBase.h"

#include <memory>
#include <mutex>

/**
 * Caches a single request for a field from another GridIObase instance.
 */
class CachedGridIO : public GridIOBase
{
public:
  CachedGridIO(GridIOBase* base) : base_(base) { }
  ~CachedGridIO();

  const gridinventory::Inventory& getInventory() const override;

  const gridinventory::ReftimeInventory& getReftimeInventory(const std::string reftime) const override;

  void setReferencetimeLimits(const std::string& min, const std::string& max) override;

  std::set<std::string> getReferenceTimes() const override;

  const gridinventory::Grid& getGrid(const std::string& reftime, const std::string& grid) override;

  const gridinventory::Taxis& getTaxis(const std::string& reftime, const std::string& taxis) override;

  const gridinventory::Zaxis& getZaxis(const std::string& reftime, const std::string& zaxis) override;

  const gridinventory::ExtraAxis& getExtraAxis(const std::string& reftime, const std::string& extraaxis) override;

  Field_p initializeField(const std::string& modelname, const std::string& reftime, const gridinventory::GridParameter& param, const std::string& level,
                          const miutil::miTime& time, const std::string& elevel) override;

  bool sourceChanged() override;

  std::string getReferenceTime() const override;

  bool referenceTimeOK(const std::string& refTime) override;

  bool makeInventory(const std::string& reftime) override;

  Field_p getData(const std::string& reftime, const gridinventory::GridParameter& param, const std::string& level, const miutil::miTime& time,
                  const std::string& elevel) override;

  bool putData(const std::string& reftime, const gridinventory::GridParameter& param, const std::string& level, const miutil::miTime& time,
               const std::string& elevel, const std::string& unit, Field_cp field, const std::string& output_time) override;

  virtual diutil::Values_p getVariable(const std::string& varName) override;

private:
  void clear();

private:
  std::mutex mutex_;
  std::unique_ptr<GridIOBase> base_;

  std::string cached_reftime_;
  gridinventory::GridParameter cached_param_;
  std::string cached_level_;
  miutil::miTime cached_time_;
  std::string cached_elevel_;

  Field_p cached_field_;
};

#endif /* DIANA_DIFIELD_CACHEDGRIDIO_H_ */
