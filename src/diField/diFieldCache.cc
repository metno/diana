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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif  /* HAVE_CONFIG_H */

#include "diFieldCache.h"
#include "diField.h"

#include <puTools/miStringFunctions.h>

#include <iostream>

#define MILOGGER_CATEGORY "diField.FieldCache"
#include "miLogger/miLogging.h"

using namespace miutil;
using namespace std;

FieldCache::FieldCache()
: exists(1), maximumsize_(0), bytesize_(0)
{
}

FieldCache::FieldCache(unsigned int s, sizetype st)
: exists(1) ,  maximumsize_(0), bytesize_(0)
{
  setMaximumsize(s,st);
}

FieldCache::~FieldCache()
{
  METLIBS_LOG_SCOPE();
  if (!exists) {
    METLIBS_LOG_ERROR("trying to delete field cache twice");
    return;
  }
}

// This method empties the cache when user no longer looks at field....
bool FieldCache::flush()
{
  METLIBS_LOG_SCOPE();

  map<FieldCacheKeyset,FieldCacheEntity>::reverse_iterator itr=fields.rbegin();

  //for(;itr!=fields.rend();itr++) {
  while(itr!=fields.rend()) {
    try {
      // delete the field and the key
      erase(itr->first);
      // set itr to the end
      itr=fields.rbegin();
    }
    catch(ModifyFieldCacheException(e) ) {
      // The field is locked, ignore and itr valid
      itr++;
      METLIBS_LOG_DEBUG("exception: " << e.what());
    }
  }
  return true;
}

//<FIELDCACHE_SECTION>
// # size and sizetype for the cache
// # sizetype=0 (BYTE), 1 (KILOBYTE), 2 (MEGABYTE)
// size=1024
// size_type=2
//</FIELDCACHE_SECTION>

bool FieldCache::parseSetup(const std::vector<std::string>& lines,
    std::vector<std::string>& errors)
{
  METLIBS_LOG_SCOPE();

  std::string key;
  int nlines = lines.size();
  int i = 0;
  // init to the defaults
  unsigned int s=1024;
  sizetype st = FieldCache::MEGABYTE;
  for (; i < nlines; i++) {
    std::string mainData = "";
    std::string additionalData = "";

    vector<std::string> vstr = miutil::split(lines[i],"=");

    if (vstr.size()==2) {
      mainData = vstr[0];
      additionalData = vstr[1];

      if (mainData == "size")
      {
        s = atoi(additionalData.c_str());
      }
      else if (mainData == "size_type")
      {
        int tmp = atoi(additionalData.c_str());
        switch(tmp)
        {
        case 0:
          st = FieldCache::BYTE;
          break;

        case 1:
          st = FieldCache::KILOBYTE;
          break;

        case 2:
          st = FieldCache::MEGABYTE;
          break;
        }
      }
    }
  }
  setMaximumsize(s,st);
  return true;
}

void FieldCache::set(Field* f, bool setlock)
throw(ModifyFieldCacheException&)
{
  METLIBS_LOG_SCOPE();
  if (!f)
    return;

  FieldCacheKeyset keyset;
  keyset.setKeys(f);

  if(fields.count(keyset))
    throw ModifyFieldCacheException("Trying to set an existing Field");

  try {
    cleanbyAge();
  } catch (ModifyFieldCacheException& e) {
    throw;
  }

  unsigned long tmpsize=bytesize_+f->bytesize();
  if (maximumsize_)
    if (tmpsize > maximumsize_) {
      long overflowsize = tmpsize - maximumsize_;

      METLIBS_LOG_DEBUG("calling cleanOverFlow to make room for " << keyset);
      METLIBS_LOG_DEBUG("State of cache\n"  << *(this));

      try {
        cleanOverflow(overflowsize*2);
      } catch (ModifyFieldCacheException& e) {
        throw;
      }
    }

  FieldCacheEntity entity;
  fields[keyset] = entity;

  try {
    fields[keyset].set(f, setlock);
  } catch (ModifyFieldCacheException& e) {
    throw;
  }

  bytesize_+=fields[keyset].bytesize();

  METLIBS_LOG_DEBUG("set() " << keyset  << ": new total size " << size());
}

Field* FieldCache::get(const FieldCacheKeyset& keyset)
{
  Fields_t::iterator it = fields.find(keyset);
  if (it != fields.end())
    return it->second.get();
  else
    return 0;
}

void FieldCache::copy(const FieldCacheKeyset& keyset, const std::string& newModelName, bool forced)
  throw(ModifyFieldCacheException&)
{
  METLIBS_LOG_SCOPE();

  if(!fields.count(keyset))
    throw ModifyFieldCacheException("Trying to copy a non-existing Field");

  FieldCacheKeyset newk = keyset;
  newk.setModel(newModelName);
  METLIBS_LOG_DEBUG(keyset <<  " --- to " << newk);

  bool fieldexists=fields.count(newk);
  if(fieldexists){
    if ( !forced )
      throw ModifyFieldCacheException("Trying to copy to an existing Field");
  }

  Field* newfield=NULL;

  try {
    newfield=fields[keyset].copy(newModelName);
  }
  catch(ModifyFieldCacheException(e)) {
    throw;
  }

  if ( !fieldexists ){
    try {
      set(newfield);
    }
    catch(ModifyFieldCacheException(e)) {
      throw;
    }

  } else {
    try {
      replace(newk,newfield,true);
    }
    catch(ModifyFieldCacheException(e)) {
      throw;
    }
  }
}

void FieldCache::replace(const FieldCacheKeyset& keyset, Field* f, bool deleteOriginal)
  throw(ModifyFieldCacheException&)
{
  METLIBS_LOG_SCOPE();
  if (not f)
    throw ModifyFieldCacheException("replacing with 0 field");

  FieldCacheKeyset newk(f);

  METLIBS_LOG_DEBUG("replace" << keyset << " with: " << newk << LOGVAL(deleteOriginal));

  Fields_t::iterator it = fields.find(keyset);
  if (it == fields.end())
    throw ModifyFieldCacheException("Trying to replace a non-existing Field");

  try {
    it->second.replace(f, deleteOriginal);
  } catch(ModifyFieldCacheException& e) {
    throw;
  }
}

void FieldCache::erase(const FieldCacheKeyset& keyset)
  throw(ModifyFieldCacheException&)
{
  METLIBS_LOG_DEBUG(LOGVAL(keyset));

  if (!fields.count(keyset))
    throw ModifyFieldCacheException("Trying to erase a non-existing Field");

  long tmpsize=fields[keyset].bytesize();

  try {
    fields[keyset].clear();
  } catch(ModifyFieldCacheException& e) {
    throw;
  }

  bytesize_-=tmpsize;
  fields.erase(keyset);
}

void FieldCache::freeField(Field* f) throw(ModifyFieldCacheException&)
{
  METLIBS_LOG_SCOPE();
  if(!f || !f->data)
    return;

  if(!f->isPartOfCache) {
    METLIBS_LOG_DEBUG("Uncached Field: " << f->modelName << " " << f->paramName
        << " " << f->validFieldTime  << " deleted");
    delete f;
    f = 0;
    return;
  }

  FieldCacheKeyset keyset;
  try {
    keyset.setKeys(f);
  } catch(...) {
    f = 0;
    throw ModifyFieldCacheException("wild pointer ");
  }

  if (!unlock(keyset) ) {
    METLIBS_LOG_WARN(keyset << " is not part of the cache - but has the isPartOfCache=true. Field deleted");
    f->isPartOfCache=false;
    delete f;
  }
}


bool FieldCache::unlock(const FieldCacheKeyset& keyset)
{
  if (fields.count(keyset)) {
    fields[keyset].unlock();
    METLIBS_LOG_DEBUG(keyset << (fields[keyset].isLocked() ? " [STILL LOCKED] " : " [ UNLOCKED ] "));
    return true;
  }
  return false;
}


void FieldCache::setMaximumsize(unsigned long s, FieldCache::sizetype st)
  throw(ModifyFieldCacheException&)
{
  if (st == FieldCache::KILOBYTE)
    s*=1024;
  if (st == FieldCache::MEGABYTE)
    s*=1024*1024;

  if (s<maximumsize_) {
    long overflowsize = maximumsize_ - s;

    try {
      cleanOverflow(overflowsize);
    }
    catch(ModifyFieldCacheException& e) {
      throw;
    }
  }

  maximumsize_=s;
}


int FieldCache::cleanOverflow(long overflowsize)
  throw(ModifyFieldCacheException&)
{
  if(fields.empty())
    return 0;

  int removedfields=0;
  int maxFieldsToRemove=fields.size();

  for(int i=0; i < maxFieldsToRemove; i++) {

    map<FieldCacheKeyset,FieldCacheEntity>::iterator itr=fields.begin();
    map<FieldCacheKeyset,FieldCacheEntity>::iterator oldest=fields.begin();

    for(;itr!=fields.end();itr++) {
      if(itr->second.isLocked())
        continue;

      if ( oldest->second.lastAccessed() > itr->second.lastAccessed() )
        oldest = itr;
    }

    if(oldest->second.isLocked()) {
      // this is only possible when there is just one field (itr-first)
      // and the field is in use....
      break;
    }

    try {
      long tmpsize=oldest->second.bytesize();
      oldest->second.clear();
      removedfields++;
      METLIBS_LOG_DEBUG("erased " << oldest->second);
      fields.erase(oldest);

      bytesize_    -= tmpsize;
      overflowsize -= tmpsize;

    }
    catch(ModifyFieldCacheException(e) ) {
      METLIBS_LOG_DEBUG("removing field failed");
    }

    if(overflowsize <=0)
      return removedfields;
  }

  METLIBS_LOG_DEBUG("bytesize=" <<  bytesize_ << " overflowsize=" <<overflowsize );
  throw (ModifyFieldCacheException("couldnt clean enough space for the new field"));
}

int FieldCache::cleanbyAge(long age)
  throw(ModifyFieldCacheException&)
{
  map<FieldCacheKeyset,FieldCacheEntity>::iterator itr=fields.begin();
  int removedfields=0;

  for(;itr!=fields.end();itr++) {
    // dont remove locked fileds
    if(itr->second.isLocked())
      continue;
    // remove only fields older than age
    if(miTime::secDiff(miTime::nowTime(), itr->second.lastAccessed()) <= age)
      continue;

    long tmpsize=itr->second.bytesize();
    bool ok=true;
    try {
      itr->second.clear();
    } catch(ModifyFieldCacheException& e) {
      ok = false;
    }

    if(ok) {
      removedfields++;
      METLIBS_LOG_DEBUG("erased " << itr->second);
      fields.erase(itr);
      itr=fields.begin(); // TODO: bad treatment!
      bytesize_-=tmpsize;
    }
  }
  return removedfields;
}


// inventory functions -------------------------------------------------

bool FieldCache::hasField(const FieldCacheKeyset& keyset)
{
  return (fields.count(keyset));
}

unsigned long FieldCache::size(FieldCache::sizetype st) const
{
  if(st == FieldCache::KILOBYTE ) return bytesize_/1024;
  if(st == FieldCache::MEGABYTE ) return bytesize_/(1024*1024);
  return bytesize_;
}

unsigned long FieldCache::maximumsize(FieldCache::sizetype st) const
{
  if (st == FieldCache::KILOBYTE)
    return maximumsize_/1024;
  if (st == FieldCache::MEGABYTE)
    return maximumsize_/(1024*1024);
  return maximumsize_;
}


ostream& operator<<(ostream& out, const FieldCache& fc)
{
  out << "Field Cache inventory:========================= " << endl;
  map<FieldCacheKeyset,FieldCacheEntity>::const_iterator itr= fc.fields.begin();

  int numlocks=0;

  for(;itr!=fc.fields.end();itr++) {
    out << itr->second << endl;
    if(itr->second.isLocked())
      numlocks++;
  }

  out << "--------------------------------------------" << endl
      << "Maximumsize:     " << fc.maximumsize() << "kb " << endl
      << "Size:            " << fc.size()        << "kb " << endl
      << "Used:            " << (fc.maximumsize() ? (( fc.size() * 100 ) / fc.maximumsize() ) : 0 ) << "% " << endl
      << "Fields in cache: " << fc.fields.size() << endl
      << "Locked Fields:   " << numlocks         << endl
      << "================================================ " << endl;

  return out;
}
