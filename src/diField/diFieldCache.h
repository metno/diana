// -*- c++ -*-
/*

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

#ifndef DIFIELDCACHE_H
#define DIFIELDCACHE_H

#include "diFieldCacheEntity.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <vector>

class Field;

/**
 * \brief Class to Keep the diFieldCache in diFieldManager
 *
 * the cache is responsible for locking, deleting etc. for all Fields in
 * DIANA. the multiple_index  container provides access by different views
 * (search by parameters, times etc.)
 */
class FieldCache {
public:
  enum sizetype{BYTE,KILOBYTE,MEGABYTE};

private:
  int exists;

  typedef boost::multi_index::multi_index_container<
    FieldCacheEntity,
    boost::multi_index::indexed_by<
      /// ordered by FieldCacheEntity operators
      boost::multi_index::ordered_unique<boost::multi_index::identity<FieldCacheEntity> >
      >
    > FieldContainer;

  /// maximum size for cache in kilobytes, 0 means no limit
  unsigned long maximumsize_;
  unsigned long bytesize_;

private:
  // hack ----
  typedef std::map<FieldCacheKeyset, FieldCacheEntity> Fields_t;
  Fields_t fields;

  /// free a lock
  bool unlock(const FieldCacheKeyset& keyset);


public:
  // the fieldCache settings
  static inline std::string section() {return "FIELDCACHE_SECTION";}

  bool parseSetup(const std::vector<std::string>& lines, std::vector<std::string>& errors);

  // clean the cache to avoid memory consumption
  bool flush();

  FieldCache();
  FieldCache(unsigned int s, sizetype st=FieldCache::KILOBYTE);
  ~FieldCache();

  /// manipulation functions-------------------------------------------

  /// put a new Field into the cache
  void set(Field*, bool setlock=false) throw(ModifyFieldCacheException&);

  /// make a local copy of your field for instance profet to profet.tmp.1
  void copy(const FieldCacheKeyset& keyset, const std::string& newModelName, bool forced=false)
    throw(ModifyFieldCacheException&);

  /// replace this field, but keep all keys and the pointer
  void replace(const FieldCacheKeyset& keyset, Field*, bool deleteOriginal=false)
    throw(ModifyFieldCacheException&);

  /// delete the field, only possible if there is no lock!
  void erase(const FieldCacheKeyset& keyset)
    throw(ModifyFieldCacheException&);

  /// free a lock or delete the field if its not in the cache
  void freeField(Field* f) throw(ModifyFieldCacheException&);

  /// get a particular field - returns NULL if the field does not exist
  Field* get(const FieldCacheKeyset& keyset);

  /// set new maximum size in kb and refacturate thelibs/diField/src/ cache

  void setMaximumsize(unsigned long s, FieldCache::sizetype st=KILOBYTE)
   throw(ModifyFieldCacheException&);

  /// clean functions - return value is the number of removed fields

  int  cleanOverflow(long overflowsize)
    throw(ModifyFieldCacheException&);
  int cleanbyAge(long age = 300)
    throw(ModifyFieldCacheException&);

  /// inventory functions -------------------------------------------------
  bool hasField(const FieldCacheKeyset& keyset);

  /// cache size estimate in bytes
  unsigned long size(        FieldCache::sizetype st=FieldCache::KILOBYTE) const;
  unsigned long maximumsize( FieldCache::sizetype st=FieldCache::KILOBYTE) const;

  bool     isActive() const { return maximumsize_!=0;}

  friend std::ostream& operator<<(std::ostream& out,const FieldCache& );
};

typedef boost::shared_ptr<FieldCache> FieldCachePtr;

#endif
