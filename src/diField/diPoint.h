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

#ifndef DIFIELD_DIPOINT_H
#define DIFIELD_DIPOINT_H 1

#include "diRectangle.h"

#include <cmath>
#include <vector>

namespace diutil {

template<class T>
class Values2 {
public:
  Values2(T x, T y)
    : mX(x), mY(y) { }
  Values2()
    : mX(0), mY(0) { }

  T x() const
    { return mX; }
  T y() const
    { return mY; }
  T& rx()
    { return mX; }
  T& ry()
    { return mY; }

  void unpack(T& x, T& y) const
    { x = mX; y = mY; }

  bool operator==(const Values2& p) const
    { return x() == p.x() && y() == p.y(); }
  bool operator!=(const Values2& p) const
    { return !(*this == p); }

  Values2 flipped() const { return Values2(mY, mX); }

  Values2& operator+=(const Values2& p)
    { mX += p.x(); mY += p.y(); return *this; }
  Values2 operator+(const Values2& p) const
    { Values2 xy(*this); xy += p; return xy; }

  Values2& operator-=(const Values2& p)
    { mX -= p.x(); mY -= p.y(); return *this; }
  Values2 operator-(const Values2& p) const
    { Values2 xy(*this); xy -= p; return xy; }

  Values2 operator-() const
    { return Values2(-mX, -mY); }

  Values2& operator*=(const Values2& p)
    { mX *= p.x(); mY *= p.y(); return *this; }
  Values2 operator*(const Values2& p) const
    { Values2 xy(*this); xy *= p; return xy; }

  Values2& operator/=(const Values2& p)
    { if (p.x()) mX /= p.x(); if (p.y() != 0) mY /= p.y(); return *this; }
  Values2 operator/(const Values2& p) const
    { Values2 xy(*this); xy /= p; return xy; }

  Values2& operator+=(T f)
    { mX += f; mY += f; return *this; }
  Values2 operator+(T f) const
    { Values2 xy(*this); xy += f; return xy; }

  Values2& operator-=(T f)
    { mX -= f; mY -= f; return *this; }
  Values2 operator-(T f) const
    { Values2 xy(*this); xy -= f; return xy; }

  Values2& operator*=(T f)
    { mX *= f; mY *= f; return *this; }
  Values2 operator*(T f) const
    { Values2 xy(*this); xy *= f; return xy; }

  Values2& operator/=(T f)
    { if (f != 0) { mX /= f; mY /= f; } return *this; }
  Values2 operator/(T f) const
    { Values2 xy(*this); xy /= f; return xy; }

  Values2 abs() const
    { return Values2(std::abs(mX), std::abs(mY)); }

private:
  T mX, mY;
};

template<class T>
Values2<T> operator/(T f, const Values2<T>& v)
{
  return Values2<T>(f/v.x(), f/v.y());
}

typedef diutil::Values2<int> PointI;
typedef diutil::Values2<float> PointF;
typedef diutil::Values2<double> PointD;

std::ostream& operator<<(std::ostream& out, const PointI& xy);
std::ostream& operator<<(std::ostream& out, const PointF& xy);
std::ostream& operator<<(std::ostream& out, const PointD& xy);

// ========================================================================

struct Rect {
  Rect()
    : x1(0), y1(0), x2(0), y2(0) { }
  Rect(int x1_, int y1_, int x2_, int y2_)
    : x1(x1_), y1(y1_), x2(x2_), y2(y2_) { }
  int width() const
    { return x2 - x1; }
  int height() const
    { return y2 - y1; }
  bool empty() const
    { return width() <= 0 || height() <= 0; }
  int x1, y1, x2, y2;
};

typedef std::vector<Rect> Rect_v;

std::ostream& operator<<(std::ostream& out, const diutil::Rect& r);

void adjustRectangle(Rectangle& r, float dx, float dy);
Rectangle adjustedRectangle(const Rectangle& r, float dx, float dy);

inline void extendRectangle(Rectangle& r, float ex)
{
  adjustRectangle(r, ex, ex);
}
inline Rectangle extendedRectangle(const Rectangle& r, float ex)
{
  return adjustedRectangle(r, ex, ex);
}

void translateRectangle(Rectangle& r, float dx, float dy);
Rectangle translatedRectangle(const Rectangle& r, float dx, float dy);

void fixAspectRatio(Rectangle& rect, float requested_w_over_h, bool extend);
Rectangle fixedAspectRatio(const Rectangle& rect, float requested_w_over_h, bool extend);

bool contains(const Rectangle& outer, const Rectangle& inner);

inline Rectangle makeRectangle(const PointF& p1, const PointF& p2)
{ return Rectangle(p1.x(), p1.y(), p2.x(), p2.y()); }

inline bool isInside(const Rectangle& r, const PointF& xy)
{ return r.isinside(xy.x(), xy.y()); }

} // namespace diutil

#endif // DIFIELD_DIPOINT_H
