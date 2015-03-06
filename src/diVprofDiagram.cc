/*
 Diana - A Free Meteorological Visualisation Tool

 $Id$

 Copyright (C) 2006 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diVprofDiagram.h"

#include "diColour.h"
#include "diGlUtilities.h"
#include "diLinetype.h"
#include "diUtilities.h"

#include <diField/diMetConstants.h>
#include <puTools/miStringFunctions.h>

#include <cmath>
#include <iomanip>
#include <sstream>

using namespace::miutil;
using namespace std;

#define MILOGGER_CATEGORY "diana.VprofDiagram"
#include <miLogger/miLogging.h>

static const float DEG_TO_RAD = M_PI / 180;

VprofDiagram::VprofDiagram(VprofOptions *vpop) :
  VprofTables(), vpopt(vpop),diagramInList(false),  drawlist(0), numtemp(0),
      numprog(0)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  setDefaults();
  plotw = ploth = 0;
  plotwDiagram = plothDiagram = -1;
}

VprofDiagram::~VprofDiagram()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofDiagram::~VprofDiagram");
#endif
  if (glIsList(drawlist))
    glDeleteLists(drawlist, 1);
  drawlist = 0;
}

void VprofDiagram::changeOptions(VprofOptions *vpop)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofDiagram::changeOptions");
#endif
  vpopt = vpop;
  newdiagram = true;
}

void VprofDiagram::changeNumber(int ntemp, int nprog)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofDiagram::changeNumber  ntemp,nprog: "<<ntemp<<" "<<nprog);
#endif
  if (ntemp != numtemp || nprog != numprog) {
    numtemp = ntemp;
    numprog = nprog;
    newdiagram = true;
  }
}

void VprofDiagram::setPlotWindow(int w, int h)
{
  plotw = w;
  ploth = h;

  resetPage();
}

void VprofDiagram::plot()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofDiagram::plot");
#endif

  vptext.clear();

  Colour cback(vpopt->backgroundColour);

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  glClearColor(cback.fR(), cback.fG(), cback.fB(), 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  if (plotw < 5 || ploth < 5)
    return;

  if (vpopt->changed)
    newdiagram = true;

  bool redraw = newdiagram;
  if (vpopt->changed)
    redraw = true;
  if (!diagramInList)
    redraw = true;

  if (plotw != plotwDiagram || ploth != plothDiagram) {
    plotwDiagram = plotw;
    plothDiagram = ploth;
    redraw = true;
  }

  if (redraw) {
    if (diagramInList && glIsList(drawlist))
      glDeleteLists(drawlist, 1);
    if (newdiagram)
      prepare();
    diagramInList = false;
  }

  float w = plotw;
  float h = ploth;
  float x1 = xysize[0][0];
  float x2 = xysize[0][1];
  float y1 = xysize[0][2];
  float y2 = xysize[0][3];
  float dx = x2 - x1;
  float dy = y2 - y1;
  x1 -= dx * 0.02;
  x2 += dx * 0.02;
  y1 -= dy * 0.02;
  y2 += dy * 0.02;
  dx = x2 - x1;
  dy = y2 - y1;
  if (w / h > dx / dy) {
    dx = (dy * w / h - dx) * 0.5;
    x1 -= dx;
    x2 += dx;
  } else {
    dy = (dx * h / w - dy) * 0.5;
    y1 -= dy;
    y2 += dy;
  }
  dx = x2 - x1;
  dy = y2 - y1;

  glLoadIdentity();
  glOrtho(x1, x2, y1, y2, -1., 1.);

  if (redraw)
    makeFontsizes(dx, dy, plotw, ploth);

  if (hardcopy) {
    // must avoid using the GL-list due to several linewidths and -types
    plotDiagram();
    fpDrawStr(true);
  } else if (redraw) {
    // YE: Should we check for existing list ?!
    // Yes, I think so!

#if !defined(USE_PAINTGL)
    if (drawlist != 0) {
      if (glIsList(drawlist))
        glDeleteLists(drawlist, 1);
    }
    drawlist = glGenLists(1);
    if (drawlist != 0)
      glNewList(drawlist, GL_COMPILE_AND_EXECUTE);
    else
      METLIBS_LOG_WARN("VprofDiagram::plot(): Unable to create new displaylist, glGenLists(1) returns 0");
#endif

    plotDiagram();

#if !defined(USE_PAINTGL)
    if (drawlist != 0)
      glEndList();
#endif

    fpDrawStr(true);
    if (drawlist != 0)
      diagramInList = true;
  } else if (glIsList(drawlist)) {
#if !defined(USE_PAINTGL)
    glCallList(drawlist);
#else
    plotDiagram();
#endif
    fpDrawStr(false);
  }

}

void VprofDiagram::setDefaults()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofDiagram::setDefaults");
#endif

  newdiagram = true;

  for (int n = 0; n < mxysize; n++)
    for (int i = 0; i < 4; i++)
      xysize[n][i] = 0.;

  chxbas = -1.;
  chybas = -1.;

  init_tables = false;
  init_cotrails = false;
  last_diagramtype = -1;
  last_tangle = -1.;
  last_rsvaxis = -1.;
  last_pmin = -1.;
  last_pmax = -1.;
}

void VprofDiagram::prepare()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofDiagram::prepare");
#endif

  // constants
  //const float t0 =273.15;
  const float p0 = 1000.;
  const float r = 287.;
  const float cp = 1004.;
  const float rcp = r / cp;
  //const float eps=0.622;
  //const float xlh=2.501e+6;
  //const float cplr=xlh/rcp;
  //const float exl=eps*xlh;
  //const float cpinv=1./cp;

  //const float dptab=idptab;
  //const float dpinv=1./dptab;
  //const float undef=+1.e+35;

  // index for some pressure levels in the tables
  //const int k25  =  25/idptab;
  //const int k50  =  50/idptab;
  const int k100 = 100 / idptab;
  const int k500 = 500 / idptab;
  //const int k700 = 700/idptab;
  const int k1000 = 1000 / idptab;

  vpopt->checkValues();

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

  kpmin = pminDiagram / idptab;
  kpmax = (pmaxDiagram + idptab - 1) / idptab;
  pmin = pminDiagram;
  pmax = pmaxDiagram;

  float tmind = vpopt->tminDiagram;
  float tmaxd = vpopt->tmaxDiagram;

  int k, n;
  float p, dx;

  if (!init_tables) {
    //initialize fixed tables
    iptab[0] = 0;
    pptab[0] = 0.;
    pitab[0] = 0.;
    pplog[0] = 0.;
    for (k = 1; k < mptab; k++) {
      iptab[k] = k * idptab;
      p = iptab[k];
      pptab[k] = p;
      pitab[k] = cp * powf(p / p0, rcp);
      pplog[k] = logf(p);
    }
    init_tables = true;
  }

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
  // yptabd:  y for hver 5. hPa, 0 - mptab*idptab hPa
  // xztabd:  x for t=0 gr.c. for hver 5. hPa, 0 - mptab*idptab hPa
  //---------------------------------------------------------------------

  if (vpopt->diagramtype != last_diagramtype || vpopt->tangle != last_tangle) {

    if (vpopt->diagramtype == 0) {
      // amble  (ln(p) under 500 hPa og p over 500 hPa).
      // 490 - 1300 hPa:
      for (k = k500 - 2; k < mptab; k++)
        yptabd[k] = pplog[k];
      // 0 - 495 hPa:
      float d5hpa = (yptabd[k500 - 2] - yptabd[k500]) * 0.5;
      for (k = 0; k < k500; k++)
        yptabd[k] = yptabd[k500] + (k500 - k) * d5hpa;
    } else if (vpopt->diagramtype == 1) {
      // exner-funksjonen (pi)
      for (k = 0; k < mptab; k++)
        yptabd[k] = pitab[k];
    } else if (vpopt->diagramtype == 2) {
      // p
      for (k = 0; k < mptab; k++)
        yptabd[k] = pptab[k];
    } else if (vpopt->diagramtype == 3) {
      // ln(p)
      for (k = 0; k < mptab; k++)
        yptabd[k] = pplog[k];
    }

    // set  yptabd("1000hPa")=0  and  yptabd("100hPa")=1
    float y1 = yptabd[k100];
    float y2 = yptabd[k1000];
    float yc = 1. / (y2 - y1);
    for (k = 0; k < mptab; k++)
      yptabd[k] = yc * (y2 - yptabd[k]);

    // define: x=0. for t=0 degree celsius in 1000 hPa
    // (xztabd in same "unit" as yptabd)
    if (vpopt->tangle < 0.1) {
      for (k = 0; k < mptab; k++)
        xztabd[k] = 0.;
    } else {
      float tantang = tanf(vpopt->tangle * DEG_TO_RAD);
      float y1000 = yptabd[k1000];
      for (k = 0; k < mptab; k++)
        xztabd[k] = tantang * (yptabd[k] - y1000);
    }

    last_diagramtype = vpopt->diagramtype;
    last_tangle = vpopt->tangle;
    last_rsvaxis = -1.;
  }

  // standard stoerrelse iflg. et amble-diagram:
  // avstand i cm mellom 100 og 1000 hPa
  float height = 27.8;
  // lengde i cm langs p=konst for 1 grad celsius
  float dx1deg = 0.418333;

  if (vpopt->rsvaxis != last_rsvaxis || pmin != last_pmin || pmax != last_pmax) {

    float y100 = yptabd[k100];
    float y1000 = yptabd[k1000];
    float scale = vpopt->rsvaxis * height / (y100 - y1000);

    dx1degree = dx1deg;

    for (k = 0; k < mptab; k++) {
      yptab[k] = scale * (yptabd[k] - y1000);
      xztab[k] = scale * xztabd[k];
    }

    last_pmin = pmin;
    last_pmax = pmax;
    last_rsvaxis = vpopt->rsvaxis;
  }

  // temperature range:  0=fixed  1=fixed.max-min  2=minimum

  float xmind, xmaxd;

  //if (itrange==0) {
  // fixed (tmin,tmax)
  xmind = xztab[k1000] + dx1deg * tmind;
  xmaxd = xztab[k1000] + dx1deg * tmaxd;
  //} else {
  //################ NO DATA AVAILABLE HERE..... ?????????
  //c------------------------------------------------------------
  //c..check the data, t and td
  //        xtmin =+undef
  //        xtmax =-undef
  //        xtdmin=+undef
  //	xtmean=0.
  //	ntmean=0
  //	pppmin=pmin-0.1
  //	pppmax=pmax+0.1
  //c..t
  //	np=1
  //	nnp=np
  //	if(lvlplt(nnp).lt.0) nnp=1
  //	nlevel=lvlplt(nnp)
  //        do k=1,nlevel
  //          p=datplt(k,nnp,1)
  //	  if(p.gt.pppmin .and. p.lt.pppmax) then
  //            x=p*dpinv
  //            i=x
  //            xz=xztab(i)+(xztab(i+1)-xztab(i))*(x-i)
  //            x=xz+dx1deg*datplt(k,np,2)
  //            xtmin=min(xtmin,x)
  //            xtmax=max(xtmax,x)
  //	    xtmean=xtmean+x
  //	    ntmean=ntmean+1
  //	  end if
  //        end do
  //	if(ntmean.gt.0) xtmean=xtmean/ntmean
  //c..td
  //	np=2
  //	nnp=np
  //	if(lvlplt(nnp).lt.0) nnp=1
  //	nlevel=lvlplt(nnp)
  //        do k=1,nlevel
  //          p=datplt(k,nnp,1)
  //	  if(p.gt.pppmin .and. p.lt.pppmax) then
  //            x=p*dpinv
  //            i=x
  //            xz=xztab(i)+(xztab(i+1)-xztab(i))*(x-i)
  //            x=xz+dx1deg*datplt(k,np,2)
  //            xtdmin=min(xtdmin,x)
  //	  end if
  //        end do
  //	if(xtmin.gt.xtmax) then
  //	  if(itrange.eq.1) then
  //            xtmin=xztab(k1000)+dx1deg*tmind
  //            xtmax=xztab(k1000)+dx1deg*tmaxd
  //	  else
  //	    xtmin=xztab(k1000)-dx1deg*5.
  //	    xtmax=xztab(k1000)+dx1deg*5.
  //	  end if
  //	end if
  //	xtdmin=min(xtdmin,xtmin)
  //c------------------------------------------------------------
  //        dxtmax=dx1deg*(tmaxd-tmind)
  //        if(itrange.eq.1) then
  //c..fixed.max-min
  //          dxt=xtmax-xtdmin
  //          if(dxt.le.dxtmax) then
  //            xmind=xtdmin-(dxtmax-dxt)*0.5
  //            xmaxd=xtmax+(dxtmax-dxt)*0.5
  //          elseif(xtmax-xtmin.le.dxtmax) then
  //	    xmind=xtmax-dxtmax
  //	    xmaxd=xtmax
  //	  else
  //            xmaxd=min(xtmean+dxtmax*0.5,xtmax)
  //            xmind=xmaxd-dxtmax
  //          end if
  //        elseif(xtmax-xtdmin.le.dxtmax) then
  //c..minimum
  //          xmind=xtdmin
  //          xmaxd=xtmax
  //        elseif(xtmax-xtmin.le.dxtmax) then
  //c..minimum (t o.k.)
  //          xmind=xtmax-dxtmax
  //          xmaxd=xtmax
  //        else
  //c..minimum (t has larger range than maximum allowed)
  //          xmaxd=min(xtmean+dxtmax*0.5,xtmax)
  //          xmind=xmaxd-dxtmax
  //        end if
  //}

  // space calculations

  float ymaxd = yptab[kpmin];
  float ymind = yptab[kpmax];

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

  clearCharsizes();
  addCharsize(chytxt);
  addCharsize(chylab);
  addCharsize(chylab * 0.8);
  addCharsize(chylab * 0.75);
  addCharsize(chylab * 0.7);
  addCharsize(chylab * 0.6);
  addCharsize(chylab * 0.5);

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

  xysize[1][0] = xmind;
  xysize[1][1] = xmaxd;
  xysize[1][2] = ymind;
  xysize[1][3] = ymaxd;
  xysize[2][0] = xmindf;
  xysize[2][1] = xmaxdf;
  xysize[2][2] = ymindf;
  xysize[2][3] = ymaxdf;

  // stuff around the p-t diagram

  float chx, chy;
  float xmin = xmindf;
  float xmax = xmaxdf;
  float ymin = ymindf;
  float ymax = ymaxdf;
  for (n = 3; n < mxysize; n++) {
    xysize[n][0] = +1.;
    xysize[n][1] = -1.;
    xysize[n][2] = ymin;
    xysize[n][3] = ymax;
  }

  // space for flight levels (numbers and ticks on axis)
  if (vpopt->pflevels) {
    if (vpopt->plabelflevels)
      dx = chxlab * 5.5;
    else
      dx = chxlab * 1.5;
    xysize[3][0] = xmin - dx;
    xysize[3][1] = xmin;
    xmin -= dx;
  }

  // space for significant wind levels, temp (observation) and prognostic
  if (vpopt->pslwind) {
    dx = chxlab * 6.5;
    xmin -= (dx * (numtemp + numprog));
    xysize[4][0] = xmin;
    xysize[4][1] = xmin + dx;
  }

  // space for wind, possibly in multiple (separate) columns
  if (vpopt->pwind) {
    dx = chxbas * 8. * vpopt->rswind;
    xysize[5][0] = xmax;
    xysize[5][1] = xmax + dx;
    xysize[5][2] = ymind - dx * 0.5;
    xysize[5][3] = ymaxd + dx * 0.5;
    if (vpopt->windseparate)
      xmax += (dx * (numtemp + numprog));
    else
      xmax += dx;
  }

  // space for vertical wind (only for prognostic)
  if (vpopt->pvwind && numprog > 0) {
    dx = chxbas * 6.5 * vpopt->rsvwind;
    xysize[6][0] = xmax;
    xysize[6][1] = xmax + dx;
    xysize[6][2] = ymind - chylab * 2.5;
    xysize[6][3] = ymaxd + chylab * 2.5;
    xmax += dx;
    ostringstream ostr;
    ostr << vpopt->rvwind;
    k = ostr.str().length();
    chx = chxlab;
    chy = chylab;
    if (chx * (k + 4) > dx) {
      chx = dx / (k + 4);
      chy = chx * chylab / chxlab;
      addCharsize(chy);
    }
  }

  // space for relative humidity
  if (vpopt->prelhum) {
    dx = chxbas * 7. * vpopt->rsrelhum;
    xysize[7][0] = xmax;
    xysize[7][1] = xmax + dx;
    xysize[7][2] = ymind - chylab * 1.5;
    xysize[7][3] = ymaxd + chylab * 1.5;
    xmax += dx;
    chx = chxlab;
    chy = chylab;
    if (chx * 3. > dx) {
      chx = dx / 3.;
      chy = chx * chylab / chxlab;
      addCharsize(chy);
    }
  }

  // space for ducting
  if (vpopt->pducting) {
    dx = chxbas * 7. * vpopt->rsducting;
    xysize[8][0] = xmax;
    xysize[8][1] = xmax + dx;
    xysize[8][2] = ymind - chylab * 1.5;
    xysize[8][3] = ymaxd + chylab * 1.5;
    xmax += dx;
    chx = chxlab;
    chy = chylab;
    if (chx * 5. > dx) {
      chx = dx / 5.;
      chy = chx * chylab / chxlab;
      addCharsize(chy);
    }
  }

  // adjustments
  for (n = 3; n < mxysize - 1; n++) {
    if (ymin > xysize[n][2])
      ymin = xysize[n][2];
    if (ymax < xysize[n][3])
      ymax = xysize[n][3];
  }
  if (numtemp + numprog > 1 && (vpopt->pwind || vpopt->pslwind)) {
    if (ymin > ymind - chylab * 2.)
      ymin = ymind - chylab * 2.;
  }
  if (vpopt->plabelp || (vpopt->pflevels && vpopt->plabelflevels)) {
    if (ymin > ymind - chylab * 2.)
      ymin = ymind - chylab * 2.;
  }
  if (vpopt->pslwind) {
    if (ymin > ymind - chylab * 0.85)
      ymin = ymind - chylab * 0.85;
    if (ymax < ymaxd + chylab * 0.85)
      ymax = ymaxd + chylab * 0.85;
  }
  for (n = 3; n < mxysize - 1; n++) {
    xysize[n][2] = ymin;
    xysize[n][3] = ymax;
  }

  // space for text
  if (vpopt->ptext) {
    float ytext = ymin;
    n = numtemp + numprog;
    if (n < vpopt->linetext)
      n = vpopt->linetext;
    if (n < 1)
      n = 1;
    ymin -= (chytxt * (1.5 * float(n) + 0.5));
    xysize[9][0] = xmin;
    xysize[9][1] = xmax;
    xysize[9][2] = ymin;
    xysize[9][3] = ytext;
  } else {
    xysize[9][2] = ymin;
    xysize[9][3] = ymin;
  }

  // total area
  xysize[0][0] = xmin;
  xysize[0][1] = xmax;
  xysize[0][2] = ymin;
  xysize[0][3] = ymax;
  //###############################################################
  //for (n=0; n<mxysize; n++)
  //  METLIBS_LOG_DEBUG("xysize "<<n<<" "<<xysize[n][0]<<" "<<xysize[n][1]
  //      <<" "<<xysize[n][2]<<" "<<xysize[n][3]);
  //###############################################################

  vpopt->changed = false;
  newdiagram = false;
}

void VprofDiagram::condensationtrails()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofDiagram::condensationtrails");
#endif
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

    p = pptab[k];

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

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofDiagram::condensationtrails finished");
#endif
}

void VprofDiagram::plotDiagram()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofDiagram::plotDiagram");
#endif

  // constants
  const float t0 = 273.15;
  //const float p0 =1000.;
  const float r = 287.;
  const float cp = 1004.;
  const float rcp = r / cp;
  const float eps = 0.622;
  const float xlh = 2.501e+6;
  const float cplr = xlh / rcp;
  const float exl = eps * xlh;
  const float cpinv = 1. / cp;

  const float dptab = idptab;
  const float dpinv = 1. / dptab;
  //const float undef=+1.e+35;

  // index for some pressure levels in the tables
  const int k25 = 25 / idptab;
  const int k50 = 50 / idptab;
  const int k100 = 100 / idptab;
  //const int k500 = 500/idptab;
  //const int k700 = 700/idptab;
  const int k1000 = 1000 / idptab;

  float dx1deg = dx1degree;

  float xmind = xysize[1][0];
  float xmaxd = xysize[1][1];
  float ymind = xysize[1][2];
  float ymaxd = xysize[1][3];
  //float xmindf= xysize[2][0];
  //float xmaxdf= xysize[2][1];
  float ymindf = xysize[2][2];
  float ymaxdf = xysize[2][3];

  float xylimit[4] = { xmind, xmaxd, ymind, ymaxd };

  // min,max temperature on diagram
  float tmin = (xmind - xztab[kpmin]) / dx1deg;
  float tmax = (xmaxd - xztab[kpmax]) / dx1deg;
  int itmin = (tmin < 0.) ? int(tmin) : int(tmin) + 1;
  int itmax = (tmax > 0.) ? int(tmax) : int(tmax) + 1;
  float tmin1000 = (xmind - xztab[k1000]) / dx1deg;
  int itmin1000 = (tmin1000 < 0.) ? int(tmin1000) : int(tmin1000) + 1;

  int i, k, kk, k1, n, it, it1, it2, itstep, numwid, kmin, kmax;
  float x1, x2, y1, y2, dx, dy, x, y, p, t = 0.0, chx, chy, numrot, dx1, dx2, dxmin,
      dymin;
  float cw = 1.0, xnext, xlast, ynext, ylast, xc, yc;

  Linetype linetype;

  fpStr.clear();

  //###############################################################
  //for (n=0; n<mxysize; n++)
  //  METLIBS_LOG_DEBUG("xysize "<<n<<" "<<xysize[n][0]<<" "<<xysize[n][1]
  //      <<" "<<xysize[n][2]<<" "<<xysize[n][3]);
  //###############################################################

  // thick lines not changed by dialogs...
  vpopt->pLinewidth2 = vpopt->pLinewidth1 + 2.;
  vpopt->tLinewidth2 = vpopt->tLinewidth1 + 2.;
  vpopt->flevelsLinewidth2 = vpopt->flevelsLinewidth1 + 2.;

  // frame around the 'real' p-t diagram
  if (vpopt->pframe) {
    Colour c(vpopt->frameColour);
    glColor3ubv(c.RGB());
    linetype = Linetype(vpopt->frameLinetype);
    if (linetype.stipple) {
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(linetype.factor, linetype.bmap);
    }
    glLineWidth(vpopt->frameLinewidth);
    glBegin(GL_LINE_LOOP);
    glVertex2f(xmind, ymind);
    glVertex2f(xmaxd, ymind);
    glVertex2f(xmaxd, ymaxd);
    glVertex2f(xmind, ymaxd);
    glEnd();
    UpdateOutput();
    glDisable(GL_LINE_STIPPLE);
  }

  // pressure lines (possibly at flight levels)
  if (vpopt->pplines) {
    vector<float> plev, ylev;
    vector<bool> hlev;
    if (vpopt->pplinesfl) {
      // flight levels
      kk = vpopt->pflightlevels.size();
      plev = vpopt->pflightlevels;
      for (k = 0; k < kk; k++) {
        if (vpopt->flightlevels[k] % 50 == 0)
          hlev.push_back(true);
        else
          hlev.push_back(false);
      }
    } else {
      // pressure levels
      kk = vpopt->plevels.size();
      plev = vpopt->plevels;
      for (k = 0; k < kk; k++) {
        int ip = int(vpopt->plevels[k] + 0.5);
        if (ip == 1000 || ip == 500)
          hlev.push_back(true);
        else
          hlev.push_back(false);
      }
    }
    for (k = 0; k < kk; k++) {
      x = plev[k] * dpinv;
      i = int(x);
      y = yptab[i] + (yptab[i + 1] - yptab[i]) * (x - i);
      ylev.push_back(y);
    }
    Colour c(vpopt->pColour);
    glColor3ubv(c.RGB());
    linetype = Linetype(vpopt->pLinetype);
    if (linetype.stipple) {
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(linetype.factor, linetype.bmap);
    }
    glLineWidth(vpopt->pLinewidth1);
    glBegin(GL_LINES);
    for (k = 0; k < kk; k++) {
      if (ylev[k] > ymind && ylev[k] < ymaxd) {
        if (!hlev[k]) {
          glVertex2f(xmind, ylev[k]);
          glVertex2f(xmaxd, ylev[k]);
        } else {
          glEnd();
          UpdateOutput();
          glLineWidth(vpopt->pLinewidth2);
          glBegin(GL_LINES);
          glVertex2f(xmind, ylev[k]);
          glVertex2f(xmaxd, ylev[k]);
          glEnd();
          UpdateOutput();
          glLineWidth(vpopt->pLinewidth1);
          glBegin(GL_LINES);
        }
      }
    }
    glEnd();
    UpdateOutput();
    glDisable(GL_LINE_STIPPLE);
    // labels (text/numbers)
    if (vpopt->plabelp) {
      //       setFontsize(chylab);
      dy = chylab * 1.2;
      y1 = ymindf + dy * 0.5;
      y2 = ymaxdf - dy * 0.5;
      x1 = xmind - chxlab * 0.75;
      if (vpopt->pplinesfl) {
        // flight levels
        for (k = 0; k < kk; k++) {
          if (ylev[k] > y1 && ylev[k] < y2) {
            if (vpopt->flightlevels[k] % 50 == 0) {
              ostringstream ostr;
              ostr << setw(3) << setfill('0') << vpopt->flightlevels[k];
              std::string str = ostr.str();
              //  	      fp->getStringSize(str.c_str(), cw, ch);
              //            fp->drawStr(str.c_str(),x1-cw,ylev[k]-chylab*0.5,0.0);
              fpInitStr(str, x1, ylev[k] - chylab * 0.5, 0.0, chylab, c, "x-w");
              y1 = ylev[k] + dy;
            }
          }
        }
        //         fp->getStringSize("FL", cw, ch);
        //         fp->drawStr("FL",x1-cw,ymind-chylab*1.25,0.0);
        fpInitStr("FL", x1 - cw, ymind - chylab * 1.25, 0.0, chylab, c);
      } else {
        // pressure levels
        int ipstep, ip;
        if (pmax - pmin > 399.)
          ipstep = 100;
        else if (pmax - pmin > 199.)
          ipstep = 50;
        else if (pmax - pmin > 99.)
          ipstep = 25;
        else
          ipstep = 10;
        for (k = 0; k < kk; k++) {
          if (ylev[k] > y1 && ylev[k] < y2) {
            ip = int(vpopt->plevels[k] + 0.5);
            if (ip % ipstep == 0) {
              ostringstream ostr;
              ostr << setw(4) << setfill(' ') << ip;
              std::string str = ostr.str();
              //  	      fp->getStringSize(str.c_str(), cw, ch);
              //  	      fp->drawStr(str.c_str(),x1-cw,ylev[k]-chylab*0.5,0.0);
              fpInitStr(str, x1, ylev[k] - chylab * 0.5, 0.0, chylab, c, "x-w");
              y1 = ylev[k] + dy;
            }
          }
        }
        //         fp->getStringSize("hPa", cw, ch);
        //         fp->drawStr("hPa",x1-cw,ymind-chylab*1.25,0.0);
        fpInitStr("hPa", x1, ymind - chylab * 1.25, 0.0, chylab, c, "x-w");
      }
      UpdateOutput();
    }
  }

  // temperature lines
  if (vpopt->ptlines) {
    int tStep = vpopt->tStep;
    if (tStep < 1)
      tStep = 5;
    it1 = (itmin / tStep) * tStep;
    it2 = (itmax / tStep) * tStep;
    if (it1 < itmin)
      it1 += tStep;
    if (it2 > itmax)
      it2 -= tStep;
    Colour c(vpopt->tColour);
    glColor3ubv(c.RGB());
    linetype = Linetype(vpopt->tLinetype);
    if (linetype.stipple) {
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(linetype.factor, linetype.bmap);
    }
    glLineWidth(vpopt->tLinewidth1);
    glBegin(GL_LINES);
    for (it = it1; it <= it2; it += tStep) {
      t = it;
      y1 = yptab[kpmax];
      y2 = yptab[kpmin];
      x1 = xztab[kpmax] + dx1deg * t;
      x2 = xztab[kpmin] + dx1deg * t;
      if (x1 < xmind) {
        y1 = y1 + (y2 - y1) * (xmind - x1) / (x2 - x1);
        x1 = xmind;
      }
      if (x2 > xmaxd) {
        y2 = y1 + (y2 - y1) * (xmaxd - x1) / (x2 - x1);
        x2 = xmaxd;
      }
      if (it % 40 != 0) {
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
      } else {
        glEnd();
        UpdateOutput();
        glLineWidth(vpopt->tLinewidth2);
        glBegin(GL_LINES);
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
        glEnd();
        UpdateOutput();
        glLineWidth(vpopt->tLinewidth1);
        glBegin(GL_LINES);
      }
    }
    glEnd();
    UpdateOutput();
    glDisable(GL_LINE_STIPPLE);
    // t numbers (labels)
    if (vpopt->plabelt) {
      float sintan = sinf(vpopt->tangle * DEG_TO_RAD);
      float costan = cosf(vpopt->tangle * DEG_TO_RAD);
      itstep = (tmax - tmin < 75) ? 5 : 10;
      if (itstep < vpopt->tStep)
        itstep = vpopt->tStep;
      float tlim = (xmaxd - xztab[kpmin]) / dx1deg;
      int itlim = (tlim < 0.) ? int(tlim) - 1 : int(tlim);
      // t numbers at the diagram top
      it1 = (itmin / itstep) * itstep;
      it2 = (itlim / itstep) * itstep;
      if (itmin > 0)
        it1 += itstep;
      if (itlim < 0)
        it2 -= itstep;
      if (it1 > -100 && it2 < +100) {
        chx = chxlab;
        chy = chylab;
        numwid = 3;
      } else {
        chx = chxlab * 0.75;
        chy = chylab * 0.75;
        numwid = 4;
      }
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
      y = yptab[kpmin] + dy;
      xnext = xmind + dx1;
      for (it = it1; it <= it2; it += itstep) {
        x = xztab[kpmin] + dx1deg * it + dx;
        if (x > xnext) {
          ostringstream ostr;
          ostr << setw(numwid) << setfill(' ') << setiosflags(ios::showpos)
              << it;
          std::string str = ostr.str();
          //           fp->drawStr(str.c_str(),x,y,numrot);
          fpInitStr(str.c_str(), x, y, numrot, chy, c, "", "SCALEFONT");
          xnext = x + dxmin;
        }
      }
      xlast = xnext - dxmin + dx2;
      // t numbers at the diagram right side
      it1 = it2 + itstep;
      it2 = (itmax / itstep) * itstep;
      if (itmax < 0)
        it2 -= itstep;
      if (it1 > -100 && it2 < +100) {
        chx = chxlab;
        chy = chylab;
        numwid = 3;
      } else {
        chx = chxlab * 0.75;
        chy = chylab * 0.75;
        numwid = 4;
      }
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
      float t1 = (xmaxd - xztab[kpmax]) / dx1deg;
      float t2 = (xmaxd - xztab[kpmin]) / dx1deg;
      y1 = yptab[kpmax];
      y2 = yptab[kpmin];
      x = xmaxd + dx;
      //       setFontsize(chy);
      for (it = it1; it <= it2; it += itstep) {
        t = it;
        y = y1 + (y2 - y1) * (t - t1) / (t2 - t1) + dy;
        if (y < ynext && y > ylast) {
          ostringstream ostr;
          ostr << setw(numwid) << setfill(' ') << setiosflags(ios::showpos)
              << it;
          std::string str = ostr.str();
          //           fp->drawStr(str.c_str(),x,y,numrot);
          fpInitStr(str.c_str(), x, y, numrot, chy, c, "", "SCALEFONT");
          ynext = y - dymin;
        }
      }
      UpdateOutput();
    }
  }

  float xline[mptab], yline[mptab];
  for (k = kpmin; k <= kpmax; k++)
    yline[k] = yptab[k];

  // dry adiabats (always base at 0 degrees celsius, 1000 hPa)
  // dry adiabats: potential temperature (th) constant,
  // cp*t=th*pi, pi=cp and th=t at 1000 hPa,
  // (above 100 hPa the 5 hPa step really is too much)
  if (vpopt->pdryadiabat) {
    itstep = vpopt->dryadiabatStep;
    if (itstep < 1)
      itstep = 5;
    Colour c(vpopt->dryadiabatColour);
    glColor3ubv(c.RGB());
    linetype = Linetype(vpopt->dryadiabatLinetype);
    if (linetype.stipple) {
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(linetype.factor, linetype.bmap);
    }
    glLineWidth(vpopt->dryadiabatLinewidth);
    // find first and last dry adiabat to be drawn (at 1000 hPa, pi=cp)
    t = (xmind - xztab[kpmax]) / dx1deg;
    float th = cp * (t + t0) / pitab[kpmax];
    int itt1 = int(th - t0);
    t = (xmaxd - xztab[kpmin]) / dx1deg;
    th = cp * (t + t0) / pitab[kpmin];
    int itt2 = int(th - t0);
    it1 = (itt1 / itstep) * itstep;
    it2 = (itt2 / itstep) * itstep;
    if (itt1 > 0)
      it1 += itstep;
    if (itt2 < 0)
      it2 -= itstep;
    if (vpopt->diagramtype != 1) {
      for (it = it1; it <= it2; it += itstep) {
        th = t0 + it;
        k1 = kpmin;
        if (k1 < k25 && it % (itstep * 8) != 0)
          k1 = k25;
        if (k1 < k50 && it % (itstep * 4) != 0)
          k1 = k50;
        if (k1 < k100 && it % (itstep * 2) != 0)
          k1 = k100;
        for (k = k1; k <= kpmax; k++) {
          t = th * cpinv * pitab[k] - t0;
          xline[k] = xztab[k] + dx1deg * t;
        }
        diutil::xyclip(kpmax - k1 + 1, &xline[k1], &yline[k1], xylimit);
      }
    } else {
      // exner function as vertical coordinate, th is a straight line
      float xl[2], yl[2];
      yl[0] = yptab[kpmin];
      yl[1] = yptab[kpmax];
      for (it = it1; it <= it2; it += itstep) {
        th = t0 + it;
        t = th * cpinv * pitab[kpmin];
        xl[0] = xztab[kpmin] + dx1deg * t;
        t = th * cpinv * pitab[kpmax];
        xl[1] = xztab[kpmax] + dx1deg * t;
        diutil::xyclip(2, xl, yl, xylimit);
      }
    }
    UpdateOutput();
    glDisable(GL_LINE_STIPPLE);
  }

  // wet adiabats (always base at 0 degrees celsius, 1000 hPa)
  if (vpopt->pwetadiabat) {
    itstep = vpopt->wetadiabatStep;
    if (itstep < 1)
      itstep = 10;
    Colour c(vpopt->wetadiabatColour);
    glColor3ubv(c.RGB());
    linetype = Linetype(vpopt->wetadiabatLinetype);
    if (linetype.stipple) {
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(linetype.factor, linetype.bmap);
    }
    glLineWidth(vpopt->wetadiabatLinewidth);
    kmin = (vpopt->wetadiabatPmin + idptab - 1) / idptab;
    if (kmin < kpmin)
      kmin = kpmin;
    if (kmin < kpmax) {
      float tempmin = vpopt->wetadiabatTmin;
      int mintemp = (vpopt->wetadiabatTmin > itmin1000) ? vpopt->wetadiabatTmin
          : itmin1000;
      it1 = (mintemp / itstep) * itstep;
      it2 = (itmax / itstep) * itstep;
      if (mintemp > 0)
        it1 += itstep;
      if (itmax < 0)
        it2 -= itstep;
      int loop1 = (kpmax > k1000) ? 1 : 2;
      int loop2 = (kmin < k1000) ? 2 : 1;
      float t1, tcl1, qcl1, tcl, qcl, tx = 0.0, pi, pi1, esat, qsat, dq, a1, a2, ytmp;
      int kstop, kstep;
      for (it = it1; it <= it2; it += itstep) {
        t1 = it;
        const MetNo::Constants::ewt_calculator ewt(t1);
        if (ewt.defined())
            esat = ewt.value();
        else
          esat = 0;
        qsat = eps * esat / pptab[k1000];
        tcl1 = cp * (t1 + t0);
        qcl1 = qsat;
        xline[k1000] = xztab[k1000] + dx1deg * t1;
        for (int loop = loop1; loop <= loop2; loop++) {
          if (loop == 1) {
            kstop = kpmax;
            kstep = +1;
          } else {
            kstop = kmin;
            kstep = -1;
          }
          pi = pitab[k1000];
          tcl = tcl1;
          qcl = qcl1;
          t = t1;
          k = k1000;
          while (k != kstop) {
            k += kstep;
            tx = t;
            pi1 = pi;
            p = pptab[k];
            pi = pitab[k];
            // lift preliminary along dry adiabat
            tcl = tcl * pi / pi1;
            // adjust humidity in some iterations
            for (int iteration = 0; iteration < 5; iteration++) {
              t = tcl / cp - t0;
              const MetNo::Constants::ewt_calculator ewt(t);
              if (!ewt.defined())
                break;
              esat = ewt.value();
              qsat = eps * esat / p;
              dq = qcl - qsat;
              a1 = cplr * qcl / tcl;
              a2 = exl / tcl;
              dq = dq / (1. + a1 * a2);
              qcl = qcl - dq;
              tcl = tcl + dq * xlh;
            }
            qcl = qsat;
            t = tcl / cp - t0;
            xline[k] = xztab[k] + dx1deg * t;
            if (t <= tempmin && loop == 2)
              kstop = k;
          }
        }
        if (loop2 == 1)
          k = kmin;
        if (kpmax - k > 0) {
          ytmp = yline[k];
          if (t < tempmin) {
            x = (tempmin - tx) / (t - tx);
            xline[k] = xline[k + 1] + (xline[k] - xline[k + 1]) * x;
            yline[k] = yline[k + 1] + (yline[k] - yline[k + 1]) * x;
          }
          diutil::xyclip(kpmax - k + 1, &xline[k], &yline[k], xylimit);
          yline[k] = ytmp;
        }
      }
    }
    UpdateOutput();
    glDisable(GL_LINE_STIPPLE);
  }

  // mixing ratio (lines for constant mixing ratio) (humidity)
  if (vpopt->pmixingratio) {
    int set = vpopt->mixingratioSet;
    if (set < 0 || set >= int(vpopt->qtable.size()))
      set = 1;
    Colour c(vpopt->mixingratioColour);
    glColor3ubv(c.RGB());
    //##################################### default i dialog ???????
    //    glEnable(GL_LINE_STIPPLE);
    //    glLineStipple(1,0xF8F8);
    //##################################### default i dialog ???????
    linetype = Linetype(vpopt->mixingratioLinetype);
    if (linetype.stipple) {
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(linetype.factor, linetype.bmap);
    }
    glLineWidth(vpopt->mixingratioLinewidth);
    float tempmin = vpopt->mixingratioTmin;
    kmin = (vpopt->mixingratioPmin + idptab - 1) / idptab;
    if (kmin < kpmin)
      kmin = kpmin;
    if (kmin < kpmax) {
      vector<float> xqsat;
      float qsat, esat, t1, t2, ytmp;
      int kstop;
      int nq = vpopt->qtable[set].size();
      for (int iq = 0; iq < nq; iq++) {
        qsat = vpopt->qtable[set][iq] * 0.001; // qtable in unit g/kg
        n = 1;
        kstop = 0;
        for (k = kmin; k <= kpmax; k++) {
          using namespace MetNo::Constants;
          esat = qsat * pptab[k] / eps;
          while (ewt[n] < esat && n < N_EWT - 1)
            n++;
          x = (esat - ewt[n - 1]) / (ewt[n] - ewt[n - 1]);
          t = -100. + (n - 1. + x) * 5.;
          xline[k] = xztab[k] + dx1deg * t;
          if (t < tempmin)
            kstop = k;
        }
        if (kstop == 0) {
          kstop = kmin;
          ytmp = yline[kstop];
        } else if (kstop < kpmax) {
          ytmp = yline[kstop];
          t1 = (xline[kstop + 1] - xztab[kstop + 1]) / dx1deg;
          t2 = (xline[kstop] - xztab[kstop]) / dx1deg;
          x = (tempmin - t1) / (t2 - t1);
          xline[kstop] = xline[kstop + 1] + (xline[kstop] - xline[kstop + 1])
              * x;
          yline[kstop] = yline[kstop + 1] + (yline[kstop] - yline[kstop + 1])
              * x;
        } else {
          ytmp = yline[kstop];
        }
        if (kpmax - kstop + 1 > 1)
          diutil::xyclip(kpmax - kstop + 1, &xline[kstop], &yline[kstop], xylimit);
        yline[kstop] = ytmp;
        xqsat.push_back(xline[kpmax]);
      }
      if (vpopt->plabelq) {
        // plot labels below the p-t diagram
        chx = chxlab * 0.8;
        chy = chylab * 0.8;
        // 	setFontsize(chy);
        xnext = xmind + chx * 2.;
        xlast = xmaxd - chx * 2.;
        y = ymind - chy * 1.25;
        for (int iq = 0; iq < nq; iq++) {
          if (xqsat[iq] > xnext && xqsat[iq] < xlast) {
            ostringstream ostr;
            ostr << vpopt->qtable[set][iq];
            std::string str = ostr.str();
            dx = str.length() * 0.5;
            //             fp->drawStr(str.c_str(),xqsat[iq],y,0.0);
            fpInitStr(str.c_str(), xqsat[iq], y, 0.0, chy, c);
            xnext = xqsat[iq] + chx * 4.;
          }
        }
      }
    }
    UpdateOutput();
    glDisable(GL_LINE_STIPPLE);
  }

  // condensation trail lines (for kondensstriper fra fly)
  if (vpopt->pcotrails) {
    Colour c(vpopt->cotrailsColour);
    glColor3ubv(c.RGB());
    linetype = Linetype(vpopt->cotrailsLinetype);
    if (linetype.stipple) {
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(linetype.factor, linetype.bmap);
    }
    glLineWidth(vpopt->cotrailsLinewidth);
    kmin = vpopt->cotrailsPmin / idptab;
    kmax = (vpopt->cotrailsPmax + idptab - 1) / idptab;
    if (kmin < kpmin)
      kmin = kpmin;
    if (kmax > kpmax)
      kmax = kpmax;
    float xline[mptab], yline[mptab];
    for (n = 0; n < 4; n++) {
      for (k = kmin; k <= kmax; k++) {
        xline[k] = xztab[k] + dx1deg * cotrails[n][k];
        yline[k] = yptab[k];
      }
      diutil::xyclip(kmax - kmin + 1, &xline[kmin], &yline[kmin], xylimit);
    }
    UpdateOutput();
    glDisable(GL_LINE_STIPPLE);
  }

  // plot stuff around the p-t diagram..................................

  // flight levels (numbers and ticks)
  if (vpopt->pflevels) {
    chx = chxlab;
    chy = chylab;
    x1 = xysize[3][0];
    x2 = xysize[3][1];
    y1 = xysize[3][2];
    y2 = xysize[3][3];
    Colour c(vpopt->flevelsColour);
    glColor3ubv(c.RGB());
    // flevelsLinetype....
    glLineWidth(vpopt->flevelsLinewidth1);
    kk = vpopt->pflightlevels.size();
    glBegin(GL_LINES);
    for (k = 0; k < kk; k++) {
      x = vpopt->pflightlevels[k] * dpinv;
      i = int(x);
      y = yptab[i] + (yptab[i + 1] - yptab[i]) * (x - i);
      if (y > y1 && y < y2) {
        if (vpopt->flightlevels[k] % 50 == 0) {
          glEnd();
          UpdateOutput();
          glLineWidth(vpopt->flevelsLinewidth2);
          glBegin(GL_LINES);
          glVertex2f(x2 - chx * 1.5, y);
          glVertex2f(x2, y);
          glEnd();
          UpdateOutput();
          glLineWidth(vpopt->flevelsLinewidth1);
          glBegin(GL_LINES);
        } else {
          glVertex2f(x2 - chx * 0.85, y);
          glVertex2f(x2, y);
        }
      }
    }
    glEnd();
    UpdateOutput();
    if (vpopt->plabelflevels) {
      //######################################################################
      //METLIBS_LOG_DEBUG(" flightlevels.size()= "<< flightlevels.size());
      //METLIBS_LOG_DEBUG("pflightlevels.size()= "<<pflightlevels.size());
      //METLIBS_LOG_DEBUG("                  kk= "<<kk);
      //######################################################################
      y1 += chy * 0.6;
      y2 -= chy * 0.6;
      //       setFontsize(chy);
      x1 += chx * 0.5;
      for (k = 0; k < kk; k++) {
        if (vpopt->flightlevels[k] % 50 == 0) {
          x = vpopt->pflightlevels[k] * dpinv;
          i = int(x);
          y = yptab[i] + (yptab[i + 1] - yptab[i]) * (x - i);
          if (y > y1 && y < y2) {
            ostringstream ostr;
            ostr << setw(3) << setfill('0') << vpopt->flightlevels[k];
            std::string str = ostr.str();
            // 	    fp->drawStr(str.c_str(),x1,y-chy*0.5,0.);
            fpInitStr(str.c_str(), x1, y - chy * 0.5, 0., chy, c);
            y1 = y + chy * 1.2;
          }
        }
      }
      //       fp->drawStr("FL",x1,ymind-chy*1.25,0.);
      fpInitStr("FL", x1, ymind - chy * 1.25, 0., chy, c);
      UpdateOutput();
    }
  }

  Colour c(vpopt->frameColour);
  Colour c2("red");
  if (c2 == c)
    c2 = Colour("green");
  glColor3ubv(c.RGB());
  linetype = Linetype(vpopt->frameLinetype);
  if (linetype.stipple) {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(linetype.factor, linetype.bmap);
  }
  glLineWidth(vpopt->frameLinewidth);
  glBegin(GL_LINES);

  y1 = xysize[9][3];
  y2 = xysize[0][3];

  // misc vertical line
  if (vpopt->plabelp) {
    if (vpopt->pflevels || vpopt->pslwind) {
      x1 = xysize[2][0];
      glVertex2f(x1, y1);
      glVertex2f(x1, y2);
    }
  }
  if (vpopt->pflevels && vpopt->plabelflevels && vpopt->pslwind) {
    x1 = xysize[3][0];
    glVertex2f(x1, y1);
    glVertex2f(x1, y2);
  }
  if (vpopt->pslwind && numtemp + numprog > 1) {
    dx = xysize[4][1] - xysize[4][0];
    x1 = xysize[4][0];
    for (n = 1; n < numtemp + numprog; n++) {
      x1 += dx;
      glVertex2f(x1, y1);
      glVertex2f(x1, y2);
    }
  }
  if (vpopt->plabelt) {
    if (vpopt->pwind || (vpopt->pvwind && numprog > 0) || vpopt->prelhum) {
      x1 = xysize[2][1];
      glVertex2f(x1, y1);
      glVertex2f(x1, y2);
    }
  }
  if (vpopt->pwind && vpopt->windseparate && numtemp + numprog > 1) {
    dx = xysize[5][1] - xysize[5][0];
    x1 = xysize[5][0];
    for (n = 1; n < numtemp + numprog; n++) {
      x1 += dx;
      glVertex2f(x1, y1);
      glVertex2f(x1, y2);
    }
  }
  glEnd();
  UpdateOutput();
  glDisable(GL_LINE_STIPPLE);

  bool sepline = vpopt->pwind;

  // vertical wind (up/down arrows showing air motion)
  if (vpopt->pvwind && numprog > 0) {
    glBegin(GL_LINES);
    if (sepline) {
      glVertex2f(xysize[6][0], xysize[9][3]);
      glVertex2f(xysize[6][0], xysize[0][3]);
    }
    glVertex2f(xysize[6][0], ymaxd);
    glVertex2f(xysize[6][1], ymaxd);
    glVertex2f(xysize[6][0], ymind);
    glVertex2f(xysize[6][1], ymind);
    glEnd();
    UpdateOutput();
    linetype = Linetype(vpopt->rangeLinetype);
    if (linetype.stipple) {
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(linetype.factor, linetype.bmap);
    }
    glLineWidth(vpopt->rangeLinewidth);
    x1 = (xysize[6][0] + xysize[6][1]) * 0.5;
    glBegin(GL_LINES);
    glVertex2f(x1, ymind);
    glVertex2f(x1, ymaxd);
    glEnd();
    UpdateOutput();
    glDisable(GL_LINE_STIPPLE);
    glLineWidth(vpopt->frameLinewidth);
    // the arrows
    // (assuming that concave polygons doesn't work properly)
    glShadeModel(GL_FLAT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor3ubv(c2.RGB());
    dx = (xysize[6][1] - xysize[6][0]) * 0.4;
    if (dx > chxlab * 0.8)
      dx = chxlab * 0.8;
    dy = (xysize[6][3] - ymaxd) * 0.5;
    if (dy > chylab * 0.9)
      dy = chylab * 0.9;
    yc = (ymaxd + xysize[6][3]) * 0.5;
    // down arrow (sinking motion, omega>0 !)
    xc = (xysize[6][0] + x1) * 0.5;
    glBegin(GL_POLYGON);
    glVertex2f(xc - dx * 0.4, yc + dy);
    glVertex2f(xc - dx * 0.4, yc);
    glVertex2f(xc + dx * 0.4, yc);
    glVertex2f(xc + dx * 0.4, yc + dy);
    glEnd();
    glBegin(GL_POLYGON);
    glVertex2f(xc - dx, yc);
    glVertex2f(xc, yc - dy);
    glVertex2f(xc + dx, yc);
    glEnd();
    UpdateOutput();
    // up arrow (raising motion, omega<0 !)
    xc = (xysize[6][1] + x1) * 0.5;
    glBegin(GL_POLYGON);
    glVertex2f(xc - dx * 0.4, yc);
    glVertex2f(xc - dx * 0.4, yc - dy);
    glVertex2f(xc + dx * 0.4, yc - dy);
    glVertex2f(xc + dx * 0.4, yc);
    glEnd();
    glBegin(GL_POLYGON);
    glVertex2f(xc - dx, yc);
    glVertex2f(xc, yc + dy);
    glVertex2f(xc + dx, yc);
    glEnd();
    UpdateOutput();
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // number showing the x-axis range of omega (+ hpa/s -)
    ostringstream ostr;
    ostr << vpopt->rvwind;
    std::string str = ostr.str();
    k = str.length();
    dx = xysize[6][1] - xysize[6][0];
    chx = chxlab;
    chy = chylab;
    if (chx * (k + 4) > dx) {
      chx = dx / (k + 4);
      chy = chx * chylab / chxlab;
    }
    //     setFontsize(chy);
    //     fp->drawStr("+",xysize[6][0]+chx*0.3,ymind-chy*1.2,0.);
    //     fp->drawStr("-",xysize[6][1]-chx*1.3,ymind-chy*1.2,0.);
    fpInitStr("+", xysize[6][0] + chx * 0.3, ymind - chy * 1.2, 0., chy, c2);
    fpInitStr("-", xysize[6][1] - chx * 1.3, ymind - chy * 1.2, 0., chy, c2);
    //     fp->getStringSize(str.c_str(), cw, ch);
    //     fp->drawStr(str.c_str(),xysize[6][0]+(dx-cw)*0.5,ymind-chy*1.2,0.);
    fpInitStr(str.c_str(), xysize[6][0] + dx * 0.5, ymind - chy * 1.2, 0., chy,
        c2, "x-w*0.5");
    //     fp->getStringSize("hPa/s", cw, ch);
    //     fp->drawStr("hPa/s",xysize[6][0]+(dx-cw)*0.5,ymind-chy*2.4,0.);
    fpInitStr("hPa/s", xysize[6][0] + dx * 0.5, ymind - chy * 2.4, 0., chy, c2,
        "x-w*0.5");
    glColor3ubv(c.RGB());
    sepline = true;
    UpdateOutput();
  }

  // relative humidity
  if (vpopt->prelhum) {
    glBegin(GL_LINES);
    if (sepline) {
      glVertex2f(xysize[7][0], xysize[9][3]);
      glVertex2f(xysize[7][0], xysize[0][3]);
    }
    glVertex2f(xysize[7][0], ymaxd);
    glVertex2f(xysize[7][1], ymaxd);
    glVertex2f(xysize[7][0], ymind);
    glVertex2f(xysize[7][1], ymind);
    glEnd();
    UpdateOutput();
    linetype = Linetype(vpopt->rangeLinetype);
    if (linetype.stipple) {
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(linetype.factor, linetype.bmap);
    }
    glLineWidth(vpopt->rangeLinewidth);
    glBegin(GL_LINES);
    dx = xysize[7][1] - xysize[7][0];
    for (n = 25; n <= 75; n += 25) {
      x1 = xysize[7][0] + dx * 0.01 * n;
      glVertex2f(x1, ymind);
      glVertex2f(x1, ymaxd);
    }
    glEnd();
    UpdateOutput();
    glDisable(GL_LINE_STIPPLE);
    // frameLinetype...
    glLineWidth(vpopt->frameLinewidth);
    chx = chxlab;
    chy = chylab;
    if (chx * 3. > dx) {
      chx = dx / 3.;
      chy = chx * chylab / chxlab;
    }
    //     setFontsize(chy);
    x = (xysize[7][0] + xysize[7][1]) * 0.5;
    //      fp->drawStr("RH",x-chx,ymaxd+0.25*chy,0.0);
    fpInitStr("RH", x - chx, ymaxd + 0.25 * chy, 0.0, chy, c);
    if (chx * 6. > dx) {
      chx = dx / 6.;
      chy = chx * chylab / chxlab;
      //       setFontsize(chy);
    }
    glColor3ubv(c2.RGB());
    //      fp->drawStr("0",xysize[7][0]+chx*0.3,ymind-1.25*chy,0.0);
    fpInitStr("0", xysize[7][0] + chx * 0.3, ymind - 1.25 * chy, 0.0, chy, c2);
    //      fp->drawStr("100",xysize[7][1]-chx*3.3,ymind-1.25*chy,0.0);
    fpInitStr("100", xysize[7][1] - chx * 3.3, ymind - 1.25 * chy, 0.0, chy, c2);
    glColor3ubv(c.RGB());
    sepline = true;
    UpdateOutput();
  }

  // ducting
  if (vpopt->pducting) {
    glBegin(GL_LINES);
    if (sepline) {
      glVertex2f(xysize[8][0], xysize[9][3]);
      glVertex2f(xysize[8][0], xysize[0][3]);
    }
    glVertex2f(xysize[8][0], ymaxd);
    glVertex2f(xysize[8][1], ymaxd);
    glVertex2f(xysize[8][0], ymind);
    glVertex2f(xysize[8][1], ymind);
    glEnd();
    UpdateOutput();
    linetype = Linetype(vpopt->rangeLinetype);
    if (linetype.stipple) {
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(linetype.factor, linetype.bmap);
    }
    glLineWidth(vpopt->rangeLinewidth);
    glBegin(GL_LINES);
    dx = xysize[8][1] - xysize[8][0];
    float rd = vpopt->ductingMax - vpopt->ductingMin;
    int istep = 50;
    if (rd > 290.)
      istep = 100;
    if (rd > 490.)
      istep = 200;
    for (n = -800; n <= 800; n += istep) {
      x1 = xysize[8][1] + float(n) * dx / rd;
      if (x1 > xysize[8][0] && x1 < xysize[8][1]) {
        glVertex2f(x1, ymind);
        glVertex2f(x1, ymaxd);
      }
    }
    glEnd();
    UpdateOutput();
    glDisable(GL_LINE_STIPPLE);
    // frameLinetype...
    glLineWidth(vpopt->frameLinewidth);
    chx = chxlab;
    chy = chylab;
    if (chx * 5. > dx) {
      chx = dx / 5.;
      chy = chx * chylab / chxlab;
    }
    x = (xysize[8][0] + xysize[8][1]) * 0.5;
    fpInitStr("DUCT", x - chx * 2., ymaxd + 0.25 * chy, 0.0, chy, c);
    std::string str1 = miutil::from_number(vpopt->ductingMin);
    std::string str3 = miutil::from_number(vpopt->ductingMax);
    float ch = chy;
    glColor3ubv(c2.RGB());
    fpInitStr(str1.c_str(), xysize[8][0], ymind - 1.25 * chy, 0.0, ch, c2,
        "ductingmin");
    fpInitStr(str3.c_str(), xysize[8][1], ymind - 1.25 * chy, 0.0, ch, c2,
        "ductingmax");
    glColor3ubv(c.RGB());
    sepline = true;
    UpdateOutput();
  }

  // line between diagram and text
  if (vpopt->ptext) {
    glBegin(GL_LINES);
    glVertex2f(xysize[0][0], xysize[9][3]);
    glVertex2f(xysize[0][1], xysize[9][3]);
    glEnd();
    UpdateOutput();
  }

  // outer frame
  if (vpopt->pframe) {
    glBegin(GL_LINE_LOOP);
    glVertex2f(xysize[0][0], xysize[0][2]);
    glVertex2f(xysize[0][1], xysize[0][2]);
    glVertex2f(xysize[0][1], xysize[0][3]);
    glVertex2f(xysize[0][0], xysize[0][3]);
    glEnd();
    UpdateOutput();
  }

  UpdateOutput();
}

void VprofDiagram::fpInitStr(const std::string& str, float x, float y, float z,
    float size, Colour c, std::string format, std::string font)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofDiagram::fpInitStr");
#endif

  fpStrInfo strInfo;
  strInfo.str = str;
  strInfo.x = x;
  strInfo.y = y;
  strInfo.z = z;
  strInfo.c = c;
  strInfo.size = size;
  strInfo.format = format;
  strInfo.font = font;
  fpStr.push_back(strInfo);

}

void VprofDiagram::fpDrawStr(bool first)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofDiagram::fpDrawStr");
#endif
  float w, h;
  int n = fpStr.size();
  for (int i = 0; i < n; i++) {
    if ((not fpStr[i].font.empty())) {
      fp->setFont(fpStr[i].font);
    }
    setFontsize(fpStr[i].size);
    glColor3ubv(fpStr[i].c.RGB());
    if (first && (not fpStr[i].format.empty())) {
      fp->getStringSize(fpStr[i].str.c_str(), w, h);
      if (fpStr[i].format == "x-w")
        fpStr[i].x -= w;
      if (fpStr[i].format == "x-w*0.5")
        fpStr[i].x -= w * 0.5;
      if (miutil::contains(fpStr[i].format, "ducting")) {
        std::string str2 = std::string("0");
        float w2, h2;
        fp->getStringSize(str2.c_str(), w2, h2);
        if (miutil::contains(fpStr[i].format, "min"))
          fpStr[i].x += w2 * 0.3;
        else
          fpStr[i].x -= w + w2 * 0.3;
      }
    }
    fp->drawStr(fpStr[i].str.c_str(), fpStr[i].x, fpStr[i].y, fpStr[i].z);
    //reset font
    fp->setFont(defaultFont);
  }
}

void VprofDiagram::plotText()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofDiagram::plotText");
#endif

  if (vpopt->ptext) {

    int i, n = vptext.size();

    setFontsize(chytxt);

    float wspace, w, h;
    fp->getStringSize("oo", wspace, h);

    vector<std::string> fctext(n);
    vector<std::string> geotext(n);
    vector<std::string> kitext(n);
    float wmod = 0., wpos = 0., wfc = 0., wgeo = 0., wtime = 0.;

    for (i = 0; i < n; i++) {
      fp->getStringSize(vptext[i].modelName.c_str(), w, h);
      if (wmod < w)
        wmod = w;
      fp->getStringSize(vptext[i].posName.c_str(), w, h);
      if (wpos < w)
        wpos = w;
    }
    for (i = 0; i < n; i++) {
      if (vptext[i].prognostic) {
        ostringstream ostr;
        ostr << "(" << setiosflags(ios::showpos) << vptext[i].forecastHour
            << ")";
        fctext[i] = ostr.str();
        fp->getStringSize(fctext[i].c_str(), w, h);
        if (wfc < w)
          wfc = w;
      }
    }
    if (vpopt->pgeotext) {
      for (i = 0; i < n; i++) {
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
        fp->getStringSize(geotext[i].c_str(), w, h);
        if (wgeo < w)
          wgeo = w;
      }
    }
    if (vpopt->pkindex) {
      for (i = 0; i < n; i++) {
        if (vptext[i].kindexFound) {
          float v = vptext[i].kindexValue;
          int k = (v > 0.) ? int(v + 0.5) : int(v - 0.5);
          ostringstream ostr;
          ostr << "K= " << setiosflags(ios::showpos) << k;
          kitext[i] = ostr.str();
        }
      }
    }

    int ltime = 16;
    fp->getStringSize("2222-22-22 23:59", wtime, h);

    float xmod = xysize[9][0] + chxtxt * 0.5;
    float xpos = xmod + wmod + wspace;
    float xfc = xpos + wpos + wspace;
    float xtime = (wfc > 0.0) ? xfc + wfc + wspace * 0.6 : xfc;
    float xgeo = xtime + wtime + wspace;
    float xkindex = (vpopt->pgeotext) ? xgeo + wgeo + wspace : xgeo;
    float y;

    for (int i = 0; i < n; i++) {
      glColor3ubv(vptext[i].colour.RGB());
      y = xysize[9][3] - chytxt * 1.5 * (vptext[i].index + 1);
      fp->drawStr(vptext[i].modelName.c_str(), xmod, y, 0.0);
      fp->drawStr(vptext[i].posName.c_str(), xpos, y, 0.0);
      if (vptext[i].prognostic)
        fp->drawStr(fctext[i].c_str(), xfc, y, 0.0);
      std::string tstr = vptext[i].validTime.isoTime().substr(0, ltime);
      fp->drawStr(tstr.c_str(), xtime, y, 0.0);
      if (vpopt->pgeotext)
        fp->drawStr(geotext[i].c_str(), xgeo, y, 0.0);
      if (vpopt->pkindex && vptext[i].kindexFound)
        fp->drawStr(kitext[i].c_str(), xkindex, y, 0.0);
    }

  }

  vptext.clear();
}
