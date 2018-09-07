/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013-2018 met.no

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

#ifndef VCROSS_DIVCROSSAXIS_HH
#define VCROSS_DIVCROSSAXIS_HH 1

#include <memory>
#include <string>
#include <vector>

namespace vcross {
namespace detail {

struct Axis {
  enum Type {
    LINEAR,      //!< linear mapping value <-> canvas
    LOGARITHMIC, //!< logarithmic mapping
    EXNER,       //!< apply exner function to value
    AMBLE        //!< ln(p) if p > 500 hPa and linear if p < 500 hPa
  };
  enum Quantity {
    TIME,     //!< forecast time (x axis only)
    DISTANCE, //!< distance from start (x axis only)
    VALUE,    //!< value of some parameter (x axis only)
    ALTITUDE, //!< height above mean sea level (vertical axis)
    HEIGHT,   //! height above ground (vertical axis)
    DEPTH,    //! depth below mean sea level, positive down, 0m topmost (vertical axis)
    PRESSURE  //! air pressure (vertical axis)
  };

  Axis(bool h);

  void setDataRange(float mi, float ma)
    { dataMin = mi; dataMax = ma; }
  void setValueRange(float mi, float ma)
    { valueMin = mi; valueMax = ma; calculateScale(); }
  void setPaintRange(float mi, float ma)
    { paintMin = mi; paintMax = ma; calculateScale(); }

  float getDataMin() const
    { return dataMin; }
  float getDataMax() const
    { return dataMax; }
  float getValueMin() const
    { return valueMin; }
  float getValueMax() const
    { return valueMax; }
  float getPaintMin() const
    { return paintMin; }
  float getPaintMax() const
    { return paintMax; }

  bool zoomIn(float paint0, float paint1);
  bool zoomOut();
  bool pan(float delta);

  float value2paint(float v, bool check=true) const;
  float paint2value(float p, bool check=true) const;
  bool legalPaint(float p) const;
  bool legalValue(float v) const;
  bool legalData(float d) const;

  void setLabel(const std::string& l)
    { mLabel = l; }
  const std::string& label() const
    { return mLabel; }

  bool setType(const std::string& t);
  void setType(Type t);
  Type type() const
    { return mType; }
  bool setQuantity(const std::string& q);
  Quantity quantity() const
    { return mQuantity; }

  bool increasing() const;

private:
  void setDefaultLabel();

  float fValueMin() const
    { return function(valueMin); }
  float fValueMax() const
    { return function(valueMax); }
  void calculateScale();

  float function(float x) const;
  float functionInverse(float x) const;

private:
  bool horizontal;
  Type mType;
  Quantity mQuantity;
  std::string mLabel;

  float dataMin, dataMax;
  float valueMin, valueMax;
  float paintMin, paintMax;
  float mScale;
};

typedef std::shared_ptr<Axis> AxisPtr;
typedef std::shared_ptr<const Axis> AxisCPtr;

} // namespace detail
} // namespace vcross

#endif // VCROSS_DIVCROSSAXIS_HH
