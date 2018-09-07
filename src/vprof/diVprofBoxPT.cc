/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2018 met.no

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

#include "diana_config.h"

#include "diVprofBoxPT.h"

#include "diField/VcrossUtil.h"
#include "diField/diMetConstants.h"
#include "diVprofAxesPT.h"
#include "diVprofOptions.h"
#include "diVprofPainter.h"
#include "diVprofUtils.h"

#include <iomanip>
#include <sstream>

#define MILOGGER_CATEGORY "diana.VprofBoxPT"
#include <miLogger/miLogging.h>

using diutil::PointF;

namespace {
const float DEG_TO_RAD = M_PI / 180;

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
vprof::PointF_v mixingRatioLine(float qsat, float pmin, float pmax, float pstep, float tempmin = -500)
{
  vprof::PointF_v ptline;
  PointF tp0;
  for (float p = pmin; p <= pmax + pstep / 4; p += pstep) {
    const float t = mixingRatioT(qsat, p);
    const PointF tp1(t, p);
    if (t >= tempmin) {
      if (p > pmin /* i.e. have pt0 */ && tp0.x() < tempmin)
        // start exactly on tempmin
        ptline.push_back(vprof::interpolateX(tempmin, tp0, tp1));
      ptline.push_back(tp1);
    }
    tp0 = tp1;
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

vprof::PointF_v generateWetAdiabat(float p0, float p1, float pstep, float t0 /* degC */, float tempmin)
{
  vprof::PointF_v wet_pt;

  PointF tpa(t0, p0);
  p1 += pstep / 4; // try to fix rounding problems
  while ((pstep < 0 && tpa.y() >= p1) || (pstep > 0 && tpa.y() <= p1)) {
    wet_pt.push_back(tpa);
    float pb = tpa.y() + pstep;
    float tb = iterateWetAdiabat(tpa.y(), tpa.x(), pb);
    tpa = PointF(tb, pb);
    if (tpa.x() < tempmin) {
      if (wet_pt.back().x() > tempmin)
        // start exactly on tempmin
        wet_pt.push_back(vprof::interpolateX(tempmin, wet_pt.back(), tpa));
      break;
    }
  }

  return wet_pt;
}

} // namespace

float VprofBoxPT::cotrails[4][mptab];
bool VprofBoxPT::init_cotrails = false;

VprofBoxPT::VprofBoxPT()
{
}

void VprofBoxPT::condensationtrails()
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
  // cc   parameter (npctab=15)
  // cc   integer ipctab(npctab)
  // cc   real    crtab(npctab,4), crtab2(npctab,4),crtab3(npctab,4)
  // c..p
  // ccc   data ipctab/ 100,  125,  150,  175,  200,  250,  300,  350,
  // ccc  +             400,  500,  600,  700,  800,  900, 1000/
  // c..t(ri=0%)
  // ccc   data crtab/-60.9,-59.0,-57.4,-56.0,-54.8,-52.7,-51.1,-49.6,
  // ccc  +           -48.3,-46.1,-44.3,-42.7,-41.3,-40.0,-38.9,
  // c..t(ri=40%)
  // ccc  +           -60.1,-58.3,-56.6,-55.2,-54.0,-51.9,-50.2,-48.5,
  // ccc  +           -47.3,-45.1,-43.2,-41.6,-40.1,-38.8,-37.7,
  // c..t(ri=100%)
  // ccc  +           -58.7,-56.8,-54.9,-53.5,-52.2,-50.0,-48.1,-46.3,
  // ccc  +           -45.2,-42.7,-40.8,-39.1,-37.6,-36.2,-34.9,
  // c..t(rw=100%)
  // ccc  +           -52.5,-50.4,-48.6,-47.1,-45.7,-43.5,-41.6,-39.9,
  // ccc  +           -38.5,-36.1,-34.1,-32.3,-30.8,-29.4,-28.1/
  //....................................................................
  //
  // below computing the lines at many more levels

  // this constant is found in one the figures in the report
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

  init_cotrails = true;
}

void VprofBoxPT::setVerticalAxis(vcross::detail::AxisPtr zaxis)
{
  VprofBoxLine::setVerticalAxis(zaxis);
  pminDiagram = axes->z->getValueMax();
  pmaxDiagram = axes->z->getValueMin();
}

void VprofBoxPT::setTAngle(float angle_deg)
{
  tangle_ = angle_deg;
  if (tangle_ < 0 || tangle_ > 80)
    tangle_ = 45;
  std::static_pointer_cast<VprofAxesPT>(axes)->setAngleT(tangle_);
}

void VprofBoxPT::setTStep(int step)
{
  tstep_ = step;
  if (tstep_ < 1)
    tstep_ = 5;
}

void VprofBoxPT::setDryAdiabatStep(int step)
{
  dryadiabatstep_ = step;
  if (dryadiabatstep_ < 1)
    dryadiabatstep_ = 5;
}

void VprofBoxPT::setWetAdiabatStep(int step)
{
  wetadiabatstep_ = step;
  if (wetadiabatstep_ < 1)
    wetadiabatstep_ = 10;
}

const std::string VprofBoxPT::key_t_angle = "t.angle";
const std::string VprofBoxPT::key_t_step = "t.step";

const std::string VprofBoxPT::key_dryadiabat = "dryadiabat";
const std::string VprofBoxPT::key_dryadiabat_step = VprofBoxPT::key_dryadiabat + ".step";

const std::string VprofBoxPT::key_wetadiabat = "wetadiabat";
const std::string VprofBoxPT::key_wetadiabat_step = VprofBoxPT::key_wetadiabat + ".step";
const std::string VprofBoxPT::key_wetadiabat_pmin = VprofBoxPT::key_wetadiabat + ".pmin";
const std::string VprofBoxPT::key_wetadiabat_tmin = VprofBoxPT::key_wetadiabat + ".tmin";

const std::string VprofBoxPT::key_mixingratio = "mixingratio";
const std::string VprofBoxPT::key_mixingratio_text = VprofBoxPT::key_mixingratio + ".text";
const std::string VprofBoxPT::key_mixingratio_pmin = VprofBoxPT::key_mixingratio + ".pmin";
const std::string VprofBoxPT::key_mixingratio_tmin = VprofBoxPT::key_mixingratio + ".tmin";
const std::string VprofBoxPT::key_mixingratio_table = VprofBoxPT::key_mixingratio + ".table";

const std::string VprofBoxPT::key_cotrails = "cotrails";
const std::string VprofBoxPT::key_cotrails_pmin = VprofBoxPT::key_cotrails + ".pmin";
const std::string VprofBoxPT::key_cotrails_pmax = VprofBoxPT::key_cotrails + ".pmax";

void VprofBoxPT::configure(const miutil::KeyValue_v& options)
{
  METLIBS_LOG_SCOPE(LOGVAL(options));

  VprofAxesPT_p axes_pt = std::make_shared<VprofAxesPT>();
  axes_pt->z = axes->z;
  axes = axes_pt;

  setFrame(true);
  setZGrid(true);
  setZTicksShowText(true);
  setXGrid(true);
  setXTicksShowText(true);
  setXValueRange(-30, 30);
  setWidth(25);

  VprofBoxLine::configure(options);

  setTAngle(30);
  tstep_ = 5;

  pdryadiabat = true;   // dry adiabats
  dryadiabatstep_ = 10; // temperature step (C at 1000hPa)
  Linestyle dryadiabatLS("black", 1, "solid");

  pwetadiabat = true;  // dry adiabats
  wetadiabatstep_ = 5; // temperature step (C at 1000hPa)
  Linestyle wetadiabatLS("darkRed", 1, "solid");
  wetadiabatPmin = 300;
  wetadiabatTmin = -50;

  pmixingratio = true; // mixing ratio
  qtable = {.1, .2, .4, 1., 2., 5., 10., 20., 30., 40., 50.};
  Linestyle mixingratioLS("magenta", 1, "longdash");
  mixingratioPmin = 300;
  mixingratioTmin = -50;

  pcotrails = true; // condensation trail lines
  Linestyle cotrailsLS("cyan", 3, "solid");
  cotrailsPmin = 100;
  cotrailsPmax = 700;

  for (const auto& kv : options) {
    if (kv.key() == key_t_angle)
      setTAngle(kv.toFloat(tangle_));
    else if (kv.key() == key_t_step)
      setTStep(kv.toInt());

    else if (kv.key() == key_dryadiabat)
      setDryAdiabat(kv.toBool());
    else if (kv.key() == key_dryadiabat_step)
      setDryAdiabatStep(kv.toInt());
    else if (vprof::kvLinestyle(dryadiabatLS, key_dryadiabat, kv))
      ; // nothing

    else if (kv.key() == key_wetadiabat)
      setWetAdiabat(kv.toBool());
    else if (kv.key() == key_wetadiabat_step)
      setWetAdiabatStep(kv.toInt());
    else if (kv.key() == key_wetadiabat_pmin)
      setWetAdiabatPMin(kv.toInt());
    else if (kv.key() == key_wetadiabat_tmin)
      setWetAdiabatTMin(kv.toInt());
    else if (vprof::kvLinestyle(wetadiabatLS, key_wetadiabat, kv))
      ; // nothing

    else if (kv.key() == key_mixingratio)
      setMixingRatio(kv.toBool());
    else if (kv.key() == key_mixingratio_text)
      plabelq = kv.toBool();
    else if (kv.key() == key_mixingratio_pmin)
      setMixingRatioPMin(kv.toInt());
    else if (kv.key() == key_mixingratio_tmin)
      setMixingRatioTMin(kv.toInt());
    else if (vprof::kvLinestyle(mixingratioLS, key_mixingratio, kv))
      ; // nothing
    else if (kv.key() == key_mixingratio_table)
      setMixingRatioTable(kv.toFloats());

    else if (kv.key() == key_cotrails)
      setCotrails(kv.toBool());
    else if (kv.key() == key_cotrails_pmin)
      setCotrailsPMin(kv.toInt());
    else if (kv.key() == key_cotrails_pmax)
      setCotrailsPMax(kv.toInt());
    else if (vprof::kvLinestyle(cotrailsLS, key_cotrails, kv))
      ; // nothing
  }
  setDryAdiabatStyle(dryadiabatLS);
  setWetAdiabatStyle(wetadiabatLS);
  setMixingRatioStyle(mixingratioLS);
  setCotrailsStyle(cotrailsLS);

#if 0
  int set = vpopt->mixingratioSet;
  if (set < 0 || set >= int(vpopt->qtable.size()))
    set = 1;
  qtable = vpopt->qtable[set];
#endif
}

void VprofBoxPT::updateLayout()
{
  VprofBoxLine::updateLayout();
  pminDiagram = axes->z->getValueMax();
  pmaxDiagram = axes->z->getValueMin();
  tminDiagram = axes->x->getValueMin();
  tmaxDiagram = axes->x->getValueMax();

  if (plabelq && pmixingratio) {
    // space for mixing ratio labels
    vcross::util::maximize(margin.y1, vprof::chybas * 0.8 * 1.5);
  }
}

void VprofBoxPT::configureXAxisLabelSpace()
{
  if (x_ticks_showtext_ && x_grid_) {
    using vcross::util::maximize;
    if (useTiltedTemperatureLabels()) {
      float sintan = sinf(tangle_ * DEG_TO_RAD);
      float costan = cosf(tangle_ * DEG_TO_RAD);
      maximize(margin.x2, costan * vprof::chxbas * 4. + costan * vprof::chybas);
      maximize(margin.y2, sintan * vprof::chxbas * 4. + sintan * vprof::chybas);
    } else {
      maximize(margin.x2, vprof::chxbas * 4.);
      maximize(margin.y2, vprof::chybas * 1.4);
    }
  }
}

void VprofBoxPT::plotDiagram(VprofPainter* p)
{
  METLIBS_LOG_SCOPE();
  VprofBoxLine::plotDiagram(p);

  if (isVerticalPressure()) {
    if (pdryadiabat)
      plotDryAdiabats(p);
    if (pwetadiabat)
      plotWetAdiabats(p);
    if (pmixingratio)
      plotMixingRatioLines(p);
    if (pcotrails)
      plotCondensationTrailLines(p);
  }
}

void VprofBoxPT::plotXAxisGrid(VprofPainter* p)
{
  METLIBS_LOG_SCOPE();
  const Rectangle area = axes->paintRange();
  const Rectangle& rdf = size();
  vcross::detail::AxisCPtr axis_z = axes->z;

  // min,max temperature on diagram
  const float tmin = std::max(-MetNo::Constants::t0, axes->paint2value(PointF(area.x1, area.y2)).x());
  const float tmax = axes->paint2value(PointF(area.x2, area.y1)).x();
  METLIBS_LOG_DEBUG(LOGVAL(tmin) << LOGVAL(tmax) << LOGVAL(tstep_));
  const int itmin = std::ceil(tmin);
  const int itmax = std::floor(tmax);

  int it1 = (itmin / tstep_) * tstep_;
  int it2 = (itmax / tstep_) * tstep_;
  if (it1 < itmin)
    it1 += tstep_;
  if (it2 > itmax)
    it2 -= tstep_;
  for (int it = it1; it <= it2; it += tstep_) {
    PointF xy1 = axes->value2paint(PointF(it, pmaxDiagram));
    PointF xy2 = axes->value2paint(PointF(it, pminDiagram));
    if (xy1.x() < area.x1)
      xy1 = vprof::interpolateX(area.x1, xy1, xy2);
    if (xy2.x() > area.x2)
      xy2 = vprof::interpolateX(area.x2, xy1, xy2);
    const bool ls1 = (it % 40 != 0);
    p->setLineStyle(ls1 ? x_grid_linestyle_minor_ : x_grid_linestyle_major_);
    p->drawLine(xy1.x(), xy1.y(), xy2.x(), xy2.y());
  }

  if (!x_ticks_showtext_)
    return;

  p->setLineStyle(x_grid_linestyle_minor_);
  const int itstep = std::max((tmax - tmin < 75) ? 5 : 10, tstep_);
  const int itlim = std::floor(std::max(-MetNo::Constants::t0, axes->paint2value(PointF(area.x2, area.y2)).x()));

  // t numbers at the diagram top
  int itl1 = (itmin / itstep) * itstep;
  int itl2 = (itlim / itstep) * itstep;
  if (itmin > 0)
    itl1 += itstep;
  if (itlim < 0)
    itl2 -= itstep;
  float chy = vprof::chybas;
  int numwid = 3;
  if (itl1 < -100 || itl2 > +100) {
    chy *= 0.75;
    numwid = 4;
  }
  float numrot, dxmin, dxtop, dytop, dx1, dx2;
  float ynext, ylast, dymin, dxright, dyright;
  if (useTiltedTemperatureLabels()) {
    const float sintan = sinf(tangle_ * DEG_TO_RAD);
    const float costan = cosf(tangle_ * DEG_TO_RAD);

    numrot = tangle_; // atan2f(costan, sintan) / DEG_TO_RAD;
    dxmin = 1.3 * chy / costan;
    dxtop = chy * 0.5 / costan;
    dytop = 0.;
    dx1 = vprof::chybas * costan;
    dx2 = vprof::chybas * 3. * sintan;

    dymin = 1.3 * chy / sintan;
    dxright = costan * chy;
    dyright = -sintan * chy + 0.5 * chy / sintan;
    ynext = rdf.y2;
    ylast = rdf.y1 + chy * costan - 0.5 * chy / sintan;
  } else {
    numrot = 0.;
    dxmin = vprof::chxbas * 4.;
    dxtop = -1.5 * vprof::chxbas;
    dytop = 0.2 * vprof::chybas;
    dx1 = 1.5 * vprof::chxbas;
    dx2 = 1.5 * vprof::chxbas;

    dymin = chy * 1.3;
    dxright = 0.5 * vprof::chxbas;
    dyright = 0.5 * chy;
    // ynext calculated later
    ylast = rdf.y1 + chy * 0.5;
  }

  vprof::TextSpacing tsx(area.x1 + dx1, 1e10, dxmin);
  const float y = axis_z->value2paint(pminDiagram, false) + dytop;
  for (int it = itl1; it <= itl2; it += itstep) {
    const float x = axes->value2paint(PointF(it, pminDiagram)).x() + dxtop;
    if (tsx.accept(x)) {
      std::ostringstream ostr;
      ostr << std::setw(numwid) << std::setfill(' ') << std::setiosflags(std::ios::showpos) << it;
      p->fpInitStr(ostr.str(), x, y, numrot, chy, x_grid_linestyle_minor_.colour, VprofPainter::ALIGN_LEFT, VprofPainter::FONT_SCALED);
    }
  }

  // t numbers at the diagram right side
  itl1 = itl2 + itstep;
  itl2 = (itmax / itstep) * itstep;
  if (itmax < 0)
    itl2 -= itstep;
  if (!useTiltedTemperatureLabels()) {
    const float xlast = tsx.next - dxmin + dx2;
    if (xlast < area.x2)
      ynext = rdf.y2;
    else
      ynext = area.y2 - chy * 0.5;
  }

  vprof::TextSpacing tsy(ynext, ylast, -dymin);
  const float t1 = axes->paint2value(PointF(area.x2, area.y1)).x();
  const float t2 = axes->paint2value(PointF(area.x2, area.y2)).x();
  const float x = area.x2 + dxright;
  for (int it = itl1; it <= itl2; it += itstep) {
    const float y = area.y1 + area.height() * (it - t1) / (t2 - t1) + dyright;
    if (tsy.accept(y)) {
      std::ostringstream ostr;
      ostr << std::setw(numwid) << std::setfill(' ') << std::setiosflags(std::ios::showpos) << it;
      p->fpInitStr(ostr.str(), x, y, numrot, chy, x_grid_linestyle_minor_.colour, VprofPainter::ALIGN_LEFT, VprofPainter::FONT_SCALED);
    }
  }
}

void VprofBoxPT::plotXAxisLabels(VprofPainter*)
{
}

void VprofBoxPT::plotDryAdiabats(VprofPainter* p)
{
  // dry adiabats (always base at 0 degrees celsius, 1000 hPa)
  // dry adiabats: potential temperature (th) constant,
  // cp*t=th*pi, pi=cp and th=t at 1000 hPa,
  // (above 100 hPa the 5 hPa step really is too much)

  const Rectangle area = axes->paintRange();
  vcross::detail::AxisCPtr axis_z = std::static_pointer_cast<const VprofAxesPT>(axes)->z;

  p->setLineStyle(dryadiabatLineStyle);

  // find first and last dry adiabat to be drawn (at 1000 hPa, pi=cp)
  const float tmax = axes->paint2value(PointF(area.x1, area.y1)).x();
  const float tmin = std::max(-MetNo::Constants::t0, axes->paint2value(PointF(area.x2, area.y2)).x());
  using namespace MetNo::Constants;
  const float thmax = cp * (tmax + t0) / vcross::util::exnerFunction(pmaxDiagram);
  const float thmin = cp * (tmin + t0) / vcross::util::exnerFunction(pminDiagram);
  const int itt1 = int(thmax - t0);
  const int itt2 = int(thmin - t0);
  int it1 = (itt1 / dryadiabatstep_) * dryadiabatstep_;
  int it2 = (itt2 / dryadiabatstep_) * dryadiabatstep_;
  if (itt1 > 0)
    it1 += dryadiabatstep_;
  if (itt2 < 0)
    it2 -= dryadiabatstep_;

  for (int it = it1; it <= it2; it += dryadiabatstep_) {
    float th = t0 + it;
    float p1 = pminDiagram;
    if (p1 < 25 && it % (dryadiabatstep_ * 8) != 0)
      p1 = 25;
    if (p1 < 50 && it % (dryadiabatstep_ * 4) != 0)
      p1 = 50;
    if (p1 < 100 && it % (dryadiabatstep_ * 2) != 0)
      p1 = 100;

    vprof::PointF_v dry_pt;
    for (float p = p1; p <= pmaxDiagram + idptab / 4; p += idptab) {
      float t = (th / cp) * vcross::util::exnerFunction(p) - t0;
      dry_pt.push_back(PointF(t, p));
    }
    plotLine(p, dry_pt);
  }
  p->enableLineStipple(false);
}

void VprofBoxPT::plotWetAdiabats(VprofPainter* p)
{
  const Rectangle area = axes->paintRange();

  // min,max temperature on diagram
  const float tmax = axes->paint2value(PointF(area.x2, area.y1)).x();
  const int itmax = std::floor(tmax);

  // wet adiabats (always base at 0 degrees celsius, 1000 hPa)
  const int wpmin = std::max(wetadiabatPmin, pminDiagram);
  if (wpmin >= pmaxDiagram)
    return;

  p->setLineStyle(wetadiabatLineStyle);

  const int mintemp = std::max(wetadiabatTmin, tminDiagram);
  const int itstep = wetadiabatstep_;
  const int it1 = (mintemp / itstep) * itstep + ((mintemp > 0) ? itstep : 0);
  const int it2 = (itmax / itstep) * itstep - ((itmax < 0) ? itstep : 0);
  for (int t = it1; t <= it2; t += itstep) {
    vprof::PointF_v wet_pt;
    const float porigin = 1000;
    if (pmaxDiagram > porigin) {
      // near ground, increase pressure from 1000
      vprof::PointF_v pt = generateWetAdiabat(porigin, pmaxDiagram, idptab, t, wetadiabatTmin /* probably irrelevant here */);
      // insert at front, reverse direction, skipping 1000hPa point
      wet_pt.insert(wet_pt.begin(), ++pt.rbegin(), pt.rend());
    }

    vprof::PointF_v pt = generateWetAdiabat(porigin, wpmin, -idptab, t, wetadiabatTmin);
    // insert at end, keeping direction, including 1000hPa point
    wet_pt.insert(wet_pt.end(), pt.begin(), pt.end());

    plotLine(p, wet_pt);
  }
  p->enableLineStipple(false);
}

void VprofBoxPT::plotMixingRatioLines(VprofPainter* p)
{
  const Rectangle area = axes->paintRange();

  // mixing ratio (lines for constant mixing ratio) (humidity)
  const float qpmin = std::max(mixingratioPmin, pminDiagram);
  if (qpmin >= pmaxDiagram)
    return;

  p->setLineStyle(mixingratioLineStyle);

  // plot labels below the p-t diagram
  const float xspacing = 2 * vprof::chxbas * 0.8;
  const float chy = vprof::chybas * 0.8;
  const float y = area.y1 - chy * 1.25;
  vprof::TextSpacing ts(area.x1 + xspacing, area.x2 - xspacing, 2 * xspacing);

  for (float qsat : qtable) {
    const float qsat_u = qsat * 0.001; // convert unit to g/kg
    const vprof::PointF_v qpt = mixingRatioLine(qsat_u, qpmin, pmaxDiagram, idptab, mixingratioTmin);
    if (qpt.size() < 2)
      continue;

    plotLine(p, qpt);

    if (plabelq) {
      // last point is for pmax, which is where the label should be
      const float xlabel = axes->value2paint(qpt.back()).x();
      if (ts.accept(xlabel)) {
        std::ostringstream ostr;
        ostr << qsat;
        p->fpInitStr(ostr.str(), xlabel, y, 0.0, chy, mixingratioLineStyle.colour);
      }
    }
  }
  p->enableLineStipple(false);
}

void VprofBoxPT::plotCondensationTrailLines(VprofPainter* p)
{
  // condensation trail lines (for kondensstriper fra fly)
  if (!init_cotrails)
    condensationtrails();

  p->setLineStyle(cotrailsLineStyle);
  const int kmin = std::max(cotrailsPmin, pminDiagram) / float(idptab);
  const int kmax = (std::min(cotrailsPmax, pmaxDiagram) + idptab - 1) / float(idptab);
  std::vector<PointF> xyline;
  xyline.reserve(std::max(kmax - kmin, 0));
  for (int n = 0; n < 4; n++) {
    for (int k = kmin; k <= kmax; k++) {
      xyline.push_back(PointF(cotrails[n][k], k * idptab));
    }
    plotLine(p, xyline);
    xyline.clear();
  }
  p->enableLineStipple(false);
}

bool VprofBoxPT::useTiltedTemperatureLabels() const
{
  return (tangle_ > 29. && tangle_ < 61.);
}

// static
const std::string& VprofBoxPT::boxType()
{
  static const std::string bt = "pt";
  return bt;
}
