
#include "diPolyContouring.h"

#include "diPlotOptions.h"
#include "diFontManager.h"
#include <diTesselation.h>

#include "poly_contouring.hh"

#include <GL/gl.h>
#if !defined(USE_PAINTGL)
#include <glp/glpfile.h>
#endif

#include <boost/foreach.hpp>

#include <cmath>
#include <vector>

#define MILOGGER_CATEGORY "diana.PolyContouring"
#include <miLogger/miLogging.h>

const float UNDEF_VALUE = 1e30;
const contouring::level_t UNDEFINED = -1000000;

inline bool isUndefined(float v)
{
  return v >= UNDEF_VALUE or v < -UNDEF_VALUE;
}

static inline int rounded_div(float value, float unit)
{
    const int i = int(value / unit);
    if (value >= 0)
        return i+1;
    else
        return i;
}

class DianaPositions {
public:
  virtual ~DianaPositions() { }
  virtual contouring::point_t position(size_t ix, size_t iy) const = 0;
};

class DianaPositionsSimple : public DianaPositions {
public:
  virtual contouring::point_t position(size_t ix, size_t iy) const
    { return contouring::point_t(ix, iy); }
};

class DianaPositionsList : public DianaPositions {
public:
  DianaPositionsList(int nx, int ny, const int ipart[], const float* xpos, const float* ypos)
    : mNX(nx), mNY(ny), mX0(ipart[0]), mY0(ipart[2]) , mXpos(xpos), mYpos(ypos) { }
  
  size_t index(size_t ix, size_t iy) const
    {  return (mY0 + iy)*mNX + (mX0+ix); }

  contouring::point_t position(size_t ix, size_t iy) const
    { const size_t i = index(ix, iy); return contouring::point_t(mXpos[i], mYpos[i]); }

private:
  int mNX, mNY, mX0, mY0;
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

// ========================================================================

class DianaLevels {
public:
  virtual ~DianaLevels() { }
  virtual contouring::level_t level_for_value(float value) const = 0;
  virtual float value_for_level(contouring::level_t l) const = 0;
  bool omit(contouring::level_t l) const { return l == OMIT_LEVEL; }
  enum { UNDEF_LEVEL = -10000000, OMIT_LEVEL = -10000001 };
};

static const int dl = 200;
class DianaLevelList : public DianaLevels {
public:
  DianaLevelList(const std::vector<float>& levels)
    : mLevels(levels) { }
  DianaLevelList(float lstart, float lstop, float lstep)
    { for (float l=lstart; l<=lstop; l+=lstep) mLevels.push_back(l); }
  virtual contouring::level_t level_for_value(float value) const
    { return (std::lower_bound(mLevels.begin(), mLevels.end(), value) - mLevels.begin()) - dl; }
  virtual float value_for_level(contouring::level_t l) const
    { return mLevels[l+dl]; }
  size_t nlevels() const
    { return mLevels.size(); }
protected:
  std::vector<float> mLevels;
};

class DianaLevelList10 : public DianaLevelList {
public:
  DianaLevelList10(const std::vector<float>& levels)
    : DianaLevelList(levels) { }
  virtual contouring::level_t level_for_value(float value) const
    { float v = log(value)/log(10); const contouring::level_t l = DianaLevelList::level_for_value(v-int(v)); return int(v)*nlevels() + l; }
  virtual float value_for_level(contouring::level_t l) const
    { return std::pow(10, l / nlevels()) + mLevels[l % nlevels()]; }
};

class DianaLevelStep : public DianaLevels {
public:
  DianaLevelStep(float step, float off)
    : mStep(step), mOff(off), mMin(1), mMax(0) { }
  void set_limits(float mini, float maxi)
    { mMin = mini; mMax = maxi; }
  virtual contouring::level_t level_for_value(float value) const;
  virtual float value_for_level(contouring::level_t l) const
    { return l*mStep + mOff; }
protected:
  float mStep, mOff, mMin, mMax;
};

contouring::level_t DianaLevelStep::level_for_value(float value) const
{
  if (mMin < mMax) {
    if (value < mMin)
      value = mMin;
    else if (value > mMax)
      value = mMax;
  }
  return rounded_div(value - mOff, mStep);
}

class DianaLevelStepOmit : public DianaLevelStep {
public:
  DianaLevelStepOmit(float step, float off)
    : DianaLevelStep(step, off) { }
  virtual contouring::level_t level_for_value(float value) const
    { if (value == mOff) return OMIT_LEVEL; else return DianaLevelStep::level_for_value(value); }
};

// ========================================================================

class DianaField : public contouring::field_t {
public:
  DianaField(int nx, int ny, const int ipart[], const float* data, const DianaLevels& levels, const DianaPositions& positions)
    : mNX(nx), mNY(ny), mX0(ipart[0]), mX1(ipart[1]), mY0(ipart[2]), mY1(ipart[3])
    , mData(data), mLevels(levels), mPositions(positions) { }
  
  virtual size_t nx() const
    { return mX1-mX0; }
  
  virtual size_t ny() const
    { return mY1-mY0; }

  virtual contouring::level_t grid_level(size_t ix, size_t iy) const
    { return mLevels.level_for_value(value(ix, iy)); }
  virtual contouring::point_t line_point(contouring::level_t level, size_t x0, size_t y0, size_t x1, size_t y1) const;
  virtual contouring::point_t grid_point(size_t x, size_t y) const
    { return position(x, y); }

  contouring::level_t undefined_level() const
    { return DianaLevels::UNDEF_LEVEL; }
  
private:
  size_t index(size_t ix, size_t iy) const
    {  return (mY0 + iy)*mNX + (mX0+ix); }

  float value(size_t ix, size_t iy) const
    { return mData[index(ix, iy)]; }

  contouring::point_t position(size_t ix, size_t iy) const
    { return mPositions.position(ix, iy); }

private:
  int mNX, mNY, mX0, mX1, mY0, mY1;
  const float *mData;
  const DianaLevels& mLevels;
  const DianaPositions& mPositions;
};

contouring::point_t DianaField::line_point(contouring::level_t level, size_t x0, size_t y0, size_t x1, size_t y1) const
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

class DianaLines : public contouring::lines_t {
public:
  DianaLines(const std::vector<int>& colours, const PlotOptions& poptions, const DianaLevels& levels, FontManager* fm);

  void add_contour_line(contouring::level_t level, const contouring::points_t& points, bool closed);
  void add_contour_polygon(contouring::level_t level, const contouring::points_t& points);

private:
  void set_colour(contouring::level_t l) const;

private:
  const std::vector<int> mColours;
  const PlotOptions& mPoptions;
  const DianaLevels& mLevels;
  FontManager* mFM;
};

DianaLines::DianaLines(const std::vector<int>& colours, const PlotOptions& poptions, const DianaLevels& levels, FontManager* fm)
  : mColours(colours), mPoptions(poptions), mLevels(levels), mFM(fm)
{
  glShadeModel(GL_FLAT);
  glDisable(GL_LINE_STIPPLE);
}

void DianaLines::set_colour(contouring::level_t l) const
{
  const int idx = std::min(l % mColours.size(), mPoptions.colours.size());
  glColor3ubv(mPoptions.colours[idx].RGB());
}

void DianaLines::add_contour_line(contouring::level_t li, const contouring::points_t& points, bool closed)
{
  set_colour(li);

  { // draw line
    glLineWidth(mPoptions.linewidth);
    glBegin(GL_LINE_STRIP);
    BOOST_FOREACH(const contouring::point_t& p, points)
        glVertex2f(p.x, p.y);
    if (closed)
      glVertex2f(points.front().x, points.front().y);
    glEnd();
  }
  if (1) { // draw label
    const int idx = int(0.1*(1 + (li % 5))) * points.size();
    contouring::points_t::const_iterator it = points.begin();
    std::advance(it, idx);
    const contouring::point_t& p0 = *it;
    ++it;
    const contouring::point_t& p1 = *it;
    const float angle = atan2f(p1.y - p0.y, p1.x - p0.x) * 180. / 3.141592654;
    std::ostringstream o;
    o << mLevels.value_for_level(li);
    mFM->drawStr(o.str().c_str(), p0.x, p0.y, angle);
  }
}

void DianaLines::add_contour_polygon(contouring::level_t level, const contouring::points_t& points)
{
#if 0
  set_colour(level);

  beginTesselation();

  glShadeModel(GL_FLAT);
  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  int j = 0, countpos = points.size();
  GLdouble *gldata= new GLdouble[3*countpos];
  BOOST_FOREACH(const contouring::point_t& p, points) {
    gldata[j++] = p.x;
    gldata[j++] = p.y;
    gldata[j++] = 0;
  }
  tesselation(gldata, 1, &countpos);

  glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
  glDisable(GL_BLEND);
  glShadeModel(GL_FLAT);
  glEdgeFlag(GL_TRUE);
#endif
}

// ########################################################################

// same parameters as diContouring::contour, most of them ignored
bool poly_contour(int nx, int ny, float z[], float xz[], float yz[],
    const int ipart[], int icxy, float cxy[], float xylim[],
    int idraw, float zrange[], float zstep, float zoff, int nlines, float rlines[], int ncol, int icol[],
    int ntyp, int ityp[], int nwid, int iwid[], int nlim, float rlim[],
    int idraw2, float zrange2[], float zstep2, float zoff2, int nlines2, float rlines2[], int ncol2, int icol2[],
    int ntyp2, int ityp2[], int nwid2, int iwid2[], int nlim2, float rlim2[],
    int ismooth, const int labfmt[], float chxlab, float chylab,
    int ibcol, int ibmap, int lbmap, int kbmap[], int nxbmap, int nybmap, float rbmap[],
    FontManager* fp, const PlotOptions& poptions, GLPfile* psoutput,
    const Area& fieldArea, const float& fieldUndef, const std::string& modelName, const std::string& paramName, const int& fhour)
{
  DianaLevels* levels;
  if (idraw == 1 or idraw == 2) {
    DianaLevelStep* ls;
    if (idraw == 1) // equally spaced lines (value)
      ls = new DianaLevelStep(zstep, zoff);
    else // idraw ==  2,  equally spaced lines, but not drawing the 0 line
      ls = new DianaLevelStepOmit(zstep, zoff);
    ls->set_limits(zrange[0], zrange[1]);
    levels = ls;
  } else if (idraw == 3) { // selected line values (in rlines) drawn
    levels = new DianaLevelList(std::vector<float>(rlines, rlines + nlines));
  } else if (idraw == 4) {
    // selected line values (the first values in rlines)
    // are drawn, the following vales drawn are the
    // previous multiplied by 10 and so on
    // (nlines=2 rlines=0.1,0.3 => 0.1,0.3,1,3,10,30,...)
    // (or the line at value=zoff)
    levels = new DianaLevelList10(std::vector<float>(rlines, rlines + nlines));
  } else {
    return false;
  }

  DianaPositions* positions;
  if (icxy == 0) { // map and field coordinates are equal
    positions = new DianaPositionsSimple();
  } else if (icxy == 1) { // using cxy to position field on map
    positions = new DianaPositionsFormula(cxy);
  } else if (icxy == 2) { // using x and y to position field on map
    positions = new DianaPositionsList(nx, ny, ipart, xz, yz);
  } else {
    delete levels;
    return false;
  }

  const DianaField df(nx, ny, ipart, z, *levels, *positions);

  METLIBS_LOG_TIME();
  DianaLines dl(std::vector<int>(icol, icol+ncol), poptions, *levels, fp);

  contouring::run(df, dl);

  delete positions;
  delete levels;

  return true;
}
