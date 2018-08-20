/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2018 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 email: diana@met.no

 This file is part of Diana

 Diana is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 Diana is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Diana; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
 vertical profile diagram/background
 */

#include "diana_config.h"

#include "diVprofDiagram.h"

#include "diColour.h"
#include "diGLPainter.h"
#include "diGlUtilities.h"
#include "diLinetype.h"
#include "diUtilities.h"

#include "diField/VcrossUtil.h"
#include "diField/diMetConstants.h"
#include "diField/diPoint.h"
#include "diField/diRectangle.h"
#include <puTools/miStringFunctions.h>

#include <cmath>
#include <iomanip>
#include <sstream>

using namespace::miutil;
using namespace std;
using diutil::PointF;

#define MILOGGER_CATEGORY "diana.VprofDiagram"
#include <miLogger/miLogging.h>

namespace {
const float DEG_TO_RAD = M_PI / 180;

bool is_invalid(float f)
{
  return isnan(f) || abs(f) >= 1e20;
}
bool is_invalid_int(int i)
{
  const int LIMIT = (1 << 30);
  return i < -LIMIT || i > LIMIT;
}

bool all_valid(const std::vector<float>& data)
{
  return std::find_if(data.begin(), data.end(), is_invalid) == data.end();
}
bool all_valid(const std::vector<int>& data)
{
  return std::find_if(data.begin(), data.end(), is_invalid_int) == data.end();
}

struct TextSpacing
{
  float next, last, spacing;
  TextSpacing(float n, float l, float s)
      : next(n)
      , last(l)
      , spacing(s)
  {
  }
  bool accept(float v)
  {
    if ((spacing > 0 && v > next && v < last) || (spacing < 0 && v < next && v > last)) {
      next = v + spacing;
      return true;
    } else {
      return false;
    }
  }
};

// standard stoerrelse iflg. et amble-diagram:
// avstand i cm mellom 100 og 1000 hPa
const float height = 27.8;

// lengde i cm langs p=konst for 1 grad celsius
const float dx1deg = 0.418333;

PointF interpolateY(float y, const PointF& p1, const PointF& p2)
{
  const float r = (y - p1.y()) / (p2.y() - p1.y());
  const float x = p1.x() + (p2.x() - p1.x()) * r;
  return PointF(x, y);
}

PointF interpolateX(float x, const PointF& p1, const PointF& p2)
{
  const float r = (x - p1.x()) / (p2.x() - p1.x());
  const float y = p1.y() + (p2.y() - p1.y()) * r;
  return PointF(x, y);
}

typedef std::vector<PointF> PointF_v;

/**
 * @brief Calculate temperature for given mixing ration and pressure.
 * @param qsat in unit kg/kg
 * @param p pressure in hPa
 * @return t in C
 */
float mixingRatioT(float qsat, float p)
{
  using namespace MetNo::Constants;
  const float esat = qsat * p / eps;
  int n = 1;
  while (ewt[n] < esat && n < N_EWT - 1)
    n++;
  const float x = (esat - ewt[n - 1]) / (ewt[n] - ewt[n - 1]);
  const float t = -100. + (n - 1. + x) * 5.;
  return t;
}

/**
 * @brief Calculate (p,t) line of constant mixing ratio qsat.
 *
 * Tries to start exactly on tempmin and to stop exactly on pmax.
 *
 * @param qsat in unit kg/kg
 * @param pmin start pressure in hPa
 * @param pmax end pressure in hPa
 * @param pstep pressure step in hPa, > 0
 * @param tempmin in C
 * @return list of (p,t) points, p in hPa, t in C
 */
PointF_v mixingRatioLine(float qsat, float pmin, float pmax, float pstep, float tempmin = -500)
{
  PointF_v ptline;
  PointF pt0;
  for (float p = pmin; p <= pmax + pstep / 4; p += pstep) {
    const float t = mixingRatioT(qsat, p);
    const PointF pt1(p, t);
    if (t >= tempmin) {
      if (p > pmin /* i.e. have pt0 */ && pt0.y() < tempmin)
        // start exactly on tempmin
        ptline.push_back(interpolateY(tempmin, pt0, pt1));
      ptline.push_back(pt1);
    }
    pt0 = pt1;
  }
  return ptline;
}

// TODO this is almost the same code as FieldCalculations showalterIndex
float iterateWetAdiabat(float pa, float ta_celsius, float pb)
{
  using namespace MetNo::Constants;

  const ewt_calculator ewt(ta_celsius);
  float esat = (ewt.defined()) ? ewt.value() : 0;
  float qsat = eps * esat / pa;

  const float pi1 = vcross::util::exnerFunction(pa);
  const float pi = vcross::util::exnerFunction(pb);

  float tcl = cp * (ta_celsius + t0) * pi / pi1;
  float qcl = qsat;

  // adjust humidity in some iterations
  for (int iteration = 0; iteration < 5; iteration++) {
    const float t = tcl / cp - t0;
    const ewt_calculator ewt(t);
    if (!ewt.defined())
      break;
    qsat = eps * ewt.value() / pb;
    float dq = qcl - qsat;
    float a1 = cplr * qcl / tcl;
    float a2 = exl / tcl;
    dq /= 1 + a1 * a2;
    qcl -= dq;
    tcl += dq * xlh;
  }
  // qcl = qsat;
  const float tb_celsius = tcl / cp - t0;
  return tb_celsius;
}

PointF_v generateWetAdiabat(float p0, float p1, float pstep, float t0 /* degC */, float tempmin)
{
  PointF_v wet_pt;

  PointF pta(p0, t0);
  p1 += pstep / 4; // try to fix rounding problems
  while ((pstep < 0 && pta.x() >= p1) || (pstep > 0 && pta.x() <= p1)) {
    wet_pt.push_back(pta);
    float pb = pta.x() + pstep;
    float tb = iterateWetAdiabat(pta.x(), pta.y(), pb);
    pta = PointF(pb, tb);
    if (pta.y() < tempmin) {
      if (wet_pt.back().y() > tempmin)
        // start exactly on tempmin
        wet_pt.push_back(interpolateY(tempmin, wet_pt.back(), pta));
      break;
    }
  }

  return wet_pt;
}

} // namespace

VprofDiagram::VprofDiagram(VprofOptions *vpop, DiGLPainter* GL)
  : gl(GL)
  , vpopt(vpop)
  , diagramInList(false)
  , drawlist(0)
  , numprog(0)
{
  METLIBS_LOG_SCOPE();

  setDefaults();
  plotw = ploth = 0;
  plotwDiagram = plothDiagram = -1;
}

VprofDiagram::~VprofDiagram()
{
  METLIBS_LOG_SCOPE();

  DiGLCanvas* canvas = gl->canvas();
  if (canvas && canvas->supportsDrawLists() && drawlist != 0 && canvas->IsList(drawlist))
    canvas->DeleteLists(drawlist, 1);
}

void VprofDiagram::changeNumber(int nprog)
{
  METLIBS_LOG_SCOPE(LOGVAL(nprog));

  if (nprog != numprog) {
    numprog = nprog;
    newdiagram = true;
  }
}

void VprofDiagram::setPlotWindow(const QSize& size)
{
  plotw = size.width();
  ploth = size.height();
}

void VprofDiagram::plot()
{
  METLIBS_LOG_SCOPE();

  vptext.clear();

  gl->clear(Colour(vpopt->backgroundColour));
  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_LINE);

  if (plotw < 5 || ploth < 5)
    return;

  if (vpopt->changed)
    newdiagram = true;

  bool redraw = newdiagram;
  if (!diagramInList)
    redraw = true;

  if (plotw != plotwDiagram || ploth != plothDiagram) {
    plotwDiagram = plotw;
    plothDiagram = ploth;
    redraw = true;
  }

  DiGLCanvas* canvas = gl->canvas();
  if (redraw) {
    if (canvas && canvas->supportsDrawLists() && diagramInList && canvas->IsList(drawlist))
      canvas->DeleteLists(drawlist, 1);
    diagramInList = false;
    if (newdiagram)
      prepare();
  }

  {
    Rectangle f(xysize[BOX_TOTAL][XMIN], xysize[BOX_TOTAL][YMIN], xysize[BOX_TOTAL][XMAX], xysize[BOX_TOTAL][YMAX]);
    const float margin = 0.02;
    diutil::adjustRectangle(f, margin * f.width(), margin * f.height());
    const bool fixed_aspect_ratio = true;
    if (fixed_aspect_ratio)
      diutil::fixAspectRatio(f, plotw / float(ploth), true);
    const float xscale = plotw / f.width(), yscale = ploth / f.height();
    // f.x1 => 0; f.x2 => plotw; px = (f.x - f.x1)/(f.x2-f.x1)*plotw
    full = Rectangle(-f.x1 * xscale, -f.y1 * yscale, xscale, yscale);

    gl->LoadIdentity();
    gl->Ortho(0, plotw, 0, ploth, -1, 1);
  }

  if (redraw) {
    // YE: Should we check for existing list ?!
    // Yes, I think so!

    if (canvas && canvas->supportsDrawLists()) {
      if (drawlist != 0 && canvas->IsList(drawlist))
        canvas->DeleteLists(drawlist, 1);
      drawlist = canvas->GenLists(1);
      if (drawlist != 0)
        gl->NewList(drawlist, DiGLPainter::gl_COMPILE_AND_EXECUTE);
      else
        METLIBS_LOG_WARN("Unable to create new displaylist, gl->GenLists(1) returns 0");
    }

    plotDiagram();

    if (drawlist != 0)
      gl->EndList();

    fpDrawStr(true);
    if (drawlist != 0)
      diagramInList = true;
  } else if (canvas && canvas->supportsDrawLists() && canvas->IsList(drawlist)) {
    gl->CallList(drawlist);
    fpDrawStr(false);
  } else {
    plotDiagram();
    fpDrawStr(false);
  }
}

void VprofDiagram::setDefaults()
{
  METLIBS_LOG_SCOPE();

  newdiagram = true;

  for (int n = 0; n < NBOXES; n++)
    for (int i = 0; i < NXYMINMAX; i++)
      xysize[n][i] = 0.;

  chxbas = -1.;
  chybas = -1.;

  init_cotrails = false;
}

void VprofDiagram::prepare()
{
  METLIBS_LOG_SCOPE();

  using vcross::util::minimize;
  using vcross::util::maximize;

  vpopt->checkValues();

  // cache for transformP
  y1000 = p_to_y(1000);
  idy_1000_100 = (vpopt->rsvaxis * height) / (y1000 - p_to_y(100));
  tan_tangle = (std::abs(vpopt->tangle) > 0.1) ? std::tan(vpopt->tangle * DEG_TO_RAD) : 0;

  {
    int pminDiagram = vpopt->pminDiagram;
    int pmaxDiagram = vpopt->pmaxDiagram;

    if (pmaxDiagram - pminDiagram < idptab * 2)
      pminDiagram = pmaxDiagram - idptab * 2;
    if (pminDiagram < 0)
      pminDiagram = 0;
    if (pmaxDiagram - pminDiagram < idptab * 2)
      pmaxDiagram = pminDiagram + idptab * 2;
    if (pmaxDiagram > idptab * (mptab - 1))
      pmaxDiagram = idptab * (mptab - 1);

    pmin = pminDiagram;
    pmax = pmaxDiagram;
  }

  const float tmind = vpopt->tminDiagram;
  const float tmaxd = vpopt->tmaxDiagram;

  // condensation trail lines (for 'kondensstriper' fra fly)
  if (vpopt->pcotrails && !init_cotrails) {
    condensationtrails();
    init_cotrails = true;
  }

  //---------------------------------------------------------------------
  // idtype:  vertikal-koordinat for diagrammet
  //            0 = amble (under 500 hPab: ln(p), over 500 hPa: p)
  //            1 = pi (exner-funksjonen)
  //            2 = p
  //            3 = ln(p)
  // tangle: vinkel mellom "vertikal" og linjer for temp.=konstant
  //         (0 - 80, vanlig: 45, amble skal ha 45 graders vinkel)
  //---------------------------------------------------------------------

  // temperature range:  0=fixed  1=fixed.max-min  2=minimum

  const float xmind = transformPT(1000, tmind).x();
  const float xmaxd = transformPT(1000, tmaxd).x();
  const float ymaxd = transformP(pmin);
  const float ymind = transformP(pmax);

  //>>>>>>>>> hva hvis hverken t eller td plottes ????????
  //>>>>>>>>> evt. ikke lovlig aa slaa av t ??? (piloter ?)
  //>>>> problem: p-t diagram in 'wd' coordinates
  //>>>>          p & t numbers should be r*chxdef in 'vp' coord.

  // character size (same "unit" as above)
  chxbas = 0.50;
  chybas = chxbas * 1.25; // ????????????????

  chxlab = chxbas * vpopt->rslabels;
  chylab = chybas * vpopt->rslabels;

  chxtxt = chxbas * vpopt->rstext;
  chytxt = chybas * vpopt->rstext;

  float xmindf = xmind;
  float xmaxdf = xmaxd;
  float ymindf = ymind;
  float ymaxdf = ymaxd;
  if (vpopt->plabelp && vpopt->pplines)
    xmindf -= chxlab * 5.5;
  if (vpopt->plabelt && vpopt->ptlines) {
    if (vpopt->tangle > 29. && vpopt->tangle < 61.) {
      float sintan = sinf(vpopt->tangle * DEG_TO_RAD);
      float costan = cosf(vpopt->tangle * DEG_TO_RAD);
      xmaxdf += (costan * chxlab * 4. + costan * chylab);
      ymaxdf += (sintan * chxlab * 4. + sintan * chylab);
    } else {
      xmaxdf += (chxlab * 4.);
      ymaxdf += (chylab * 1.4);
    }
  }
  if (vpopt->plabelq)
    ymindf -= (chylab * 0.8 * 1.5);

  xysize[BOX_PT_AXES][XMIN] = xmind;
  xysize[BOX_PT_AXES][XMAX] = xmaxd;
  xysize[BOX_PT_AXES][YMIN] = ymind;
  xysize[BOX_PT_AXES][YMAX] = ymaxd;
  xysize[BOX_PT][XMIN] = xmindf;
  xysize[BOX_PT][XMAX] = xmaxdf;
  xysize[BOX_PT][YMIN] = ymindf;
  xysize[BOX_PT][YMAX] = ymaxdf;

  // stuff around the p-t diagram

  float xmin = xmindf;
  float xmax = xmaxdf;
  float ymin = ymindf;
  float ymax = ymaxdf;
  for (int n = BOX_FL; n < NBOXES; n++) {
    xysize[n][XMIN] = +1.;
    xysize[n][XMAX] = -1.;
    xysize[n][YMIN] = ymin;
    xysize[n][YMAX] = ymax;
  }

  // space for flight levels (numbers and ticks on axis)
  if (vpopt->pflevels) {
    const float dx = chxlab * (vpopt->plabelflevels ? 5.5 : 1.5);
    xysize[BOX_FL][XMIN] = xmin - dx;
    xysize[BOX_FL][XMAX] = xmin;
    xmin -= dx;
  }

  // space for significant wind levels, temp (observation) and prognostic
  if (vpopt->pslwind) {
    float dx = chxlab * 6.5;
    xmin -= (dx * numprog);
    xysize[BOX_SIG_WIND][XMIN] = xmin;
    xysize[BOX_SIG_WIND][XMAX] = xmin + dx;
  }

  // space for wind, possibly in multiple (separate) columns
  if (vpopt->pwind) {
    float dx = chxbas * 8. * vpopt->rswind;
    xysize[BOX_WIND][XMIN] = xmax;
    xysize[BOX_WIND][XMAX] = xmax + dx;
    xysize[BOX_WIND][YMIN] = ymind - dx * 0.5;
    xysize[BOX_WIND][YMAX] = ymaxd + dx * 0.5;
    if (vpopt->windseparate)
      xmax += (dx * numprog);
    else
      xmax += dx;
  }

  // space for vertical wind (only for prognostic)
  if (vpopt->pvwind && numprog > 0) {
    float dx = chxbas * 6.5 * vpopt->rsvwind;
    xysize[BOX_VER_WIND][XMIN] = xmax;
    xysize[BOX_VER_WIND][XMAX] = xmax + dx;
    xysize[BOX_VER_WIND][YMIN] = ymind - chylab * 2.5;
    xysize[BOX_VER_WIND][YMAX] = ymaxd + chylab * 2.5;
    xmax += dx;
  }

  // space for relative humidity
  if (vpopt->prelhum) {
    float dx = chxbas * 7. * vpopt->rsrelhum;
    xysize[BOX_REL_HUM][XMIN] = xmax;
    xysize[BOX_REL_HUM][XMAX] = xmax + dx;
    xysize[BOX_REL_HUM][YMIN] = ymind - chylab * 1.5;
    xysize[BOX_REL_HUM][YMAX] = ymaxd + chylab * 1.5;
    xmax += dx;
  }

  // space for ducting
  if (vpopt->pducting) {
    float dx = chxbas * 7. * vpopt->rsducting;
    xysize[BOX_DUCTING][XMIN] = xmax;
    xysize[BOX_DUCTING][XMAX] = xmax + dx;
    xysize[BOX_DUCTING][YMIN] = ymind - chylab * 1.5;
    xysize[BOX_DUCTING][YMAX] = ymaxd + chylab * 1.5;
    xmax += dx;
  }

  // adjustments
  for (int n = BOX_FL; n <= BOX_DUCTING; n++) {
    minimize(ymin, xysize[n][YMIN]);
    maximize(ymax, xysize[n][YMAX]);
  }
  if (numprog > 1 && (vpopt->pwind || vpopt->pslwind)) {
    minimize(ymin, ymind - chylab * 2.);
  }
  if (vpopt->plabelp || (vpopt->pflevels && vpopt->plabelflevels)) {
    minimize(ymin, ymind - chylab * 2.);
  }
  if (vpopt->pslwind) {
    minimize(ymin, ymind - chylab * 0.85);
    maximize(ymax, ymaxd + chylab * 0.85);
  }
  for (int n = BOX_FL; n <= BOX_DUCTING; n++) {
    xysize[n][YMIN] = ymin;
    xysize[n][YMAX] = ymax;
  }

  // space for text
  if (vpopt->ptext) {
    const float ytext = ymin;
    const int n = std::max(1, std::max(numprog, vpopt->linetext));
    ymin -= (chytxt * (1.5 * n + 0.5));
    xysize[BOX_TEXT][XMIN] = xmin;
    xysize[BOX_TEXT][XMAX] = xmax;
    xysize[BOX_TEXT][YMIN] = ymin;
    xysize[BOX_TEXT][YMAX] = ytext;
  } else {
    xysize[BOX_TEXT][YMIN] = ymin;
    xysize[BOX_TEXT][YMAX] = ymin;
  }

  // total area
  xysize[BOX_TOTAL][XMIN] = xmin;
  xysize[BOX_TOTAL][XMAX] = xmax;
  xysize[BOX_TOTAL][YMIN] = ymin;
  xysize[BOX_TOTAL][YMAX] = ymax;

  vpopt->changed = false;
  newdiagram = false;
}

void VprofDiagram::condensationtrails()
{
  METLIBS_LOG_SCOPE();
  //
  // compute lines for evaluation of possibility for
  // condensation trails (kondensstriper fra fly)
  //
  // Documentation:
  //   DNMI Technical Report No. 2, 'Varsling av kondensstriper'
  //   av Eigil Hesstvedt, 1959.
  //
  //....................................................................
  //
  // lines according to tables for evaluation of condensation trails
  // (kondensstriper fra fly, iflg:
  //  DNMI Technical Report No. 2, 'Varsling av kondensstriper'
  //  av Eigil Hesstvedt, 1959.
  //  Benytter kolonne 'C' i tabellen, som anbefalt.)
  //
  //....................................................................
  //..condensation trail lines
  //cc   parameter (npctab=15)
  //cc   integer ipctab(npctab)
  //cc   real    crtab(npctab,4), crtab2(npctab,4),crtab3(npctab,4)
  //c..p
  //ccc   data ipctab/ 100,  125,  150,  175,  200,  250,  300,  350,
  //ccc  +             400,  500,  600,  700,  800,  900, 1000/
  //c..t(ri=0%)
  //ccc   data crtab/-60.9,-59.0,-57.4,-56.0,-54.8,-52.7,-51.1,-49.6,
  //ccc  +           -48.3,-46.1,-44.3,-42.7,-41.3,-40.0,-38.9,
  //c..t(ri=40%)
  //ccc  +           -60.1,-58.3,-56.6,-55.2,-54.0,-51.9,-50.2,-48.5,
  //ccc  +           -47.3,-45.1,-43.2,-41.6,-40.1,-38.8,-37.7,
  //c..t(ri=100%)
  //ccc  +           -58.7,-56.8,-54.9,-53.5,-52.2,-50.0,-48.1,-46.3,
  //ccc  +           -45.2,-42.7,-40.8,-39.1,-37.6,-36.2,-34.9,
  //c..t(rw=100%)
  //ccc  +           -52.5,-50.4,-48.6,-47.1,-45.7,-43.5,-41.6,-39.9,
  //ccc  +           -38.5,-36.1,-34.1,-32.3,-30.8,-29.4,-28.1/
  //....................................................................
  //
  // below computing the lines at many more levels

  //this constant is found in one the figures in the report
  const float dqdt = 3.56e-5;

  const float tr = 273.15;
  const float ewtr = 6.11;
  const float eitr = 6.11;
  const float xlvr = 2.501e+6;
  const float xlsr = 2.835e+6;
  const float cpv = 1850.;
  const float cw = 4220.;
  const float ci = 2100.;
  const float ra = 287.;
  const float rv = 461.;
  const float eps = ra / rv;

  const float clw = cpv - cw;
  const float cw1 = (cpv - cw) / rv;
  const float cw2 = xlvr / (rv * tr);
  const float cli = cpv - ci;
  const float ci1 = (cpv - ci) / rv;
  const float ci2 = xlsr / (rv * tr);

  int k, n, it, it1, itb;
  float p, st, q1, t1, q2, t2, q3, t3, tc, t, xlv, ew, dqdt1, dqdt2;
  float trw100, qrw100, trw000, tri040, tri100, trixxx;
  float rh, tx1, tx2, xls, dq, ei;
  bool end;

  int itbegin = -90;

  for (k = 1; k < mptab; k++) {

    p = idptab * k;

    it = itbegin - 1;
    // t(rw=100%)
    for (n = 0; n < 3; n++) {
      st = powf(10., float(-n));
      q1 = t1 = q2 = t2 = 0.;
      it1 = it + 1;
      end = false;
      while (!end) {
        it++;
        tc = it * st;
        t = tc + tr;
        xlv = xlvr + clw * tc;
        ew = ewtr * powf((t / tr), cw1) * expf(-xlv / (rv * t) + cw2);
        q3 = q2;
        t3 = t2;
        q2 = q1;
        t2 = t1;
        t1 = tc;
        q1 = eps * ew / p;
        if (it > it1 + 1) {
          dqdt1 = (q1 - q2) / (t1 - t2);
          dqdt2 = (q2 - q3) / (t2 - t3);
          end = (dqdt1 >= dqdt && dqdt2 <= dqdt);
        }
      }
      if (n == 0)
        itbegin = it - 2;
      it = (it - 1) * 10 - 12;
    }

    trw100 = t2;
    qrw100 = q2;
    trw000 = trw100 - qrw100 / dqdt;
    tri040 = +1.e+35;
    tri100 = +1.e+35;

    itb = int(trw000 - 3.);

    for (int irh = 40; irh <= 100; irh += 60) {
      rh = irh * 0.01;
      it = itb;
      for (n = 0; n < 3; n++) {
        st = powf(10., float(-n));
        t1 = tx1 = 0.;
        it1 = it + 1;
        end = false;
        while (!end) {
          it++;
          tc = it * st;
          t = tc + tr;
          xls = xlsr + cli * tc;
          ei = eitr * powf((t / tr), ci1) * expf(-xls / (rv * t) + ci2);
          t2 = t1;
          tx2 = tx1;
          t1 = tc;
          q1 = rh * eps * ei / p;
          dq = qrw100 - q1;
          tx1 = tc + dq / dqdt;
          end = (it > it1 && tx1 > trw100);
        }
        it = (it - 1) * 10 - 2;
      }
      trixxx = t2 + (t1 - t2) * (trw100 - tx2) / (tx1 - tx2);
      if (irh == 40)
        tri040 = trixxx;
      else
        tri100 = trixxx;
    }

    cotrails[0][k] = trw000;
    cotrails[1][k] = tri040;
    cotrails[2][k] = tri100;
    cotrails[3][k] = trw100;

  }

  // extrapolation to "impossible" top
  for (n = 0; n < 4; n++)
    cotrails[n][0] = 2. * cotrails[n][1] - cotrails[n][2];
}

void VprofDiagram::drawLine(const diutil::PointF& p1, const diutil::PointF& p2)
{
  METLIBS_LOG_SCOPE();
  float x1, y1, x2, y2;
  to_pixel(p1).unpack(x1, y1);
  to_pixel(p2).unpack(x2, y2);
  METLIBS_LOG_DEBUG(LOGVAL(full) << LOGVAL(p1) << LOGVAL(p2) << LOGVAL(x1) << LOGVAL(y1) << LOGVAL(x2) << LOGVAL(y2));
  gl->drawLine(x1, y1, x2, y2);
}

void VprofDiagram::drawRect(bool fill, float x1, float y1, float x2, float y2)
{
  to_pixel(PointF(x1, y1)).unpack(x1, y1);
  to_pixel(PointF(x2, y2)).unpack(x2, y2);
  gl->drawRect(fill, x1, y1, x2, y2);
}

void VprofDiagram::drawText(const std::string& text, float x, float y, float angle)
{
  to_pixel(PointF(x, y)).unpack(x, y);
  gl->drawText(text, x, y, angle);
}

void VprofDiagram::plotDiagram()
{
  METLIBS_LOG_SCOPE();

  const float xmind = xysize[BOX_PT_AXES][XMIN];
  const float xmaxd = xysize[BOX_PT_AXES][XMAX];
  const float ymind = xysize[BOX_PT_AXES][YMIN];
  const float ymaxd = xysize[BOX_PT_AXES][YMAX];
  // float xmindf= xysize[BOX_PT_LABELS][XMIN];
  // float xmaxdf= xysize[BOX_PT_LABELS][XMAX];
  const float ymindf = xysize[BOX_PT][YMIN];
  const float ymaxdf = xysize[BOX_PT][YMAX];

  // min,max temperature on diagram
  const float tmin = t_from_xp(xmind, pmin);
  const float tmax = t_from_xp(xmaxd, pmax);
  const int itmin = std::ceil(tmin);
  const int itmax = std::floor(tmax);
  int itmin1000 = std::trunc(t_from_xp(xmind, 1000));

  fpStr.clear();

  // thick lines not changed by dialogs...
  vpopt->pLinewidth2 = vpopt->pLinewidth1 + 2.;
  vpopt->tLinewidth2 = vpopt->tLinewidth1 + 2.;
  vpopt->flevelsLinewidth2 = vpopt->flevelsLinewidth1 + 2.;

  // frame around the 'real' p-t diagram
  if (vpopt->pframe) {
    gl->setLineStyle(Colour(vpopt->frameColour), vpopt->frameLinewidth,
        Linetype(vpopt->frameLinetype));
    drawRect(false, xmind, ymind, xmaxd, ymaxd);
    gl->Disable(DiGLPainter::gl_LINE_STIPPLE);
  }

  // pressure lines (possibly at flight levels)
  if (vpopt->pplines) {
    int kk;
    vector<float> plev, ylev;
    vector<bool> hlev;
    if (vpopt->pplinesfl) {
      // flight levels
      kk = vpopt->pflightlevels.size();
      plev = vpopt->pflightlevels;
      for (int k = 0; k < kk; k++)
        hlev.push_back(vpopt->flightlevels[k] % 50 == 0);
    } else {
      // pressure levels
      kk = vpopt->plevels.size();
      plev = vpopt->plevels;
      for (int k = 0; k < kk; k++) {
        int ip = std::lround(vpopt->plevels[k]);
        hlev.push_back(ip == 1000 || ip == 500);
      }
    }
    for (int k = 0; k < kk; k++) {
      ylev.push_back(transformP(plev[k]));
    }
    const Colour c(vpopt->pColour);
    gl->setLineStyle(c, vpopt->pLinewidth1,
        Linetype(vpopt->pLinetype));
    for (int k = 0; k < kk; k++) {
      if (ylev[k] > ymind && ylev[k] < ymaxd) {
        const int lw = !hlev[k] ? vpopt->pLinewidth1 : vpopt->pLinewidth2;
        gl->LineWidth(lw);
        drawLine(xmind, ylev[k], xmaxd, ylev[k]);
      }
    }
    gl->Disable(DiGLPainter::gl_LINE_STIPPLE);
    // labels (text/numbers)
    if (vpopt->plabelp) {
      const float dy = chylab * 1.2;
      TextSpacing ts(ymindf + dy * 0.5, ymaxdf - dy * 0.5, dy);
      float x1 = xmind - chxlab * 0.75;
      if (vpopt->pplinesfl) {
        // flight levels
        for (int k = 0; k < kk; k++) {
          if (hlev[k] && ts.accept(ylev[k])) {
            ostringstream ostr;
            ostr << setw(3) << setfill('0') << vpopt->flightlevels[k];
            fpInitStr(ostr.str(), x1, ylev[k] - chylab * 0.5, 0.0, chylab, c, ALIGN_RIGHT);
          }
        }
        fpInitStr("FL", x1 - 1, ymind - chylab * 1.25, 0.0, chylab, c);
      } else {
        // pressure levels
        int ipstep;
        if (pmax - pmin > 399.)
          ipstep = 100;
        else if (pmax - pmin > 199.)
          ipstep = 50;
        else if (pmax - pmin > 99.)
          ipstep = 25;
        else
          ipstep = 10;
        for (int k = 0; k < kk; k++) {
          const int ip = std::lround(vpopt->plevels[k]);
          if (ip % ipstep == 0 && ts.accept(ylev[k])) {
            ostringstream ostr;
            ostr << setw(4) << setfill(' ') << ip;
            fpInitStr(ostr.str(), x1, ylev[k] - chylab * 0.5, 0.0, chylab, c, ALIGN_RIGHT);
          }
        }
        fpInitStr("hPa", x1, ymind - chylab * 1.25, 0.0, chylab, c, ALIGN_RIGHT);
      }
    }
  }

  // temperature lines
  if (vpopt->ptlines) {
    int tStep = vpopt->tStep;
    if (tStep < 1)
      tStep = 5;
    int it1 = (itmin / tStep) * tStep;
    int it2 = (itmax / tStep) * tStep;
    if (it1 < itmin)
      it1 += tStep;
    if (it2 > itmax)
      it2 -= tStep;
    const Colour c(vpopt->tColour);
    gl->setLineStyle(c, vpopt->tLinewidth1,
        Linetype(vpopt->tLinetype));
    for (int it = it1; it <= it2; it += tStep) {
      PointF xy1 = transformPT(pmax, it);
      PointF xy2 = transformPT(pmin, it);
      if (xy1.x() < xmind)
        xy1 = interpolateX(xmind, xy1, xy2);
      if (xy2.x() > xmaxd)
        xy2 = interpolateX(xmaxd, xy1, xy2);
      const int lw = (it % 40 != 0) ? vpopt->tLinewidth1 : vpopt->tLinewidth2;
      gl->LineWidth(lw);
      drawLine(xy1.x(), xy1.y(), xy2.x(), xy2.y());
    }
    gl->Disable(DiGLPainter::gl_LINE_STIPPLE);
    // t numbers (labels)
    if (vpopt->plabelt) {
      float sintan = sinf(vpopt->tangle * DEG_TO_RAD);
      float costan = cosf(vpopt->tangle * DEG_TO_RAD);
      int itstep = std::max((tmax - tmin < 75) ? 5 : 10, vpopt->tStep);
      int itlim = std::floor(t_from_xp(xmaxd, pmin));
      // t numbers at the diagram top
      int it1 = (itmin / itstep) * itstep;
      int it2 = (itlim / itstep) * itstep;
      if (itmin > 0)
        it1 += itstep;
      if (itlim < 0)
        it2 -= itstep;
      float chy = chylab;
      int numwid = 3;
      if (it1 < -100 || it2 > +100) {
        chy *= 0.75;
        numwid = 4;
      }
      float numrot, dxmin, dx, dy, dx1, dx2;
      if (vpopt->tangle > 29. && vpopt->tangle < 61.) {
        numrot = atan2f(costan, sintan) / DEG_TO_RAD;
        dxmin = 1.3 * chy / costan;
        dx = chy * 0.5 / costan;
        dy = 0.;
        dx1 = chylab * costan;
        dx2 = chylab * 3. * sintan;
      } else {
        numrot = 0.;
        dxmin = chxlab * 4.;
        dx = -1.5 * chxlab;
        dy = 0.2 * chylab;
        dx1 = 1.5 * chxlab;
        dx2 = 1.5 * chxlab;
      }
      //       setFontsize(chy);
      TextSpacing tsx(xmind + dx1, 1e10, dxmin);
      float y = transformP(pmin) + dy;
      for (int it = it1; it <= it2; it += itstep) {
        const float x = transformPT(pmin, it).x() + dx;
        if (tsx.accept(x)) {
          ostringstream ostr;
          ostr << setw(numwid) << setfill(' ') << setiosflags(ios::showpos) << it;
          fpInitStr(ostr.str(), x, y, numrot, chy, c, ALIGN_LEFT, FONT_SCALED);
        }
      }
      // t numbers at the diagram right side
      it1 = it2 + itstep;
      it2 = (itmax / itstep) * itstep;
      if (itmax < 0)
        it2 -= itstep;
      const float xlast = tsx.next - dxmin + dx2;
      float ynext, ylast, dymin;
      if (vpopt->tangle > 29. && vpopt->tangle < 61.) {
        dymin = 1.3 * chy / sintan;
        dx = costan * chy;
        dy = -sintan * chy + 0.5 * chy / sintan;
        ynext = ymaxdf;
        ylast = ymindf + chy * costan - 0.5 * chy / sintan;
      } else {
        dymin = chy * 1.3;
        dx = 0.5 * chxlab;
        dy = 0.5 * chy;
        if (xlast < xmaxd)
          ynext = ymaxdf;
        else
          ynext = ymaxd - chy * 0.5;
        ylast = ymindf + chy * 0.5;
      }
      TextSpacing tsy(ynext, ylast, -dymin);
      float t1 = t_from_xp(xmaxd, pmax);
      float t2 = t_from_xp(xmaxd, pmin);
      float y1 = transformP(pmax);
      float y2 = transformP(pmin);
      const float x = xmaxd + dx;
      //       setFontsize(chy);
      for (int it = it1; it <= it2; it += itstep) {
        const float y = y1 + (y2 - y1) * (it - t1) / (t2 - t1) + dy;
        if (tsy.accept(y)) {
          ostringstream ostr;
          ostr << setw(numwid) << setfill(' ') << setiosflags(ios::showpos) << it;
          fpInitStr(ostr.str(), x, y, numrot, chy, c, ALIGN_LEFT, FONT_SCALED);
        }
      }
    }
  }

  // dry adiabats (always base at 0 degrees celsius, 1000 hPa)
  // dry adiabats: potential temperature (th) constant,
  // cp*t=th*pi, pi=cp and th=t at 1000 hPa,
  // (above 100 hPa the 5 hPa step really is too much)
  if (vpopt->pdryadiabat) {
    gl->setLineStyle(Colour(vpopt->dryadiabatColour), vpopt->dryadiabatLinewidth,
        Linetype(vpopt->dryadiabatLinetype));

    int itstep = vpopt->dryadiabatStep;
    if (itstep < 1)
      itstep = 5;
    // find first and last dry adiabat to be drawn (at 1000 hPa, pi=cp)
    const float tmax = t_from_xp(xmind, pmax);
    const float tmin = t_from_xp(xmaxd, pmin);
    using namespace MetNo::Constants;
    const float thmax = cp * (tmax + t0) / vcross::util::exnerFunction(pmax);
    const float thmin = cp * (tmin + t0) / vcross::util::exnerFunction(pmin);
    const int itt1 = int(thmax - t0);
    const int itt2 = int(thmin - t0);
    int it1 = (itt1 / itstep) * itstep;
    int it2 = (itt2 / itstep) * itstep;
    if (itt1 > 0)
      it1 += itstep;
    if (itt2 < 0)
      it2 -= itstep;

    for (int it = it1; it <= it2; it += itstep) {
      float th = t0 + it;
      float p1 = pmin;
      if (p1 < 25 && it % (itstep * 8) != 0)
        p1 = 25;
      if (p1 < 50 && it % (itstep * 4) != 0)
        p1 = 50;
      if (p1 < 100 && it % (itstep * 2) != 0)
        p1 = 100;

      PointF_v dry_pt;
      for (float p = p1; p <= pmax + idptab / 4; p += idptab) {
        float t = (th / cp) * vcross::util::exnerFunction(p) - t0;
        dry_pt.push_back(PointF(p, t));
      }
      drawPT(dry_pt);
    }
    gl->Disable(DiGLPainter::gl_LINE_STIPPLE);
  }

  // wet adiabats (always base at 0 degrees celsius, 1000 hPa)
  if (vpopt->pwetadiabat) {
    const int wpmin = std::max(float(vpopt->wetadiabatPmin), pmin);
    if (wpmin < pmax) {
      gl->setLineStyle(Colour(vpopt->wetadiabatColour), vpopt->wetadiabatLinewidth, Linetype(vpopt->wetadiabatLinetype));

      const float tempmin = vpopt->wetadiabatTmin;
      const int mintemp = (tempmin > itmin1000) ? tempmin : itmin1000;
      const int itstep = vpopt->wetadiabatStep >= 1 ? vpopt->wetadiabatStep : 10;
      const int it1 = (mintemp / itstep) * itstep + ((mintemp > 0) ? itstep : 0);
      const int it2 = (itmax / itstep) * itstep - ((itmax < 0) ? itstep : 0);
      for (int t = it1; t <= it2; t += itstep) {
        PointF_v wet_pt;
        const float porigin = 1000;
        if (pmax > porigin) {
          // near ground, increase pressure from 1000
          PointF_v pt = generateWetAdiabat(porigin, pmax, idptab, t, tempmin /* probably irrelevant here */);
          // insert at front, reverse direction, skipping 1000hPa point
          wet_pt.insert(wet_pt.begin(), ++pt.rbegin(), pt.rend());
        }

        PointF_v pt = generateWetAdiabat(porigin, wpmin, -idptab, t, tempmin);
        // insert at end, keeping direction, including 1000hPa point
        wet_pt.insert(wet_pt.end(), pt.begin(), pt.end());

        drawPT(wet_pt);
      }
      gl->Disable(DiGLPainter::gl_LINE_STIPPLE);
    }
  }

  // mixing ratio (lines for constant mixing ratio) (humidity)
  if (vpopt->pmixingratio) {
    const float qpmin = std::max(float(vpopt->mixingratioPmin), pmin);
    if (qpmin < pmax) {
      const Colour c(vpopt->mixingratioColour);
      gl->setLineStyle(c, vpopt->mixingratioLinewidth, Linetype(vpopt->mixingratioLinetype));

      // plot labels below the p-t diagram
      const float xspacing = 2 * chxlab * 0.8;
      const float chy = chylab * 0.8;
      const float y = ymind - chy * 1.25;
      TextSpacing ts(xmind + xspacing, xmaxd - xspacing, 2 * xspacing);

      int set = vpopt->mixingratioSet;
      if (set < 0 || set >= int(vpopt->qtable.size()))
        set = 1;
      for (float qsat : vpopt->qtable[set]) {
        const float qsat_u = qsat * 0.001; // convert unit to g/kg
        const PointF_v qpt = mixingRatioLine(qsat_u, qpmin, pmax, idptab, vpopt->mixingratioTmin);
        if (qpt.size() < 2)
          continue;

        drawPT(qpt);

        if (vpopt->plabelq) {
          // last point is for pmax, which is where the label should be
          const float xlabel = transformPT(qpt.back().x(), qpt.back().y()).x();
          if (ts.accept(xlabel)) {
            ostringstream ostr;
            ostr << qsat;
            fpInitStr(ostr.str(), xlabel, y, 0.0, chy, c);
          }
        }
      }
      gl->Disable(DiGLPainter::gl_LINE_STIPPLE);
    }
  }

  // condensation trail lines (for kondensstriper fra fly)
  if (vpopt->pcotrails) {
    const Colour c(vpopt->cotrailsColour);
    gl->setLineStyle(c, vpopt->cotrailsLinewidth,
        Linetype(vpopt->cotrailsLinetype));
    const int kmin = std::max(float(vpopt->cotrailsPmin), pmin) / idptab;
    const int kmax = (std::min(float(vpopt->cotrailsPmax), pmax) + idptab - 1) / idptab;
    std::vector<PointF> xyline;
    xyline.reserve(std::max(kmax - kmin, 0));
    for (int n = 0; n < 4; n++) {
      for (int k = kmin; k <= kmax; k++) {
        xyline.push_back(PointF(k * idptab, cotrails[n][k]));
      }
      drawPT(xyline);
      xyline.clear();
    }
    gl->Disable(DiGLPainter::gl_LINE_STIPPLE);
  }

  // plot stuff around the p-t diagram..................................

  // flight levels (numbers and ticks)
  if (vpopt->pflevels) {
    const float chx = chxlab;
    const float chy = chylab;
    const float x2 = xysize[BOX_FL][XMAX];
    const float y1 = xysize[BOX_FL][YMIN];
    const float y2 = xysize[BOX_FL][YMAX];
    TextSpacing ts(y1 + chy * 0.6, y2 - chy * 0.6, chy * 1.2);
    const float x1l = xysize[BOX_FL][XMIN] + chx * 0.5;

    const Colour c(vpopt->flevelsColour);
    gl->setLineStyle(c, vpopt->flevelsLinewidth1);
    // flevelsLinetype....
    const size_t kk = vpopt->pflightlevels.size();
    for (size_t k = 0; k < kk; k++) {
      const float y = transformP(vpopt->pflightlevels[k]);
      const bool level50 = (vpopt->flightlevels[k] % 50 == 0);
      if (y > y1 && y < y2) {
        const int lw = !level50 ? vpopt->flevelsLinewidth1 : vpopt->flevelsLinewidth2;
        gl->LineWidth(lw);
        drawLine(x2 - chx * (!level50 ? 0.85 : 1.5), y, x2, y);
      }
      if (vpopt->plabelflevels && level50) {
        if (ts.accept(y)) {
          ostringstream ostr;
          ostr << setw(3) << setfill('0') << vpopt->flightlevels[k];
          fpInitStr(ostr.str(), x1l, y - chy * 0.5, 0., chy, c);
        }
      }
    }
    if (vpopt->plabelflevels)
      fpInitStr("FL", x1l, ymind - chy * 1.25, 0., chy, c);
  }

  Colour c(vpopt->frameColour);
  Colour c2("red");
  if (c2 == c)
    c2 = Colour("green");
  gl->setLineStyle(c, vpopt->frameLinewidth,
        Linetype(vpopt->frameLinetype));

  float y1 = xysize[BOX_TEXT][YMAX];
  float y2 = xysize[BOX_TOTAL][YMAX];

  // misc vertical line
  if (vpopt->plabelp) {
    if (vpopt->pflevels || vpopt->pslwind) {
      float x1 = xysize[BOX_PT][XMIN];
      drawLine(x1, y1, x1, y2);
    }
  }
  if (vpopt->pflevels && vpopt->plabelflevels && vpopt->pslwind) {
    float x1 = xysize[BOX_FL][XMIN];
    drawLine(x1, y1, x1, y2);
  }
  if (vpopt->pslwind && numprog > 1) {
    float dx = xysize[BOX_SIG_WIND][XMAX] - xysize[BOX_SIG_WIND][XMIN];
    float x1 = xysize[BOX_SIG_WIND][XMIN];
    for (int n = 1; n < numprog; n++) {
      x1 += dx;
      drawLine(x1, y1, x1, y2);
    }
  }
  if (vpopt->plabelt) {
    if (vpopt->pwind || (vpopt->pvwind && numprog > 0) || vpopt->prelhum) {
      float x1 = xysize[BOX_PT][XMAX];
      drawLine(x1, y1, x1, y2);
    }
  }
  if (vpopt->pwind && vpopt->windseparate && numprog > 1) {
    float dx = xysize[BOX_WIND][XMAX] - xysize[BOX_WIND][XMIN];
    float x1 = xysize[BOX_WIND][XMIN];
    for (int n = 1; n < numprog; n++) {
      x1 += dx;
      drawLine(x1, y1, x1, y2);
    }
  }
  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);

  bool sepline = vpopt->pwind;

  // vertical wind (up/down arrows showing air motion)
  if (vpopt->pvwind && numprog > 0) {
    if (sepline)
      drawLine(xysize[BOX_VER_WIND][XMIN], xysize[BOX_TEXT][YMAX], xysize[BOX_VER_WIND][XMIN], xysize[BOX_TOTAL][YMAX]);
    drawLine(xysize[BOX_VER_WIND][XMIN], ymaxd, xysize[BOX_VER_WIND][XMAX], ymaxd);
    drawLine(xysize[BOX_VER_WIND][XMIN], ymind, xysize[BOX_VER_WIND][XMAX], ymind);
    gl->setLineStyle(c, vpopt->rangeLinewidth, Linetype(vpopt->rangeLinetype));
    float x1 = (xysize[BOX_VER_WIND][XMIN] + xysize[BOX_VER_WIND][XMAX]) * 0.5;
    drawLine(x1, ymind, x1, ymaxd);
    gl->Disable(DiGLPainter::gl_LINE_STIPPLE);
    gl->LineWidth(vpopt->frameLinewidth);
    // the arrows
    // (assuming that concave polygons doesn't work properly)
    gl->ShadeModel(DiGLPainter::gl_FLAT);
    gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
    gl->setColour(c2, false);
    float dx = std::min((xysize[BOX_VER_WIND][XMAX] - xysize[BOX_VER_WIND][XMIN]) * 0.4, chxlab * 0.8);
    float dy = std::min((xysize[BOX_VER_WIND][YMAX] - ymaxd) * 0.5, chylab * 0.9);
    float yc = (ymaxd + xysize[BOX_VER_WIND][YMAX]) * 0.5;

    float xa1, ya1, xa2, ya2;
    // down arrow (sinking motion, omega>0 !)
    float xc = (xysize[BOX_VER_WIND][XMIN] + x1) * 0.5;
    to_pixel(xc, yc + dy).unpack(xa1, ya1);
    to_pixel(xc, yc - dy).unpack(xa2, ya2);
    gl->drawArrow(xa1, ya1, xa2, ya2);
    // up arrow (raising motion, omega<0 !)
    xc = (xysize[BOX_VER_WIND][XMAX] + x1) * 0.5;
    to_pixel(xc, yc - dy).unpack(xa1, ya1);
    to_pixel(xc, yc + dy).unpack(xa2, ya2);
    gl->drawArrow(xa1, ya1, xa2, ya2);

    // number showing the x-axis range of omega (+ hpa/s -)
    ostringstream ostr;
    ostr << vpopt->rvwind;
    std::string str = ostr.str();
    int k = str.length();
    dx = xysize[BOX_VER_WIND][XMAX] - xysize[BOX_VER_WIND][XMIN];
    float chx = chxlab;
    float chy = chylab;
    if (chx * (k + 4) > dx) {
      chx = dx / (k + 4);
      chy = chx * chylab / chxlab;
    }
    fpInitStr("+", xysize[BOX_VER_WIND][XMIN] + chx * 0.3, ymind - chy * 1.2, 0., chy, c2);
    fpInitStr("-", xysize[BOX_VER_WIND][XMAX] - chx * 1.3, ymind - chy * 1.2, 0., chy, c2);
    fpInitStr(str, xysize[BOX_VER_WIND][XMIN] + dx * 0.5, ymind - chy * 1.2, 0., chy, c2, ALIGN_CENTER);
    fpInitStr("hPa/s", xysize[BOX_VER_WIND][XMIN] + dx * 0.5, ymind - chy * 2.4, 0., chy, c2, ALIGN_CENTER);
    gl->setColour(c, false);
    sepline = true;
  }

  // relative humidity
  if (vpopt->prelhum) {
    if (sepline)
      drawLine(xysize[BOX_REL_HUM][XMIN], xysize[BOX_TEXT][YMAX], xysize[BOX_REL_HUM][XMIN], xysize[BOX_TOTAL][YMAX]);
    drawLine(xysize[BOX_REL_HUM][XMIN], ymaxd, xysize[BOX_REL_HUM][XMAX], ymaxd);
    drawLine(xysize[BOX_REL_HUM][XMIN], ymind, xysize[BOX_REL_HUM][XMAX], ymind);
    gl->setLineStyle(c, vpopt->rangeLinewidth, Linetype(vpopt->rangeLinetype));
    float dx = xysize[BOX_REL_HUM][XMAX] - xysize[BOX_REL_HUM][XMIN];
    for (int n = 25; n <= 75; n += 25) {
      float x1 = xysize[BOX_REL_HUM][XMIN] + dx * 0.01 * n;
      drawLine(x1, ymind, x1, ymaxd);
    }
    gl->Disable(DiGLPainter::gl_LINE_STIPPLE);
    // frameLinetype...
    gl->LineWidth(vpopt->frameLinewidth);
    float chx = chxlab;
    float chy = chylab;
    if (chx * 3. > dx) {
      chx = dx / 3.;
      chy = chx * chylab / chxlab;
    }
    float x = (xysize[BOX_REL_HUM][XMIN] + xysize[BOX_REL_HUM][XMAX]) * 0.5;
    fpInitStr("RH", x - chx, ymaxd + 0.25 * chy, 0.0, chy, c);
    if (chx * 6. > dx) {
      chx = dx / 6.;
      chy = chx * chylab / chxlab;
    }
    fpInitStr("0", xysize[BOX_REL_HUM][XMIN] + chx * 0.3, ymind - 1.25 * chy, 0.0, chy, c2);
    fpInitStr("100", xysize[BOX_REL_HUM][XMAX] - chx * 3.3, ymind - 1.25 * chy, 0.0, chy, c2);
    sepline = true;
  }

  // ducting
  if (vpopt->pducting) {
    if (sepline)
      drawLine(xysize[BOX_DUCTING][XMIN], xysize[BOX_TEXT][YMAX], xysize[BOX_DUCTING][XMIN], xysize[BOX_TOTAL][YMAX]);
    drawLine(xysize[BOX_DUCTING][XMIN], ymaxd, xysize[BOX_DUCTING][XMAX], ymaxd);
    drawLine(xysize[BOX_DUCTING][XMIN], ymind, xysize[BOX_DUCTING][XMAX], ymind);
    gl->setLineStyle(c, vpopt->rangeLinewidth, Linetype(vpopt->rangeLinetype));
    float dx = xysize[BOX_DUCTING][XMAX] - xysize[BOX_DUCTING][XMIN];
    float rd = vpopt->ductingMax - vpopt->ductingMin;
    int istep = 50;
    if (rd > 290.)
      istep = 100;
    if (rd > 490.)
      istep = 200;
    for (int n = -800; n <= 800; n += istep) {
      float x1 = xysize[BOX_DUCTING][XMAX] + float(n) * dx / rd;
      if (x1 > xysize[BOX_DUCTING][XMIN] && x1 < xysize[BOX_DUCTING][XMAX]) {
        drawLine(x1, ymind, x1, ymaxd);
      }
    }
    gl->Disable(DiGLPainter::gl_LINE_STIPPLE);
    // frameLinetype...
    gl->LineWidth(vpopt->frameLinewidth);
    float chx = chxlab;
    float chy = chylab;
    if (chx * 5. > dx) {
      chx = dx / 5.;
      chy = chx * chylab / chxlab;
    }
    float x = (xysize[BOX_DUCTING][XMIN] + xysize[BOX_DUCTING][XMAX]) * 0.5;
    fpInitStr("DUCT", x - chx * 2., ymaxd + 0.25 * chy, 0.0, chy, c);
    std::string str1 = miutil::from_number(vpopt->ductingMin);
    std::string str3 = miutil::from_number(vpopt->ductingMax);
    float ch = chy;
    fpInitStr(str1, xysize[BOX_DUCTING][XMIN], ymind - 1.25 * chy, 0.0, ch, c2, ALIGN_DUCTINGMIN);
    fpInitStr(str3, xysize[BOX_DUCTING][XMAX], ymind - 1.25 * chy, 0.0, ch, c2, ALIGN_DUCTINGMAX);
    sepline = true;
  }

  // line between diagram and text
  if (vpopt->ptext) {
    drawLine(xysize[BOX_TOTAL][XMIN], xysize[BOX_TEXT][YMAX], xysize[BOX_TOTAL][XMAX], xysize[BOX_TEXT][YMAX]);
  }

  // outer frame
  if (vpopt->pframe) {
    drawRect(false, xysize[BOX_TOTAL][XMIN], xysize[BOX_TOTAL][YMIN], xysize[BOX_TOTAL][XMAX], xysize[BOX_TOTAL][YMAX]);
  }
}

void VprofDiagram::fpInitStr(const std::string& str, float x, float y, float angle, float size, const Colour& c, Alignment format, Font font)
{
  METLIBS_LOG_SCOPE();

  fpStrInfo strInfo;
  strInfo.str = str;
  strInfo.x = x;
  strInfo.y = y;
  strInfo.angle = angle;
  strInfo.c = c;
  strInfo.size = size;
  strInfo.format = format;
  strInfo.font = font;
  fpStr.push_back(strInfo);
}

void VprofDiagram::fpDrawStr(bool first)
{
  METLIBS_LOG_SCOPE();
  gl->setFont(diutil::SCALEFONT);

  for (fpStrInfo& s : fpStr) {
    setFontsize(s.size);
    gl->setColour(s.c, false);
    if (first && (s.format != ALIGN_LEFT)) {
      const float w = getTextWidth(s.str);
      if (s.format == ALIGN_RIGHT)
        s.x -= w;
      if (s.format == ALIGN_CENTER)
        s.x -= w * 0.5;
      if ((s.format == ALIGN_DUCTINGMIN) || (s.format == ALIGN_DUCTINGMAX)) {
        const float w2 = getTextWidth("0");
        if (s.format == ALIGN_DUCTINGMIN)
          s.x += w2 * 0.3;
        else
          s.x -= w + w2 * 0.3;
      }
    }
    drawText(s.str, s.x, s.y, s.angle);
  }
}

void VprofDiagram::plotText()
{
  METLIBS_LOG_SCOPE();
  using vcross::util::maximize;

  if (vpopt->ptext) {

    const int n = vptext.size();

    setFontsize(chytxt);

    const float wspace = getTextWidth("oo");

    vector<std::string> fctext(n);
    vector<std::string> geotext(n);
    vector<std::string> kitext(n);
    float wmod = 0, wpos = 0, wfc = 0, wgeo = 0;

    for (int i = 0; i < n; i++) {
      maximize(wmod, getTextWidth(vptext[i].modelName));
      maximize(wpos, getTextWidth(vptext[i].posName));
    }
    for (int i = 0; i < n; i++) {
      if (vptext[i].prognostic) {
        ostringstream ostr;
        ostr << "(" << setiosflags(ios::showpos) << vptext[i].forecastHour << ")";
        fctext[i] = ostr.str();
        maximize(wfc, getTextWidth(fctext[i]));
      }
    }
    if (vpopt->pgeotext) {
      for (int i = 0; i < n; i++) {
        ostringstream ostr;
        ostr << "   (" << fabsf(vptext[i].latitude);
        if (vptext[i].latitude >= 0.0)
          ostr << "N";
        else
          ostr << "S";
        ostr << "  " << fabsf(vptext[i].longitude);
        if (vptext[i].longitude >= 0.0)
          ostr << "E)";
        else
          ostr << "W)";
        geotext[i] = ostr.str();
        maximize(wgeo, getTextWidth(geotext[i]));
      }
    }
    if (vpopt->pkindex) {
      for (int i = 0; i < n; i++) {
        if (vptext[i].kindexFound) {
          int k = std::lround(vptext[i].kindexValue);
          ostringstream ostr;
          ostr << "K= " << setiosflags(ios::showpos) << k;
          kitext[i] = ostr.str();
        }
      }
    }

    const float wtime = getTextWidth("2222-22-22 23:59 UTC");

    float xmod = xysize[BOX_TEXT][XMIN] + chxtxt * 0.5;
    float xpos = xmod + wmod + wspace;
    float xfc = xpos + wpos + wspace;
    float xtime = (wfc > 0.0) ? xfc + wfc + wspace * 0.6 : xfc;
    float xgeo = xtime + wtime + wspace;
    float xkindex = (vpopt->pgeotext) ? xgeo + wgeo + wspace : xgeo;

    for (int i = 0; i < n; i++) {
      gl->setColour(vptext[i].colour, false);
      float y = xysize[BOX_TEXT][YMAX] - chytxt * 1.5 * (vptext[i].index + 1);
      drawText(vptext[i].modelName, xmod, y);
      drawText(vptext[i].posName, xpos, y);
      if (vptext[i].prognostic)
        drawText(fctext[i], xfc, y, 0.0);
      std::string tstr = vptext[i].validTime.format("$date %H:%M UTC", "", true);
      drawText(tstr, xtime, y);
      if (vpopt->pgeotext)
        drawText(geotext[i], xgeo, y);
      if (vpopt->pkindex && vptext[i].kindexFound)
        drawText(kitext[i], xkindex, y);
    }
  }

  vptext.clear();
}

float VprofDiagram::getTextWidth(const std::string& text) const
{
  float w, h;
  gl->getTextSize(text, w, h);
  w /= full.x2;
  h /= full.y2;
  return w;
}

void VprofDiagram::setFontsize(float chy)
{
  chy *= 0.8;
  const float fs = vcross::util::constrain_value(chy * full.y2, 5.0f, 35.0f);
  gl->setFontSize(fs);
}

float VprofDiagram::p_to_y(float p) const
{
  switch (vpopt->diagramtype) {
  case 0: { // amble  (ln(p) under 500 hPa og p over 500 hPa) (under means height under, pressure above)
    const float d5hpa = (std::log(490) - std::log(500)) / 10;
    return (p > 500) ? std::log(p) : (std::log(500) + (500 - p) * d5hpa);
  }
  case 1: // exner-funksjonen (pi)
    return vcross::util::exnerFunction(p);
  case 2: // p
    return p;
  case 3: // ln(p)
    return std::log(p);
  default:
    return p;
  }
}

float VprofDiagram::t_from_xp(float x, float p) const
{
  return (x - dx_from_p(p)) / dx1deg;
}

float VprofDiagram::dx_from_p(float p) const
{
  if (tan_tangle != 0)
    return dx_from_y(transformP(p));
  else
    return 0;
}

float VprofDiagram::dx_from_y(float y) const
{
  return y * tan_tangle;
}

PointF VprofDiagram::to_pixel(const diutil::PointF& xy) const
{
  return PointF(full.x1 + xy.x() * full.x2, full.y1 + xy.y() * full.y2);
}

// transforms (p, t) pair to (x,y) pair in screen coordinates
PointF VprofDiagram::transformPT(float p, float t) const
{
  float yy = transformP(p);
  float xx = dx1deg * t + dx_from_y(yy);
  return PointF(xx, yy);
}

// transforms p to x in screen coordinates
float VprofDiagram::transformP(float p) const
{
  const float y = p_to_y(p);
  return (y1000 - y) * idy_1000_100;
}

void VprofDiagram::drawPT(const std::vector<PointF>& pt)
{
  const size_t nlevel = pt.size();
  if (nlevel < 2)
    return;
  vector<float> x, y;
  x.reserve(nlevel);
  y.reserve(nlevel);
  for (size_t k = 0; k < nlevel; k++) {
    const PointF xy = to_pixel(transformPT(pt[k].x(), pt[k].y()));
    x.push_back(xy.x());
    y.push_back(xy.y());
  }
  float clip[4];
  to_pixel(PointF(xysize[BOX_PT_AXES][XMIN], xysize[BOX_PT_AXES][YMIN])).unpack(clip[0], clip[2]);
  to_pixel(PointF(xysize[BOX_PT_AXES][XMAX], xysize[BOX_PT_AXES][YMAX])).unpack(clip[1], clip[3]);
  diutil::xyclip(x.size(), &x[0], &y[0], clip, gl);
}

void VprofDiagram::drawPT(const std::vector<float>& p, const std::vector<float>& t)
{
  if (t.size() >= p.size() && all_valid(p) && all_valid(t)) {
    const size_t nlevel = std::min(p.size(), t.size());
    vector<float> x, y;
    x.reserve(nlevel);
    y.reserve(nlevel);
    for (size_t k = 0; k < nlevel; k++) {
      const PointF xy = to_pixel(transformPT(p[k], t[k]));
      x.push_back(xy.x());
      y.push_back(xy.y());
    }
    float clip[4];
    to_pixel(PointF(xysize[BOX_PT_AXES][XMIN], xysize[BOX_PT_AXES][YMIN])).unpack(clip[0], clip[2]);
    to_pixel(PointF(xysize[BOX_PT_AXES][XMAX], xysize[BOX_PT_AXES][YMAX])).unpack(clip[1], clip[3]);
    diutil::xyclip(x.size(), &x[0], &y[0], clip, gl);
  }
}

void VprofDiagram::drawPX(const std::vector<float>& pp, const std::vector<float>& v, float x0, float scale, float xlim0, float xlim1)
{
  if (!all_valid(v))
    return;

  const size_t nlevel = std::min(pp.size(), v.size());
  vector<float> x, y;
  x.reserve(nlevel);
  y.reserve(nlevel);
  for (unsigned int k = 0; k < nlevel; k++) {
    const PointF xy = to_pixel(PointF(x0 + scale * v[k], transformP(pp[k])));
    x.push_back(xy.x());
    y.push_back(xy.y());
  }
  float clip[4];
  to_pixel(PointF(xlim0, xysize[BOX_PT_AXES][YMIN])).unpack(clip[0], clip[2]);
  to_pixel(PointF(xlim1, xysize[BOX_PT_AXES][YMAX])).unpack(clip[1], clip[3]);
  diutil::xyclip(x.size(), &x[0], &y[0], clip, gl);
}

void VprofDiagram::plotValues(int nplot, const VprofValues& values, bool isSelectedRealization)
{
  METLIBS_LOG_SCOPE(LOGVAL(nplot) << LOGVAL(isSelectedRealization));
  METLIBS_LOG_DEBUG("start plotting '" << values.text.posName << "'");

  const int nstyles = std::min(vpopt->dataColour.size(), std::min(vpopt->dataLinewidth.size(), vpopt->windLinewidth.size()));
  const int istyle = nplot % nstyles;

  gl->Enable(DiGLPainter::gl_BLEND);
  gl->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);

  Colour c(vpopt->dataColour[istyle]);
  if (!isSelectedRealization)
    c.set(Colour::alpha, 0x3F);
  const float dataWidth = vpopt->dataLinewidth[istyle];
  const float windWidth = vpopt->windLinewidth[istyle];
  gl->setLineStyle(c, dataWidth, true);

  // T
  if (vpopt->ptttt and values.tt.size() >= values.ptt.size()) {
    if (all_valid(values.tt)) {
      drawPT(values.ptt, values.tt);
    }
  }

  // Td
  if (vpopt->ptdtd && values.td.size() > 0) {
    if (all_valid(values.td)) {
      gl->Enable(DiGLPainter::gl_LINE_STIPPLE);
      gl->LineStipple(1, 0xFFC0);
      drawPT(values.ptd.empty() ? values.ptt : values.ptd, values.td);
    }
  }

  if (1) {
    if (values.cloudbase_p > 0) {
      PointF cloudbase_xy = transformPT(values.cloudbase_p, values.cloudbase_t);
      gl->setColour(Colour(255, 0, 0, 128), true);
      gl->drawCircle(true, cloudbase_xy.x(), cloudbase_xy.y(), 0.5);
    }
  }

  // wind (u(e/w) and v(n/s)in unit knots)
  if (isSelectedRealization && vpopt->pwind && values.uu.size() > 0) {
    if (all_valid(values.uu) && all_valid(values.vv)) {
      float xw = xysize[BOX_WIND][XMAX] - xysize[BOX_WIND][XMIN];
      float x0 = xysize[BOX_WIND][XMIN] + xw * 0.5;
      if (vpopt->windseparate)
        x0 += xw * float(nplot);
      float ylim1 = xysize[BOX_PT_AXES][YMIN];
      float ylim2 = xysize[BOX_PT_AXES][YMAX];
      float flagl = xw * 0.5 * 0.85;

      gl->setLineStyle(c, windWidth, true);
      const float windScale = values.windInKnots ? 1 : (3600.0 / 1852.0);
      const vector<float>& pp = values.puv.empty() ? values.ptt : values.puv;
      const size_t nlevel = std::min(pp.size(), std::min(values.uu.size(), values.vv.size()));
      for (unsigned int k = 0; k < nlevel; k++) {
        const float yy = transformP(pp[k]);
        if (yy >= ylim1 && yy <= ylim2) {
          float wx, wy;
          to_pixel(PointF(x0, yy)).unpack(wx, wy);
          gl->drawWindArrow(values.uu[k] * windScale, values.vv[k] * windScale, wx, wy, flagl * full.x2, false);
        }
      }
    } else {
      METLIBS_LOG_INFO("invalid uu/vv values");
    }
  }

  if (isSelectedRealization && vpopt->pslwind && values.dd.size() > 0) {
    if (all_valid(values.dd)) {
      const vector<float>& pp = values.puv.empty() ? values.ptt : values.puv;
      // significant wind levels, wind as numbers (temp and prog)
      // (other levels also if space)
      float dchy = chylab * 1.3;
      float ylim1 = xysize[BOX_SIG_WIND][YMIN] + dchy * 0.5;
      float ylim2 = xysize[BOX_SIG_WIND][YMAX] - dchy * 0.5;
      int k1 = -1;
      int k2 = -1;
      const size_t nlevel = std::min(pp.size(), std::min(values.dd.size(), values.ff.size()));
      ;
      for (unsigned int k = 0; k < nlevel; k++) {
        const float yy = transformP(pp[k]);
        if (yy > ylim1 && yy < ylim2) {
          if (k1 == -1)
            k1 = k;
          k2 = k;
        }
      }

      if (k1 >= 0) {
        std::vector<float> used;
        const float x = xysize[BOX_SIG_WIND][XMIN] + (xysize[BOX_SIG_WIND][XMAX] - xysize[BOX_SIG_WIND][XMIN]) * nplot + chxlab * 0.5;
        setFontsize(chylab);

        for (int sig = 3; sig >= 0; sig--) {
          for (int k = k1; k <= k2; k++) {
            if (values.sigwind[k] == sig) {
              const float yy = transformP(pp[k]);
              ylim1 = yy - dchy;
              ylim2 = yy + dchy;
              size_t i = 0;
              while (i < used.size() && (used[i] < ylim1 || used[i] > ylim2))
                i++;
              if (i == used.size()) {
                used.push_back(yy);
                int idd = (values.dd[k] + 5) / 10;
                int iff = values.ff[k];
                if (idd == 0 && iff > 0)
                  idd = 36;
                ostringstream ostr;
                ostr << setw(2) << setfill('0') << idd << "-" << setw(3) << setfill('0') << iff;
                std::string str = ostr.str();
                const float y = yy - chylab * 0.5;
                drawText(str, x, y, 0.0);
              }
            }
          }
        }
      }
    } else {
      METLIBS_LOG_INFO("invalid dd values");
    }
  }

  gl->setLineStyle(c, dataWidth, true);

  // vertical wind, omega
  if (vpopt->pvwind) {
    float dx = xysize[BOX_VER_WIND][XMAX] - xysize[BOX_VER_WIND][XMIN];
    float x0 = xysize[BOX_VER_WIND][XMIN] + dx * 0.5;
    float scale = -dx / vpopt->rvwind;
    drawPX(values.pom.empty() ? values.ptt : values.pom, values.om, x0, scale, xysize[BOX_VER_WIND][XMIN], xysize[BOX_VER_WIND][XMAX]);
  }

  // relative humidity
  if (vpopt->prelhum) {
    float dx = xysize[BOX_REL_HUM][XMAX] - xysize[BOX_REL_HUM][XMIN];
    float x0 = xysize[BOX_REL_HUM][XMIN];
    float scale = dx / 100.;
    drawPX(values.ptt, values.rhum, x0, scale, xysize[BOX_REL_HUM][XMIN], xysize[BOX_REL_HUM][XMAX]);
  }

  // ducting
  if (vpopt->pducting) {
    float rd = vpopt->ductingMax - vpopt->ductingMin;
    float dx = xysize[BOX_DUCTING][XMAX] - xysize[BOX_DUCTING][XMIN];
    float x0 = xysize[BOX_DUCTING][XMIN] + dx * (0.0 - vpopt->ductingMin) / rd;
    float scale = dx / rd;
    drawPX(values.ptt, values.duct, x0, scale, xysize[BOX_DUCTING][XMIN], xysize[BOX_DUCTING][XMAX]);
  }

  if (isSelectedRealization) {
    if (values.text.realization >= 0) {
      // text indicating which realization, below PT diagram
      const float x = xysize[BOX_PT_AXES][XMIN] + 3.0 * chxlab;
      const float y = xysize[BOX_PT_AXES][YMIN] - 2.5 * chylab;
      ostringstream ostr;
      ostr << "member " << values.text.realization;
      drawText(ostr.str(), x, y, 0.0);
    }

    // text from data values
    vptext.push_back(values.text);
    vptext.back().index = nplot;
    vptext.back().colour = c;
  }
}
