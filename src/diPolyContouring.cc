
#include "diPolyContouring.h"

#include "diPlotOptions.h"
#include "diFontManager.h"
#include <diTesselation.h>

#include <GL/gl.h>
#if !defined(USE_PAINTGL)
#include <glp/glpfile.h>
#endif

#include <boost/make_shared.hpp>

#include <cmath>

#define MILOGGER_CATEGORY "diana.PolyContouring"
#include <miLogger/miLogging.h>

const float UNDEF_VALUE = 1e30;

inline bool isUndefined(float v)
{
  return isnan(v) or v >= UNDEF_VALUE or v < -UNDEF_VALUE;
}

static inline int rounded_div(float value, float unit)
{
    const int i = int(value / unit);
    if (value >= 0)
        return i+1;
    else
        return i;
}

int find_index(bool repeat, int available, int i)
{
  if (repeat) {
    i %= available;
    if (i<0)
      i += available;
    return i;
  }
  else if (i<=0)
    return 0;
  else if (i<available)
    return i;
  else
    return available-1;
}


class DianaArrayIndex {
public:
  DianaArrayIndex(size_t nx, size_t x0, size_t y0, size_t x1, size_t y1, size_t step=0)
    : mNX(nx), mX0(x0), mY0(y0), mStep(std::max((size_t)1, step)), mSX((x1-x0)/mStep), mSY((y1-y0)/mStep) { }

  size_t size_x() const
    { return mSX; }

  size_t size_y() const
    { return mSY; }

  size_t index(size_t ix, size_t iy) const
    {  return (mY0 + iy*mStep)*mNX + (mX0+ix*mStep); }

  size_t operator()(size_t ix, size_t iy) const
    { return index(ix, iy); }

private:
  size_t mNX, mX0, mY0, mStep, mSX, mSY;
};

// ########################################################################

class DianaPositionsSimple : public DianaPositions {
public:
  virtual contouring::point_t position(size_t ix, size_t iy) const
    { return contouring::point_t(ix, iy); }
};

class DianaPositionsList : public DianaPositions {
public:
  DianaPositionsList(const DianaArrayIndex& index, const float* xpos, const float* ypos)
    : mIndex(index), mXpos(xpos), mYpos(ypos) { }
  
  contouring::point_t position(size_t ix, size_t iy) const
    { const size_t i = mIndex(ix, iy); return contouring::point_t(mXpos[i], mYpos[i]); }

private:
  const DianaArrayIndex& mIndex;
  const float *mXpos, *mYpos;
};

class DianaPositionsFormula : public DianaPositions {
public:
  DianaPositionsFormula(const float* coeff)
    : cxy(coeff) { }
  
  contouring::point_t position(size_t ix, size_t iy) const
    { const float x = cxy[0]+cxy[1]*ix+cxy[2]*iy;
      const float y = cxy[3]+cxy[4]*ix+cxy[5]*iy;
      return contouring::point_t(x, y); }

private:
  const float *cxy;
};

// ########################################################################

DianaLevelList::DianaLevelList(const std::vector<float>& levels)
  : mLevels(levels)
{
}

DianaLevelList::DianaLevelList()
{
}

DianaLevelList::DianaLevelList(float lstart, float lstop, float lstep)
{
  for (float l=lstart; l<=lstop; l+=lstep)
    mLevels.push_back(l);
}

float DianaLevelList::value_for_level(contouring::level_t l) const
{
  return l != UNDEF_LEVEL ? mLevels[l] : UNDEF_VALUE;
}

contouring::level_t DianaLevelList::level_for_value(float value) const
{
  if (isUndefined(value))
    return UNDEF_LEVEL;
  return std::lower_bound(mLevels.begin(), mLevels.end(), value) - mLevels.begin();
}

// ------------------------------------------------------------------------

DianaLevelList10::DianaLevelList10(const std::vector<float>& levels, size_t count)
{
  for (size_t i = 0; i<std::min(count, levels.size()); ++i) {
    float l = levels[i];
    if (l <= 0)
      continue;
    if (not mLevels.empty() and (l >= BASE*mLevels.front() or l <= mLevels.back()))
      break;
    mLevels.push_back(l);
  }
  const size_t nLevels = mLevels.size(); // remember number of specified levels
  if (nLevels > 0) {
    for (size_t f=BASE; mLevels.size()<count; f *= BASE) {
      for (size_t i=0; i<nLevels && mLevels.size()<count; ++i)
        mLevels.push_back(f * mLevels[i]);
    }
  }
}

//------------------------------------------------------------------------

DianaLevelLog::DianaLevelLog(const std::vector<float>& levels)
{
  for (size_t i=0; i<levels.size(); ++i) {
     float l = levels[i];
    if (l > BASE or (l>0 and l<1)) {
      const float logBl = log(l) / log(BASE), flogBl = logBl - floor(logBl);
      l = pow(BASE, flogBl);
    }
    if (l >= 1 and l < BASE and (mLevels.empty() or l > mLevels.back()))
      mLevels.push_back(l);
    else
      METLIBS_LOG_WARN("illegal log.line.values element @" << i << '=' << levels[i]);
  }
}

contouring::level_t DianaLevelLog::level_for_value(float value) const
{
  if (isUndefined(value) or value < 0 or mLevels.empty())
    return UNDEF_LEVEL;
  if (value == 0)
    return UNDEF_LEVEL;
  const double logBv = log(value)/log(BASE), ilogBv = floor(logBv);
  const double v_0_B = std::pow(BASE, logBv - ilogBv);
  const contouring::level_t l = DianaLevelList::level_for_value(v_0_B);
  return ilogBv*nlevels() + l;
}

float DianaLevelLog::value_for_level(contouring::level_t l) const
{
  if (l == UNDEF_LEVEL)
    return UNDEF_VALUE;
  const int ilogBv = floor(l / double(nlevels())), lvl = abs(l) % nlevels();
  return std::pow(double(BASE), ilogBv) * mLevels[lvl];
}

//------------------------------------------------------------------------

contouring::level_t DianaLevelStep::level_for_value(float value) const
{
  if (isUndefined(value))
    return UNDEF_LEVEL;
  if (mMin < mMax) {
    if (value < mMin)
      value = mMin;
    else if (value > mMax)
      value = mMax;
  }
  return rounded_div(value - mOff, mStep);
}

float DianaLevelStep::value_for_level(contouring::level_t l) const
{
  return l != UNDEF_LEVEL ? l*mStep + mOff : UNDEF_VALUE;
}

//------------------------------------------------------------------------

contouring::level_t DianaLevelStepOmit::level_for_value(float value) const
{
  if (value == mOff)
    return UNDEF_LEVEL;
  else
    return DianaLevelStep::level_for_value(value);
}

// ########################################################################

contouring::point_t DianaFieldBase::line_point(contouring::level_t level, size_t x0, size_t y0, size_t x1, size_t y1) const
{
    const float v0 = value(x0, y0);
    const float v1 = value(x1, y1);
    const float vl = mLevels.value_for_level(level);
    const float c = (vl-v0)/(v1-v0);

    const contouring::point_t p0 = position(x0, y0);
    const contouring::point_t p1 = position(x1, y1);
    const float x = (1-c)*p0.x + c*p1.x;
    const float y = (1-c)*p0.y + c*p1.y;
    return contouring::point_t(x, y);
}

// ########################################################################

class DianaField : public DianaFieldBase {
public:
  DianaField(const DianaArrayIndex& index, const float* data, const DianaLevels& levels, const DianaPositions& positions)
    : DianaFieldBase(levels, positions)
    , mIndex(index), mData(data)
    { }
  
  virtual size_t nx() const
    { return mIndex.size_x(); }
  
  virtual size_t ny() const
    { return mIndex.size_y(); }

protected:
  virtual float value(size_t ix, size_t iy) const
    { return mData[mIndex(ix, iy)]; }

private:
  const DianaArrayIndex& mIndex;
  const float *mData;
};

// ########################################################################

class DianaLines : public contouring::lines_t {
public:
  DianaLines(const PlotOptions& poptions, const DianaLevels& levels, FontManager* fm);

  void add_contour_line(contouring::level_t level, const contouring::points_t& points, bool closed);
  void add_contour_polygon(contouring::level_t level, const contouring::points_t& points);

  void paint();

private:
  typedef std::vector<contouring::point_t> point_v;
  typedef std::vector<point_v> point_vv;
  typedef std::map<contouring::level_t, point_vv> lines_m;
  struct polygons_t {
    int count() const
      { return lengths.size(); }
    std::vector<int> lengths;
    std::vector<GLdouble> points; // x, y, 0
  };
  typedef std::map<contouring::level_t, polygons_t> polygons_m;

private:
  void paint_polygons();
  void paint_lines();
  void paint_coloured_lines(int linewidth, const Colour& colour,
      const Linetype& linetype, const point_vv& lines, contouring::level_t li, bool label);
  void paint_labels(const point_v& points, contouring::level_t li);

private:
  const PlotOptions& mPlotOptions;
  const DianaLevels& mLevels;
  FontManager* mFM;

  lines_m m_lines;
  polygons_m m_polygons;
};

DianaLines::DianaLines(const PlotOptions& poptions, const DianaLevels& levels, FontManager* fm)
  : mPlotOptions(poptions), mLevels(levels), mFM(fm)
{
}

void DianaLines::paint()
{
  paint_polygons();
  paint_lines();
}

void DianaLines::paint_polygons()
{
  METLIBS_LOG_TIME(LOGVAL(mPlotOptions.undefMasking));
  glShadeModel(GL_FLAT);
  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  const int ncolours = mPlotOptions.palettecolours.size();
  const int ncolours_cold = mPlotOptions.palettecolours_cold.size();

  for (polygons_m::const_iterator it = m_polygons.begin(); it != m_polygons.end(); ++it) {
    contouring::level_t li = it->first;
    const polygons_t& p = it->second;
    METLIBS_LOG_TIME(LOGVAL(li));
    
    if (li == DianaLevels::UNDEF_LEVEL) {
      if (mPlotOptions.undefMasking != 1)
        continue;
      glColor3ubv(mPlotOptions.undefColour.RGB());
    } else if (li <= 0 and ncolours_cold) {
      const int idx = find_index(mPlotOptions.repeat, ncolours_cold, -li);
      glColor3ubv(mPlotOptions.palettecolours_cold[idx].RGB());
    } else {
      if (not ncolours_cold and li <= 0)
        continue;
      const int idx = find_index(mPlotOptions.repeat, ncolours, li - 1);
      glColor3ubv(mPlotOptions.palettecolours[idx].RGB());
    }

    { //METLIBS_LOG_TIME("tesselation");
      beginTesselation();
      GLdouble* gldata = const_cast<GLdouble*>(&p.points[0]);
      tesselation(gldata, p.count(), &p.lengths[0]);
      endTesselation();
    }
  }

  glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
  glDisable(GL_BLEND);
  glShadeModel(GL_FLAT);
  glEdgeFlag(GL_TRUE);
}

void DianaLines::paint_lines()
{
  glShadeModel(GL_FLAT);

  for (lines_m::const_iterator it = m_lines.begin(); it != m_lines.end(); ++it) {
    if (it->first == DianaLevels::UNDEF_LEVEL) {
      if (mPlotOptions.undefMasking)
        paint_coloured_lines(mPlotOptions.undefLinewidth, mPlotOptions.undefColour, mPlotOptions.undefLinetype,
            it->second, it->first, false);
    } else {
      paint_coloured_lines(mPlotOptions.linewidth, mPlotOptions.linecolour, mPlotOptions.linetype,
          it->second, it->first, mPlotOptions.valueLabel);
    }
  }
}

void DianaLines::paint_coloured_lines(int linewidth, const Colour& colour,
    const Linetype& linetype, const point_vv& lines, contouring::level_t li, bool label)
{
  if (linewidth <= 0)
    return;

  if (linetype.stipple) {
    glLineStipple(linetype.factor, linetype.bmap);
    glEnable(GL_LINE_STIPPLE);
  } else {
    glDisable(GL_LINE_STIPPLE);
  }
  glLineWidth(linewidth);

  glColor3ubv(colour.RGB());

  for (point_vv::const_iterator itP = lines.begin(); itP != lines.end(); ++itP) {
    const point_v& points = *itP;
      
    // draw line
    glBegin(GL_LINE_STRIP);
    for (point_v::const_iterator it = points.begin(); it != points.end(); ++it)
      glVertex2f(it->x, it->y);
    glEnd();
    
    if (label)
      paint_labels(points, li);
  }
}

void DianaLines::paint_labels(const point_v& points, contouring::level_t li)
{
  if (points.size() < 10)
    return;

  std::ostringstream o;
  o << mLevels.value_for_level(li);
  const std::string lbl = o.str();

  float lbl_w = 0, lbl_h = 0;
  if (not mFM->getStringSize(lbl.c_str(), lbl_w, lbl_h))
    return;
  if (lbl_h <= 0 or lbl_w <= 0)
    return;
  const float lbl_w2 = lbl_w * lbl_w;

  size_t idx = int(0.1*(1 + (std::abs(li+100000) % 5))) * points.size();
  for (; idx + 1 < points.size(); idx += 5) {
    contouring::point_t p0 = points.at(idx), p1;
    const int idx0 = idx;
    for (idx += 1; idx < points.size(); ++idx) {
      p1 = points.at(idx);
      const float dy = p1.y - p0.y, dx = p1.x - p0.x;
      if (dx*dx + dy*dy >= lbl_w2)
        break;
    }
    if (idx >= points.size())
      break;

    if (p1.x < p0.x)
      std::swap(p0, p1);
    const float angle_deg = atan2f(p1.y - p0.y, p1.x - p0.x) * 180. / M_PI;
    if (angle_deg < -45 or angle_deg > 45)
      continue;

    // check that line is somewhat straight under label
    size_t idx2 = idx0 + 1;
    for (; idx2 < idx; ++idx2) {
      const contouring::point_t p2 = points.at(idx2);
      const float a = atan2f(p2.y - p0.y, p2.x - p0.x) * 180. / M_PI;
      if (std::abs(a - angle_deg) > 15)
        break;
    }
    if (idx2 < idx)
      continue;

    // label angle seems ok, find position
#if 0
    // centered on line
    const float f = lbl_h/(2*lbl_w), tx = p0.x + (p1.y-p0.y)*f, ty = p0.y - (p1.x-p0.x)*f;
#else
    // sitting on top of line
    const float tx = p0.x, ty = p0.y;
#endif
    mFM->drawStr(lbl.c_str(), tx, ty, angle_deg);
    idx += 10*(idx - idx0);
  }
}

void DianaLines::add_contour_line(contouring::level_t li, const contouring::points_t& cpoints, bool closed)
{
  if (li == DianaLevels::UNDEF_LEVEL and mPlotOptions.undefMasking != 2)
    return;
  if (li != DianaLevels::UNDEF_LEVEL and not mPlotOptions.options_1)
    return;

  point_v points(cpoints.begin(), cpoints.end());
  if (closed)
    points.push_back(cpoints.front());
  m_lines[li].push_back(points);
}

void DianaLines::add_contour_polygon(contouring::level_t level, const contouring::points_t& cpoints)
{
  if (level == DianaLevels::UNDEF_LEVEL and mPlotOptions.undefMasking != 1)
    return;
  if (level != DianaLevels::UNDEF_LEVEL and mPlotOptions.palettecolours.empty() and mPlotOptions.palettecolours_cold.empty())
    return;

  polygons_t& polygons = m_polygons[level];
  polygons.lengths.push_back(cpoints.size());
  for (contouring::points_t::const_iterator p = cpoints.begin(); p != cpoints.end(); ++p) {
    polygons.points.push_back(p->x);
    polygons.points.push_back(p->y);
    polygons.points.push_back(0);
  }
}

// ########################################################################

boost::shared_ptr<DianaLevels> dianaLevelsForPlotOptions(const PlotOptions& poptions, float fieldUndef)
{
  if (not poptions.linevalues.empty()) {
    return boost::make_shared<DianaLevelList>(poptions.linevalues);
  } else if (not poptions.loglinevalues.empty()) {
    // selected line values (the first values in rlines)
    // are drawn, the following vales drawn are the
    // previous multiplied by 10 and so on
    // (nlines=2 rlines=0.1,0.3 => 0.1,0.3,1,3,10,30,...)
    // (or the line at value=zoff)
    return boost::make_shared<DianaLevelList10>(poptions.loglinevalues, poptions.palettecolours.size());
  } else {
    boost::shared_ptr<DianaLevelStep> ls;
    if (poptions.zeroLine) // equally spaced lines (value)
      ls = boost::make_shared<DianaLevelStep>(poptions.lineinterval, poptions.base);
    else // idraw ==  2,  equally spaced lines, but not drawing the 0 line
      ls = boost::make_shared<DianaLevelStepOmit>(poptions.lineinterval, poptions.base);
    if (poptions.minvalue > -fieldUndef or poptions.maxvalue < fieldUndef)
      ls->set_limits(poptions.minvalue, poptions.maxvalue);
    return ls;
  }
}

// ########################################################################

// same parameters as diContouring::contour, most of them ignored
bool poly_contour(int nx, int ny, int ix0, int iy0, int ix1, int iy1,
    const float z[], const float xz[], const float yz[],
    FontManager* fp, const PlotOptions& poptions, float fieldUndef)
{
  DianaLevels_p levels = dianaLevelsForPlotOptions(poptions, fieldUndef);

  const DianaArrayIndex index(nx, ix0, iy0, ix1, iy1, poptions.lineSmooth);
  DianaPositions_p positions = boost::make_shared<DianaPositionsList>(index, xz, yz);

  const DianaField df(index, z, *levels, *positions);
  DianaLines dl(poptions, *levels, fp);

  { METLIBS_LOG_TIME("contouring");
    contouring::run(df, dl);
  }

  { METLIBS_LOG_TIME("painting");
    dl.paint();
  }

  return true;
}
