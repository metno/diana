/*
  Copyright (C) 2006 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "diana_config.h"

#include "diFieldCacheEntity.h"
#include "diField.h"

#include <iostream>

using namespace miutil;

FieldCacheEntity::~FieldCacheEntity()
{
  if (field) {
    field->isPartOfCache = false;
    delete field;
    field = 0;
  }
}

void FieldCacheEntity::set(Field* f)
{
  if (field) {
    try {
      clear();
    } catch(ModifyFieldCacheException& e) {
      throw;
    }
  }
  field     = f;
  keyset_.setKeys(field);
  locks = 1;
  bytesize_ =  field->bytesize();
  field->isPartOfCache=true;
  field->lastAccessed=miTime::nowTime();
}

Field* FieldCacheEntity::get()
{
  locks += 1;
  field->lastAccessed = miTime::nowTime();
  return field;
}

void FieldCacheEntity::replace(Field* f)
{
  if (f) {
    if (!field)
      field = new Field(*f);
    else
      (*field) = (*f);

    field->isPartOfCache = true;
    field->lastAccessed = miTime::nowTime();
  }
}

void FieldCacheEntity::clear()
{
  if (locks)
    throw ModifyFieldCacheException("trying to delete a locked field");

  field->isPartOfCache = false;
  delete field;
  field = 0;
  bytesize_=0;
}

void FieldCacheEntity::unlock()
{
  if (locks)
    locks -= 1;
}

std::ostream& operator<<(std::ostream& out,const FieldCacheEntity& e)
{
  out << e.keyset_
      << " | locks : " << e.locks
      << " | size: "   << e.bytesize_ << "b ";
  return out;
}
