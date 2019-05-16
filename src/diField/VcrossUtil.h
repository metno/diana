/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2018 met.no

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
#include <fimex/Units.h>

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

enum UnitConvertibility {
  UNITS_MISMATCH=0,  //! no unit conversion known
  UNITS_CONVERTIBLE, //! units are convertible with a function
  UNITS_LINEAR,      //! units are convertible with a linear transformation
  UNITS_IDENTICAL    //! units are the same
};

UnitConvertibility unitConvertibility(const std::string& ua, const std::string& ub);

inline bool unitsIdentical(const std::string& ua, const std::string& ub)
{ return unitConvertibility(ua, ub) == UNITS_IDENTICAL; }

inline bool unitsConvertible(const std::string& ua, const std::string& ub)
{ return unitConvertibility(ua, ub) != UNITS_MISMATCH; }

//! result unit after multiplication or division
std::string unitsMultiplyDivide(const std::string& ua, const std::string& ub, bool multiply);

//! result unit after taking root
std::string unitsRoot(const std::string& u, int root=2);

MetNoFimex::UnitsConverter_p unitConverter(const std::string& ua, const std::string& ub);

Values_cp unitConversion(Values_cp valuesIn, const std::string& unitIn, const std::string& unitOut);
float unitConversion(float valueIn, const std::string& unitIn, const std::string& unitOut);

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
