
#ifndef POLY_CONTOURING_HH
#define POLY_CONTOURING_HH 1

#include "reversible_list.hh"
#include <boost/pool/pool_alloc.hpp>
#include <stdexcept>

namespace contouring {

typedef int level_t;

struct point_t {
    float x, y;
    point_t() : x(0), y(0) { }
    point_t(float xx, float yy)
        : x(xx), y(yy) { }
};

typedef boost::fast_pool_allocator<point_t,boost::default_user_allocator_new_delete,boost::details::pool::null_mutex> points_allocator;
typedef reversible_list<point_t, points_allocator> points_t;

class field_t {
public:
    virtual ~field_t() { }
    virtual size_t nx() const = 0;
    virtual size_t ny() const = 0;
    virtual level_t grid_level(size_t ix, size_t iy) const = 0;
    virtual level_t undefined_level() const = 0;

    virtual point_t line_point(level_t level, size_t x0, size_t y0, size_t x1, size_t y1) const = 0;
    virtual point_t grid_point(size_t x, size_t y) const = 0;
};

class lines_t {
public:
    virtual ~lines_t() { }

    virtual void add_contour_line(level_t level, const points_t& points, bool closed) = 0;
    virtual void add_contour_polygon(level_t level, const points_t& points) = 0;
};

class too_many_levels : public std::overflow_error
{
public:
  too_many_levels(level_t ix, level_t iy, level_t lbl, level_t lbr, level_t ltr, level_t ltl)
    : overflow_error(fmt_many_levels(ix, iy, lbl, lbr, ltr, ltl)) { }

private:
  static std::string fmt_many_levels(level_t ix, level_t iy, level_t lbl, level_t lbr, level_t ltr, level_t ltl);
};

void run(const field_t& field, lines_t& lines);

} // namespace contouring

#endif // POLY_CONTOURING_HH
