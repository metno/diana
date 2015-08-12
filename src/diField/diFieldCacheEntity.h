/* -*- c++ -*-

  Copyright (C) 2013 met.no

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

#ifndef DIFIELDCACHEENTITY_H
#define DIFIELDCACHEENTITY_H

#include "diFieldCacheKeyset.h"
#include "diFieldExceptions.h"
#include <puTools/miTime.h>
#include <iosfwd>

class Field;

/**
 * \brief Class to Keep one entity of a diField in diFieldCache
 *
 * This objects includes locking and id-tracking og diFields in the Cache.
 */

class FieldCacheEntity {

private:

  Field* field;

  /// number of locks on that field
  int locks;

  /// keys are kept separately from the field, because they can
  /// stay constant while the field is replaced by an edited version

  FieldCacheKeyset keyset_;

  /// size estimate in bytes
  unsigned long bytesize_;

public:
  FieldCacheEntity() : field(0), locks(0) {}
  FieldCacheEntity(Field* f) : field(0), locks(0) { set(f); }
  ~FieldCacheEntity();

  /// functions -------------------------------

  /// set a field (construction)
  void set(Field*, bool setlock=false)     throw(ModifyFieldCacheException&);

  /// replace this field, but keep all keys and the original pointer
  void replace(Field*, bool deleteOriginal=false) throw(ModifyFieldCacheException&);

  /// delete the field, only possible if there is no lock!
  /// (forced is called by the destructor...)
  void clear(bool forced=false)  throw(ModifyFieldCacheException&);

  /// open a lock
  void unlock();

  /// make a local copy of your field for instance profet to profet.tmp.1
  Field* copy(const std::string& name) throw(ModifyFieldCacheException&);

  /// get a field

  Field* get();



  /// keys -----------------------------------

  const std::string& model() const { return keyset_.model();  }
  const miutil::miTime& reference() const { return keyset_.reference();  }
  const std::string& name()  const { return keyset_.name();   }
  const miutil::miTime& valid() const { return keyset_.valid();  }

  const FieldCacheKeyset& keyset() const { return keyset_;}

  const std::string& key() const { return keyset_.key();}

  /// diverse

  /// size in bytes (estimated)
  unsigned long bytesize()      const { return bytesize_;}


  bool isLocked() const { return bool(locks);}
  const miutil::miTime& lastAccessed() const { return field->lastAccessed; }

  /// operators ------------------------------

  bool operator<( const FieldCacheEntity& e) { return key()< e.key(); }
  bool operator>( const FieldCacheEntity& e) { return key()> e.key(); }
  bool operator==(const FieldCacheEntity& e) { return key()==e.key(); }
  bool operator!=(const FieldCacheEntity& e) { return key()!=e.key(); }
  bool operator<=(const FieldCacheEntity& e) { return key()<=e.key(); }
  bool operator>=(const FieldCacheEntity& e) { return key()>=e.key(); }
  friend std::ostream& operator<<(std::ostream& out, const FieldCacheEntity& e);
};

#endif
