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

#include "diUnitsConverter.h"

#include <converter.h>
#include <udunits2.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace diutil {

namespace {

class NoOpUnitsConverter : public UnitsConverter
{

public:
  NoOpUnitsConverter() {}
  void convert(const double* from_begin, const double* from_end, double* to_begin) const override {}
  void convert(const float* from_begin, const float* from_end, float* to_begin) const override {}

  UnitConvertibility convertibility() const override { return UNITS_IDENTICAL; }
};

class LinearUnitsConverter : public UnitsConverter
{
  double dscale_;
  double doffset_;
  double fscale_;
  double foffset_;

public:
  LinearUnitsConverter(double scale, double offset)
      : dscale_(scale)
      , doffset_(offset)
      , fscale_(scale)
      , foffset_(offset)
  {
  }
  ~LinearUnitsConverter() {}

  void convert(const double* from_begin, const double* from_end, double* to_begin) const override;
  void convert(const float* from_begin, const float* from_end, float* to_begin) const override;

  UnitConvertibility convertibility() const override { return UNITS_LINEAR; }
};

void LinearUnitsConverter::convert(const double* from_begin, const double* from_end, double* to_begin) const
{
  const auto* f = from_begin;
  auto* t = to_begin;
  for (; f != from_end; ++f, ++t)
    *t = *f * dscale_ + doffset_;
}

void LinearUnitsConverter::convert(const float* from_begin, const float* from_end, float* to_begin) const
{
  const auto* f = from_begin;
  auto* t = to_begin;
  for (; f != from_end; ++f, ++t)
    *t = *f * fscale_ + foffset_;
}

class UdUnitsConverter : public UnitsConverter
{
public:
  UdUnitsConverter(cv_converter* conv);
  ~UdUnitsConverter();

  void convert(const double* from_begin, const double* from_end, double* to_begin) const override;
  void convert(const float* from_begin, const float* from_end, float* to_begin) const override;

  UnitsConverter_p toLinear() const;
  UnitConvertibility convertibility() const override { return UNITS_CONVERTIBLE; }

private:
  cv_converter* conv_;
};

UdUnitsConverter::UdUnitsConverter(cv_converter* conv)
    : conv_(conv)
{
}

UdUnitsConverter::~UdUnitsConverter()
{
  cv_free(conv_);
}

void UdUnitsConverter::convert(const double* from_begin, const double* from_end, double* to_begin) const
{
  cv_convert_doubles(conv_, from_begin, from_end - from_begin, to_begin);
}

void UdUnitsConverter::convert(const float* from_begin, const float* from_end, float* to_begin) const
{
  cv_convert_floats(conv_, from_begin, from_end - from_begin, to_begin);
}

UnitsConverter_p UdUnitsConverter::toLinear() const
{
  // check some points
  const auto offset = convert1(0.0);
  if (!std::isfinite(offset))
    return nullptr;

  auto scale = convert1(1.0) - offset;
  if (!std::isfinite(scale))
    return nullptr;

  const double vals[] = {10., 100., 1000., -1000.};
  const int N = sizeof(vals) / sizeof(vals[0]);
  double cvals[N];
  convert(vals, vals + N, cvals);
  for (int i = 0; i < N; ++i) {
    if ((!std::isfinite(cvals[i])) || (std::fabs(cvals[i] - (vals[i] * scale + offset)) > 1e-5))
      return nullptr;
  }

  // make sure, scale is determined at a place where offset is no longer numerically superior
  if (scale != 0 && offset != 0) {
    const double temp = -offset / scale;
    const double scale2 = (convert1(temp) - offset) / temp;
    if (fabs(scale - scale2) < 1e-3) {
      // should only increase precision, not nan/inf or other cases
      scale = scale2;
    }
  }

  if (scale == 1 && offset == 0)
    return std::make_shared<NoOpUnitsConverter>();

  return std::make_shared<LinearUnitsConverter>(scale, offset);
}

// static
void handleUdUnitError(int unitErrCode, const std::string& context = std::string())
{
  if (unitErrCode != UT_SUCCESS)
    throw std::runtime_error("udunits error: " + context); // TODO
}

ut_system* initUtSystem()
{
  ut_set_error_message_handler(&ut_ignore);
  ut_system* us = ut_read_xml(0);
  handleUdUnitError(ut_get_status());
  return us;
}

// static
ut_system* getUtSystem()
{
  static ut_system* utSystem = initUtSystem();
  return utSystem;
}

std::shared_ptr<ut_unit> make_shared_unit(ut_unit* u, const std::string& error_context)
{
  std::shared_ptr<ut_unit> p(u, ut_free);
  handleUdUnitError(ut_get_status(), error_context); // throws exception if an error was detected
  return p;
}

std::shared_ptr<ut_unit> make_shared_unit(const std::string& unit)
{
  return make_shared_unit(ut_parse(getUtSystem(), unit.c_str(), UT_UTF8), unit);
}

bool unitConversion(UnitsConverter_p uconv, size_t volume, float undefIn, float undefOut, const float* vIn, float* vOut)
{
  if (uconv) {
    const auto vInEnd = vIn + volume;
    while (vIn != vInEnd) {
      auto vInUndef = std::find(vIn, vInEnd, undefIn);
      uconv->convert(vIn, vInUndef, vOut);
      vOut += std::distance(vIn, vInUndef);

      for (vIn = vInUndef; vIn != vInEnd && *vIn == undefIn; ++vIn, ++vOut)
        *vOut = undefOut;
    }
    return true;
  } else {
    return false;
  }
}

} // namespace

UnitsConverter::~UnitsConverter() {}

UnitConvertibility unitConvertibility(const std::string& from, const std::string& to)
{
  if (auto conv = unitConverter(from, to))
    return conv->convertibility();
  else
    return UNITS_MISMATCH;
}

std::string unitsMultiplyDivide(const std::string& ua, const std::string& ub, bool multiply)
{
  if (ub.empty())
    return ua;

  try {
    auto us = getUtSystem();

    auto aUnit = make_shared_unit(ua.empty() ? std::string("1") : ua);
    auto bUnit = make_shared_unit(ub);
    auto mUnit = make_shared_unit((multiply ? ut_multiply : ut_divide)(aUnit.get(), bUnit.get()), "result");

    char buf[128];
    const int len = ut_format(mUnit.get(), buf, sizeof(buf), UT_ASCII | UT_DEFINITION);
    if (len >= 0 and len < int(sizeof(buf)))
      return buf;
  } catch (std::exception& ex) {
    // METLIBS_LOG_ERROR(ex.what());
  }
  return "";
}

std::string unitsRoot(const std::string& u, int root)
{
  if (u.empty())
    return u;

  try {
    auto us = getUtSystem();
    auto iUnit = make_shared_unit(u);
    auto rUnit = make_shared_unit(ut_root(iUnit.get(), root), "result");

    char buf[128];
    const int len = ut_format(rUnit.get(), buf, sizeof(buf), UT_ASCII | UT_DEFINITION);
    if (len >= 0 and len < int(sizeof(buf)))
      return buf;
  } catch (std::exception& ex) {
    // METLIBS_LOG_ERROR(ex.what());
  }
  return "";
}

// ------------------------------------------------------------------------

UnitsConverter_p unitConverter(const std::string& from, const std::string& to)
{
  if (from == to)
    return std::make_shared<NoOpUnitsConverter>();

  auto fromUnit = make_shared_unit(from);
  auto toUnit = make_shared_unit(to);
  try {
    cv_converter* conv = ut_get_converter(fromUnit.get(), toUnit.get());
    handleUdUnitError(ut_get_status(), "'" + from + "' converted to '" + to + "'");
    auto uc = std::make_shared<UdUnitsConverter>(conv);
    if (auto lin = uc->toLinear())
      return lin;
    else
      return uc;
  } catch (std::exception& ex) {
    return nullptr;
  }
}

// ------------------------------------------------------------------------

bool unitConversion(const std::string& unitIn, const std::string& unitOut, size_t volume, float undefval, const float* vIn, float* vOut)
{
  return unitConversion(unitConverter(unitIn, unitOut), volume, undefval, undefval, vIn, vOut);
}

// ------------------------------------------------------------------------

Values_cp unitConversion(Values_cp valuesIn, const std::string& unitIn, const std::string& unitOut)
{
  if (!valuesIn)
    return nullptr;
  auto uconv = unitConverter(unitIn, unitOut);
  if (!uconv)
    return nullptr;
  if (uconv->convertibility() == UNITS_IDENTICAL)
    return valuesIn;

  auto valuesOut = std::make_shared<Values>(valuesIn->shape());
  unitConversion(uconv, valuesOut->shape().volume(), valuesIn->undefValue(), valuesOut->undefValue(), &valuesIn->values()[0], &valuesOut->values()[0]);
  return valuesOut;
}

// ------------------------------------------------------------------------

float unitConversion(float valueIn, const std::string& unitIn, const std::string& unitOut)
{
  if (auto uconv = unitConverter(unitIn, unitOut))
    return uconv->convert1(valueIn);
  else
    return valueIn;
}

} // namespace diutil
