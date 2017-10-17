#include "diGridReprojection.h"

#include "diProjection.h"

#include <cmath>
#include <list>
#include <memory>

#define MILOGGER_CATEGORY "diField.GridReprojection"
#include <miLogger/miLogging.h>

namespace {

using diutil::PointI;
using diutil::PointD;

struct Reprojection {
  Projection p_map;
  Projection p_data;

  Reprojection(const Projection& pm, const Projection& pd)
    : p_map(pm), p_data(pd) { }

  bool operator()(size_t n, PointD* xy) const
    { return p_data.convertPoints(p_map, n, xy, false); }
};

inline bool is_undef(double d)
{
  return d == HUGE_VAL || std::isnan(d) || std::isinf(d);
}

inline bool is_undef(const PointD& xy)
{
  return is_undef(xy.x()) || is_undef(xy.y());
}

const int SMALL = 4;

enum {
  f00, f01, f11, f10, // corners
  // next 6 must be together for split_horizontally
  fm0, fm0O, // x=center y=0
  fm1, fm1O, // x=center y=1
  fmm, fmmO, // center
  f0m, f0mO, // x=0 y=center
  f1m, f1mO, // x=0 y=center
  // previous 6 must be together for split_vertically
  fN
};

// f00 -l- fm0 -r- f10
//  .       .       .
// f0m -l- fmm -r- f1m
//  .       .       .
// f01 -l- fm1 -r- f11

// f..L is one screen pixel LEFT of f..
// f..U is one screen pixel UP from f..
// horizontal split: 6 new points, at "l" and "r" plus their LEFT and UP => 3*6 new points

struct PixelArea {
  PointI s0, s1;
  PointD m0, m1;
  PointD xyf[fN];

  int sw() const { return s1.x() - s0.x(); }
  int sh() const { return s1.y() - s0.y(); }
  float mw() const { return m1.x() - m0.x(); }
  float mh() const { return m1.y() - m0.y(); }

  PixelArea();
};

PixelArea::PixelArea()
{
  std::fill(xyf, xyf+fN, PointD(0, 0));
}

typedef std::vector<PixelArea> PixelArea_v;

PointD pixel_with_offset(const PixelArea& d0, int xsOff, int ysOff)
{
  const PointI dxy = PointI(xsOff, ysOff) - d0.s0;
  return d0.m0 + PointD(dxy.x()*d0.mw()/float(d0.sw()),
                        dxy.y()*d0.mh()/float(d0.sh()));
}

inline PointD pixel_O(const PixelArea& d0, int xsOff, int ysOff)
{
  return pixel_with_offset(d0, xsOff+1, ysOff+1);
}

const float SCL = 0.8f, MIN = 1e-6;

inline bool check_diff(float v, float lim)
{
  return lim > MIN && v > lim*SCL;
}

bool pixel_is_bad(const PointD& linear, const PointD& projected, const PointD& limit)
{
  if (is_undef(projected) || is_undef(limit))
    return true;

  const PointD dxy = (linear - projected).abs();

  const PointD dpul = (limit - projected).abs();

  if (check_diff(dxy.x(), dpul.x())) {
    return true;
  } else if (check_diff(dxy.y(), dpul.y())) {
    return true;
  } else {
    return false;
  }
}

bool should_subdivide(const PixelArea& et)
{
#if 0
  if (et.ws() >= BIG || et.hs() >= BIG)
    return true;
#endif

  if (et.sw() <= SMALL || et.sh() <= SMALL)
    return false;

  const PointD& xy00 = et.xyf[f00];
  const PointD& xy01 = et.xyf[f01];
  const PointD& xy11 = et.xyf[f11];
  const PointD& xy10 = et.xyf[f10];

  if (is_undef(xy00) || is_undef(xy01) || is_undef(xy11) || is_undef(xy10))
    return true;

  if (pixel_is_bad((xy00 + xy10 + xy11 + xy01)/4,
                   et.xyf[fmm], et.xyf[fmmO]))
    return true;

  if (pixel_is_bad((xy00 + xy10)/2,
                   et.xyf[fm0], et.xyf[fm0O]))
    return true;

  if (pixel_is_bad((xy11 + xy01)/2,
                   et.xyf[fm1], et.xyf[fm1O]))
    return true;

  if (pixel_is_bad((xy00 + xy01)/2,
                   et.xyf[f0m], et.xyf[f0mO]))
    return true;

  if (pixel_is_bad((xy10 + xy11)/2,
                   et.xyf[f1m], et.xyf[f1mO]))
    return true;

  return false;
}

inline PointD map_center(const PixelArea& e)
{
  return (e.m0 + e.m1)/2;
}

inline PointI scr_center(const PixelArea& e)
{
  return (e.s0 + e.s1)/2;
}

inline void copy_fXY(PixelArea& to, const PixelArea& from, int fXY)
{
  to.xyf[fXY] = from.xyf[fXY];
}

inline void copy_fXY(PixelArea& to, int fXYto, const PixelArea& from, int fXYfrom)
{
  to.xyf[fXYto] = from.xyf[fXYfrom];
}

void centerpoints_horizontally(PixelArea& lr)
{
  const PointD map_mm = map_center(lr);

  lr.xyf[fm0] = lr.xyf[fmm] = lr.xyf[fm1] = map_mm;
  lr.xyf[fm0].ry() = lr.m0.y();
  lr.xyf[fm1].ry() = lr.m1.y();

  const PointI scr_mm = scr_center(lr);
  const int sx = scr_mm.x();

  // O=offset pixel
  lr.xyf[fm0O] = pixel_O(lr, sx, lr.s0.y());
  lr.xyf[fmmO] = pixel_O(lr, sx, scr_mm.y());
  lr.xyf[fm1O] = pixel_O(lr, sx, lr.s1.y());
}

void centerpoints_vertically(PixelArea& tb)
{
  const PointD map_mm = map_center(tb);

  tb.xyf[f0m] = tb.xyf[fmm] = tb.xyf[f1m] = map_mm;
  tb.xyf[f0m].rx() = tb.m0.x();
  tb.xyf[f1m].rx() = tb.m1.x();

  const PointI scr_mm = scr_center(tb);
  const int sy = scr_mm.y();

  // O=offset pixel
  tb.xyf[f0mO] = pixel_O(tb, tb.s0.x(),  sy);
  tb.xyf[fmmO] = pixel_O(tb, scr_mm.x(), sy);
  tb.xyf[f1mO] = pixel_O(tb, tb.s1.x(),  sy);
}

void split_horizontally(PixelArea_v& ev, const Reprojection& reproject)
{
  const PixelArea big = ev.back(); // make a copy
  PixelArea& left = ev.back();
  PixelArea right;

  // screen points
  const int bsxm = (big.s0.x() + big.s1.x())/2;
  left.s0  = big.s0;
  left.s1  = PointI(bsxm, big.s1.y());
  right.s0 = PointI(bsxm, big.s0.y());
  right.s1 = big.s1;

  // map points
  const float bmxm = big.m0.x() + left.sw()*big.mw()/float(big.sw());
  left.m0  = big.m0;
  left.m1  = PointD(bmxm, big.m1.y());
  right.m0 = PointD(bmxm, big.m0.y());
  right.m1 = big.m1;

  // points with existing reprojection
  copy_fXY(left, big, f00);
  copy_fXY(left, big, f01);
  copy_fXY(left, f10, big, fm0);
  copy_fXY(left, f11, big, fm1);

  copy_fXY(right, f00, big, fm0);
  copy_fXY(right, f01, big, fm1);
  copy_fXY(right, big, f10);
  copy_fXY(right, big, f11);

  copy_fXY(left, big, f0m);
  copy_fXY(left, big, f0mO);
  copy_fXY(left, f1m,  big, fmm);
  copy_fXY(left, f1mO, big, fmmO);

  copy_fXY(right, f0m,  big, fmm);
  copy_fXY(right, f0mO, big, fmmO);
  copy_fXY(right, big, f1m);
  copy_fXY(right, big, f1mO);

  // points needing reprojection
  centerpoints_horizontally(left);
  centerpoints_horizontally(right);
  const int fbegin = fm0, fend = fmmO, N=6;
  static_assert((N-1) == (fend - fbegin), "error in f00..fN enum");
  reproject(N, &left.xyf[fbegin]);
  reproject(N, &right.xyf[fbegin]);

  ev.push_back(right);
}

void split_vertically(PixelArea_v& ev, const Reprojection& reproject)
{
  const PixelArea big = ev.back(); // make a copy
  PixelArea& top = ev.back();
  PixelArea bottom;

  // screen points, same as split_horizontally with x <-> y
  const int bsym = (big.s0.y() + big.s1.y())/2;
  top.s0    = big.s0;
  top.s1    = PointI(big.s1.x(), bsym);
  bottom.s0 = PointI(big.s0.x(), bsym);
  bottom.s1 = big.s1;

  // map points, same as split_horizontally with x <-> y
  const float bmym = big.m0.y() + top.sh()*big.mh()/float(big.sh());
  top.m0    = big.m0;
  top.m1    = PointD(big.m1.x(), bmym);
  bottom.m0 = PointD(big.m0.x(), bmym);
  bottom.m1 = big.m1;

  // points with existing reprojection
  copy_fXY(top, big, f00);
  copy_fXY(top, big, f10);
  copy_fXY(top, f01, big, f0m);
  copy_fXY(top, f11, big, f1m);

  copy_fXY(bottom, f00, big, f0m);
  copy_fXY(bottom, f10, big, f1m);
  copy_fXY(bottom, big, f01);
  copy_fXY(bottom, big, f11);

  copy_fXY(top, big, fm0);
  copy_fXY(top, big, fm0O);
  copy_fXY(top, fm1,  big, fmm);
  copy_fXY(top, fm1O, big, fmmO);

  copy_fXY(bottom, fm0,  big, fmm);
  copy_fXY(bottom, fm0O, big, fmmO);
  copy_fXY(bottom, big, fm1);
  copy_fXY(bottom, big, fm1O);

  // points needing reprojection
  centerpoints_vertically(top);
  centerpoints_vertically(bottom);
  const int fbegin = fmm, fend = f1mO, N=6;
  static_assert((N-1) == (fend - fbegin), "error in f00..fN enum");
  reproject(N, &top.xyf[fbegin]);
  reproject(N, &bottom.xyf[fbegin]);

  ev.push_back(bottom);
}

void subdivide_back(PixelArea_v& ev, const Reprojection& reproject)
{
  const PixelArea& big = ev.back();
  const bool wide = (big.sw() > big.sh());
  if (wide)
    split_horizontally(ev, reproject);
  else
    split_vertically(ev, reproject);

#ifdef DBG_PIXEL_REPROJECTION
  const PixelArea& d0 = ev[ev.size()-2], d1 = ev[ev.size()-1];
  DBG(METLIBS_LOG_DEBUG(LOGVAL(wide) << LOGVAL(d0) << LOGVAL(d1)));
#endif // DBG_PIXEL_REPROJECTION
}

struct ReproQuad {
  PointI s0, swh;
  PointD f00, f10, f01, f11;
};

typedef std::vector<ReproQuad> ReproQuad_v;
typedef std::shared_ptr<ReproQuad_v> ReproQuad_vp;
typedef std::shared_ptr<const ReproQuad_v> ReproQuad_vcp;

struct ReproBuffer {
  PointI screensize;
  Rectangle maparea;
  Reprojection reproject;

  ReproBuffer(const PointI& scr, const Rectangle& map, const Projection& pm, const Projection& pd)
    : screensize(scr), maparea(map), reproject(pm, pd) { }

  bool operator==(const ReproBuffer& other) const;

  ReproQuad_vp script;
};

bool ReproBuffer::operator==(const ReproBuffer& other) const
{
  return screensize == other.screensize
      && maparea == other.maparea
      && reproject.p_map == other.reproject.p_map
      && reproject.p_data == other.reproject.p_data;
}

typedef std::shared_ptr<ReproBuffer> ReproBuffer_p;
typedef std::shared_ptr<const ReproBuffer> ReproBuffer_cp;
typedef std::list<ReproBuffer_cp> ReproBuffer_cpl;

void divide(ReproBuffer& rb)
{
  rb.script = std::make_shared<ReproQuad_v>();

  PixelArea_v ev;
  { // initial area
    PixelArea e0;
    e0.s0 = PointI(0,0);
    e0.s1 = rb.screensize;
    e0.m0 = PointD(rb.maparea.x1, rb.maparea.y1);
    e0.m1 = PointD(rb.maparea.x2, rb.maparea.y2);

    e0.xyf[f00] = e0.m0;
    e0.xyf[f01] = PointD(e0.m0.x(), e0.m1.y());
    e0.xyf[f10] = PointD(e0.m1.x(), e0.m0.y());
    e0.xyf[f11] = e0.m1;

    centerpoints_horizontally(e0);
    centerpoints_vertically(e0); // duplicate calculation of fmm, no big problem

    rb.reproject(fN, e0.xyf);

    ev.push_back(e0);
  }

  while (!ev.empty()) {
    PixelArea& et = ev.back();
    const int w = et.sw(), h = et.sh();
    if (w < 1 || h < 1) {
      ev.pop_back();
    } else if (should_subdivide(et)) {
      subdivide_back(ev, rb.reproject);
    } else {
      if (!(is_undef(et.xyf[f00]) || is_undef(et.xyf[f10]) || is_undef(et.xyf[f01]) || is_undef(et.xyf[f11]))) {
        rb.script->push_back(ReproQuad());
        ReproQuad& q = rb.script->back();
        q.s0 = et.s0;
        q.swh = PointI(w, h);
        q.f00 = et.xyf[f00];
        q.f10 = et.xyf[f10];
        q.f01 = et.xyf[f01];
        q.f11 = et.xyf[f11];
      }
      ev.pop_back();
    }
  }
}

} // namespace

GridReprojectionCB::~GridReprojectionCB()
{
}

void GridReprojectionCB::pixelQuad(const diutil::PointI &s0, const PointD& pxy00, const PointD& pxy10,
                                   const PointD& pxy01, const PointD& pxy11, int w)
{
  const PointD pdxy0 = (pxy10-pxy00)/w;
  pixelLine(s0, pxy00, pdxy0, w);
}

void GridReprojectionCB::linearQuad(const diutil::PointI &s0, const PointD& fxy00, const PointD& fxy10,
                                    const PointD& fxy01, const PointD& fxy11, const diutil::PointI &wh)
{
  const float h = wh.y(), hi = 1/h;
  PointD pxy00 = fxy00, pxy10 = fxy10;
  const PointD dy0 = (fxy01 - fxy00)*hi;
  const PointD dy1 = (fxy11 - fxy10)*hi;
  for (int ih0=0, ih1=h; ih0<h; ++ih0, --ih1) {
    const PointD pxy01 = pxy00 + dy0;
    const PointD pxy11 = pxy10 + dy1;
    pixelQuad(s0 + PointI(0, ih0), pxy00,  pxy10, pxy01, pxy11, wh.x());
    pxy00 = pxy01;
    pxy10 = pxy11;
  }
}

GridReprojection::~GridReprojection()
{
}

class CachedGridReprojection : public GridReprojection {
public:
  void reproject(const diutil::PointI &size, const Rectangle& mr,
                 const Projection& p_map, const Projection& p_data,
                 GridReprojectionCB& cb) override;
private:
  ReproQuad_vcp divide(const diutil::PointI& size, const Rectangle& mr,
                       const Projection& pm, const Projection& pd);

private:
  ReproBuffer_cpl cache;
};

void CachedGridReprojection::reproject(const diutil::PointI& size, const Rectangle& mr,
                                       const Projection& p_map, const Projection& p_data,
                                       GridReprojectionCB& cb)
{
  METLIBS_LOG_SCOPE();

  ReproQuad_vcp script = this->divide(size, mr, p_map, p_data);
  for (const ReproQuad& q : *script) {
    cb.linearQuad(q.s0, q.f00, q.f10, q.f01, q.f11, q.swh);
  }
}

ReproQuad_vcp CachedGridReprojection::divide(const diutil::PointI& size, const Rectangle& mr,
                                             const Projection& pm, const Projection& pd)
{
  ReproBuffer_p rb = std::make_shared<ReproBuffer>(size, mr, pm, pd);
  for (ReproBuffer_cp c : cache) {
    if (*c == *rb)
      return c->script;
  }

  ::divide(*rb);

  while (cache.size() > 20)
    cache.pop_front();
  cache.push_back(rb);

  return rb->script;
}

// static
GridReprojection_p GridReprojection::instance_;

// static
GridReprojection_p GridReprojection::instance()
{
  if (!instance_)
    instance_ = std::make_shared<CachedGridReprojection>();
  return instance_;
}

// static
void GridReprojection::instance(GridReprojection_p i)
{
  instance_ = i;
}
