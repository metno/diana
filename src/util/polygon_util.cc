
#include "polygon_util.h"

#include "diField/diRectangle.h"

namespace diutil {

namespace detail {

enum {
  INSIDE = 0,
  OUT_X_LEFT  = 2, OUT_X_RIGHT =  3, OUT_X_MASK =  3,
  OUT_Y_BELOW = 8, OUT_Y_ABOVE = 12, OUT_Y_MASK = 12
};

int where(const Rectangle& rect, const QPointF& point)
{
  int w = INSIDE;
  if (point.x() < rect.x1)
    w |= OUT_X_LEFT;
  else if (point.x() > rect.x2)
    w |= OUT_X_RIGHT;
  if (point.y() < rect.y1)
    w |= OUT_Y_BELOW;
  else if (point.y() > rect.y2)
    w |= OUT_Y_ABOVE;
  return w;
}

} // namespace detail

QPolygonF trimToRectangle(const Rectangle& rect, const QPolygonF& polygon)
{
  QPolygonF trimmed;

  const int last = polygon.size();
  for (int k0 = 0; k0 < last; k0++) {
    QPointF pp0 = polygon.at(k0);
    int w0 = detail::where(rect, pp0);
    trimmed << pp0;
    if (w0 == detail::INSIDE)
      continue;

    int k1 = k0 + 1, w1;
    while (k1 < last && ((w1 = detail::where(rect, polygon.at(k1))) == w0))
      k1 += 1;

    if (k1 > k0 + 1)
      trimmed << polygon.at(k1 - 1);
    k0 = k1 - 1;
  }

  return trimmed;
}

} // namespace diutil
