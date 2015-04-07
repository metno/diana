
#include "diPoint.h"

namespace diutil {

void adjustRectangle(Rectangle& r, float dx, float dy)
{
  r.x1 -= dx;
  r.y1 -= dy;
  r.x2 += dx;
  r.y2 += dy;
}

Rectangle adjustedRectangle(const Rectangle& r, float dx, float dy)
{
  Rectangle ar = r;
  adjustRectangle(ar, dx, dy);
  return ar;
}

void translateRectangle(Rectangle& r, float dx, float dy)
{
  r.x1 += dx;
  r.y1 += dy;
  r.x2 += dx;
  r.y2 += dy;
}

Rectangle translatedRectangle(const Rectangle& r, float dx, float dy)
{
  Rectangle ar = r;
  translateRectangle(ar, dx, dy);
  return ar;
}

void fixAspectRatio(Rectangle& rect, float requested_w_over_h, bool extend)
{
  const float w_over_h = rect.width() / rect.height();

  float dwid = 0, dhei = 0;
  if ((requested_w_over_h > w_over_h) == extend) {
    // change map width
    dwid = (requested_w_over_h * rect.height() - rect.width())/2;
  } else { // change map height
    dhei = (rect.width() / requested_w_over_h - rect.height())/2;
  }

  adjustRectangle(rect, dwid, dhei);
}

Rectangle fixedAspectRatio(const Rectangle& rect, float requested_w_over_h, bool extend)
{
  Rectangle r(rect);
  fixAspectRatio(r, requested_w_over_h, extend);
  return r;
}

} // namespace diutil
