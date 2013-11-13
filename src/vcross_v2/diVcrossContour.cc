
#include "diVcrossContour.h"

#include "diVcrossAxis.h"
#include "diFontManager.h"

#include <GL/gl.h>
#if !defined(USE_PAINTGL)
#include <glText/glText.h>
#endif

#include <boost/foreach.hpp>

#include <cmath>

#define MILOGGER_CATEGORY "diana.VcrossContour"
#include <miLogger/miLogging.h>

namespace VcrossPlotDetail {

const float UNDEF_VALUE = 1e30;

inline bool isUndefined(float v)
{
  return std::isnan(v) or v >= UNDEF_VALUE or v < -UNDEF_VALUE;
}

void VCContourField::setLevels(float lstep)
{
  float vMin = UNDEF_VALUE, vMax = UNDEF_VALUE;
  for (size_t ix=0; ix<nx(); ++ix) {
    for (size_t iy=0; iy<ny(); ++iy) {
      const float v = mData[index(ix, iy)];
      if (isUndefined(v))
        continue;
      if (isUndefined(vMin) or v < vMin)
        vMin = v;
      if (isUndefined(vMax) or v > vMax)
        vMax = v;
    }
  }
  
  if (not isUndefined(vMin)) {
    const float lstart = ((int)(vMin / lstep)+1) * lstep;
    const float lstop  = ((int)(vMax / lstep)+1) * lstep;
    setLevels(lstart, lstop, lstep);
  }
}

void VCContourField::setLevels(float lstart, float lstop, float lstep)
{
  mLstepInv  = 1/lstep;
  mLevels.clear();
  for (float l=lstart; l<=lstop; l+=lstep)
    mLevels.push_back(l);
}

float VCContourField::value(int ix, int iy) const
{
  const contouring::Point p = position(ix, iy);
  if (not (mXpos->legalPaint(p.x) and mYpos->legalPaint(p.y)))
    return UNDEF_VALUE;
  return mData[index(ix, iy)];
}

contouring::Point VCContourField::position(int ix, int iy) const
{
  const float vx = mXval[ix], vy = mYval->value(iy, ix);
  return contouring::Point(mXpos->value2paint(vx), mYpos->value2paint(vy));
}

contouring::Point VCContourField::point(int levelIndex, int x0, int y0, int x1, int y1) const
{
  const float v0 = value(x0, y0);
  const float v1 = value(x1, y1);
  const float c = (mLevels[levelIndex]-v0)/(v1-v0);
  
  const contouring::Point p0 = position(x0, y0);
  const contouring::Point p1 = position(x1, y1);
  const float x = (1-c)*p0.x + c*p1.x; // FIXME interpolate before calling position?
  const float y = (1-c)*p0.y + c*p1.y;
  return contouring::Point(x, y);
}

int VCContourField::level_point(int ix, int iy) const
{
  const float v = value(ix, iy);
  if (isUndefined(v))
    return Field::UNDEFINED;
  return level_value(v);
}

int VCContourField::level_center(int cx, int cy) const
{
  const float v_00 = value(cx, cy), v_10 = value(cx+1, cy), v_01 = value(cx, cy+1), v_11 = value(cx+1, cy+1);
  if (isUndefined(v_00) or isUndefined(v_01) or isUndefined(v_10) or isUndefined(v_11))
    return Field::UNDEFINED;

  const float avg = 0.25*(v_00 + v_01 + v_10 + v_11);
  return level_value(avg);
}

int VCContourField::level_value(float value) const
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

VCContouring::VCContouring(contouring::Field* field, FontManager* fm, const PlotOptions& poptions)
  : PolyContouring(field)
  , mFM(fm)
  , mPlotOptions(poptions)
{
  glShadeModel(GL_FLAT);
  glDisable(GL_LINE_STIPPLE);
}

VCContouring::~VCContouring()
{
}

void VCContouring::emitLine(int li, contouring::Polyline& points, bool close)
{
  const bool highlight = ((li % 5) == 0);

  glColor3ubv(mPlotOptions.linecolour.RGB());
  int lw = mPlotOptions.linewidth;
  if (highlight)
    lw += 1;

  VCContourField* vcf = static_cast<VCContourField*>(mField);;
  { // draw line
    glLineWidth(lw);
    glBegin(GL_LINE_STRIP);
    BOOST_FOREACH(const contouring::Point& p, points) {
      glVertex2f(p.x, p.y);
    }
    if (close)
      glVertex2f(points.front().x, points.front().y);
    glEnd();
  }
  if (highlight) { // draw label
    const int idx = int(0.1*(1 + (li % 5))) * points.size();
    contouring::Polyline::const_iterator it = points.begin();
    std::advance(it, idx);
    const contouring::Point& p0 = *it;
    ++it;
    const contouring::Point& p1 = *it;
    const float angle = atan2f(p1.y - p0.y, p1.x - p0.x) * 180. / 3.141592654;
    std::ostringstream o;
    o << vcf->getLevel(li);
    mFM->drawStr(o.str().c_str(), p0.x, p0.y, angle);
  }
}

} // namespace VcrossPlotDetail
