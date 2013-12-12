
#include "diVcrossUtil.h"

#include "diFontManager.h"

#include <diField/diMetConstants.h>

#include <puTools/miStringFunctions.h>

#include <GL/gl.h>
#if !defined(USE_PAINTGL)
#include <glText/glText.h>
#include <glp/glpfile.h>
#endif

#include <cmath>
#include <ostream>

namespace VcrossUtil {

float exnerFunction(float p)
{
  // cp, p0inv, and kappa are defined in metlibs/diField/diMetConstants.h
  using namespace MetNo::Constants;
  return cp * std::pow(p * p0inv, kappa);
}

float exnerFunctionInverse(float e)
{
  // invert e=cp*exp(kappa*log(p*p0inv))
  using namespace MetNo::Constants;
  return std::exp(std::log(e/cp)/kappa)/p0inv;
}

float coriolisFactor(float lat /* radian */)
{
  // FIXME see diField/diProjection.cc: Projection::getMapRatios
  const float EARTH_OMEGA = 2*M_PI / (24*3600); // earth rotation in radian / s
  return 2 * EARTH_OMEGA * sin(lat);
}

static void writeLonEWLatNS(std::ostream& out, float lonlat, char EN, char WS)
{
  out << std::abs(lonlat);
  if (lonlat >= 0)
    out << EN;
  else
    out << WS;
}

void writeLonEW(std::ostream& out, float lon)
{
  writeLonEWLatNS(out, lon, 'E', 'W');
}

void writeLatNS(std::ostream& out, float lat)
{
  writeLonEWLatNS(out, lat, 'N', 'S');
}

void updateMaxStringWidth(FontManager* fp, float& w, const std::string& txt)
{
  float txt_w, dummy_h;
  fp->getStringSize(txt.c_str(), txt_w, dummy_h);
  maximize(w, txt_w);
}

QSizeF charSize(FontManager* fp, const char* txt)
{
  float cx=0, cy=0;
  fp->getCharSize('0', cx, cy);
  return QSizeF(cx, cy*0.75);
}

bool parseNameAndArgs(const std::string& def, std::string& func, std::vector<std::string>& vars)
{
  const size_t k1 = def.rfind('('), k2 = def.rfind(')');
  if (k1 != std::string::npos && k2 != std::string::npos && k2 > k1 + 1) {
    func = def.substr(0, k1);
    vars = miutil::split(def.substr(k1 + 1, k2 - k1 - 1), 0, ",");
    return true;
  } else {
    return false;
  }
}

// FIXME code copied from imetlibs/metgdata/src/VerticalProfile.cc

// Computing the height by the barometric equation.
// using the standard atmosphere T0=15degC / mslp=1013.25 / deltaT=6.5K/km

float pressureFromHeight(float height)
{
  float buf = (1 - (0.0065 * height) / 288.15);
  return 1013.25 * powf(buf, 5.255);
}

float heightFromPressure(float pressure)
{
  float k = 1 / 5.255;
  return (1 - powf((pressure / 1013.25), k)) * 288.15 / 0.0065;
}

void xyclip(int npos, float *x, float *y, float xylim[4])
{
  //  plotter del(er) av sammenhengende linje som er innenfor gitt
  //  omraade, ogsaa linjestykker mellom 'nabopunkt' som begge er
  //  utenfor omraadet.
  //  (farge, linje-type og -tykkelse maa vaere satt paa forhaand)
  //
  //  grafikk: OpenGL
  //
  //  input:
  //  ------
  //  x(npos),y(npos): linje med 'npos' punkt (npos>1)
  //  xylim(1-4):      x1,x2,y1,y2 for aktuelt omraade

  int nint, nc, n, i, k1, k2;
  float xa, xb, ya, yb, xint = 0.0, yint = 0.0, x1, x2, y1, y2;
  float xc[4], yc[4];

  if (npos < 2)
    return;

  xa = xylim[0];
  xb = xylim[1];
  ya = xylim[2];
  yb = xylim[3];

  if (x[0] < xa || x[0] > xb || y[0] < ya || y[0] > yb) {
    k2 = 0;
  } else {
    k2 = 1;
    nint = 0;
    xint = x[0];
    yint = y[0];
  }

  for (n = 1; n < npos; ++n) {
    k1 = k2;
    k2 = 1;

    if (x[n] < xa || x[n] > xb || y[n] < ya || y[n] > yb)
      k2 = 0;

    // sjekk om 'n' og 'n-1' er innenfor
    if (k1 + k2 == 2)
      continue;

    // k1+k2=1: punkt 'n' eller 'n-1' er utenfor
    // k1+k2=0: sjekker om 2 nabopunkt som begge er utenfor omraadet
    //          likevel har en del av linja innenfor.

    x1 = x[n - 1];
    y1 = y[n - 1];
    x2 = x[n];
    y2 = y[n];

    // sjekker om 'n-1' og 'n' er utenfor paa samme side
    if (k1 + k2 == 0 && ((x1 < xa && x2 < xa) || (x1 > xb && x2 > xb) || (y1
        < ya && y2 < ya) || (y1 > yb && y2 > yb)))
      continue;

    // sjekker alle skjaerings-muligheter
    nc = -1;
    if (x1 != x2) {
      nc++;
      xc[nc] = xa;
      yc[nc] = y1 + (y2 - y1) * (xa - x1) / (x2 - x1);
      if (yc[nc] < ya || yc[nc] > yb || (xa - x1) * (xa - x2) > 0.)
        nc--;
      nc++;
      xc[nc] = xb;
      yc[nc] = y1 + (y2 - y1) * (xb - x1) / (x2 - x1);
      if (yc[nc] < ya || yc[nc] > yb || (xb - x1) * (xb - x2) > 0.)
        nc--;
    }
    if (y1 != y2) {
      nc++;
      yc[nc] = ya;
      xc[nc] = x1 + (x2 - x1) * (ya - y1) / (y2 - y1);
      if (xc[nc] < xa || xc[nc] > xb || (ya - y1) * (ya - y2) > 0.)
        nc--;
      nc++;
      yc[nc] = yb;
      xc[nc] = x1 + (x2 - x1) * (yb - y1) / (y2 - y1);
      if (xc[nc] < xa || xc[nc] > xb || (yb - y1) * (yb - y2) > 0.)
        nc--;
    }

    if (k2 == 1) {
      // foerste punkt paa linjestykke innenfor
      nint = n - 1;
      xint = xc[0];
      yint = yc[0];
    } else if (k1 == 1) {
      // siste punkt paa linjestykke innenfor
      glBegin(GL_LINE_STRIP);
      glVertex2f(xint, yint);
      for (i = nint + 1; i < n; i++)
        glVertex2f(x[i], y[i]);
      glVertex2f(xc[0], yc[0]);
      glEnd();
    } else if (nc > 0) {
      // to 'nabopunkt' utenfor, men del av linja innenfor
      glBegin(GL_LINE_STRIP);
      glVertex2f(xc[0], yc[0]);
      glVertex2f(xc[1], yc[1]);
      glEnd();
    }
  }

  if (k2 == 1) {
    // siste punkt er innenfor omraadet
    glBegin(GL_LINE_STRIP);
    glVertex2f(xint, yint);
    for (i = nint + 1; i < npos; i++)
      glVertex2f(x[i], y[i]);
    glEnd();
  }
}

//copy from diAnnotationPlot, move to std::string?

std::vector<std::string> split(const std::string eString, const char s1, const char s2)
{
  /*finds entries delimited by s1 and s2
   (f.ex. s1=<,s2=>) <"this is the entry">
   anything inside " " is ignored as delimiters
   f.ex. <"this is also > an entry">
   */
  int stop, start, stop2, start2, len;
  std::vector<std::string> vec;

  if (eString.empty())
    return vec;

  len = eString.length();
  start = eString.find_first_of(s1, 0) + 1;
  while (start >= 1 && start < len) {
    start2 = eString.find_first_of('"', start);
    stop = eString.find_first_of(s2, start);
    while (start2 > -1 && start2 < stop) {
      // search for second ", which should come before >
      stop2 = eString.find_first_of('"', start2 + 1);
      if (stop2 > -1) {
        start2 = eString.find_first_of('"', stop2 + 1);
        stop = eString.find_first_of(s2, stop2 + 1);
      } else
        start2 = -1;
    }

    //if maching s2 found, add entry
    if (stop > 0 && stop < len)
      vec.push_back(eString.substr(start, stop - start));

    //next s1
    if (stop < len)
      start = eString.find_first_of(s1, stop) + 1;
    else
      start = len;
  }
  return vec;
}

void plotArrow(const float& x0, const float& y0, const float& dx, const float& dy, bool thinArrows)
{

  // arrow layout
  const float afac = -1. / 3.;
  const float sfac = -afac * 0.57735;
  const float wfac = 1. / 38.;

  float x1 = x0 + dx;
  float y1 = y0 + dy;

  if (!thinArrows) {

    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glShadeModel(GL_FLAT);
    // each arrow splitted into three triangles
    glBegin(GL_TRIANGLES);
    // arrow head
    glVertex2f(x1, y1);
    glVertex2f(x1 + afac * dx - sfac * dy, y1 + afac * dy + sfac * dx);
    glVertex2f(x1 + afac * dx + sfac * dy, y1 + afac * dy - sfac * dx);
    // the rest as two triangles (1)
    glVertex2f(x0 - wfac * dy, y0 + wfac * dx);
    glVertex2f(x1 + afac * dx - wfac * dy, y1 + afac * dy + wfac * dx);
    glVertex2f(x1 + afac * dx + wfac * dy, y1 + afac * dy - wfac * dx);
    // the rest as two triangles (2)
    glVertex2f(x1 + afac * dx + wfac * dy, y1 + afac * dy - wfac * dx);
    glVertex2f(x0 + wfac * dy, y0 - wfac * dx);
    glVertex2f(x0 - wfac * dy, y0 + wfac * dx);
    glEnd();

  } else {

    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x1 + afac * dx - sfac * dy, y1 + afac * dy + sfac * dx);
    glVertex2f(x1, y1);
    glVertex2f(x1 + afac * dx + sfac * dy, y1 + afac * dy - sfac * dx);
    glVertex2f(x1, y1);
    glVertex2f(x0, y0);
    glEnd();

  }
}

} // namespace VcrossUtil
