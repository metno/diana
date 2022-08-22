/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2020-2022 met.no

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

#include "CachedGridIO.h"

#define MILOGGER_CATEGORY "diField.CachedGridIO"
#include "miLogger/miLogging.h"

CachedGridIO::~CachedGridIO() {}

const std::string& CachedGridIO::getSourceName() const
{
  return base_->getSourceName();
}

const gridinventory::Inventory& CachedGridIO::getInventory() const
{
  return base_->getInventory();
}

const gridinventory::ReftimeInventory& CachedGridIO::getReftimeInventory(const std::string reftime) const
{
  return base_->getReftimeInventory(reftime);
}

void CachedGridIO::setReferencetimeLimits(const std::string& min, const std::string& max)
{
  base_->setReferencetimeLimits(min, max);
}

std::set<std::string> CachedGridIO::getReferenceTimes() const
{
  return base_->getReferenceTimes();
}

const gridinventory::Grid& CachedGridIO::getGrid(const std::string& reftime, const std::string& grid)
{
  return base_->getGrid(reftime, grid);
}

const gridinventory::Taxis& CachedGridIO::getTaxis(const std::string& reftime, const std::string& taxis)
{
  return base_->getTaxis(reftime, taxis);
}

const gridinventory::Zaxis& CachedGridIO::getZaxis(const std::string& reftime, const std::string& zaxis)
{
  return base_->getZaxis(reftime, zaxis);
}

const gridinventory::ExtraAxis& CachedGridIO::getExtraAxis(const std::string& reftime, const std::string& extraaxis)
{
  return base_->getExtraAxis(reftime, extraaxis);
}

Field_p CachedGridIO::initializeField(const std::string& modelname, const std::string& reftime, const gridinventory::GridParameter& param,
                                      const std::string& level, const miutil::miTime& time, const std::string& elevel)
{
  return base_->initializeField(modelname, reftime, param, level, time, elevel);
}

bool CachedGridIO::sourceChanged()
{
  const bool changed = base_->sourceChanged();
  if (changed)
    clear();
  return changed;
}

bool CachedGridIO::makeInventory(const std::string& reftime)
{
  clear();
  return base_->makeInventory(reftime);
}

std::string CachedGridIO::getReferenceTime() const
{
  return base_->getReferenceTime();
}

bool CachedGridIO::referenceTimeOK(const std::string& refTime)
{
  return base_->referenceTimeOK(refTime);
}

Field_p CachedGridIO::getData(const std::string& reftime, const gridinventory::GridParameter& param, const std::string& level, const miutil::miTime& time,
                              const std::string& elevel)
{
  METLIBS_LOG_TIME(LOGVAL(reftime) << LOGVAL(param.name) << LOGVAL(level) << LOGVAL(time) << LOGVAL(elevel));

  std::lock_guard<std::mutex> lock(mutex_);
  if (cached_field_ && cached_reftime_ == reftime && cached_param_ == param && cached_level_ == level && cached_time_ == time && cached_elevel_ == elevel) {
    METLIBS_LOG_DEBUG("cache hit");
    return cached_field_;
  }

  METLIBS_LOG_DEBUG("cache miss");
  cached_reftime_ = reftime;
  cached_param_ = param;
  cached_level_ = level;
  cached_time_ = time;
  cached_elevel_ = elevel;
  cached_field_ = base_->getData(cached_reftime_, cached_param_, cached_level_, cached_time_, cached_elevel_);
  return cached_field_;
}

bool CachedGridIO::putData(const std::string& reftime, const gridinventory::GridParameter& param, const std::string& level, const miutil::miTime& time,
                           const std::string& elevel, const std::string& unit, Field_cp field, const std::string& output_time)
{
  clear();
  return base_->putData(reftime, param, level, time, elevel, unit, field, output_time);
}

vcross::Values_p CachedGridIO::getVariable(const std::string& varName)
{
  return base_->getVariable(varName);
}

void CachedGridIO::clear()
{
  std::lock_guard<std::mutex> lock(mutex_);
  cached_field_.reset();
}
