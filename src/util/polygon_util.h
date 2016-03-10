
#ifndef DIANA_UTIL_POLYGON_UTIL_H
#define DIANA_UTIL_POLYGON_UTIL_H 1

#include <QPolygonF>

class Rectangle;

namespace diutil {

/*!
 * Remove polygon segments where the points stay in the same "sector"
 * outside of rect.
 *
 * WARNING: this will probably not work for polygons with holes
 */
QPolygonF trimToRectangle(const Rectangle& rect, const QPolygonF& polygon);

} // namespace diutil

#endif // DIANA_UTIL_POLYGON_UTIL_H
