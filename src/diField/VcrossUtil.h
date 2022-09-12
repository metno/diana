/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2022 met.no

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

#ifndef VCROSSUTIL_HH
#define VCROSSUTIL_HH 1

#include "VcrossData.h"
#include <puTools/miTime.h>

namespace vcross {
namespace util {

/** Mapping for amble diagram, linear for p < 500hPa, logarithmic for p > 500hPa.
 * \param p pressure in hPa
 */
float ambleFunction(float p);

//! Inverse of ambleFunction.
float ambleFunctionInverse(float x);

float exnerFunction(float p);

float exnerFunctionInverse(float e);

float coriolisFactor(float lat /* radian */);

/*! Change idx, wrapping around at max. max is rounded up to max%step==0.
 */ 
int stepped_index(int idx, int step, int max);

/*! Change idx as in stepped_index, and return true if it actually changed.
 */ 
bool step_index(int& idx, int step, int max);

template<typename T>
void from_set(std::vector<T>& v, const std::set<T>& s)
{
  v.clear();
  v.insert(v.end(), s.begin(), s.end());
}

// ########################################################################

bool isCommentLine(const std::string& line);
bool nextline(std::istream& config, std::string& line);

int vc_select_cs(const std::string& ucs, Inventory_cp inv);

extern const char SECONDS_SINCE_1970[];
extern const char DAYS_SINCE_0[];

Time from_miTime(const miutil::miTime& mitime);
miutil::miTime to_miTime(const std::string& unit, Time::timevalue_t value);
inline miutil::miTime to_miTime(const Time& time)
{ return to_miTime(time.unit, time.value); }

// ########################################################################

struct WindArrowFeathers {
  int n50, n10, n05;
  WindArrowFeathers()
    : n50(0), n10(0), n05(0) { }
  WindArrowFeathers(int i50, int i10, int i05)
    : n50(i50), n10(i10), n05(i05) { }
};

/*!
 * Count feathers for a wind arrow.
 * \param ff_knots wind speed in knots
 */
WindArrowFeathers countFeathers(float ff_knots);

float FL_to_hPa(float fl);

float hPa_to_FL(float hPa);

} // namespace util
} // namespace vcross

#endif // VCROSSUTIL_HH
