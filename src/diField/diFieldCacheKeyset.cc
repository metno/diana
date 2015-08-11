/*
  Copyright (C) 2006-2015 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diFieldCacheKeyset.h"

#include <iostream>

using namespace miutil;

FieldCacheKeyset::FieldCacheKeyset(const FieldCacheKeyset& fck)
{
  model_ = fck.model_;
  reference_ = fck.reference_;
  name_  = fck.name_;
  valid_ = fck.valid_;
  key_   = fck.key_;
  level_ = fck.level_;
  idnum_ = fck.idnum_;
  makeKey();
}

void FieldCacheKeyset::setModel(const std::string& m)
{
 model_ = m;
 makeKey();
}

void FieldCacheKeyset::setReference (const miutil::miTime& r)
{
  reference_ = r;
  makeKey();
}

void FieldCacheKeyset::setName(const std::string& n)
{
  name_  = n;
  makeKey();
}

void FieldCacheKeyset::setLevel(const std::string& l)
{
  level_=l;
  makeKey();
}

void FieldCacheKeyset::setIdnum(const std::string& i)
{
  idnum_=i;
  makeKey();
}

void FieldCacheKeyset::setValid(const miutil::miTime& v)
{
  valid_ = v;
  makeKey();
}

void FieldCacheKeyset::setKeys(const std::string& m, const miutil::miTime& r, const std::string& n,
    const std::string& l, const std::string& i, const miutil::miTime& v)
{
  model_ = m;
  reference_ = r;
  name_  = n;
  level_ = l;
  idnum_ = i;
  valid_ = v;
  makeKey();
}

void FieldCacheKeyset::setKeys(const Field* field)
{
  setKeys(field->modelName,
      field->analysisTime,
      field->paramName,
      field->leveltext,
      field->idnumtext,
      field->validFieldTime);
}

void FieldCacheKeyset::makeKey()
{
  key_=model_+reference_.isoTime()+name_+level_+idnum_+valid_.isoTime();
}

std::ostream& operator<<(std::ostream& out,const FieldCacheKeyset& e )
{
  out << "Model: "   << e.model()
      << " | reference: " << e.reference()
      << " | Name: " << e.name()
      << " | valid: " << e.valid()
      << " | level: " << e.level()
      << " | idnum: " << e.idnum();

  return out;
}
