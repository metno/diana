
#ifndef VCROSSUTIL_HH
#define VCROSSUTIL_HH 1

#include "VcrossData.h"
#include <puTools/miTime.h>
#include <fimex/Units.h>

namespace vcross {
namespace util {

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

template<typename T1, typename T2>
void minimize(T1& a, const T2& b)
{
  if (b < a)
    a = b;
}

template<typename T1, typename T2>
void maximize(T1& a, const T2& b)
{
  if (b > a)
    a = b;
}

template<typename T1, typename T2>
void minimaximize(T1& mi, T1& ma, const T2& b)
{
  if (b > ma)
    ma = b;
  if (b < mi)
    mi = b;
}

template<typename T1>
bool value_between(const T1& v, const T1& lim0, const T1& lim1)
{
  if (lim0 <= lim1)
    return lim0 <= v and v <= lim1;
  else
    return lim1 <= v and v <= lim0;
}

template<typename T1>
T1 constrain_value(const T1& v, const T1& lim0, const T1& lim1)
{
  if (lim0 <= lim1) {
    if (v < lim0)
      return lim0;
    else if (lim1 < v)
      return lim1;
    else
      return v;
  } else {
    if (v < lim1)
      return lim1;
    else if (lim0 < v)
      return lim0;
    else
      return v;
  }
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

typedef boost::shared_ptr<MetNoFimex::UnitsConverter> UnitsConverterPtr;
UnitsConverterPtr unitConverter(const std::string& ua, const std::string& ub);

Values_cp unitConversion(Values_cp valuesIn, const std::string& unitIn, const std::string& unitOut);
float unitConversion(float valueIn, const std::string& unitIn, const std::string& unitOut);

// ########################################################################

bool startsWith(const std::string& text, const std::string& start);
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
 * \param ff_knot wind speed in knot
 */
WindArrowFeathers countFeathers(float ff_knot);

} // namespace util
} // namespace vcross

#endif // VCROSSUTIL_HH
