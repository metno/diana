
#ifndef DIPOINT_H
#define DIPOINT_H 1

#include <diField/diRectangle.h>

namespace diutil {

void adjustRectangle(Rectangle& r, float dx, float dy);
Rectangle adjustedRectangle(const Rectangle& r, float dx, float dy);

void translateRectangle(Rectangle& r, float dx, float dy);
Rectangle translatedRectangle(const Rectangle& r, float dx, float dy);

void fixAspectRatio(Rectangle& rect, float requested_w_over_h, bool extend);
Rectangle fixedAspectRatio(const Rectangle& rect, float requested_w_over_h, bool extend);


template<class T>
class Values2 {
public:
  Values2(T x, T y)
    : mX(x), mY(y) { }
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

private:
  T mX, mY;
};

} // namespace diutil

typedef diutil::Values2<float> XY;

inline Rectangle makeRectangle(const XY& p1, const XY& p2)
{ return Rectangle(p1.x(), p1.y(), p2.x(), p2.y()); }

#endif // DIPOINT_H
