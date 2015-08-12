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

#ifndef DIFIELDCACHEKEYSET_H
#define DIFIELDCACHEKEYSET_H

#include "diField.h"
#include <puTools/miTime.h>
#include <iosfwd>

/**
 * \brief Class to Keep one the keys fot one field in the cache
 *
 * all members are private and can only be set by functions
 * to ensure that the key_ is allways uptodate...
 */

class FieldCacheKeyset {
private:
  // modelname - like  HIRLAM.20km.00
  std::string model_;

  miutil::miTime reference_;

  // parametername - like MSLP
  std::string name_;

  // valid time of the field
  miutil::miTime   valid_;

  // the string key (sum of all keys as a string)
  std::string key_;


  // levelName from field
  std::string level_;

  // idnum from field
  std::string idnum_;

  // build the key from all entries
  void makeKey();


public:
  // init miTime objects
  FieldCacheKeyset() : reference_("19700106"),valid_("19700106") {}
  FieldCacheKeyset(const Field* field) {setKeys(field);}
  FieldCacheKeyset(const FieldCacheKeyset&);

  /// set variables and run makeKey again....

  void setKeys(const Field* field);
  void setKeys(const std::string& m, const miutil::miTime& r, const std::string& n,
      const std::string& l, const std::string& i, const miutil::miTime& v);
  void setModel(const std::string& m);
  void setReference(const miutil::miTime& r);
  void setName(const std::string& n);
  void setLevel(const std::string& l);
  void setIdnum(const std::string& i);
  void setValid(const miutil::miTime& v);

  /// access variables

  const miutil::miTime& valid() const { return valid_;}
  const miutil::miTime& reference() const { return reference_;}
  const std::string& name()  const { return name_; }
  const std::string& model() const { return model_;}
  const std::string& level() const { return level_;}
  const std::string& idnum() const { return idnum_;}
  const std::string& key()   const { return key_;  }

  bool operator> (const FieldCacheKeyset& e) const { return key_>e.key_;  }
  bool operator==(const FieldCacheKeyset& e) const { return key_==e.key_; }
  bool operator!=(const FieldCacheKeyset& e) const { return key_!=e.key_; }
  bool operator<=(const FieldCacheKeyset& e) const { return key_<=e.key_; }
  bool operator>=(const FieldCacheKeyset& e) const { return key_>=e.key_; }
  bool operator< (const FieldCacheKeyset& e) const { return key_<e.key_;  }
  friend std::ostream& operator<<(std::ostream& out, const FieldCacheKeyset& e );
};

#endif
