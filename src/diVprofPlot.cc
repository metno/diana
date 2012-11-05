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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <sstream>
#include <diVprofPlot.h>

#include <diColour.h>

#include <math.h>

using namespace std; using namespace miutil;

//#define DEBUGPRINT 1

// Default constructor
VprofPlot::VprofPlot()
: VprofTables(), windInKnots(true)
{
#ifdef DEBUGPRINT
  cerr << "++ VprofPlot::Default Constructor" << endl;
#endif
}


// Destructor
VprofPlot::~VprofPlot() {
#ifdef DEBUGPRINT
  cerr << "++ VprofPlot::Destructor" << endl;
#endif
}


bool VprofPlot::plot(VprofOptions *vpopt, int nplot)
{
#ifdef DEBUGPRINT
  cerr << "++ VprofPlot::plot " << nplot << endl;
#endif

  if (!text.posName.exists()) return false;

#ifdef DEBUGPRINT
  cerr << "++ VprofPlot::plot start plotting " << text.posName << endl;
#endif

  const float dptab=idptab;
  const float dpinv=1./dptab;
  //const float rad=3.141592654/180.;
  //const float undef=+1.e+35;
  const float dx1deg= dx1degree;

  unsigned int i, nlevel=0;
  float x,y;
  float *xx= new float[maxLevels];
  float *xz= new float[maxLevels];
  float *yy= new float[maxLevels];

  i= vpopt->dataColour.size();
  if (i>vpopt->dataLinewidth.size()) i= vpopt->dataLinewidth.size();
  if (i>vpopt->windLinewidth.size()) i= vpopt->windLinewidth.size();
  i= nplot%i;

  Colour c= Colour(vpopt->dataColour[i]);
  glColor3ubv(c.RGB());

  float dataWidth= vpopt->dataLinewidth[i];
  float windWidth= vpopt->windLinewidth[i];
  glLineWidth(dataWidth);

  // levels for T (always)
  if (ptt.size()>0) {
    nlevel= ptt.size();
#ifdef DEBUGPRINT
	cerr << "ptt.size()," << nlevel << endl;
#endif
	for (unsigned int k=0; k<nlevel; k++) {
#ifdef DEBUGPRINT
	  cerr << ptt[k] << endl;
#endif
      x= ptt[k]*dpinv;
      i= int(x);
      yy[k]= yptab[i]+(yptab[i+1]-yptab[i])*(x-i);
      xz[k]= xztab[i]+(xztab[i+1]-xztab[i])*(x-i);
    }
  }

  // T
  if (vpopt->ptttt) {
    for (unsigned int k=0; k<nlevel; k++)
      xx[k]= xz[k]+dx1deg*tt[k];
    xyclip(nlevel,xx,yy,&xysize[1][0]);
    UpdateOutput();
  }

  // levels for Td (if not same as T levels)
  if (vpopt->ptdtd && ptd.size()>0) {
    nlevel= ptd.size();
#ifdef DEBUGPRINT
	cerr << "ptd.size()," << nlevel << endl;
#endif
    for (unsigned int k=0; k<nlevel; k++) {
#ifdef DEBUGPRINT
      cerr << ptd[k] << endl;
#endif
      x= ptd[k]*dpinv;
      i= int(x);
      yy[k]= yptab[i]+(yptab[i+1]-yptab[i])*(x-i);
      xz[k]= xztab[i]+(xztab[i+1]-xztab[i])*(x-i);
    }
  }

  // Td
  if (vpopt->ptdtd && td.size()>0) {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1,0xFFC0);
    for (unsigned int k=0; k<nlevel; k++)
      xx[k]= xz[k]+dx1deg*td[k];
    xyclip(nlevel,xx,yy,&xysize[1][0]);
    UpdateOutput();
    glDisable(GL_LINE_STIPPLE);
  }



  // levels for wind and significant levels (if not same as T levels)
  if ((vpopt->pwind || vpopt->pslwind) && puv.size()>0) {
    nlevel= puv.size();
#ifdef DEBUGPRINT
	cerr << "puv.size()," << nlevel << endl;
#endif
    for (unsigned int k=0; k<nlevel; k++) {
#ifdef DEBUGPRINT
	  cerr << puv[k] << endl;
#endif
      x= puv[k]*dpinv;
      i= int(x);
      yy[k]= yptab[i]+(yptab[i+1]-yptab[i])*(x-i);
    }
  }

  // wind (u(e/w) and v(n/s)in unit knots)
  if (vpopt->pwind && uu.size()>0) {
    float xw= xysize[5][1] - xysize[5][0];
    float x0= xysize[5][0] + xw*0.5;
    if (vpopt->windseparate) x0+=xw*float(nplot);
    float ylim1= xysize[1][2];
    float ylim2= xysize[1][3];

    int   n,n50,n10,n05;
    float ff,gu,gv,gx,gy,dx,dy,dxf,dyf;
    float flagl = xw * 0.5 * 0.85;
    float flagstep = flagl/10.;
    float flagw = flagl * 0.35;
    float hflagw = 0.6;

    vector<float> vx,vy; // keep vertices for 50-knot flags

    glLineWidth(windWidth);
    glBegin(GL_LINES);

    for (unsigned int k=0; k<nlevel; k++) {
      if (yy[k]>=ylim1 && yy[k]<=ylim2) {
        gx= x0;
        gy= yy[k];
        ff= sqrtf(uu[k]*uu[k]+vv[k]*vv[k]);
        if(!windInKnots)
          ff *= 1.94384; // 1 knot = 1 m/s * 3600s/1852m

        if (ff>0.00001){
          gu= uu[k]/ff;
          gv= vv[k]/ff;

          // find no. of 50,10 and 5 knot flags
          if (ff<182.49) {
            n05  = int(ff*0.2 + 0.5);
            n50  = n05/10;
            n05 -= n50*10;
            n10  = n05/2;
            n05 -= n10*2;
          } else if (ff<190.) {
            n50 = 3;  n10 = 3;  n05 = 0;
          } else if(ff<205.) {
            n50 = 4;  n10 = 0;  n05 = 0;
          } else if (ff<225.) {
            n50 = 4;  n10 = 1;  n05 = 0;
          } else {
            n50 = 5;  n10 = 0;  n05 = 0;
          }

          dx = flagstep*gu;
          dy = flagstep*gv;
          dxf = -flagw*gv - dx;
          dyf =  flagw*gu - dy;

          // direction
          glVertex2f(gx,gy);
          gx = gx - flagl*gu;
          gy = gy - flagl*gv;
          glVertex2f(gx,gy);

          // 50-knot flags, store for plot below
          if (n50>0) {
            for (n=0; n<n50; n++) {
              vx.push_back(gx);
              vy.push_back(gy);
              gx+=dx*2.;  gy+=dy*2.;
              vx.push_back(gx+dxf);
              vy.push_back(gy+dyf);
              vx.push_back(gx);
              vy.push_back(gy);
            }
            gx+=dx; gy+=dy;
          }
          // 10-knot flags
          for (n=0; n<n10; n++) {
            glVertex2f(gx,gy);
            glVertex2f(gx+dxf,gy+dyf);
            gx+=dx; gy+=dy;
          }
          // 5-knot flag
          if (n05>0) {
            if (n50+n10==0) { gx+=dx; gy+=dy; }
            glVertex2f(gx,gy);
            glVertex2f(gx+hflagw*dxf,gy+hflagw*dyf);
          }
        }
      }
    }
    glEnd();
    UpdateOutput();

    unsigned int vi= vx.size();
    if (vi>=3) {
      // draw 50-knot flags
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glBegin(GL_TRIANGLES);
      for (i=0; i<vi; i++)
        glVertex2f(vx[i],vy[i]);
      glEnd();
      UpdateOutput();
    }
    glLineWidth(dataWidth);
  }

  if (vpopt->pslwind && dd.size()>0) {
    // significant wind levels, wind as numbers (temp and prog)
    // (other levels also if space)
    float dchy= chylab*1.3;
    float ylim1= xysize[4][2]+dchy*0.5;
    float ylim2= xysize[4][3]-dchy*0.5;
    int k1= -1;
    int k2= -1;
    for (unsigned int k=0; k<nlevel; k++) {
      if (yy[k]>ylim1 && yy[k]<ylim2) {
        if (k1==-1) k1= k;
        k2= k;
      }
    }

    if (k1>=0) {
      float *used= new float[nlevel];
      unsigned int nused=0;
      x= xysize[4][0]+(xysize[4][1]-xysize[4][0])*nplot+chxlab*0.5;
      setFontsize(chylab);

      for (int sig=3; sig>=0; sig--) {
        for (int k=k1; k<=k2; k++) {
          if (sigwind[k]==sig) {
            ylim1= yy[k]-dchy;
            ylim2= yy[k]+dchy;
            i= 0;
            while (i<nused && (used[i]<ylim1 || used[i]>ylim2)) i++;
            if (i==nused) {
              used[nused++]= yy[k];
              int idd= (dd[k]+5)/10;
              int iff= ff[k];
              if (idd==0 && iff>0) idd=36;
              ostringstream ostr;
              ostr << setw(2) << setfill('0') << idd << "-"
              << setw(3) << setfill('0') << iff;
              miString str= ostr.str();
              y= yy[k]-chylab*0.5;
              fp->drawStr(str.c_str(),x,y,0.0);
            }
          }
        }
      }
      delete[] used;
    }
  }
  UpdateOutput();

  // levels for vertical wind, omega (if not same as T levels)
  if (vpopt->pvwind && pom.size()>0) {
    nlevel= pom.size();
    for (unsigned int k=0; k<nlevel; k++) {
      x= pom[k]*dpinv;
      i= int(x);
      yy[k]= yptab[i]+(yptab[i+1]-yptab[i])*(x-i);
    }
  }

  float xylimit[4] = { 0., 0., xysize[1][2], xysize[1][3] };

  // vertical wind, omega
  if (vpopt->pvwind && om.size()>0) {
    float dx= xysize[6][1] - xysize[6][0];
    float x0= xysize[6][0] + dx*0.5;
    float scale= -dx/vpopt->rvwind;
    for (unsigned int k=0; k<nlevel; k++)
      xx[k]= x0 + scale*om[k];
    xylimit[0]= xysize[6][0];
    xylimit[1]= xysize[6][1];
    xyclip(nlevel,xx,yy,xylimit);
    UpdateOutput();
  }

  if (!prognostic && (vpopt->prelhum || vpopt->pducting)) {
    nlevel= pcom.size();
    for (unsigned int k=0; k<nlevel; k++) {
      x= pcom[k]*dpinv;
      i= int(x);
      yy[k]= yptab[i]+(yptab[i+1]-yptab[i])*(x-i);
    }
  }

  // levels for relative humidity (same as T levels)
  if (vpopt->prelhum && ptt.size()>0 && (ptt.size() == ptd.size())) {
    nlevel= ptt.size();
    for (unsigned int k=0; k<nlevel; k++) {
      x= ptt[k]*dpinv;
      i= int(x);
      yy[k]= yptab[i]+(yptab[i+1]-yptab[i])*(x-i);
    }
  }

  // relative humidity
  if (vpopt->prelhum) {
    float dx= xysize[7][1] - xysize[7][0];
    float x0= xysize[7][0];
    float scale= dx/100.;
    if (rhum.size()==0) {
      if (prognostic)
        relhum(tt,td);
      else
        relhum(tcom,tdcom);
    }
    if (rhum.size()==nlevel) {
      for (unsigned int k=0; k<nlevel; k++)
        xx[k]= x0 + scale*rhum[k];
      xylimit[0]= xysize[7][0];
      xylimit[1]= xysize[7][1];
      xyclip(nlevel,xx,yy,xylimit);
    }
    UpdateOutput();
  }

  // levels for ducting (same as T levels)
  if (vpopt->pducting && ptt.size()>0 && (ptt.size() == ptd.size())) {
    nlevel= ptt.size();
    for (unsigned int k=0; k<nlevel; k++) {
      x= ptt[k]*dpinv;
      i= int(x);
      yy[k]= yptab[i]+(yptab[i+1]-yptab[i])*(x-i);
    }
  }

  // ducting
  if (vpopt->pducting) {
    float rd= vpopt->ductingMax - vpopt->ductingMin;
    float dx= xysize[8][1] - xysize[8][0];
    float x0= xysize[8][0] + dx * (0.0 - vpopt->ductingMin)/rd;
    float scale= dx/rd;
    if (duct.size()==0) {
      if (prognostic)
        ducting(ptt,tt,td);
      else
        ducting(pcom,tcom,tdcom);
    }
    if (duct.size()==nlevel) {
      for (unsigned int k=0; k<nlevel; k++)
        xx[k]= x0 + scale*duct[k];
      xylimit[0]= xysize[8][0];
      xylimit[1]= xysize[8][1];
      xyclip(nlevel,xx,yy,xylimit);
    }
    UpdateOutput();
  }

  if (vpopt->pkindex && !text.kindexFound) {
    if (prognostic)
      kindex(ptt,tt,td);
    else
      kindex(pcom,tcom,tdcom);
    UpdateOutput();
  }

  // text
  text.index= nplot;
  text.colour= c;
  vptext.push_back(text);

  UpdateOutput();

  delete[] xx;
  delete[] xz;
  delete[] yy;

  return true;
}


void VprofPlot::relhum(const vector<float>& tt,
    const vector<float>& td) {
#ifdef DEBUGPRINT
  cerr << "++ VprofPlot::relhum(...)" << endl;
#endif

  int nlev= tt.size();
  rhum.resize(nlev);
  int i;
  float x,et,etd,rhx;

  for (int k=0; k<nlev; k++) {
    x= (tt[k]+100.)*0.2;
    i= int(x);
    if (i<0) i= 0;
    if (i>mewtab-2) i= mewtab-2;
    et= ewt[i]+(ewt[i+1]-ewt[i])*(x-i);
    x= (td[k]+100.)*0.2;
    i= int(x);
    if (i<0) i= 0;
    if (i>mewtab-2) i= mewtab-2;
    etd= ewt[i]+(ewt[i+1]-ewt[i])*(x-i);
    rhx= 100.*etd/et;
    if (rhx<  0.) rhx=   0.;
    if (rhx>100.) rhx= 100.;
    rhum[k]= rhx;
  }
}


void VprofPlot::ducting(const vector<float>& pp,
    const vector<float>& tt,
    const vector<float>& td) {
#ifdef DEBUGPRINT
  cerr << "++ VprofPlot::ducting(...)" << endl;
#endif

  // p,t,td -> ducting index
  //
  // t and td in degrees celsius (and at the same levels)
  //
  // ducting = 77.6*(p/t)+373000.*(q*p)/(eps*t*t)
  //
  // q*p/eps = rh*qsat*p/eps = rh*(eps*e(t)/p)*p/eps
  //         = rh*e(t) = (e(td)/e(t))*e(t) = e(td)
  //
  //    =>  ducting = 77.6*(p/t)+373000.*e(td)/(t*t)
  //
  // output: duct(k) = (ducting(k+1)-ducting(k))/(dz*0.001) + 157.

  const float g= 9.8;
  const float r= 287.;
  const float cp= 1004.;
  const float p0= 1000.;
  const float t0= 273.15;
  const float ginv= 1./g;
  const float rcp = r/cp;
  const float p0inv= 1./p0;

  int nlev= pp.size();
  if (nlev<2) {
    duct.clear();
    return;
  }

  duct.resize(nlev);
  int   i,k;
  float x,etd,tk,pi1,pi2,th1,th2,dz;

  for (k=0; k<nlev; k++) {
    x= (td[k]+100.)*0.2;
    i= int(x);
    if (i<0) i= 0;
    if (i>mewtab-2) i= mewtab-2;
    etd= ewt[i]+(ewt[i+1]-ewt[i])*(x-i);
    tk= tt[k] + t0;
    ////duct[k]= 77.6*(pp[k]/tk) + 373000.*etd/(tk*tk);
    duct[k]= 77.6*(pp[k]/tk) + 373256.*etd/(tk*tk);
  }

  pi2= cp*powf(pp[0]*p0inv,rcp);
  th2= cp*(tt[0]+t0)/pi2;

  for (k=1; k<nlev; k++) {
    pi1= pi2;
    th1= th2;
    pi2= cp*powf(pp[k]*p0inv,rcp);
    th2= cp*(tt[k]+t0)/pi2;
    dz= (th1+th2)*0.5*(pi1-pi2)*ginv;
    ////duct[k-1]= (duct[k]-duct[k-1])/(dz*0.001);
    duct[k-1]= (duct[k]-duct[k-1])/(dz*0.001) + 157.;
  }

  if (pp[0]>pp[nlev-1]) {
    duct[nlev-1]= duct[nlev-2];
  } else {
    for (k=nlev-1; k>0; k--)
      duct[k]= duct[k-1];
    duct[0]= duct[1];
  }
}


void VprofPlot::kindex(const vector<float>& pp,
    const vector<float>& tt,
    const vector<float>& td) {

  // K-index = (t+td)850 - (t-td)700 - (t)500

#ifdef DEBUGPRINT
  cerr << "++ VprofPlot::kindex(...)" << endl;
#endif

  const float dptab=idptab;
  const float dpinv=1./dptab;
  const float pfind[3]= { 850., 700., 500. };

  float tfind[3], tdfind[3];

  text.kindexFound= false;

  int nlev= pp.size();
  if (nlev<2) return;

  bool pIncreasing= (pp[0]<pp[nlev-1]);
  if (pIncreasing) {
    if (pp[0]>500. || pp[nlev-1]<850.) return;
  } else {
    if (pp[0]<850. || pp[nlev-1]>500.) return;
  }

  float x,pi,pi1,pi2;
  int   i,k,n;

  for (n=0; n<3; n++) {
    k= 1;
    if (pIncreasing)
      while (k<nlev && pfind[n]>pp[k]) k++;
    else
      while (k<nlev && pfind[n]<pp[k]) k++;
    if (k==nlev) k--;

    // linear interpolation in exner function
    x= pp[k-1]*dpinv;
    i= int(x);
    pi1= pitab[i]+(pitab[i+1]-pitab[i])*(x-i);
    x= pp[k]*dpinv;
    i= int(x);
    pi2= pitab[i]+(pitab[i+1]-pitab[i])*(x-i);
    x= pfind[n]*dpinv;
    i= int(x);
    pi= pitab[i]+(pitab[i+1]-pitab[i])*(x-i);

    tfind[n]=  tt[k-1] + (tt[k]-tt[k-1])*(pi-pi1)/(pi2-pi1);
    tdfind[n]= td[k-1] + (td[k]-td[k-1])*(pi-pi1)/(pi2-pi1);
  }

  // K-index = (t+td)850 - (t-td)700 - (t)500

  text.kindexValue= (tfind[0]+tdfind[0]) - (tfind[1]-tdfind[1]) - tfind[2];
  text.kindexFound= true;
}
