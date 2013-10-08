
#include "diPolyContouring.h"

#include "diContouring.h"
#include "diFontManager.h"
#include "PolyContouring.h"

#include <GL/gl.h>
#if !defined(USE_PAINTGL)
#include <glp/glpfile.h>
#endif

#include <boost/foreach.hpp>

#include <cmath>
#include <vector>

#define M_TIME
#define MILOGGER_CATEGORY "diana.PolyContouring"
#include <miLogger/miLogging.h>

const float UNDEF_VALUE = 1e30;

inline bool isUndefined(float v)
{
  return v >= UNDEF_VALUE or v < -UNDEF_VALUE;
}

class DianaField : public contouring::Field {
public:
  DianaField(int nx, int ny, const int ipar[], const float* data, const float* xpos, const float* ypos)
    : mNX(nx), mNY(ny), mX0(ipar[0]), mX1(ipar[1]), mY0(ipar[2]), mY1(ipar[3])
    , mData(data), mXpos(xpos), mYpos(ypos) { }
    
  virtual ~DianaField() { }

  virtual int nx() const
    { return mX1-mX0; }
  
  virtual int ny() const
    { return mY1-mY0; }

  virtual int nlevels() const
    { return mLevels.size(); }

  virtual int level_point(int ix, int iy) const;
  virtual int level_center(int cx, int cy) const;

  virtual contouring::Point point(int levelIndex, int x0, int y0, int x1, int y1) const;

  void setLevels(float lstart, float lstop, float lstep);
  const std::vector<float>& levels() const
    { return mLevels; }

  float getLevel(int idx) const
    { return mLevels[idx]; }
  
private:
  int index(int ix, int iy) const
    {  return (mY0 + iy)*mNX + (mX0+ix); }

  float value(int ix, int iy) const
    { return mData[index(ix, iy)]; }

  contouring::Point position(int ix, int iy) const
    { int i = index(ix, iy); return contouring::Point(mXpos[i], mYpos[i]); }

  int level_value(float value) const;

private:
  std::vector<float> mLevels;
  
  int mNX, mNY, mX0, mX1, mY0, mY1;
  const float *mData, *mXpos, *mYpos;
  float mLstepInv;
};

void DianaField::setLevels(float lstart, float lstop, float lstep)
{
  mLstepInv  = 1/lstep;
  mLevels.clear();
  for (float l=lstart; l<=lstop; l+=lstep)
    mLevels.push_back(l);
}

contouring::Point DianaField::point(int levelIndex, int x0, int y0, int x1, int y1) const
{
    const float v0 = value(x0, y0);
    const float v1 = value(x1, y1);
    const float c = (mLevels[levelIndex]-v0)/(v1-v0);

    const contouring::Point p0 = position(x0, y0);
    const contouring::Point p1 = position(x1, y1);
    const float x = (1-c)*p0.x + c*p1.x;
    const float y = (1-c)*p0.y + c*p1.y;
    return contouring::Point(x, y);
}

int DianaField::level_point(int ix, int iy) const
{
  const float v = value(ix, iy);
  if (isUndefined(v))
    return Field::UNDEFINED;
  return level_value(v);
}

int DianaField::level_center(int cx, int cy) const
{
  const float v_00 = value(cx, cy), v_10 = value(cx+1, cy), v_01 = value(cx, cy+1), v_11 = value(cx+1, cy+1);
  if (isUndefined(v_00) or isUndefined(v_01) or isUndefined(v_10) or isUndefined(v_11))
    return Field::UNDEFINED;

  const float avg = 0.25*(v_00 + v_01 + v_10 + v_11);
  return level_value(avg);
}

int DianaField::level_value(float value) const
{
#if 1
  if (value < mLevels.front())
    return 0;
  if (value >= mLevels.back())
    return mLevels.size();
  return (int)((value-mLevels.front()) * mLstepInv - 1000) + 1000;
#else
  return std::lower_bound(mLevels.begin(), mLevels.end(), value) - mLevels.begin();
#endif
}

// ########################################################################

class DianaContouring : public contouring::PolyContouring {
public:
  DianaContouring(DianaField* field, FontManager* fm);
  ~DianaContouring();
  virtual void emitLine(int li, contouring::Polyline& points, bool close);
private:
  FontManager* mFM;
};

DianaContouring::DianaContouring(DianaField* field, FontManager* fm)
  : PolyContouring(field)
  , mFM(fm)
{
  glShadeModel(GL_FLAT);
  glDisable(GL_LINE_STIPPLE);
}

DianaContouring::~DianaContouring()
{
}

void DianaContouring::emitLine(int li, contouring::Polyline& points, bool close)
{
  DianaField* df = static_cast<DianaField*>(mField);
  { // set some colour
#if 1
    float fraction = 0.5;
    if (df->nlevels() >= 2) {
      const float l0 = df->levels().front(), l1 = df->levels().back();
      if (abs(l1 - l0) >= 1e-3)
        fraction = (df->levels()[li] - l0)/(l1 - l0);
    }
    unsigned char rgb[3] = { (unsigned char)(0xFF*fraction), 0, (unsigned char)(0x80*(1-fraction)) };
#else
    unsigned char rgb[3] = { 0, 0, 0xff };
#endif
    glColor3ubv(rgb);
  }

  { // draw line
    glLineWidth((li % 5) == 0 ? 2 : 1);
    glBegin(GL_LINE_STRIP);
    BOOST_FOREACH(const contouring::Point& p, points)
        glVertex2f(p.x, p.y);
    if (close)
      glVertex2f(points.front().x, points.front().y);
    glEnd();
  }
  { // draw label
    const int idx = int(0.1*(1 + (li % 5))) * points.size();
    contouring::Polyline::const_iterator it = points.begin();
    std::advance(it, idx);
    const contouring::Point& p0 = *it;
    ++it;
    const contouring::Point& p1 = *it;
    const float angle = atan2f(p1.y - p0.y, p1.x - p0.x) * 180. / 3.141592654;
    std::ostringstream o;
    o << static_cast<DianaField*>(mField)->getLevel(li);
    mFM->drawStr(o.str().c_str(), p0.x, p0.y, angle);
  }
}


// ########################################################################

static inline float rounded(float value, float unit)
{
  float r = (value>=0) ? 0.5 : -0.5;
  return int(value / unit + r) * unit;
}

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
  bool supported = (idraw == 1) and (icxy == 2);
  if (not supported) {
    return contour(nx, ny, z, xz, yz, ipart, icxy, cxy, xylim,
        idraw,  zrange,  zstep,  zoff,  nlines,  rlines,  ncol,  icol,  ntyp,  ityp,  nwid,  iwid,  nlim,  rlim,
        idraw2, zrange2, zstep2, zoff2, nlines2, rlines2, ncol2, icol2, ntyp2, ityp2, nwid2, iwid2, nlim2, rlim2,
        ismooth, labfmt, chxlab, chylab, ibcol, ibmap, lbmap, kbmap, nxbmap, nybmap, rbmap,
        fp, poptions, psoutput, fieldArea, fieldUndef, modelName, paramName, fhour);
  }

#if 0
  {
    std::ofstream dump(("/tmp/field_dump/" + modelName + "_" + paramName + ".dat").c_str());
    dump << "# model " << modelName << " param " << paramName << std::endl;
    dump << "level-range 0 0 " << zstep << std::endl;
    dump << "nx " << (ipart[1] - ipart[0]) << std::endl;
    dump << "ny " << (ipart[3] - ipart[2]) << std::endl;
    dump << "data" << std::endl;
    for (int iy=ipart[2]; iy<ipart[3]; ++iy)
      for (int ix=ipart[0]; ix<ipart[1]; ++ix)
        dump << z[iy*nx + ix] << ' ';
    dump.close();
  }
#endif

  METLIBS_LOG_TIME();
  METLIBS_LOG_DEBUG(LOGVAL(icxy));
  DianaField df(nx, ny, ipart, z, xz, yz);
  float lstart = zrange[0], lstop = zrange[1];
  if (zrange[1] < zrange[0]) {
    METLIBS_LOG_TIME();
    METLIBS_LOG_DEBUG("find min/max" << LOGVAL(zstep));
    bool have_defined = false;
    for (int i=0; i<nx*ny; ++i) {
      if (z[i] > UNDEF_VALUE or z[i] < -UNDEF_VALUE)
        continue;
      float zi = rounded(z[i], zstep);
      if (have_defined) {
        lstart = std::min(zi, lstart);
        lstop  = std::max(zi, lstop);
      } else {
        lstart = lstop = zi;
      }
      have_defined = true;
    }
    if (not have_defined)
      return true; // TODO only undefined values
  }
  METLIBS_LOG_DEBUG(LOGVAL(lstart) << LOGVAL(lstop) << LOGVAL(zstep));
  df.setLevels(lstart, lstop, zstep);
  DianaContouring dc(&df, fp);
  dc.makeLines();
  return true;
}
