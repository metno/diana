/*
 * GridCollection.h
 *
 *  Created on: Mar 15, 2010
 *      Author: audunc
 */
/*
 $Id:$

 Copyright (C) 2006 met.no

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

#ifndef GRIDDATAKEY_H_
#define GRIDDATAKEY_H_

#include <string>
#include <puTools/miTime.h>

class GridDataKey {
public:


   std::string modelName;
   miutil::miTime refTime;
   std::string paramName;
   std::string grid;
   std::string zaxis;
   std::string taxis;
   std::string eaxis;
   std::string version;
   std::string vlevel;
   miutil::miTime time;
   std::string elevel;
   int  time_tolerance;
   int refhour;
   int refoffset;
   GridDataKey() :
     time_tolerance(0), refhour(-1), refoffset(0)
   {
   }

};

#endif /* GRIDDATAKEY_H_ */
