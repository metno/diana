#ifndef DIFIELDEXCEPTIONS_H_
#define DIFIELDEXCEPTIONS_H_

/*
  $Id$

  Copyright (C) 2007 met.no

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

#include <exception>
#include <string>

/*
 * a collection of all exceptions thrown in diField
 */

/* the base class for all diField exceptions; */

class DiFieldException : public std::exception {
protected:
  std::string message;
public:
  DiFieldException() {}
  DiFieldException(std::string msg) { setMessage(msg);}
  ~DiFieldException()  throw() {}

  void setMessage(std::string msg){message=msg;}
  const char* what() const throw()
  {return (message.empty() ? "DatabaseException" :  message.c_str()); }
};


/**
 * An exception thrown when an attempt to modify the cache fails
 */

class ModifyFieldCacheException : public DiFieldException{
public:
  ModifyFieldCacheException(std::string m="") {setMessage("Failed to modify FieldCache: "+m);}
  ~ModifyFieldCacheException() throw() {}
};


class FieldDataException : public DiFieldException {
public:
  FieldDataException(std::string m="") {setMessage("Field Data Error: "+m);}
  FieldDataException(const std::string intro, std::string message)
  { setMessage(intro + std::string(", reason:") + message);}
  ~FieldDataException() throw(){}
};




#endif /* DIFIELDEXCEPTIONS_H_ */
