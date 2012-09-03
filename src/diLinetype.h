/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diLinetype.h 3273 2010-05-18 17:32:21Z dages $

  Copyright (C) 2006 met.no

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
#ifndef diLinetype_h
#define diLinetype_h

#include <vector>
#include <map>
#include <puTools/miString.h>
#include <puCtools/porttypes.h>

using namespace std;

/**
   \brief Line stippling for plotting

   Contains definitions for one type of line stippling, and static lists of all defined types
*/

class Linetype {
public:
  // Constructors
  Linetype();
  Linetype(const miutil::miString& _name);

  // Assignment operator
  Linetype& operator=(const Linetype &rhs);
  // Equality operator
  bool operator==(const Linetype &rhs) const;

  /// clear static data
  static void init();
  /// define a new lint type from name, bitmap and repeat factor
  static void define(const miutil::miString& _name,
		     uint16 _bmap= 0xFFFF, int _factor= 1);

  /// return default line type
  static Linetype getDefaultLinetype() { return defaultLinetype; }

  /// return names of defined line types
  static vector<miutil::miString> getLinetypeNames() { return linetypeSequence; }
  /// return all line types in string code
  static vector<miutil::miString> getLinetypeInfo();
  /// return all line names and types in string code
  static void getLinetypeInfo(vector<miutil::miString>& name,
			      vector<miutil::miString>& pattern);

  miutil::miString name;     ///< name of line type
  bool     stipple;  ///< not solid
  uint16   bmap;     ///< bitmap defing stipple pattern
  int      factor;   ///< repeat factor

private:
  static map<miutil::miString,Linetype> linetypes;
  static vector<miutil::miString> linetypeSequence;
  static Linetype defaultLinetype;

  // Copy members
  void memberCopy(const Linetype& rhs);

};

#endif
