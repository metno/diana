/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2022 met.no

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

#include "diContouring.h"

#include "diField/diArea.h"
#include "diGLPainter.h"
#include "diGridConverter.h"
#include "diImageGallery.h"
#include "diPlotOptions.h"
#include "polyStipMasks.h"
#include "util/plotoptions_util.h"

#include <mi_fieldcalc/math_util.h>

#include <puTools/miStringFunctions.h>

#include <QPolygonF>
#include <QString>

#include <cmath>
#include <fstream>
#include <set>
#include <sstream>
#include <vector>

#define MILOGGER_CATEGORY "diana.Contouring"
#include <miLogger/miLogging.h>

using namespace miutil;

const int maxLines=1000000;

namespace {
const float RAD_TO_DEG = 180 / M_PI;

/// temporary field contouring info
struct LinePart {
  int   pfirst,plast;
  bool  addfirst,addlast;
  float xfirst,yfirst,xlast,ylast;
  int   connect[2][2];  // connection[0=first/1=last][0=left/1=right]
  bool  closed;
};

/// field contour data and misc info to make closed lines for shading
struct ContourLine {
  bool  original;
  int   ivalue;
  float value;
  bool  highInside;
  bool  highLeft;
  bool  closed;
  int   izleft;
  int   izright;
  int   direction;  // 1=counterclockwise  -1=clockwise  0=no polygon
  int   npos;
  float *xpos;
  float *ypos;
  float xmin;
  float xmax;
  float ymin;
  float ymax;
  //....................................
  bool  undefLine;
  bool  corner;
  int   connect[2][2];  // connection[0=first/1=last][0=left/1=right]
  //....................................
  int   use;
  bool  isoLine;
  bool  undefTouch;
  bool  undefLeft;
  bool  undefRight;
  bool  outer;
  bool  inner;
  std::vector<int> joined;
};

// functions called from contour

/// sorts isolines to contour and may add colour,linetype and width to each
int sortlines(float zvmin, float zvmax, int mvalue, int nvalue,
              float zvalue[], int ivalue[],
              int icolour[], int linetype[], int linewidth[],
              int idraw, float zrange[], float zstep, float zoff,
              int nlines, float rlines[],
              int ncol, int icol[], int ntyp, int ityp[],
              int nwid, int iwid[], int nlim, float rlim[],
              const float& fieldUndef);

/// line smoothing (inserts extra points by a spline methode)
int smoothline(int npos, float x[], float y[], int nfirst, int nlast,
               int ismooth, float xsmooth[], float ysmooth[]);


/// converts field positions to map
void posConvert(int npos, float *x, float *y, float *cxy);
/// converts field positions to map
void posConvert(int npos, float *x, float *y,
                int nx, int ny, const float* xz, const float* yz);
/// joins contour parts (also along undefined areas) to closed contours
void joinContours(std::vector<ContourLine*>& contourlines, int idraw,
                  bool drawBorders, int iconv);
/// find points where a horizontal line crosses a closed contour
std::vector<float> findCrossing(float ycross, int n, float *x, float *y);

/// uses OpenGL tesselation (triangulation) to shade between isolines
void fillContours(DiGLPainter* gl, std::vector<ContourLine*>& contourlines,
                  int nx, int ny, float z[],
                  int iconv, float *cxy, const float* xz, const float* yz, int idraw,
                  const PlotOptions& poptions, bool drawBorders,
                  const Area& fieldArea,
                  float zrange[], float zstep, float zoff,
                  const float& fieldUndef);

/// replaces undefined values with relatively sensible values
void replaceUndefinedValues(int nx, int ny, float *f, bool fillAll,
                            const float& fieldUndef);

/// draw part of contour line
void drawLine(DiPainter* gl, int start, int stop, float* x, float* y);

/// get CL index
void getCLindex(std::vector<ContourLine*>& contourlines, std::vector< std::vector<int> >& clind,
                const PlotOptions& poptions, bool drawBorders, const float& fieldUndef);

} // namespace

bool contour(int nx, int ny, float z[], const float xz[], const float yz[],
    const int ipart[], int icxy, float cxy[], float xylim[],
    int idraw, float zrange[], float zstep, float zoff,
    int nlines, float rlines[],
    int ncol, int icol[], int ntyp, int ityp[],
    int nwid, int iwid[], int nlim, float rlim[],
    int idraw2, float zrange2[], float zstep2, float zoff2,
    int nlines2, float rlines2[],
    int ncol2, int icol2[], int ntyp2, int ityp2[],
    int nwid2, int iwid2[], int nlim2, float rlim2[],
    int ismooth, const int labfmt[], float chxlab, float chylab,
    int ibcol,
    int ibmap, int lbmap, int kbmap[],
    int nxbmap, int nybmap, float rbmap[],
    DiGLPainter* gl, const PlotOptions& poptions,
    const Area& fieldArea, const float& fieldUndef)
{
  //  NAME:
  //     contour
  //
  //  PURPOSE:
  //     Contouring of a scalar field by drawing isolines.
  //     Misc. options for subarea, line smoothing, label drawing,
  //     colour, line type and line width.
  //
  //  GRAPHICS:
  //     OpenGL
  //
  //  SYNOPSIS:
  //     .................................................................
  //     subroutine contour(nx,ny,z,xz,yz,ipart,icxy,cxy,xylim,
  //    +                   idraw,zrange,zstep,zoff,nlines,rlines,
  //    +                   ncol,icol,ntyp,ityp,nwid,rwid,nlim,rlim,
  //    +                  idraw2,zrange2,zstep2,zoff2,nlines2,rlines2,
  //    +                  ncol2,icol2,ntyp2,ityp2,nwid2,rwid2,nlim2,rlim2,
  //    +                   ismooth,labfmt,chxlab,chylab,ibcol,
  //    +                   ibmap,lbmap,kbmap,nxbmap,nybmap,rbmap,
  //    +                   ierror)
  //
  //     integer nx,ny,icxy,idraw,nlines,ncol,ntyp,nwid,nlim,ismooth
  //     integer ibcol,ibmap,lbmap,nxbmap,nybmap
  //     integer ierror
  //     integer ipart(4),icol(ncol),ityp(ntyp),labfmt(3)
  //     integer kbmap(lbmap)
  //     real    zstep,zoff,chxlab,chylab
  //     real    z(nx,ny),xz(nx,ny),yz(nx,ny)
  //     real    cxy(6),xylim(4),rwid(nwid),rlim(nlim),zrange(2)
  //     real    rlines(nlines),rbmap(4)
  //     integer idraw2,nlines2,ncol2,ntyp2,nwid2,nlim2
  //     integer icol2(ncol2),ityp2(ntyp2)
  //     real    zrange2(2),zstep2,zoff2,rlines2(nlines2)
  //     real    rwid2(nwid2),rlim2(nlim2)
  //     .................................................................
  //
  //  INPUT:
  //     nx       : field x dimension
  //     ny       : field y dimension
  //     z        : the field
  //     xz       : map x positions, only used if icxy=2
  //     yz       : map y positions, only used if icxy=2
  //     ipart    : part of field to contour, ix1,ix2,iy1,iy2
  //     icxy     : x,y coordinate conversion from field to map
  //                    icxy=0: map and field coordinates are equal
  //                        =1: using cxy to position field on map
  //                        =2: using x and y to position field on map
  //     cxy      : field positioning on map, only used if icxy=1
  //                    x_plot=cxy[0]+cxy[1]*x_field+cxy[2]*y_field
  //                    y_plot=cxy[3]+cxy[4]*x_field+cxy[5]*y_field
  //     xylim    : map area, xylim(1)=x1 (left)
  //                          xylim(2)=x2 (right)
  //                          xylim(3)=y1 (top)
  //                          xylim(4)=y2 (bottom)
  //
  //     idraw    : 1 = equally spaced lines (value)
  //                2 = equally spaced lines, but not drawing the 0 line
  //                    (or the line at value=zoff)
  //                3 = selected line values (in rlines) drawn
  //                4 = selected line values (the first values in rlines)
  //                    are drawn, the following vales drawn are the
  //                    previous multiplied by 10 and so on
  //                    (nlines=2 rlines=0.1,0.3 => 0.1,0.3,1,3,10,30,...)
  //     zrange   : range of values to contour,
  //                    zrange(1): minimum value
  //                    zrange(2): maximum value
  //                unlimited range if zrange(1)>zrange(2),
  //                only used if idraw=1,2
  //     zstep    : line spacing value, only used if idraw=1,2
  //     zoff     : offset (or zero displacemet), usually 0.0,
  //                (zstep=10.,zoff=0. => lines at ...-10,0,+10,...
  //                 zstep=10.,zoff=5. => lines at ...-5,+5,+15,...)
  //                only used if idraw=1,2
  //     nlines   : no. of selected line values,  only used if idraw=3,4
  //     rlines   : the selected line values, only used if idraw=3,4
  //     ncol     : no. of colours
  //     icol     : the colours
  //     ntyp     : no. of line types
  //     ityp     : the line types
  //     nwid     : no. of line widths
  //     rwid     : the line widths
  //     nlim     : no. of limits for colours, line types and widths
  //     rlim     : the lower limits for colours etc.
  //                (only used if nlim>1, the first value never used)
  //
  //     idraw2   : 0   = no 'secondary' plot specifications
  //                1-4 = 'secondary' plot specification, as idraw
  //     zrange2,zstep2,zoff2,nlines2,rlines2 :
  //                'secondary' plot specifications,
  //                as above (zrange,...rlines)
  //     ncol2,icol2,ntyp2,ityp2,nwid2,rwid2,nlim2,rlim2 :
  //                'secondary' plot specifications,
  //                as above (ncol,...rlim)
  //
  //     labfmt   : contour label format (numbers on the lines),
  //                   labfmt(1)=-2: automatic format (one format)
  //                            =-1: automatic format (for each label)
  //                            = 0: no labels
  //                            = 1: fortran I format (e.g. I6)
  //                            = 2: fortran F format (e.g. F6.2)
  //                            = 3: fortran E format (e.g. E12.5.e1)
  //                            = 4: fortran E format (e.g. E12.5.e2)
  //                   labfmt(2):    field width (e.g. 6 in I6 and F6.2)
  //                   labfmt(3):    fraction field (e.g. 2 in F6.2)
  //     ismooth  : smoothing of lines, 1,2,3,..., 0=no smoothing
  //     chxlab   : character x size for labels
  //     chylab   : character y size for labels
  //     ibcol    : blank out background when plotting labels,
  //                   ibcol<0  : no blanking (standard)
  //                   ibcol>=0 : blank background with this colour (0 is
  //                              usually the background colour index)
  //     ibmap    : usage of bit map to avoid plotting contour labels on
  //                top of labels from other fields (or other stuff),
  //                   ibmap=0 : off
  //                   ibmap=1 : on
  //     lbmap    : length of bit map array (words)
  //     kbmap    : the bit map, only used if ibmap=1
  //     nxbmap   : bit map size in x direction (bits)
  //     nybmap   : bit map size in y direction (bits)
  //     rbmap    : bit map positioning, only used if ibmap=1,
  //                  rbmap(1): x minimum (<=xylim(1))
  //                  rbmap(2): y minimum (<=xylim(3))
  //                  rbmap(3): x resolution
  //                  rbmap(4): y resolution
  //                reasonable resolution:  rbmap(3)=rbmap(4)=chylab/2
  //                index in bit map: ixbmap=int((x-rbmap(1))/rbmap(3))+1
  //                                  iybmap=int((y-rbmap(2))/rbmap(3))+1
  //
  //  OUTPUT:
  //     ierror   : 0 = no error
  //                1 = too small work arrays
  //                2 = too many line values to contour
  //                3 = unknown coordinate transformation (icxy)
  //
  //  NOTES:
  //     The 'secondary' plot options may be used to plot extra lines,
  //     or modify colour, line type or width for some of the lines.
  //     No isoline will be plotted more than once.
  //     Overlapping of contours is possible when smoothing lines.
  //
  //-----------------------------------------------------------------------
  //  DNMI/FoU  xx.xx.198x  Anstein Foss ... based on horrible GPGS source
  //  DNMI/FoU  xx.xx.1987  Anstein Foss ... speedup (drawing polylines)
  //  DNMI/FoU  xx.xx.199x  Anstein Foss ... misc. improvements
  //  DNMI/FoU  xx.xx.199x  Anstein Foss ... x,y conversions
  //  DNMI/FoU  08.03.1996  Anstein Foss ... contour,conperf routines
  //  DNMI/FoU  22.03.1996  Anstein Foss ... misc. improvements
  //  DNMI/FoU  25.03.1996  Anstein Foss ... the con1pos routine etc.
  //  DNMI/FoU  05.05.1999  Anstein Foss ... 'secondary' options, idraw2,..
  //  DNMI/FoU  18.08.1999  Anstein Foss ... C/C++ and OpenGL + glText/DNMI
  //  DNMI/FoU  04.03.2000  Anstein Foss ... using (some) PlotOptions
  //  DNMI/FoU  09.03.2000  Anstein Foss ... using icol,ityp,iwid as indecies
  //  DNMI/FoU  20.11.2002  Anstein Foss ... shading (demo colours only)
  //  DNMI/FoU  22.05.2004  Anstein Foss ... class borders
  //-----------------------------------------------------------------------

  // max no. of isoline values to draw
  const int mvalue = 800;

  // partitioning field in max mblock*mblock parts
  const int mblock = 12;

  // no. of bits per int word
  const int nbitwd = sizeof(int)*8;

  const int ijcorner[8][2] = { {1,0}, {0,0}, {0,1}, {1,1},
      {1,0}, {0,1}, {0,0}, {1,1} };
  const int initmove[4][2] = { {0,-1}, {-1,0}, {0,+1}, {+1,0} };

  const int isides[8] = { 0,1,2,3,0,1,2,3 };
  const int newside[2][4][3] = { { {1,3,2}, {2,0,3}, {3,1,0}, {0,2,1}},
      {{3,1,2}, {0,2,3}, {1,3,0},{ 2,0,1 }}};
  const int newtest[2][4] = { {3,2,1,0}, {1,0,3,2} };

  const float xfrac[8] = { -1., 0.,+1., 0.,   -1., 0.,+1., 0. };
  const float yfrac[8] = {  0.,+1., 0.,-1.,   +1., 0.,+1., 0. };

  float zvalue[mvalue+1];
  int   icolour[mvalue+1], linetype[mvalue+1], linewidth[mvalue+1];
  int   ivalue[mvalue+1];

  int   iundef[mblock][mblock];
  float zmaxb[mblock][mblock], zminb[mblock][mblock];

  int   iab[4], iaddbit[4], ijmove[8][2], izg[2][2], jzg[2][2];
  float zz[8], zzg[2][2], xlc[5], ylc[5], xlb[4], ylb[4];
  float dclab[4] = {0,0,0,0};

  int   bit[nbitwd];

  int   err, i, j, k, k1, k2=0, l, n, n1, n2, n3, np1, np2, label;
  int   ix1, ix2, iy1, iy2, ix0, iy0, ixrange, iyrange;
  int   maxpos, mpos, liused, ibsize, jbsize, nxbox, nybox;
  int   iconv, nnfrmt, nnwdth, nzfrmt, nlabx=0, nlaby=0;
  int   ib, jb, istart, jstart, iend, jend, iud, nvalue;
  int   icolx, ltypx, iwidx, lev, iabeq;
  int   istartl, jstartl=0, ibox1, jbox1, kbox, iz1, iz2, jz1, jz2;
  int   ibdim, ij, ik, ibit, iwrd, ibit2, iwrd2,  newend;
  int   iend1=0, iend2=0, jend1=0, jend2=0, iside, iside1;
  int   npos, nposm1, linend, ic1, ic2, ic3, ic4, iab1, iab2;
  int   ns, np, nfrst, nlast, km, ni, nj, iz, jz, nzz, nspace2;
  int   nbx, nby, ibm, jbm;
  float chrx=0, chry, rchrx, rchry, zhigh;
  float xlim1, xlim2, ylim1, ylim2, dxeq, dyeq;
  float slab1f, dslab=0, stalab=0, endlab=0, dylab=0, dylabh=0, dxlab1, dxlab=0;
  float dxlab2, splim2=0, zmin, zmax, zminbl, zmaxbl, zmean;
  float zzstp=0, zzstp1, zzstp2, zc, ftmp, ftmp1, ftmp2, dxx=0, dyy=0;
  float zz1, zz2, xh, yh, frac, dzzza, dzzzb, zavg;
  float sp2, sp2lim, smax, stest, dxylab, space, space2, arc, dxz, dyz;
  float dzdx, dzdy, dzdg, ratio, xs, xs1, xs2, xe, xe1, xe2, angle;
  float ys, ys1, ys2, ye, ye1, ye2;
  float xlab=0, ylab=0, xlabel=0, ylabel=0, xend=0, yend=0;
  float xbmap, ybmap, dxbx, dybx, dxby, dyby, xbm, ybm;
  float gx1, gx2, gy1, gy2;

  float *x=NULL, *y=NULL, *s=NULL, *xsmooth=NULL, *ysmooth=NULL;
  int   *iused=NULL, *indxgx=NULL, *indxgy=NULL, *kab=NULL;

  std::vector<ContourLine*> contourlines;
  bool highLeft;
  bool closed;

  bool drawBorders = poptions.discontinuous;
  bool shading = poptions.contourShading;

  if (drawBorders) {
    if (idraw==1 || idraw==2) {
      i= int(zstep+0.5);
      if (fabsf(zstep-float(i))>0.01) zstep= float(i);
    } else if (idraw==3 || idraw==4) {
      for (j=0; j<nlines; j++) {
        i= int(rlines[j]+0.5);
        if (fabsf(rlines[j]-float(i))>0.01) rlines[j]= float(i);
      }
    }
    if (idraw2==1 || idraw2==2) {
      i= int(zstep2+0.5);
      if (fabsf(zstep2-float(i))>0.01) zstep2= float(i);
    } else if (idraw2==3 || idraw2==4) {
      for (j=0; j<nlines2; j++) {
        i= int(rlines2[j]+0.5);
        if (fabsf(rlines2[j]-float(i))>0.01) rlines2[j]= float(i);
      }
    }
  }

  if (drawBorders) ismooth=0;

  // check input

  err = 0;

  // Impose minimum and maximum limits on the size of the array used.
  if (nx<2 || nx>100000 ||
      ny<2 || ny>100000) {
    METLIBS_LOG_WARN("CONTOUR ERROR. nx,ny: " << nx << " " << ny);
    err = 1;
  }

  if (icxy<0 || icxy>2) {
    METLIBS_LOG_WARN("CONTOUR ERROR. icxy: " << icxy);
    err = 1;
  }

  if (ipart[0]<0 || ipart[0]>=ipart[1] || ipart[1]>nx-1 ||
      ipart[2]<0 || ipart[2]>=ipart[3] || ipart[3]>ny-1) {
    METLIBS_LOG_WARN("CONTOUR ERROR. ipart: "
    << ipart[0] << " " << ipart[1] << " "
    << ipart[2] << " " << ipart[3]);
    err = 1;
  }

  if (xylim[0]>=xylim[1] || xylim[2]>=xylim[3]) {
    METLIBS_LOG_WARN("CONTOUR ERROR. xylim: "
    << xylim[0] << " " << xylim[1] << " "
    << xylim[2] << " " << xylim[3]);
    err = 1;
  }

  if (idraw<0 || idraw>4) {
    METLIBS_LOG_WARN("CONTOUR ERROR. idraw: " << idraw);
    err = 1;
  }

  if (ncol<1 || ntyp<1 || nwid<1) {
    METLIBS_LOG_WARN("CONTOUR ERROR. ncol,ntyp,nwid: "
    << ncol << " " << ntyp << " " << nwid);
    err = 1;
  }

  if ((idraw==1 || idraw==2) && zstep<=0.0) {
    METLIBS_LOG_WARN("CONTOUR ERROR. zstep: " << zstep);
    err = 1;
  }

  if (idraw==3 || idraw==4) {
    if (nlines<1 || nlines>mvalue) {
      METLIBS_LOG_WARN("CONTOUR ERROR. nlines: " << nlines);
      err = 1;
    }
    else {
      k = 1;
      for (n=1; n<nlines; ++n) if (rlines[n]<=rlines[n-1]) k=0;
      if (k==0) {
        METLIBS_LOG_WARN("CONTOUR ERROR. sequence of line values.");
        err = 1;
      }
    }
  }

  if (idraw2>0)
  {
    if (idraw2<1 || idraw2>4) {
      METLIBS_LOG_WARN("CONTOUR ERROR. idraw(2): " << idraw2);
      err = 1;
    }

    if ((idraw2==1 || idraw2==2) && zstep2<=0.0) {
      METLIBS_LOG_WARN("CONTOUR ERROR. zstep(2): " << zstep2);
      err = 1;
    }

    if (idraw2==3 || idraw2==4) {
      if (nlines2<1 || nlines2>mvalue) {
        METLIBS_LOG_WARN("CONTOUR ERROR. nlines(2): " << nlines2);
        err = 1;
      }
      else {
        k = 1;
        for (n=1; n<nlines2; ++n) if (rlines2[n]<=rlines2[n-1]) k=0;
        if (k==0) {
          METLIBS_LOG_WARN("CONTOUR ERROR. sequence of line values(2).");
          err = 1;
        }
      }
    }
  }

  if (ismooth>100) {
    METLIBS_LOG_WARN("CONTOUR ERROR. ismooth: " << ismooth);
    err = 1;
  }

  if (labfmt[0] != 0 && (chxlab<=0. || chylab<=0.)) {
    METLIBS_LOG_WARN("CONTOUR ERROR. chxlab,chylab: "
    << chxlab << " " << chylab);
    err = 1;
  }

  if (labfmt[0] != 0) {
    k = 1;
    if (labfmt[0]<-2 || labfmt[0]>4) k=0;
    if (labfmt[0]>0  && (labfmt[1]<1 || labfmt[1]>20)) k=0;
    if (labfmt[0]==2 && (labfmt[2]<0 || labfmt[2]>labfmt[1]-2)) k=0;
    if (labfmt[0]==3 && (labfmt[2]<0 || labfmt[2]>labfmt[1]-5)) k=0;
    if (labfmt[0]==4 && (labfmt[2]<0 || labfmt[2]>labfmt[1]-6)) k=0;
    if (k==0) {
      METLIBS_LOG_WARN("CONTOUR ERROR. label format.");
      err = 1;
    }
  }

  if (ibmap==1)
  {
    k = 1;
    if (nxbmap<1 || nybmap<1 || rbmap[2]<=0. || rbmap[3]<=0.) k = 0;
    else if (lbmap < (nxbmap*nybmap+nbitwd-1)/nbitwd) k = 0;
    else {
      i=int((xylim[0]-rbmap[0])/rbmap[2]);
      j=int((xylim[2]-rbmap[1])/rbmap[3]);
      if(i<0 || i>=nxbmap || j<0 || j>=nybmap) k=0;
      i=int((xylim[1]-rbmap[0])/rbmap[2]);
      j=int((xylim[3]-rbmap[1])/rbmap[3]);
      if(i<0 || i>=nxbmap || j<0 || j>=nybmap) k=0;
    }
    if(k==0) {
      METLIBS_LOG_WARN("CONTOUR ERROR. bit map parameters.");
      err = 1;
    }
  }

  if (err != 0) return false;

  // work arrays etc.

  ix1 = ipart[0];
  ix2 = ipart[1];
  iy1 = ipart[2];
  iy2 = ipart[3];
  ix0 = ix1-1;
  iy0 = iy1-1;
  ixrange = ix2-ix1+1;
  iyrange = iy2-iy1+1;

  gx1= float(ix1);
  gx2= float(ix2);
  gy1= float(iy1);
  gy2= float(iy2);

  // partition area into max mblock x mblock squares
  // with minimum size 5 x 5 gridpoints

  ibsize = (ixrange-1+mblock-1)/mblock;
  if (ibsize<5)       ibsize = 5;
  if (ibsize>ixrange) ibsize = ixrange;
  jbsize = (iyrange-1+mblock-1)/mblock;
  if (jbsize<5)       jbsize = 5;
  if (jbsize>iyrange) jbsize = iyrange;

  nxbox = (ixrange-1+ibsize-1)/ibsize;
  nybox = (iyrange-1+jbsize-1)/jbsize;

  iaddbit[0] = 0;
  iaddbit[1] = 1;
  iaddbit[2] = ixrange*2;
  iaddbit[3] = 3;

  for (n=0; n<4; ++n) {
    ijmove[n][0]   = initmove[n][0];
    ijmove[n][1]   = initmove[n][1];
    ijmove[4+n][0] = -ixrange;
    ijmove[4+n][1] = -iyrange;
  }

  // x,y conversion method
  iconv = icxy;

  // value used when testing if undefined
  zhigh = 0.9*fieldUndef;

  //map or window limits
  xlim1 = xylim[0];
  xlim2 = xylim[1];
  ylim1 = xylim[2];
  ylim2 = xylim[3];

  // minimum distance between line positions
  dxeq = float(ix2-ix1)*1.e-5;
  dyeq = float(iy2-iy1)*1.e-5;

  // initialize variables and get current character
  // size for contour labeling

  nnfrmt = labfmt[0];
  nnwdth = labfmt[1];
  //  nnfrac = labfmt[2];
  nzfrmt = nnfrmt;

  if (shading) nzfrmt= nnfrmt= 0;

  if (nnfrmt != 0) {

    // character size in x-direction (incl. space)
    chrx = chylab;
    // character size in y-direction (incl. space)
    chry = chylab;
    // relative character size in x-direction inside box (estimate)
    rchrx = 1.0;
    // relative character size in y-direction inside box (estimate)
    rchry = 1.0;

    // part of line before first label
    slab1f = 0.2;

    // min. distance between two labels (along the same line)
    dslab = 0.7*(xlim2-xlim1+ylim2-ylim1)*0.5;

    // misc. label stuff
    stalab = chrx*rchrx*0.25;
    endlab = stalab;
    dylab  = chry*rchry;
    dylabh = dylab*0.5;
    dclab[0] = chrx*rchrx*0.25;
    dclab[2] = dylab+dclab[0]*2.;
    dclab[3] = dylab+dclab[0]*2.;
    if(nnfrmt>0) {
      dxlab1 = chrx*(float(nnwdth)-1.+rchrx);
      dxlab  = stalab+dxlab1+endlab;
      dxlab2 = dxlab*dxlab;
      splim2 = (dxlab+0.1*chrx*rchrx)*(dxlab+0.1*chrx*rchrx);
      dclab[1] = dxlab1+dclab[0]*2.;
    }
    if (iconv==0) {
      nlabx = int(dylabh+0.5);
      nlaby = nlabx;
    } else if (iconv==1) {
      nlabx = int(dylabh / miutil::absval(cxy[1], cxy[4]) + 0.5);
      nlaby = int(dylabh / miutil::absval(cxy[2], cxy[5]) + 0.5);
    }
  }

  for (i=0; i<nbitwd; ++i) {
    bit[i] = 1 << i;
    //clearbit[i] = ~bit[i];
  }

  bool undefOriginal= false;

  int onebitsize= (ixrange*iyrange+nbitwd-1)/nbitwd;
  int *undefSquare=    new int[onebitsize];
  int *undefAllSquare= new int[onebitsize];

  for (i=0; i<onebitsize; ++i)
    undefSquare[i]= undefAllSquare[i]= 0;

  j=iy1;
  while (j<=iy2 && !undefOriginal) {
    for (i=ix1; i<=ix2; i++)
      if (z[j*nx+i]==fieldUndef) undefOriginal= true;
    j++;
  }

  if (undefOriginal) {
    for (j=iy1; j<iy2; j++) {
      for (i=ix1; i<ix2; i++) {
        int ij= j*nx+i;
        if (z[ij]   ==fieldUndef || z[ij+1]   ==fieldUndef ||
            z[ij+nx]==fieldUndef || z[ij+nx+1]==fieldUndef) {
          ibit = (j-iy1)*ixrange + (i-ix1);
          iwrd = ibit/nbitwd;
          ibit = ibit%nbitwd;
          undefSquare[iwrd]|=bit[ibit];
          if (z[ij]   ==fieldUndef && z[ij+1]   ==fieldUndef &&
              z[ij+nx]==fieldUndef && z[ij+nx+1]==fieldUndef)
            undefAllSquare[iwrd]|=bit[ibit];
        }
      }
    }
    for (j=iy1; j<iy2; j++) {
      int ij= j*nx+ix2;
      if (z[ij]==fieldUndef || z[ij+nx]==fieldUndef) {
        ibit = (j-iy1)*ixrange + (i-ix1);
        iwrd = ibit/nbitwd;
        ibit = ibit%nbitwd;
        undefSquare[iwrd]|=bit[ibit];
        if (z[ij]==fieldUndef && z[ij+nx]==fieldUndef)
          undefAllSquare[iwrd]|=bit[ibit];
      }
    }
    for (i=ix1; i<ix2; i++) {
      int ij= iy2*nx+i;
      if (z[ij]==fieldUndef || z[ij+1]==fieldUndef) {
        ibit = (j-iy1)*ixrange + (i-ix1);
        iwrd = ibit/nbitwd;
        ibit = ibit%nbitwd;
        undefSquare[iwrd]|=bit[ibit];
        if (z[ij]==fieldUndef && z[ij+1]==fieldUndef)
          undefAllSquare[iwrd]|=bit[ibit];
      }
    }
  }

  bool undefReplaced= false;
  float *zOriginal= 0;
  float *zrep= 0;

  if (undefOriginal) {
    undefReplaced= true;
    zOriginal= z;
    z= new float[nx*ny];
    zrep= z;
    for (i=0; i<nx*ny; ++i)
      z[i]= zOriginal[i];
    if (!drawBorders)
      replaceUndefinedValues(nx,ny,z,false,fieldUndef);
  }

  zmin = +fieldUndef;
  zmax = -fieldUndef;
  int iudef= 0;

  for (jb=0; jb<nybox; ++jb) {
    jstart = iy1+jb*jbsize;
    jend   = (jstart+jbsize<iy2) ? jstart+jbsize+1 : iy2+1;
    for (ib=0; ib<nxbox; ++ib) {
      istart = ix1+ib*ibsize;
      iend   = (istart+ibsize<ix2) ? istart+ibsize+1 : ix2+1;
      zminbl = +fieldUndef;
      zmaxbl = -fieldUndef;
      iud = 0;
      for (j=jstart; j<jend; ++j) {
        ij = j*nx + istart;
        for (i=istart; i<iend; ++i, ++ij) {
          if (z[ij]<zhigh) {
            if (zminbl>z[ij]) zminbl = z[ij];
            if (zmaxbl<z[ij]) zmaxbl = z[ij];
          }
          else  iud++;
        }
      }
      zminb[jb][ib]  = zminbl;
      zmaxb[jb][ib]  = zmaxbl;
      iundef[jb][ib] = iud;
      if (zmin>zminbl) zmin = zminbl;
      if (zmax<zmaxbl) zmax = zmaxbl;
      iudef+=iud;
    }
  }

  if (zmin>zmax) {
    if (undefReplaced) {
      delete[] z;
      z= zOriginal;  // but pointer not returned!!
    }
    delete[] undefSquare;
    delete[] undefAllSquare;
    return true;
  }

  zmean = (zmin+zmax)*0.5;
  if (zmin<0. && zmax>0.) zmean = 0.;
  else if (zmin>=0. && zmin<0.1*zmax) zmean = -zmax;
  else if (zmax<=0. && zmax>0.1*zmin) zmean = -zmin;

  nvalue = 0;

  if(idraw>0){
    nvalue = sortlines(zmin,zmax,mvalue,nvalue,
        zvalue,ivalue,icolour,linetype,linewidth,
        idraw,zrange,zstep,zoff,nlines,rlines,
        ncol,icol,ntyp,ityp,nwid,iwid,nlim,rlim,
        fieldUndef);
    if (nvalue < 0) {
      METLIBS_LOG_WARN("CONTOUR ERROR.  Line colour/type/width spec.");
    }
  }

  if (idraw2>0 ) {
    nvalue = sortlines(zmin,zmax,mvalue,nvalue,
        zvalue,ivalue,icolour,linetype,linewidth,
        idraw2,zrange2,zstep2,zoff2,nlines2,rlines2,
        ncol2,icol2,ntyp2,ityp2,nwid2,iwid2,nlim2,rlim2,
        fieldUndef);
    if (nvalue < 0) {
      METLIBS_LOG_WARN("CONTOUR ERROR.  Line colour/type/width spec(2).");
    }
  }

  if (nvalue<1 && !shading) {
    if (undefReplaced) {
      delete[] z;
      z= zOriginal;  // but pointer not returned!!
    }
    delete[] undefSquare;
    delete[] undefAllSquare;
    return true;
  }

  // max no. of positions on one line (arrays x,y,s,indxgx,indxgy)
  maxpos = (ixrange+iyrange)*2 + 2;

  mpos = maxpos-2;

  // 'iused' bit array
  liused = (2*ixrange*iyrange+nbitwd-1)/nbitwd;

  if (ismooth>0) {
    n = (mpos-1)*(ismooth+1) + 1;
    xsmooth = new float[n];
    ysmooth = new float[n];
  }

  iused = new int[liused];
  x = new float[maxpos];
  y = new float[maxpos];
  s = new float[maxpos];
  indxgx = new int[maxpos];
  indxgy = new int[maxpos];
  kab    = new int[(ibsize+1)*(jbsize+1)];

  icolx = -1;
  ltypx = -1;
  iwidx = -1;

  int firstLev=0;
  //if (zmin>zvalue[0]) firstLev=1;
  if (zmin>zvalue[0]) {
    firstLev=1;
  } else if (zrange[0]<=zrange[1] && zrange[0]>zvalue[0]) {
    firstLev=1;
  }
  //###################################################################
  //METLIBS_LOG_DEBUG("firstLev,zmin,zmax,zrange,zvalue[0]: "
  //    <<firstLev<<"  "<<zmin<<" "<<zmax<<"  "<<zrange[0]<<" "<<zrange[1]
  //    <<" "<<zvalue[0]);
  //###################################################################

  if (nvalue>firstLev+1)         zzstp2 = zvalue[firstLev+1]-zvalue[firstLev];
  else if (idraw==1 || idraw==2) zzstp2 = zstep;
  else                           zzstp2 = 0.;

  // otherwise colours may not work...
  gl->ShadeModel(DiGLPainter::gl_FLAT);

  // main loop over each contour level (zc=zvalue[])

  int nundef= nvalue;
  if (undefOriginal && shading) {
    ivalue[nvalue]= ivalue[nvalue-1] + 1;
    zvalue[nvalue]= fieldUndef;
    nvalue++;
  }

  std::vector<LinePart> vlp;
  LinePart alp;
  alp.connect[0][0]= -1;
  alp.connect[0][1]= -1;
  alp.connect[1][0]= -1;
  alp.connect[1][1]= -1;
  alp.closed= false;

  std::vector<std::map<float, int>> verAttach(ixrange + 1); // verAttach[i][y]
  std::vector<std::map<float, int>> horAttach(iyrange + 1); // horAttach[j][x]

  // borderConnect[ij][iside][line]
  std::map<int, std::map<int, int>> borderConnect;

  // set corners
  float xcorner[4],ycorner[4],zcorner[4];

  xcorner[0]= gx2;
  xcorner[1]= gx2;
  xcorner[2]= gx1;
  xcorner[3]= gx1;
  ycorner[0]= gy1;
  ycorner[1]= gy2;
  ycorner[2]= gy2;
  ycorner[3]= gy1;
  if (undefReplaced) {
    zcorner[0]= zOriginal[iy1*nx+ix2];
    zcorner[1]= zOriginal[iy2*nx+ix2];
    zcorner[2]= zOriginal[iy2*nx+ix1];
    zcorner[3]= zOriginal[iy1*nx+ix1];
  } else {
    zcorner[0]= z[iy1*nx+ix2];
    zcorner[1]= z[iy2*nx+ix2];
    zcorner[2]= z[iy2*nx+ix1];
    zcorner[3]= z[iy1*nx+ix1];
  }

  if (shading) {
    // add coners as "lines" with one position
    for (n=0; n<4; n++) {
      int l= 0;
      while (l<nvalue && zcorner[n]>=zvalue[l]) l++;
      l--;
      ContourLine *cl= new ContourLine();
      cl->original= true;
      if (l>=0) cl->ivalue= ivalue[l];
      else      cl->ivalue= -300;
      cl->value=  zcorner[n];
      cl->highInside= true;
      cl->highLeft= true;
      cl->closed= false;
      cl->izleft=  int(zcorner[n]);
      cl->izright= int(zcorner[n]);
      cl->direction= 0;
      cl->npos= 1;
      cl->xpos= new float[1];
      cl->ypos= new float[1];
      cl->xpos[0]= xcorner[n];
      cl->ypos[0]= ycorner[n];
      cl->corner= true;
      cl->isoLine=    false;
      cl->undefTouch= false;
      cl->undefLeft=  false;
      cl->undefRight= false;
      cl->outer=      false;
      cl->inner=      false;
      cl->undefLine= (zcorner[n]==fieldUndef);
      cl->connect[0][0]= -1;
      cl->connect[0][1]= -1;
      cl->connect[1][0]= -1;
      cl->connect[1][1]= -1;
      contourlines.push_back(cl);
    }

    horAttach[0]      [xcorner[0]]= 0;
    verAttach[ixrange][ycorner[1]]= 1;
    horAttach[iyrange][xcorner[2]]= 2;
    verAttach[0]      [ycorner[3]]= 3;

    horAttach[0]      [xcorner[3]]= 3 + maxLines;
    verAttach[ixrange][ycorner[0]]= 0 + maxLines;
    horAttach[iyrange][xcorner[1]]= 1 + maxLines;
    verAttach[0]      [ycorner[2]]= 2 + maxLines;
  }

  QString strlabel;

  for (lev=firstLev; lev<nvalue; ++lev)
  {

    zc = zvalue[lev];

    if (lev<nundef) {

      zzstp1 = zzstp2;
      if (lev<nvalue-1)               zzstp2 = zvalue[lev+1] - zc;
      else if (idraw==1 || idraw==2) zzstp2 = zstep;
      zzstp = (zzstp1 < zzstp2) ? zzstp1 : zzstp2;

      // line type
      if (ltypx != linetype[lev]) {
        ltypx = linetype[lev];
        const auto& lt = ltypx == 1 ? poptions.linetype_2 : poptions.linetype;
        if (lt.stipple) {
          gl->LineStipple(lt.factor, lt.bmap);
          gl->Enable(DiGLPainter::gl_LINE_STIPPLE);
        } else {
          gl->Disable(DiGLPainter::gl_LINE_STIPPLE);
        }
      }

      // line width
      if (iwidx != linewidth[lev]) {
        iwidx = linewidth[lev];
        const auto& lw = iwidx == 1 ? poptions.linewidth_2 : poptions.linewidth;
        gl->LineWidth(lw);
      }

      // line and text (label) colour
      if (icolx != icolour[lev]) {
        icolx = icolour[lev];
        const Colour* lc = nullptr;
        if (poptions.colours.size() > 2) {
          if (icolx >= 0 && icolx < poptions.colours.size())
            lc = &poptions.colours[icolx];
        } else if (icolx == 1) {
          lc = &poptions.linecolour_2;
        } else {
          lc = &poptions.linecolour;
        }
        if (lc)
          gl->setColour(*lc, false);
      }

      // label format for each label
      if (nzfrmt<0 && !drawBorders) {

        strlabel= QString::number(zc);
        // numberformat(zc,1,nnfrmt,nnwdth,nnfrac);
        //-----------------------------------------
        //nnfrmt=1;
        //nnwdth=4;
        //nnfrac=0;
        //-----------------------------------------

        gl->getTextSize(strlabel, dxlab1, dxlab2);

        dxlab  = stalab+dxlab1+endlab;
        dxlab2 = dxlab*dxlab;
        splim2 = (dxlab+0.1*chrx*rchrx);
        splim2 = splim2*splim2;
        dclab[1] = dxlab1+dclab[0]*2.;
      }

      // handling field value equal isoline value
      if       ( zc>0. ) iabeq = +1;
      else if ( zc<0. ) iabeq = -1;
      else if ( zmin==0. && zmax>0 ) iabeq = -1;
      else iabeq = +1;

    } else {
      // contouring undefined areas (as the last line)
      zhigh= fieldUndef*1.1;
      zc= fieldUndef*0.9;
      float *zhelp= z;
      z= zOriginal;
      zOriginal= zhelp;
      undefReplaced= false;
      for (jb=0; jb<nybox; ++jb) {
        for (ib=0; ib<nxbox; ++ib) {
          zminb[jb][ib]= fieldUndef*0.8;  // => unnecessary search !!!!!
          zmaxb[jb][ib]= fieldUndef;
          iundef[jb][ib]= 0;
        }
      }
      for (i=0; i<onebitsize; ++i)
        undefSquare[i]= undefAllSquare[i]= 0;
      iabeq=+1;
    }

    if (!drawBorders || lev==firstLev || lev==nundef) {
      // clear bit matrix
      for (i=0; i<liused; ++i) iused[i] = 0;
    }

    // search grid for unused part of contour

    istartl = 0;
    ibox1  =  0;
    jbox1  =  0;
    kbox   =  0;

    startSearch:

    // search for start of line in the boxes

    istartl++;

    for (jb=jbox1; jb<nybox; ++jb) {
      jstart = iy1+jb*jbsize;
      jend = (jstart+jbsize<iy2) ? jstart+jbsize : iy2;
      jend++;

      for (ib=ibox1; ib<nxbox; ++ib) {

        if (zminb[jb][ib]<=zc && zmaxb[jb][ib]>=zc) {
          istart = ix1+ib*ibsize;
          iend = (istart+ibsize<ix2) ? istart+ibsize : ix2;
          iend++;
          ibdim = iend-istart;

          if (kbox==0) {
            // scan box
            if(iundef[jb][ib]==0) {
              // no undefined values in this box
              for (j=jstart; j<jend; ++j) {
                ij = j*nx + istart;
                ik = (j-jstart)*ibdim;
                for (i=istart; i<iend; ++i, ++ij, ++ik) {
                  if      (z[ij] > zc) kab[ik] = +1;
                  else if (z[ij] < zc) kab[ik] = -1;
                  else                 kab[ik] = iabeq;
                }
              }
            } else {
              // undefined values in this box
              for (j=jstart; j<jend; ++j) {
                ij = j*nx + istart;
                ik = (j-jstart)*ibdim;
                for (i=istart; i<iend; ++i, ++ij, ++ik) {
                  if (z[ij]<zhigh) {
                    if      (z[ij] > zc) kab[ik] = +1;
                    else if (z[ij] < zc) kab[ik] = -1;
                    else                 kab[ik] = iabeq;
                  }
                  else kab[ik] = 99;
                }
              }
            }

            // keep search loops below a short as possible
            // (avoid unnecessary work)
            istartl = istart;
            jstartl = jstart;
            iend1 = iend-1;
            if (jb<nybox-1) jend1 = jend-1;
            else            jend1 = jend;
            if (iundef[jb][ib] == 0) iend2 = istart+1;
            else                     iend2 = iend;
            jend2 = jend-1;
            if (undefOriginal || drawBorders) {
              iend2= iend;
              jend1= jend;
            }
            kbox  = 1;
          }

          if (kbox==1) {
            iside = 0;

            for (j=jstartl; j<jend1; ++j) {
              ik = (j-jstart)*ibdim + (istartl-istart);
              for (i=istartl; i<iend1; ++i, ++ik) {
                if (kab[ik]+kab[ik+1]==0) {
                  ibit = ((j-iy1)*ixrange + (i-ix1))*2;
                  iwrd = ibit/nbitwd;
                  ibit = ibit%nbitwd;
                  if ((iused[iwrd] & bit[ibit]) == 0) {
                    ibit2 = (j-iy1)*ixrange + (i-ix1);
                    iwrd2 = ibit2/nbitwd;
                    ibit2 = ibit2%nbitwd;
                    if (((undefAllSquare[iwrd2] & bit[ibit2]) == 0) &&
                        (((undefSquare[iwrd2] & bit[ibit2]) == 0) ||
                            zOriginal[j*nx+i]!=fieldUndef ||
                            zOriginal[j*nx+i+1]!=fieldUndef))
                      goto startContour;
                  }
                }
              }
              istartl = istart;
            }
            jstartl=jstart;
            kbox=2;
          }

          if (kbox==2) {
            iside = 1;

            for (j=jstartl; j<jend2; ++j) {
              ik = (j-jstart)*ibdim + (istartl-istart);
              for (i=istartl; i<iend2; ++i, ++ik) {
                if (kab[ik]+kab[ik+ibdim]==0) {
                  ibit = ((j-iy1)*ixrange + (i-ix1))*2 + 1;
                  iwrd = ibit/nbitwd;
                  ibit = ibit%nbitwd;
                  if ((iused[iwrd] & bit[ibit]) == 0) {
                    ibit2 = (j-iy1)*ixrange + (i-ix1);
                    iwrd2 = ibit2/nbitwd;
                    ibit2 = ibit2%nbitwd;
                    if (((undefAllSquare[iwrd2] & bit[ibit2]) == 0) &&
                        (((undefSquare[iwrd2] & bit[ibit2]) == 0) ||
                            zOriginal[j*nx+i]!=fieldUndef ||
                            zOriginal[j*nx+i+1]!=fieldUndef))
                      goto startContour;
                  }
                }
              }
              istartl = istart;
            }
            jstartl = jstart;
          }
          kbox = 0;

        }
      }
      ibox1 = 0;
    }

    // go to end of "lev" loop
    continue;

    startContour:

    // start of contour found
    ibox1  = ib;
    jbox1  = jb;
    istartl = i;
    jstartl = j;

    //----------------------------------------------------------------------
    //  (i,j) is bottom left corner of square.
    //
    //  zz,iab[0]: bottom right .... (i+1,j)
    //         1 : bottom left  .... (i,j)
    //         2 : top    left  .... (i,j+1)
    //         3 : top    right .... (i+1,j+1)
    //
    //  sides of square:
    //   0 - bottom, y=j,   x between i and i+1, zc between zz[0] and zz[1]
    //   1 - left,   x=i,   y between j and j+1, zc between zz[1] and zz[2]
    //   2 - top,    y=j+1, x between i and i+1, zc between zz[2] and zz[3]
    //   3 - right,  x=i+1, y between j and j+1, zc between zz[3] and zz[0]
    //----------------------------------------------------------------------

    iside1 = iside;
    ic1 = iside;
    ic2 = isides[iside+1];

    zz[ic1] = z[i+ijcorner[ic1][0]+(j+ijcorner[ic1][1])*nx];
    if      (zz[ic1]>zc) iab[ic1]=+1;
    else if (zz[ic1]<zc) iab[ic1]=-1;
    else                 iab[ic1]=iabeq;

    zz[ic2] = z[i+ijcorner[ic2][0]+(j+ijcorner[ic2][1])*nx];
    if      (zz[ic2]>zc) iab[ic2]=+1;
    else if (zz[ic2]<zc) iab[ic2]=-1;
    else                 iab[ic2]=iabeq;

    highLeft= (iab[ic1]==1);
    float zreal= (highLeft) ? zz[ic2] : zz[ic1];

    float zleft=  zz[ic1];
    float zright= zz[ic2];
    int  izleft=  -1;
    int  izright= -1;
    if (drawBorders && shading) {
      izleft=  int(zz[ic1]+0.1);
      izright= int(zz[ic2]+0.1);
    }

    zz1  = zz[ic1];
    zz2  = zz[ic2];
    iab1 = iab[ic1];
    iab2 = iab[ic2];

    if      (iside==0 && j==iy2) linend=1;
    else if (iside==1 && i==ix2) linend=1;
    else                         linend=0;

    // store first position and mark side as used in bit-map.

    if (lev<nundef && !drawBorders)
      frac= 0.002f + 0.996f * (zc-zz[ic1])/(zz[ic2]-zz[ic1]);
    else
      frac= 0.5f;
    npos = 1;

    xh = float(i+ijcorner[ic1][0]);
    yh = float(j+ijcorner[ic1][1]);
    x[npos] = xh+frac*xfrac[ic1];
    y[npos] = yh+frac*yfrac[ic1];

    iused[iwrd]|=bit[ibit];

    // start tracing the line

    while (linend<2) {

      if (npos==mpos) {
        maxpos*=2;
        mpos= maxpos-2;
        float *xnew= new float[maxpos];
        float *ynew= new float[maxpos];
        int ncp= npos+1;
        for (int icp=1; icp<ncp; ++icp) {
          xnew[icp]= x[icp];
          ynew[icp]= y[icp];
        }
        delete[] x;
        delete[] y;
        delete[] indxgx;
        delete[] indxgy;
        delete[] s;
        x= xnew;
        y= ynew;
        indxgx= new int[maxpos];
        indxgy= new int[maxpos];
        s = new float[maxpos];

        if (ismooth>0) {
          delete[] xsmooth;
          delete[] ysmooth;
          int nsm = (mpos-1)*(ismooth+1) + 1;
          xsmooth = new float[nsm];
          ysmooth = new float[nsm];
        }
      }

      newend = 0;
      i = i+ijmove[iside][0];
      j = j+ijmove[iside][1];

      if (i>ix0 && i<ix2 && j>iy0 && j<iy2) {
        ibit2 = (j-iy1)*ixrange + (i-ix1);
        iwrd2 = ibit2/nbitwd;
        ibit2 = ibit2%nbitwd;
        if ((undefAllSquare[iwrd2] & bit[ibit2]) == 0) {
          ic3 = iside;
          ic4 = isides[ic3+1];
          ic1 = isides[ic3+2];
          ic2 = isides[ic3+3];
          iside = ic1;

          // two corners from last square
          zz[ic1]  = zz[ic4];
          zz[ic2]  = zz[ic3];
          iab[ic1] = iab[ic4];
          iab[ic2] = iab[ic3];

          // two new corners
          zz[ic3] = z[i+ijcorner[ic3][0]+(j+ijcorner[ic3][1])*nx];
          if (zz[ic3]<zhigh) {
            if      (zz[ic3]>zc) iab[ic3]=+1;
            else if (zz[ic3]<zc) iab[ic3]=-1;
            else                 iab[ic3]=iabeq;
          } else                 iab[ic3]=99;

          zz[ic4] = z[i+ijcorner[ic4][0]+(j+ijcorner[ic4][1])*nx];;
          if (zz[ic4]<zhigh) {
            if      (zz[ic4]>zc) iab[ic4]=+1;
            else if (zz[ic4]<zc) iab[ic4]=-1;
            else                 iab[ic4]=iabeq;
          } else                 iab[ic4]=99;

          // find exit side

          if ((iab[0]+iab[2])*(iab[1]+iab[3]) != -4) {

            // not saddle point
            k = 1;
            n = 0;
            while (k!=0 && n<3) {
              ic1 = newside[0][iside][n];
              ic2 = isides[ic1+1];
              k   = iab[ic1]+iab[ic2];
              n++;
            }
            if (k==0 && drawBorders && lev<nundef) {
              for (n=0; n<4; n++)
                if (zz[n]!=zleft && zz[n]!=zright) k=1;
            }

            if (k != 0) newend= -1;

          } else if (!drawBorders || lev==nundef) {

            // saddle point
            int ic1begin = ic1;
            if (lev<nundef) {
              float frac1= (zc-zz[ic1])/(zz[ic2]-zz[ic1]);
              float frac2= (zc-zz[ic2])/(zz[ic3]-zz[ic2]);
              float frac3= (zc-zz[ic3])/(zz[ic4]-zz[ic3]);
              float frac4= (zc-zz[ic4])/(zz[ic1]-zz[ic4]);
              if ((frac1>0.8 && frac2<0.2) ||
                  (frac3>0.8 && frac4<0.2)) {
                ic1= ic2;
              } else if ((frac1<0.2 && frac4>0.8) ||
                  (frac3<0.2 && frac2>0.8)) {
                ic1= ic4;
              } else if (i>0 && i+2<nx && j>0 && j+2<ny && lev<nundef) {
                // try to maintain large scale direction of ridges etc.
                // crossing gridsquares in a diagonal direction
                // (avoid making noisy fields look more noisy than they are)
                ij = i+j*nx;
                if (z[ij-nx]  <zhigh && z[ij+1-nx]  <zhigh &&
                    z[ij-1]   <zhigh && z[ij]       <zhigh &&
                    z[ij+1]   <zhigh && z[ij+2]     <zhigh &&
                    z[ij-1+nx]<zhigh && z[ij+nx]    <zhigh &&
                    z[ij+1+nx]<zhigh && z[ij+2+nx]  <zhigh &&
                    z[ij+nx*2]<zhigh && z[ij+1+nx*2]<zhigh ) {
                  dzzza = (z[ij+1]-z[ij-nx])*(z[ij+2+nx]-z[ij+1])
                  +(z[ij+nx]-z[ij-1])*(z[ij+1+nx*2]-z[ij+nx]);
                  dzzzb = (z[ij]-z[ij+1-nx])*(z[ij-1+nx]-z[ij])
                  +(z[ij+1+nx]-z[ij+2])*(z[ij+nx*2]-z[ij+1+nx]);
                  if (dzzza>dzzzb) ic1 = newtest[0][iside];
                  else             ic1 = newtest[1][iside];
                }
              }
            }

            if (ic1begin==ic1) {
              zavg = (zz[0]+zz[1]+zz[2]+zz[3])*0.25;
              if (zavg>zmean && iab[ic1]==+1) ic1 = newside[0][iside][0];
              else if (zavg>zmean)            ic1 = newside[1][iside][0];
              else if (iab[ic1]==-1)          ic1 = newside[0][iside][0];
              else                            ic1 = newside[1][iside][0];
            }
            ic2 = isides[ic1+1];
          }
          else newend= -1;
        }
        else newend= -1;
      }
      else  newend=-1;

      closed= false;

      if (newend==0) {

        // if side is marked as used, we have a closed contour,

        iside = ic1;
        ibit = ((j-iy1)*ixrange + (i-ix1))*2 + iaddbit[ic1];
        iwrd = ibit/nbitwd;
        ibit = ibit%nbitwd;
        if ((iused[iwrd] & bit[ibit]) != 0) {
          newend = 1;
          if (i==istartl && j==jstartl && iside==iside1) {
            closed=true;
            linend=2;
          }
        }
      }

      if(newend >= 0) {

        // store position and mark side as used in bit-map.
        if (lev<nundef && !drawBorders)
          frac= 0.002f + 0.996f * (zc-zz[ic1])/(zz[ic2]-zz[ic1]);
        else
          frac= 0.5f;
        npos++;

        xh = float(i+ijcorner[ic1][0]);
        yh = float(j+ijcorner[ic1][1]);
        x[npos] = xh+frac*xfrac[ic1];
        y[npos] = yh+frac*yfrac[ic1];

        // avoid lines going through gridpoints to be recorded multiply
        // (crusual for the line smoothing in smoothline)
        if (fabsf(x[npos]-x[npos-1])<dxeq &&
            fabsf(y[npos]-y[npos-1])<dyeq) {
          // should keep positions at boundaries
          x[npos-1]= x[npos];
          y[npos-1]= y[npos];
          npos--;
        }

        if (newend==0) iused[iwrd]|=bit[ibit];
      }

      if (newend != 0) {
        linend++;

        if (linend==1) {
          highLeft= !highLeft;
          float ztmp= zleft;
          zleft=  zright;
          zright= ztmp;
          n= izleft;
          izleft= izright;
          izright= n;
          // turn the line
          n1 = npos/2 + 1;
          n2 = npos+1;
          for (n=1; n<n1; ++n) {
            xh = x[n];
            yh = y[n];
            x[n] = x[n2-n];
            y[n] = y[n2-n];
            x[n2-n] = xh;
            y[n2-n] = yh;
          }
          // and prepare for further search
          ic3 = isides[iside1+2];
          ic4 = isides[iside1+3];
          iside = ic3;
          i = istartl-ijmove[iside][0];
          j = jstartl-ijmove[iside][1];
          zz[ic4]  = zz1;
          zz[ic3]  = zz2;
          iab[ic4] = iab1;
          iab[ic3] = iab2;
        }
      }
    }

    bool linesplit= false;
    vlp.clear();

    if (!drawBorders && undefOriginal && lev<nundef && npos>=2) {

      int line_code, line_number= contourlines.size();
      int line_number1= line_number;
      bool prevundef;

      alp.pfirst= 1;
      alp.addfirst= false;
      alp.closed= false;

      i= int(x[1] + 0.5);
      j= int(y[1] + 0.5);
      prevundef= (zOriginal[j*nx+i]==fieldUndef);

      int nadjust= 0;

      for (n=1; n<npos; n++) {
        i= int((x[n]+x[n+1])*0.5);
        j= int((y[n]+y[n+1])*0.5);
        ibit2 = (j-iy1)*ixrange + (i-ix1);
        iwrd2 = ibit2/nbitwd;
        ibit2 = ibit2%nbitwd;
        if ((undefSquare[iwrd2] & bit[ibit2]) != 0) {

          float xx,yy,xtmp,ytmp,xuc[4],yuc[4];
          int suc[4];
          xx= float(i)+0.5;
          yy= float(j)+0.5;
          // keep away from problem points...
          if (n!=nadjust) {
            if (fabsf(x[n]-xx)<fabsf(y[n]-yy)) {
              if (x[n]<xx)
                x[n]= xx - 0.01 - (xx-x[n])*0.96;
              else
                x[n]= xx + 0.01 + (x[n]-xx)*0.96;
            } else {
              if (y[n]<yy)
                y[n]= yy - 0.01 - (yy-y[n])*0.96;
              else
                y[n]= yy + 0.01 + (y[n]-yy)*0.96;
            }
          }
          if (fabsf(x[n+1]-xx)<fabsf(y[n+1]-yy)) {
            if (x[n+1]<xx)
              x[n+1]= xx - 0.01 - (xx-x[n+1])*0.96;
            else
              x[n+1]= xx + 0.01 + (x[n+1]-xx)*0.96;
          } else {
            if (y[n+1]<yy)
              y[n+1]= yy - 0.01 - (yy-y[n+1])*0.96;
            else
              y[n+1]= yy + 0.01 + (y[n+1]-yy)*0.96;
          }
          nadjust= n+1;
          xuc[0]= x[n];
          yuc[0]= y[n];
          suc[0]= -1;
          int nuc= 1;
          if (y[n]!=y[n+1]) {
            xuc[nuc]= x[n]+(x[n+1]-x[n])*(yy-y[n])/(y[n+1]-y[n]);
            if (xuc[nuc]>float(i) && xuc[nuc]<float(i+1)) {
              yuc[nuc]= yy;
              suc[nuc++]= 0;
            }
          }
          if (x[n]!=x[n+1]) {
            yuc[nuc]= y[n]+(y[n+1]-y[n])*(xx-x[n])/(x[n+1]-x[n]);
            if (yuc[nuc]>float(j) && yuc[nuc]<float(j+1)) {
              xuc[nuc]= xx;
              suc[nuc++]= 1;
            }
          }
          xuc[nuc]=   x[n+1];
          yuc[nuc]=   y[n+1];
          suc[nuc++]= -1;

          if (nuc==4) {
            if (xuc[1]>xx-0.01 && xuc[1]<xx+0.01 &&
                yuc[2]>yy-0.01 && yuc[2]<yy+0.01) {
              bool udef[4];
              udef[0]= (zOriginal[i+j*nx]      ==fieldUndef);
              udef[1]= (zOriginal[i+1+j*nx]    ==fieldUndef);
              udef[2]= (zOriginal[i+1+(j+1)*nx]==fieldUndef);
              udef[3]= (zOriginal[i+(j+1)*nx]  ==fieldUndef);
              int iu1= int((x[n]+xx)*0.5+0.5);
              int ju1= int((y[n]+yy)*0.5+0.5);
              int iu2= int((x[n+1]+xx)*0.5+0.5);
              int ju2= int((y[n+1]+yy)*0.5+0.5);
              int n1,n2;
              if      (iu1==i   && ju1==j  ) n1=0;
              else if (iu1==i+1 && ju1==j  ) n1=1;
              else if (iu1==i+1 && ju1==j+1) n1=2;
              else                           n1=3;
              if      (iu2==i   && ju2==j  ) n2=0;
              else if (iu2==i+1 && ju2==j  ) n2=1;
              else if (iu2==i+1 && ju2==j+1) n2=2;
              else                           n2=3;
              if (udef[n1] && udef[n2]) {
                nuc=0;
              } else {
                if (n1==0 || n1==2) {
                  if (udef[1]) {
                    if (n1==0) {
                      xuc[1]= xx;
                      yuc[1]= yy - 0.01;
                      suc[1]= 1;
                      xuc[2]= xx + 0.01;
                      yuc[2]= yy;
                      suc[2]= 0;
                    } else {
                      xuc[1]= xx + 0.01;
                      yuc[1]= yy;
                      suc[1]= 0;
                      xuc[2]= xx;
                      yuc[2]= yy - 0.01;
                      suc[2]= 1;
                    }
                  } else {
                    if (n1==0) {
                      xuc[1]= xx - 0.01;
                      yuc[1]= yy;
                      suc[1]= 0;
                      xuc[2]= xx;
                      yuc[2]= yy + 0.01;
                      suc[2]= 1;
                    } else {
                      xuc[1]= xx;
                      yuc[1]= yy + 0.01;
                      suc[1]= 1;
                      xuc[2]= xx - 0.01;
                      yuc[2]= yy;
                      suc[2]= 0;
                    }
                  }
                } else {
                  if (udef[0]) {
                    if (n1==1) {
                      xuc[1]= xx;
                      yuc[1]= yy - 0.01;
                      suc[1]= 1;
                      xuc[2]= xx - 0.01;
                      yuc[2]= yy;
                      suc[2]= 0;
                    } else {
                      xuc[1]= xx - 0.01;
                      yuc[1]= yy;
                      suc[1]= 0;
                      xuc[2]= xx;
                      yuc[2]= yy - 0.01;
                      suc[2]= 1;
                    }
                  } else {
                    if (n1==1) {
                      xuc[1]= xx + 0.01;
                      yuc[1]= yy;
                      suc[1]= 0;
                      xuc[2]= xx;
                      yuc[2]= yy + 0.01;
                      suc[2]= 1;
                    } else {
                      xuc[1]= xx;
                      yuc[1]= yy + 0.01;
                      suc[1]= 1;
                      xuc[2]= xx + 0.01;
                      yuc[2]= yy;
                      suc[2]= 0;
                    }
                  }
                }
              }
            } else {
              float dx1,dy1,dx2,dy2,ds1,ds2;
              dx1= xuc[1]-xuc[0];
              dy1= yuc[1]-yuc[0];
              dx2= xuc[2]-xuc[0];
              dy2= yuc[2]-yuc[0];
              ds1= dx1*dx1+dy1*dy1;
              ds2= dx2*dx2+dy2*dy2;
              if (ds2 < ds1) {
                xtmp= xuc[1];
                ytmp= yuc[1];
                k = suc[1];
                xuc[1]= xuc[2];
                yuc[1]= yuc[2];
                suc[1]= suc[2];
                xuc[2]= xtmp;
                yuc[2]= ytmp;
                suc[2]= k;
              }
            }
          }
          if (nuc>1) {
            bool currentundef;
            for (k=0; k<nuc-1; k++) {
              i= int((xuc[k]+xuc[k+1])*0.5 + 0.5);
              j= int((yuc[k]+yuc[k+1])*0.5 + 0.5);
              currentundef= (zOriginal[j*nx+i]==fieldUndef);
              if (currentundef != prevundef) {
                prevundef= currentundef;
                linesplit= true;
                if (currentundef) {
                  alp.plast= n;
                  alp.addlast= (fabsf(x[n]-xuc[k])>0.005 ||
                      fabsf(y[n]-yuc[k])>0.005);
                  alp.xlast= xuc[k];
                  alp.ylast= yuc[k];
                  vlp.push_back(alp);
                  line_code= line_number + maxLines;
                  line_number++;
                } else {
                  alp.pfirst= n+1;
                  alp.addfirst= (fabsf(x[n+1]-xuc[k])>0.005 ||
                      fabsf(y[n+1]-yuc[k])>0.005);
                  alp.xfirst= xuc[k];
                  alp.yfirst= yuc[k];
                  line_code= line_number;
                }
                if (shading) {
                  if (suc[k]==0)
                    horAttach[int(yuc[k])-iy0][xuc[k]]= line_code;
                  else
                    verAttach[int(xuc[k])-ix0][yuc[k]]= line_code;
                }
              }
            }
          } else {
            prevundef= true;
          }

        } else if (prevundef) {
          alp.pfirst= n;
          alp.addfirst= false;
          prevundef= false;
        }
      }

      if (!prevundef) {
        alp.plast= npos;
        alp.addlast= false;
        alp.closed= (closed && !linesplit);
        vlp.push_back(alp);
      }
      if (shading && closed && linesplit) {
        // may have to connect start and end of line (was closed)
        int nlp= vlp.size() - 1;
        if (vlp[0].pfirst==1 && vlp[nlp].plast==npos) {
          vlp[nlp].plast-=1;
          vlp[0].connect[0][0]= line_number + maxLines;
          vlp[0].connect[0][1]= line_number + maxLines;
          vlp[nlp].connect[1][0]= line_number1;
          vlp[nlp].connect[1][1]= line_number1;
        }
      }

    } else if (shading && lev==nundef) {

      if (!closed) {
        for (i=npos; i>0; i--) {
          x[i+1]= x[i];
          y[i+1]= y[i];
        }
        npos++;
      } else {
        npos++;
        x[npos]= x[2];
        y[npos]= y[2];
      }
      for (n=1; n<npos; n++) {
        float dx= x[n+1]-x[n];
        float dy= y[n+1]-y[n];
        if (fabsf(dx)<0.1 || fabsf(dy)<0.1) {
          x[n]= (x[n]+x[n+1])*0.5;
          y[n]= (y[n]+y[n+1])*0.5;
        } else {
          i= int((x[n]+x[n+1])*0.5 + 0.5);
          j= int((y[n]+y[n+1])*0.5 + 0.5);
          if ((z[j*nx+i]==fieldUndef &&  highLeft) ||
              (z[j*nx+i]!=fieldUndef && !highLeft)) {
            if (dx*dy>0.) x[n]= x[n+1];
            else          y[n]= y[n+1];
          } else {
            if (dx*dy>0.) y[n]= y[n+1];
            else          x[n]= x[n+1];
          }
        }
      }
      if (closed) npos--;

      if (iconv!=2) {
        float dxp= x[2]-x[1];
        float dyp= y[2]-y[1];
        np= 2;
        for (n=2; n<npos; n++) {
          float dx= x[n+1]-x[n];
          float dy= y[n+1]-y[n];
          if (fabsf(dxp-dx)>0.1 && fabsf(dyp-dy)>0.1) {
            x[np]=   x[n];
            y[np++]= y[n];
          }
          dxp= dx;
          dyp= dy;
        }
        x[np]= x[npos];
        y[np]= y[npos];
        npos= np;
      }

      int line_number= contourlines.size();
      int nlp= 0;

      int line_number1= line_number;
      int nlp1,nlp2,end1,end2,cline1,cline2;
      int prevconnect,currconnect;
      int currcode=0,currend=0,currline,prevcode=0,prevend=0,prevline=0;
      int keepcode=0,keepend=0,keepconnect=0;
      float xydir;
      std::map<float, int>::iterator p, p1, p2;

      alp.addfirst= false;
      alp.addlast= false;
      alp.pfirst= -1;
      alp.plast=  -1;
      alp.closed= false;
      vlp.push_back(alp);
      vlp[nlp].pfirst= 1;
      int keepline= -1;

      for (n=1; n<npos; n++) {
        if (fabsf(x[n]-x[n+1])<0.1) {
          i= int(x[n])-ix0;
          xydir= y[n+1] - y[n];
          float ylower= (y[n]<y[n+1]) ? y[n] : y[n+1];
          float yupper= (y[n]>y[n+1]) ? y[n] : y[n+1];
          p1= verAttach[i].lower_bound(ylower);
          p2= verAttach[i].upper_bound(yupper);
        } else {
          j= int(y[n])-iy0;
          xydir= x[n+1] - x[n];
          float xlower= (x[n]<x[n+1]) ? x[n] : x[n+1];
          float xupper= (x[n]>x[n+1]) ? x[n] : x[n+1];
          p1= horAttach[j].lower_bound(xlower);
          p2= horAttach[j].upper_bound(xupper);
        }
        if (p1!=p2) {
          vlp[nlp].plast= n;
          vlp.push_back(alp);  // next undef line part
          if (xydir>0.) {
            nlp1= nlp;
            nlp2= nlp+1;
            end1= 1;
            end2= 0;
            cline1= line_number + maxLines;
            cline2= line_number + 1;
            prevconnect= (highLeft) ? 1 : 0;
            currconnect= (highLeft) ? 0 : 1;
          } else {
            nlp1= nlp+1;
            nlp2= nlp;
            end1= 0;
            end2= 1;
            cline1= line_number + 1;
            cline2= line_number + maxLines;
            prevconnect= (highLeft) ?  0 : 1;
            currconnect= (highLeft) ?  1 : 0;
          }
          for (p=p1; p!=p2; p++) {
            currcode= p->second;
            currend=  currcode/maxLines;
            currline= currcode%maxLines;
            contourlines[currline]->undefTouch= true;
            if (p==p1) {
              vlp[nlp1].connect[end1][0]= currcode;
              vlp[nlp1].connect[end1][1]= currcode;
              contourlines[currline]->connect[currend][currconnect]= cline1;
              if (xydir<0.) {
                keepcode=currcode;
                keepend= currend;
                keepline=currline;
                keepconnect= currconnect;
              }
            } else {
              contourlines[prevline]->connect[prevend][prevconnect]= currcode;
              contourlines[currline]->connect[currend][currconnect]= prevcode;
            }
            prevcode= currcode;
            prevend=  currend;
            prevline= currline;
          }
          if (xydir>0.) {
            keepcode=currcode;
            keepend= currend;
            keepline=currline;
            keepconnect= prevconnect;
          }
          contourlines[prevline]->connect[prevend][prevconnect]= cline2;
          vlp[nlp2].connect[end2][0]= prevcode;
          vlp[nlp2].connect[end2][1]= prevcode;
          nlp++;
          vlp[nlp].pfirst= n+1;
          line_number++;
          linesplit= true;
        }
      }

      if (!closed || !linesplit) {
        vlp[nlp].plast= npos;
        vlp[nlp].closed= closed;
      } else if (vlp[nlp].pfirst==npos && keepline>=0) {
        contourlines[keepline]->connect[keepend][keepconnect]= line_number1;
        vlp[0].connect[0][0]= keepcode;
        vlp[0].connect[0][1]= keepcode;
        vlp.pop_back();
      } else {
        vlp[nlp].plast= npos-1;
        vlp[0].connect[0][0]= line_number + maxLines;
        vlp[0].connect[0][1]= line_number + maxLines;
        vlp[nlp].connect[1][0]= line_number1;
        vlp[nlp].connect[1][1]= line_number1;
      }

    } else if (drawBorders) {

      if (npos>1) {
        // much like first part of code for lev==nundef above
        if (!closed) {
          for (i=npos; i>0; i--) {
            x[i+1]= x[i];
            y[i+1]= y[i];
          }
          npos++;
        } else {
          npos++;
          x[npos]= x[2];
          y[npos]= y[2];
        }
        for (n=1; n<npos; n++) {
          float dx= x[n+1]-x[n];
          float dy= y[n+1]-y[n];
          if (fabsf(dx)<0.1 || fabsf(dy)<0.1) {
            x[n]= (x[n]+x[n+1])*0.5;
            y[n]= (y[n]+y[n+1])*0.5;
          } else {
            i= int((x[n]+x[n+1])*0.5);
            j= int((y[n]+y[n+1])*0.5);
            x[n]= float(i)+0.5f;
            y[n]= float(j)+0.5f;
          }
        }
        if (closed) npos--;
      }

      alp.pfirst= 1;
      alp.plast= npos;
      alp.addfirst= false;
      alp.addlast=  false;
      alp.closed= closed;
      vlp.push_back(alp);

    } else if (npos>1) {

      alp.pfirst= 1;
      alp.plast= npos;
      alp.addfirst= false;
      alp.addlast=  false;
      alp.closed= closed;
      vlp.push_back(alp);

    }

    int nlp= vlp.size();

    if (drawBorders && !closed && lev<nundef) {
      // ### We need to ensure that the ends connect correctly in datasets that
      // ### wrap around.
      // connect loose ends (inside the frame)
      int ncl= contourlines.size();
      float px,py;
      for (int ilp=0; ilp<nlp; ilp++) {
        if (vlp[ilp].pfirst<vlp[ilp].plast) {
          if (vlp[ilp].connect[0][0]<0 && !vlp[ilp].addfirst) {
            n= vlp[ilp].pfirst;
            px= x[n] + (x[n]-x[n+1])*0.1;
            py= y[n] + (y[n]-y[n+1])*0.1;
            if (px>gx1 && px<gx2 && py>gy1 && py<gy2) {
              i= int(px);
              j= int(py);
              px= float(i)+0.5f;
              py= float(j)+0.5f;
              vlp[ilp].xfirst= px;
              vlp[ilp].yfirst= py;
              vlp[ilp].addfirst= true;
              if (shading) {
                // mark the loose end
                int index=j*nx+i;
                int line_code= ncl+ilp;
                ibit2 = (j-iy1)*ixrange + (i-ix1);
                iwrd2 = ibit2/nbitwd;
                ibit2 = ibit2%nbitwd;
                if ((undefSquare[iwrd2] & bit[ibit2]) == 0) {
                  if      (fabsf(y[n]-float(j))  <0.1) iside=0;
                  else if (fabsf(y[n]-float(j+1))<0.1) iside=2;
                  else if (fabsf(x[n]-float(i))  <0.1) iside=1;
                  else                                 iside=3;
                  borderConnect[index][iside]= line_code;
                } else if (fabsf(px-x[n])<0.1) {
                  if (zOriginal[index]==fieldUndef || zOriginal[index+nx]==fieldUndef)
                    horAttach[j-iy0][px-0.01f]= line_code;
                  else
                    horAttach[j-iy0][px+0.01f]= line_code;
                } else if (fabsf(py-y[n])<0.1) {
                  if (zOriginal[index]==fieldUndef || zOriginal[index+1]==fieldUndef)
                    verAttach[i-ix0][py-0.01f]= line_code;
                  else
                    verAttach[i-ix0][py+0.01f]= line_code;
                  //		  } else {
                  //		    ........................mer seinere ???.............
                }
              }
            }
          }
          if (vlp[ilp].connect[1][0]<0 && !vlp[ilp].addlast) {
            n= vlp[ilp].plast;
            px= x[n] + (x[n]-x[n-1])*0.1;
            py= y[n] + (y[n]-y[n-1])*0.1;
            if (px>gx1 && px<gx2 && py>gy1 && py<gy2) {
              i= int(px);
              j= int(py);
              px= float(i)+0.5f;
              py= float(j)+0.5f;
              vlp[ilp].xlast= px;
              vlp[ilp].ylast= py;
              vlp[ilp].addlast= true;
              if (shading) {
                // mark the loose end
                int index=j*nx+i;
                int line_code= ncl+ilp+maxLines;
                ibit2 = (j-iy1)*ixrange + (i-ix1);
                iwrd2 = ibit2/nbitwd;
                ibit2 = ibit2%nbitwd;
                if ((undefSquare[iwrd2] & bit[ibit2]) == 0) {
                  if      (fabsf(y[n]-float(j))  <0.1) iside=0;
                  else if (fabsf(y[n]-float(j+1))<0.1) iside=2;
                  else if (fabsf(x[n]-float(i))  <0.1) iside=1;
                  else                                 iside=3;
                  borderConnect[index][iside]= line_code;
                } else if (fabsf(px-x[n])<0.1) {
                  if (zOriginal[index]==fieldUndef || zOriginal[index+nx]==fieldUndef)
                    horAttach[j-iy0][px-0.01f]= line_code;
                  else
                    horAttach[j-iy0][px+0.01f]= line_code;
                } else if (fabsf(py-y[n])<0.1) {
                  if (zOriginal[index]==fieldUndef || zOriginal[index+1]==fieldUndef)
                    verAttach[i-ix0][py-0.01f]= line_code;
                  else
                    verAttach[i-ix0][py+0.01f]= line_code;
                  //		  } else {
                  //		    ........................mer seinere ???.............
                }
              }
            }
          }
        } else if (vlp[ilp].pfirst==vlp[ilp].plast) {
          //...............................................................
          bool usepos= true;
          //...............................................................
          float px1,py1,px2,py2;
          int index1,index2,isideb1,isideb2;
          n= vlp[ilp].pfirst;
          i= int(x[n]+0.1);
          j= int(y[n]+0.1);
          px1=px2=x[n];
          py1=py2=y[n];
          index1=index2=j*nx+i;
          if (fabsf(y[n]-float(j))<0.1) {
            //...............................................................
            if (undefOriginal && (zOriginal[index1]==fieldUndef ||
                zOriginal[index1+1]==fieldUndef)) usepos=false;
            //...............................................................
            bool pdir;
            if (!vlp[ilp].addfirst && !vlp[ilp].addlast)
              pdir= (z[index1]==zleft);
            else if ((vlp[ilp].addfirst && vlp[ilp].yfirst<y[n]) ||
                (vlp[ilp].addlast  && vlp[ilp].ylast >y[n]))
              pdir= true;
            else
              pdir= false;
            if (pdir) {
              py1-=0.5;
              py2+=0.5;
              isideb1=2;
              isideb2=0;
              index1-=nx;
            } else {
              py1+=0.5;
              py2-=0.5;
              isideb1=0;
              isideb2=2;
              index2-=nx;
            }
          } else {
            //...............................................................
            if (undefOriginal && (zOriginal[index1]==fieldUndef ||
                zOriginal[index1+nx]==fieldUndef)) usepos=false;
            //...............................................................
            bool pdir;
            if (!vlp[ilp].addfirst && !vlp[ilp].addlast)
              pdir= (z[index1]==zright);
            else if ((vlp[ilp].addfirst && vlp[ilp].xfirst<x[n]) ||
                (vlp[ilp].addlast  && vlp[ilp].xlast >x[n]))
              pdir= true;
            else
              pdir= false;
            if (pdir) {
              px1-=0.5;
              px2+=0.5;
              isideb1=3;
              isideb2=1;
              index1--;
            } else {
              px1+=0.5;
              px2-=0.5;
              isideb1=1;
              isideb2=3;
              index2--;
            }
          }
          if (usepos && vlp[ilp].connect[0][0]<0 && !vlp[ilp].addfirst) {
            if (px1>gx1 && px1<gx2 && py1>gy1 && py1<gy2) {

              i= index1%nx;
              j= index1/nx;
              ibit2 = (j-iy1)*ixrange + (i-ix1);
              iwrd2 = ibit2/nbitwd;
              ibit2 = ibit2%nbitwd;
              if ((undefAllSquare[iwrd2] & bit[ibit2]) == 0) {
                vlp[ilp].xfirst= px1;
                vlp[ilp].yfirst= py1;
                vlp[ilp].addfirst= true;
                if (shading) {
                  // mark the loose end
                  int line_code= ncl+ilp;
                  if ((undefSquare[iwrd2] & bit[ibit2]) == 0) {
                    borderConnect[index1][isideb1]= line_code;
                  } else if (fabsf(px1-x[n])<0.1) {
                    if (zOriginal[index1]==fieldUndef || zOriginal[index1+nx]==fieldUndef)
                      horAttach[j-iy0][px1-0.01f]= line_code;
                    else
                      horAttach[j-iy0][px1+0.01f]= line_code;
                  } else if (fabsf(py1-y[n])<0.1) {
                    if (zOriginal[index1]==fieldUndef || zOriginal[index1+1]==fieldUndef)
                      verAttach[i-ix0][py1-0.01f]= line_code;
                    else
                      verAttach[i-ix0][py1+0.01f]= line_code;
                    //		    } else {
                    //		      ........................mer seinere ???.............
                  }
                }
              }
            }
          }
          if (usepos && vlp[ilp].connect[1][0]<0 && !vlp[ilp].addlast) {
            if (px2>gx1 && px2<gx2 && py2>gy1 && py2<gy2) {

              i= index2%nx;
              j= index2/nx;
              ibit2 = (j-iy1)*ixrange + (i-ix1);
              iwrd2 = ibit2/nbitwd;
              ibit2 = ibit2%nbitwd;
              if ((undefAllSquare[iwrd2] & bit[ibit2]) == 0) {
                vlp[ilp].xlast= px2;
                vlp[ilp].ylast= py2;
                vlp[ilp].addlast= true;
                if (shading) {
                  // mark the loose end
                  int line_code= ncl+ilp+maxLines;
                  if ((undefSquare[iwrd2] & bit[ibit2]) == 0) {
                    borderConnect[index2][isideb2]= line_code;
                  } else if (fabsf(px2-x[n])<0.1) {
                    if (zOriginal[index2]==fieldUndef || zOriginal[index2+nx]==fieldUndef)
                      horAttach[j-iy0][px2-0.01f]= line_code;
                    else
                      horAttach[j-iy0][px2+0.01f]= line_code;
                  } else if (fabsf(py2-y[n])<0.1) {
                    if (zOriginal[index2]==fieldUndef || zOriginal[index2+1]==fieldUndef)
                      verAttach[i-ix0][py2-0.01f]= line_code;
                    else
                      verAttach[i-ix0][py2+0.01f]= line_code;
                    //		    } else {
                    //		      ........................mer seinere ???.............
                  }
                }
              }
            }
          }
        }
      }
    }

    if (!closed && nlp>0 && shading) {
      int iatt[2],catt[2];
      int natt= 0;
      if (!vlp[0].addfirst &&
          (vlp[0].pfirst<vlp[0].plast || vlp[0].addlast || lev==nundef)) {
        iatt[natt]  = vlp[0].pfirst;
        catt[natt++]= contourlines.size();
      }
      if (!vlp[nlp-1].addlast &&
          (vlp[nlp-1].pfirst<vlp[nlp-1].plast || vlp[nlp-1].addfirst || lev==nundef)) {
        iatt[natt]  = vlp[nlp-1].plast;
        catt[natt++]= contourlines.size()+nlp-1 + maxLines;
      }
      for (n=0; n<natt; n++) {
        j= iatt[n];
        float dist[4];
        dist[0]= y[j] - gy1;
        dist[1]= gx2  - x[j];
        dist[2]= gy2  - y[j];
        dist[3]= x[j] - gx1;
        k= 0;
        for (i=1; i<4; i++)
          if (dist[i]<dist[k]) k=i;
        if (dist[k]<0.01) {  // hmmmmm.....
          if      (k==0) horAttach[0]      [x[j]]= catt[n];
          else if (k==1) verAttach[ixrange][y[j]]= catt[n];
          else if (k==2) horAttach[iyrange][x[j]]= catt[n];
          else           verAttach[0]      [y[j]]= catt[n];
        }
      }
    }

    float *xtotal= x;
    float *ytotal= y;
    int *indxgxtotal= indxgx;
    int *indxgytotal= indxgy;
    alp.addfirst= false;
    alp.addlast=  false;

    for (int ilp=0; ilp<nlp; ilp++) {

      if (alp.addfirst) {
        xtotal[alp.pfirst]= alp.xfirst;
        ytotal[alp.pfirst]= alp.yfirst;
      }
      if (alp.addlast) {
        xtotal[alp.plast]= alp.xlast;
        ytotal[alp.plast]= alp.ylast;
      }
      alp.addfirst= vlp[ilp].addfirst;
      alp.addlast=  vlp[ilp].addlast;
      if (alp.addfirst) {
        alp.pfirst= vlp[ilp].pfirst - 1;
        alp.xfirst= xtotal[alp.pfirst];
        alp.yfirst= ytotal[alp.pfirst];
        xtotal[alp.pfirst]= vlp[ilp].xfirst;
        ytotal[alp.pfirst]= vlp[ilp].yfirst;
      }
      if (alp.addlast) {
        alp.plast= vlp[ilp].plast + 1;
        alp.xlast= xtotal[alp.plast];
        alp.ylast= ytotal[alp.plast];
        xtotal[alp.plast]= vlp[ilp].xlast;
        ytotal[alp.plast]= vlp[ilp].ylast;
      }

      n= (vlp[ilp].addfirst) ? vlp[ilp].pfirst-2 : vlp[ilp].pfirst-1;
      i= (vlp[ilp].addlast)  ? vlp[ilp].plast+1  : vlp[ilp].plast;
      npos= i-n;

      x= &xtotal[n];
      y= &ytotal[n];
      indxgx= &indxgxtotal[n];
      indxgy= &indxgytotal[n];

      closed= vlp[ilp].closed;

      if (!closed) {
        np1 = 1;
        np2 = npos;
      } else {
        np1  = 0;
        np2  = npos+1;
        x[0] = x[npos-1];
        y[0] = y[npos-1];
        x[np2] = x[2];
        y[np2] = y[2];
      }

      if (nzfrmt==0 || shading || lev==nundef) {
        // draw line without labeling of contour
        if (ismooth<1 || lev==nundef){
          if (lev<nundef && !shading) {
            if      (iconv==1) posConvert(npos, &x[1], &y[1], cxy);
            else if (iconv==2) posConvert(npos, &x[1], &y[1], nx, ny, xz, yz);
            drawLine(gl, 1,npos,x,y);
          }
          if (shading) {
            ContourLine *cl= new ContourLine();
            cl->original= true;
            if (lev==nundef && !linesplit) {
              int l= 0;
              while (l<nvalue && zreal>=zvalue[l]) l++;
              l--;
              if (l>=0) cl->ivalue= ivalue[l];
              else      cl->ivalue= -300;
              cl->value= zreal;
            } else {
              cl->ivalue= ivalue[lev];
              cl->value= zc;
            }
            cl->highLeft= highLeft;
            cl->closed= closed;
            cl->izleft=  izleft;
            cl->izright= izright;
            // pointreduction......................................................
            if (drawBorders && iconv<2 && lev!=nundef && npos>2) {
              // remove points on straight lines (faster shading)
              float dx1, dx2=x[2]-x[1];
              float dy1, dy2=y[2]-y[1];
              np=2;
              for (i=2; i<npos; ++i) {
                dx1= dx2;
                dy1= dy2;
                dx2=x[i+1]-x[i];
                dy2=y[i+1]-y[i];
                if (fabsf(dx2-dx1)>0.1f || fabsf(dy2-dy1)>0.1f) {
                  if (!((fabsf(dx1)<0.1f && fabsf(dx2)<0.1f) ||
                      (fabsf(dy1)<0.1f && fabsf(dy2)<0.1f))) {
                    x[np]= x[i];
                    y[np]= y[i];
                    np++;
                  }
                }
              }
              x[np]= x[npos];
              y[np]= y[npos];
              npos= np;
            }
            // ..................................................................
            cl->npos= npos;
            cl->xpos= new float[npos];
            cl->ypos= new float[npos];
            for (i=0; i<npos; ++i) {
              cl->xpos[i]= x[i+1];
              cl->ypos[i]= y[i+1];
            }
            cl->undefLine= (lev==nundef);
            cl->isoLine=   (lev!=nundef);
            cl->undefTouch= false;
            cl->undefLeft=  false;
            cl->undefRight= false;
            cl->outer=      false;
            cl->inner=      false;
            cl->direction= 0; // not determined yet
            cl->highInside= false; // but not determined yet
            cl->corner= false;
            cl->connect[0][0]= vlp[ilp].connect[0][0];
            cl->connect[0][1]= vlp[ilp].connect[0][1];
            cl->connect[1][0]= vlp[ilp].connect[1][0];
            cl->connect[1][1]= vlp[ilp].connect[1][1];
            contourlines.push_back(cl);
          }
        } else {
          if (!closed) {
            ns = 1;
            np = npos;
            nfrst = 0;
            nlast = npos-1;
          } else {
            ns = 0;
            np = npos+2;
            nfrst = 1;
            nlast = npos;
          }
          l = smoothline(np,&x[ns],&y[ns],nfrst,nlast,
              ismooth,&xsmooth[0],&ysmooth[0]);
          if (l>1) {
            if (!shading) {
              if      (iconv==1) posConvert(l, xsmooth, ysmooth, cxy);
              else if (iconv==2) posConvert(l, xsmooth, ysmooth, nx, ny, xz, yz);
              drawLine(gl, 0,l-1,xsmooth,ysmooth);
            }
            if (shading) {
              ContourLine *cl= new ContourLine();
              cl->original= true;
              cl->ivalue= ivalue[lev];
              cl->value= zc;
              cl->highInside= false; // but not determined yet
              cl->highLeft= highLeft;
              cl->closed= closed;
              cl->izleft=  izleft;
              cl->izright= izright;
              cl->direction= 0;
              cl->npos= l;
              cl->xpos= new float[l];
              cl->ypos= new float[l];
              for (i=0; i<l; ++i) {
                cl->xpos[i]= xsmooth[i];
                cl->ypos[i]= ysmooth[i];
              }
              cl->isoLine=    true;
              cl->undefTouch= false;
              cl->undefLeft=  false;
              cl->undefRight= false;
              cl->outer=  false;
              cl->inner=  false;
              cl->corner= false;
              cl->undefLine= (lev==nundef);
              cl->connect[0][0]= vlp[ilp].connect[0][0];
              cl->connect[0][1]= vlp[ilp].connect[0][1];
              cl->connect[1][0]= vlp[ilp].connect[1][0];
              cl->connect[1][1]= vlp[ilp].connect[1][1];
              contourlines.push_back(cl);
            }
          }
        }
        continue; // ilp loop
      }

      // contour labeling

      for (n=1; n<=npos; n++) {
        indxgx[n]= int(x[n]);
        indxgy[n]= int(y[n]);
      }

      if      (iconv==1) posConvert(npos, &x[1], &y[1], cxy);
      else if (iconv==2) posConvert(npos, &x[1], &y[1], nx, ny, xz, yz);

      // label format for each label
      if (drawBorders) {
        float zcb1= (zleft<zright) ? zleft : zright;
        float zcb2= (zleft>zright) ? zleft : zright;
        strlabel = QString("%1 / %2").arg(zcb1).arg(zcb2);
        gl->getTextSize(strlabel, dxlab1, dxlab2);
        dxlab  = stalab+dxlab1+endlab;
        dxlab2 = dxlab*dxlab;
        splim2 = (dxlab+0.1*chrx*rchrx);
        splim2 = splim2*splim2;
        dclab[1] = dxlab1+dclab[0]*2.;
        zzstp= fabsf(zleft-zright);
      }

      // compute length of each segment (used for labeling).
      nposm1 = npos-1;
      s[1] = 0.;

      for (n=2; n<=npos; ++n) {
        dxx = x[n]-x[n-1];
        dyy = y[n]-y[n-1];
        s[n] = s[n - 1] + miutil::absval(dxx, dyy);
      }

      smax=  s[npos]-dxlab*2.0;
      stest= dslab-s[npos]*slab1f;

      // avoid vertical labels (if not vertical straight line)
      if (drawBorders) {
        dxylab=0.00;
      } else {
        dxx = x[npos]-x[1];
        dyy = y[npos]-y[1];
        if (miutil::absval(dxx, dyy) / (s[npos] - s[1]) < 0.90)
          dxylab=0.40;
        else
          dxylab=0.00;
      }

      n3 = 1;

      while (n3<npos) {
        n1 = n3;
        n  = n3;

        // check if label is to be added to contour.
        while (stest<=dslab && n<nposm1) {
          n++;
          stest+=(s[n]-s[n-1]);
        }
        if (stest<=dslab) {
          label = -1;
        } else {
          label = 0;
          n--;
        }

        while (label==0 && s[n+1]<smax && n<nposm1) {
          n++;

          // check horizontal space

          space2 = 0.;
          n2=n;
          n3=n;
          while (space2<=dxlab2 && n3<npos) {
            std::vector<std::string> bstr1, bstr2;

            n3++;
            dxx = x[n3]-x[n];
            dyy = y[n3]-y[n];
            space2 = dxx*dxx+dyy*dyy;
          }

          // avoid vertical label (if not vertical straight line)
          if (fabsf(dxx)<fabsf(dyy)*dxylab) space2=0.0;

          if (space2>dxlab2) {
            space = sqrtf(space2);
            arc   = s[n3]-s[n];

            if (space/arc>0.95) {
              // check vertical space

              // find centre position (better than position no. mean)
              sp2lim = space2*0.25;
              sp2 = 0.;
              km = n;
              while (sp2<sp2lim && km<n3) {
                km++;
                dxx = x[km]-x[n];
                dyy = y[km]-y[n];
                sp2 = dxx*dxx+dyy*dyy;
              }
              km--;
              iz = (indxgx[km]<ix2-1) ? indxgx[km] : ix2-1;
              jz = (indxgy[km]<iy2-1) ? indxgy[km] : iy2-1;
              ij = iz + jz*nx;
              if (iconv==2) {
                dxx = (xz[ij+1]-xz[ij]+xz[ij+1+nx]-xz[ij+nx])*0.5;
                dyy = (yz[ij+1]-yz[ij]+yz[ij+1+nx]-yz[ij+nx])*0.5;
                if (dxx != 0. || dyy != 0.)
                  nlabx = int(dylabh / miutil::absval(dxx, dyy) + 0.5);
                else nlabx=0;

                dxx = (xz[ij+nx]-xz[ij]+xz[ij+1+nx]-xz[ij+1])*0.5;
                dyy = (yz[ij+nx]-yz[ij]+yz[ij+1+nx]-yz[ij+1])*0.5;
                if(dxx != 0. || dyy != 0.)
                  nlaby = int(dylabh / miutil::absval(dxx, dyy) + 0.5);
                else nlaby = 0;
              }

              if (nlabx+nlaby==0) {
                iz1 = iz;
                iz2 = iz+1;
                jz1 = jz;
                jz2 = jz+1;
                zzg[0][0] = z[iz1+jz1*nx];
                zzg[0][1] = z[iz2+jz1*nx];
                zzg[1][0] = z[iz1+jz2*nx];
                zzg[1][1] = z[iz2+jz2*nx];
              } else {
                iz1 = (iz-nlabx  >ix1) ? iz-nlabx   : ix1;
                iz2 = (iz+nlabx+1<ix2) ? iz+nlabx+1 : ix2;
                jz1 = (jz-nlaby  >iy1) ? jz-nlaby   : iy1;
                jz2 = (jz+nlaby+1<iy2) ? jz+nlaby+1 : iy2;
                izg[0][0] = iz1;
                izg[0][1] = iz;
                izg[1][0] = iz+1;
                izg[1][1] = iz2;
                jzg[0][0] = jz1;
                jzg[0][1] = jz;
                jzg[1][0] = jz+1;
                jzg[1][1] = jz2;
                for (nj=0; nj<2; ++nj) {
                  for (ni=0; ni<2; ++ni) {
                    zzg[nj][ni] = 0.;
                    nzz = 0;
                    for (j=jzg[nj][0]; j<=jzg[nj][1]; ++j) {
                      for (i=izg[ni][0]; i<=izg[ni][1]; ++i) {
                        if (z[i+j*nx]<zhigh) {
                          zzg[nj][ni] = zzg[nj][ni]+z[i+j*nx];
                          nzz++;
                        }
                      }
                    }
                    if (nzz>0) zzg[nj][ni] = zzg[nj][ni]/float(nzz);
                    else       zzg[nj][ni] = zhigh;
                  }
                }
              }

              if (iconv==0) {
                dxx = float(iz2-iz1+1)*0.5; dxx = dxx*dxx;
                dyy = float(jz2-jz1+1)*0.5; dyy = dyy*dyy;
              } else if (iconv==1) {
                dxx = float(iz2-iz1+1)*0.5;
                dyy = float(jz2-jz1+1)*0.5;
                dxx = (cxy[1]*dxx)*(cxy[1]*dxx) + (cxy[4]*dxx)*(cxy[4]*dxx);
                dyy = (cxy[2]*dyy)*(cxy[2]*dyy) + (cxy[5]*dyy)*(cxy[5]*dyy);
              } else {
                dxz = (xz[iz2+jz1*nx]+xz[iz+1+jz1*nx]
                       -xz[iz1+jz1*nx]-xz[iz+jz1*nx]
                       +xz[iz2+jz2*nx]+xz[iz+1+jz2*nx]
                       -xz[iz1+jz2*nx]-xz[iz+jz2*nx])*0.5;
                dyz = (yz[iz2+jz1*nx]+yz[iz+1+jz1*nx]
                       -yz[iz1+jz1*nx]-yz[iz+jz1*nx]
                       +yz[iz2+jz2*nx]+yz[iz+1+jz2*nx]
                       -yz[iz1+jz2*nx]-yz[iz+jz2*nx])*0.5;
                dxx = dxz*dxz+dyz*dyz;
                if (dxx==0.) dxx=1.;
                dxz = (xz[iz1+jz2*nx]+xz[iz1+(jz+1)*nx]
                       -xz[iz1+jz1*nx]-xz[iz1+jz*nx]
                       +xz[iz2+jz2*nx]+xz[iz2+(jz+1)*nx]
                       -xz[iz2+jz1*nx]-xz[iz2+jz*nx])*0.5;
                dyz = (yz[iz1+jz2*nx]+yz[iz1+(jz+1)*nx]
                       -yz[iz1+jz1*nx]-yz[iz1+jz*nx]
                       +yz[iz2+jz2*nx]+yz[iz2+(jz+1)*nx]
                       -yz[iz2+jz1*nx]-yz[iz2+jz*nx])*0.5;
                dyy = dxz*dxz+dyz*dyz;
                if (dyy==0.) dyy=1.;
              }

              if (zzg[0][0]<zhigh && zzg[0][1]<zhigh &&
                  zzg[1][0]<zhigh && zzg[1][1]<zhigh) {
                ftmp1 = fabsf(zzg[0][1]-zzg[0][0]);
                ftmp2 = fabsf(zzg[1][1]-zzg[1][0]);
                dzdx = (ftmp1>ftmp2) ? ftmp1 : ftmp2;
                ftmp1 = fabsf(zzg[1][0]-zzg[0][0]);
                ftmp2 = fabsf(zzg[1][1]-zzg[0][1]);
                dzdy = (ftmp1>ftmp2) ? ftmp1 : ftmp2;
              } else {
                dzdx = 0.;
                dzdy = 0.;
                if (zzg[0][0]<zhigh && zzg[0][1]<zhigh) {
                  dzdx = zzg[0][1]-zzg[0][0];
                } else if (zzg[1][0]<zhigh && zzg[1][1]<zhigh) {
                  dzdx = zzg[1][1]-zzg[1][0];
                } else if (zzg[0][0]<zhigh && zzg[1][1]<zhigh) {
                  dzdx = zzg[1][1]-zzg[0][0];
                  dxx  = dxx+dyy;
                } else if (zzg[1][0]<zhigh && zzg[0][1]<zhigh) {
                  dzdx = zzg[0][1]-zzg[1][0];
                  dxx  = dxx+dyy;
                }
                if (zzg[0][0]<zhigh && zzg[1][0]<zhigh)
                  dzdy = zzg[1][0]-zzg[0][0];
                else if (zzg[0][1]<zhigh && zzg[1][1]<zhigh)
                  dzdy = zzg[1][1]-zzg[0][1];
              }
              dzdg = sqrtf(dzdx*dzdx/dxx + dzdy*dzdy/dyy);

              if (zzstp>dylab*dzdg) {
                // check the nearest parts of the line (horizontal)
                xh = (x[n]+x[n3])*0.5;
                yh = (y[n]+y[n3])*0.5;
                dxx = x[n]-xh;
                dyy = y[n]-yh;
                space2 = dxx*dxx+dyy*dyy;
                arc = sqrtf(space2);
                k1 = n;
                while (space2<dxlab2 && k1>2) {
                  k1--;
                  dxx = x[k1]-xh;
                  dyy = y[k1]-yh;
                  space2 = dxx*dxx+dyy*dyy;
                }
                if (space2>=dxlab2) ratio = sqrtf(space2)/(arc+s[n]-s[k1]);
                else                ratio = 1.;
                if (ratio>0.80) {
                  dxx = x[n3]-xh;
                  dyy = y[n3]-yh;
                  space2 = dxx*dxx+dyy*dyy;
                  arc = sqrtf(space2);
                  k2 = n3;
                  while (space2<dxlab2 && k2<npos) {
                    k2++;
                    dxx = x[k2]-xh;
                    dyy = y[k2]-yh;
                    space2 = dxx*dxx+dyy*dyy;
                  }
                  if (space2>=dxlab2) ratio = sqrtf(space2)/(arc+s[k2]-s[n3]);
                  else                ratio = 1.;
                }

                if (ratio>0.80) {
                  // draw label at same angle as current line segment
                  // find start and end position
                  // (as far from position 'n' and 'n3' as possible)
                  xs1 = x[n];
                  xs2 = x[n+1];
                  xe1 = x[n3-1];
                  xe2 = x[n3];
                  ys1 = y[n];
                  ys2 = y[n+1];
                  ye1 = y[n3-1];
                  ye2 = y[n3];
                  dxx = xe2-xs1;
                  dyy = ye2-ys1;
                  space2 = dxx*dxx+dyy*dyy;

                  // nspace2 test may be needed due to precision problems
                  nspace2 = 0;
                  while (space2>splim2 && nspace2<100) {
                    xs = (xs1+xs2)*0.5;
                    ys = (ys1+ys2)*0.5;
                    xe = (xe1+xe2)*0.5;
                    ye = (ye1+ye2)*0.5;
                    if ((xe-xs)*(xe-xs)+(ye-ys)*(ye-ys) > dxlab2) {
                      xs1 = xs;
                      ys1 = ys;
                      xe2 = xe;
                      ye2 = ye;
                    } else {
                      xs2 = xs;
                      ys2 = ys;
                      xe1 = xe;
                      ye1 = ye;
                    }
                    dxx = xe2-xs1;
                    dyy = ye2-ys1;
                    space2 = dxx*dxx+dyy*dyy;
                    nspace2++;
                  }

                  xlabel = xs1;
                  ylabel = ys1;
                  xend = xe2;
                  yend = ye2;

                  if (dxx>=0.0) {
                    xlab = xs1;
                    ylab = ys1;
                  } else {
                    dxx = -dxx;
                    dyy = -dyy;
                    xlab = xe2;
                    ylab = ye2;
                  }

                  space = sqrtf(space2);
                  dxx = dxx/space;
                  dyy = dyy/space;
                  xlab = xlab+stalab*dxx+dylabh*dyy;
                  ylab = ylab+stalab*dyy-dylabh*dxx;

                  // avoid labels crossing border (frame)

                  xlc[0] = xlab-dclab[0]*dxx+dclab[0]*dyy;
                  ylc[0] = ylab-dclab[0]*dyy-dclab[0]*dxx;
                  xlc[1] = xlc[0]+dclab[1]*dxx;
                  ylc[1] = ylc[0]+dclab[1]*dyy;
                  xlc[2] = xlc[1]-dclab[2]*dyy;
                  ylc[2] = ylc[1]+dclab[2]*dxx;
                  xlc[3] = xlc[0]-dclab[3]*dyy;
                  ylc[3] = ylc[0]+dclab[3]*dxx;

                  if (xlc[0]>xlim1 && xlc[3]>xlim1 &&
                      xlc[1]<xlim2 && xlc[2]<xlim2 &&
                      ylc[0]>ylim1 && ylc[1]>ylim1 &&
                      ylc[2]<ylim2 && ylc[3]<ylim2) {
                    // check if other parts of the line are very close
                    xh = x[km];
                    yh = y[km];
                    space = (dxlab<dylab) ? dxlab : dylab;
                    space2 = space*space*2.;
                    for (i=1; i<=k1-1; ++i) {
                      dxz = x[i]-xh;
                      dyz = y[i]-yh;
                      ftmp = dxz*dxz+dyz*dyz;
                      if (space2>ftmp) space2 = ftmp;
                    }
                    for (i=k2+1; i<=npos; ++i) {
                      dxz = x[i]-xh;
                      dyz = y[i]-yh;
                      ftmp = dxz*dxz+dyz*dyz;
                      if (space2>ftmp) space2 = ftmp;
                    }

                    if (space2 > space*space) {
                      label = 1;
                      if (ibmap==1) {
                        // check if position is free in the bit map
                        for (k=0; k<4; ++k) {
                          xlb[k] = (xlc[k]-rbmap[0])/rbmap[2];
                          ylb[k] = (ylc[k]-rbmap[1])/rbmap[3];
                        }
                        nbx = int(xlb[1]) - int(xlb[0]);
                        j   = int(ylb[1]) - int(ylb[0]);
                        nby = int(ylb[3]) - int(ylb[0]);
                        i   = int(xlb[3]) - int(xlb[0]);
                        if (ylb[1]>ylb[0]) i = -i;
                        else               j = -j;
                        if (nbx<j) nbx = j;
                        if (nby<i) nby = i;
                        xbmap = xlb[0];
                        ybmap = ylb[0];
                        dxbx = (xlb[1]-xbmap)/float(nbx);
                        dybx = (ylb[1]-ybmap)/float(nbx);
                        dxby = (xlb[3]-xbmap)/float(nby);
                        dyby = (ylb[3]-ybmap)/float(nby);
                        for (jbm=0; jbm<=nby; ++jbm) {
                          ybm = float(jbm);
                          for (ibm=0; ibm<=nbx; ++ibm) {
                            xbm = float(ibm);
                            i=int(xbmap+dxby*ybm+dxbx*xbm);
                            j=int(ybmap+dyby*ybm+dybx*xbm);
                            ibit = j*nxbmap+i;
                            iwrd = ibit/nbitwd;
                            ibit = ibit%nbitwd;
                            if ((kbmap[iwrd] & bit[ibit]) != 0) label=0;
                          }
                        }
                        if (label==1) {
                          // this was the last test for label,
                          // mark label area as used in the bitmap
                          for (jbm=0; jbm<=nby; ++jbm) {
                            ybm = float(jbm);
                            for (ibm=0; ibm<=nbx; ++ibm) {
                              xbm = float(ibm);
                              i=int(xbmap+dxby*ybm+dxbx*xbm);
                              j=int(ybmap+dyby*ybm+dybx*xbm);
                              ibit = j*nxbmap+i;
                              iwrd = ibit/nbitwd;
                              ibit = ibit%nbitwd;
                              kbmap[iwrd]|=bit[ibit];
                            }
                          }
                        }
                      }
                    } else n=n3-1;
                  } else n=n3-1;
                }
              }
            }
          }

          // speed up search on long lines
          //???	  if (label==0 && n3-n>3 && n<npos-10) n=(n+n3)/2-1;
        }

        if (label != 1) {
          n2 = npos;
          n3 = npos;
        }

        // draw line from position 'n1' to 'n2'

        if (ismooth<1) {
          drawLine(gl, n1,n2,x,y);
        } else {
          // line smoothing
          if (n1>np1) {
            ns    = n1-1;
            np    = n2-n1+2;
            nfrst = 1;
            nlast = np-1;
          } else {
            ns    = n1;
            np    = n2-n1+1;
            nfrst = 0;
            nlast = np-1;
          }
          if (n2<np2) np++;
          l = smoothline(np,&x[ns],&y[ns],nfrst,nlast,
              ismooth,&xsmooth[0],&ysmooth[0]);
          if (l>1) {
            drawLine(gl, 0,l-1,xsmooth,ysmooth);
          }
        }

        if(label != 1) continue; // ilp loop

        // draw line from position 'n2' to (xlabel,ylabel)
        float xx[2], yy[2];
        xx[0] = x[n2];
        yy[0] = y[n2];
        xx[1] = xlabel;
        yy[1] = ylabel;
        drawLine(gl, 0,1,xx,yy);

        if (ibcol>=0) {
          // blank background for label
          xlc[4] = xlc[0];
          ylc[4] = ylc[0];
          // pfa(5,xlc,ylc);
        }

        angle = atan2f(dyy,dxx) * RAD_TO_DEG;

//        GLint *  params = new int[4];
//        glGetIntegerv(DiGLPainter::gl_CURRENT_COLOR,  params);
//        glColor3ubv(poptions.textcolour.RGB());
        gl->drawText(strlabel,xlab,ylab,angle);

        // needed after drawStr, otherwise colour change may not work
        gl->ShadeModel(DiGLPainter::gl_FLAT);
//        glColor4i(params[0],params[1],params[2],params[3]);

        // draw line from (xend,yend) to 'n3'
        xx[0] = xend;
        yy[0] = yend;
        xx[1] = x[n3];
        yy[1] = y[n3];
        drawLine(gl, 0,1,xx,yy);

        // for next label
        dxx = x[n3]-xend;
        dyy = y[n3]-yend;
        stest = miutil::absval(dxx, dyy);

      }  // end while (n3<npos)

    }  // end ilp loop

    x= xtotal;
    y= ytotal;
    indxgx= indxgxtotal;
    indxgy= indxgytotal;

    goto startSearch;

  }  // end "lev" loop


  if (ismooth>0) {
    delete[] xsmooth;
    delete[] ysmooth;
  }
  delete[] iused;
  delete[] x;
  delete[] y;
  delete[] s;
  delete[] indxgx;
  delete[] indxgy;
  delete[] kab;

  if (shading && drawBorders && borderConnect.size()) {

    std::map<int, std::map<int, int>>::iterator pbc = borderConnect.begin();
    std::map<int, std::map<int, int>>::iterator pbcend = borderConnect.end();
    std::map<int, int>::iterator pc, pcend;
    int lcodes[4],lines[4],lends[4];

    for (; pbc!=pbcend; pbc++) {
      pc=    pbc->second.begin();
      pcend= pbc->second.end();
      n=0;
      for (; pc!=pcend; pc++) {
        i= pc->second;
        lcodes[n]=  i;
        lines[n]=   i%maxLines;
        lends[n++]= i/maxLines;
      }
      for (i=0; i<n; i++) {
        j= (i+1)%n;  // j=next i=current
        contourlines[lines[i]]->connect[lends[i]][0]= lcodes[j];
        contourlines[lines[j]]->connect[lends[j]][1]= lcodes[i];
      }
    }
  }

  borderConnect.clear();

  int ncl= contourlines.size();

  if (shading && contourlines.size()>0) {
    // connect lines crossing boundaries (and corner points)
    std::map<float, int>::iterator p, pend;
    std::map<float, int>::reverse_iterator r, rend;
    std::vector<int> sideAttach;

    pend= horAttach[0].end();
    for (p=horAttach[0].begin(); p!=pend; p++)
      sideAttach.push_back(p->second);
    int n0= sideAttach.size();

    pend= verAttach[ixrange].end();
    for (p=verAttach[ixrange].begin(); p!=pend; p++)
      sideAttach.push_back(p->second);
    int n1= sideAttach.size();

    rend= horAttach[iyrange].rend();
    for (r=horAttach[iyrange].rbegin(); r!=rend; r++)
      sideAttach.push_back(r->second);
    int n2= sideAttach.size();

    rend= verAttach[0].rend();
    for (r=verAttach[0].rbegin(); r!=rend; r++)
      sideAttach.push_back(r->second);

    int currcode,currend,currline=0;
    int nn= sideAttach.size();
    int prevcode= sideAttach[nn-1];
    int prevend=  prevcode/maxLines;
    int prevline= prevcode%maxLines;

    if (iconv!=2) {

      for (n=0; n<nn; n++) {
        currcode= sideAttach[n];
        currend=  currcode/maxLines;
        currline= currcode%maxLines;
        contourlines[prevline]->connect[prevend][0]= currcode;
        contourlines[currline]->connect[currend][1]= prevcode;
        prevcode= currcode;
        prevend=  currend;
        prevline= currline;
      }

    } else {

      // add points following curved side (iconv==2)
      float xprev,yprev,xcurr,ycurr;
      int nside,i1,i2,j1,j2;
      float *xside=0, *yside=0;
      ContourLine *clprev=0, *clcurr=0;
      ncl= contourlines.size();

      clprev= contourlines[prevline];
      if (prevend==0) i= 0;
      else            i= clprev->npos - 1;
      xprev= clprev->xpos[i];
      yprev= clprev->ypos[i];

      for (n=0; n<nn; n++) {
        currcode= sideAttach[n];
        currend=  currcode/maxLines;
        currline= currcode%maxLines;
        clcurr= contourlines[currline];
        if (currend==0) i= 0;
        else            i= clcurr->npos - 1;
        xcurr= clcurr->xpos[i];
        ycurr= clcurr->ypos[i];
        nside= 0;
        if (n<n0) {
          i1= int(xprev+1.1);
          i2= int(xcurr-0.1);
          if (i1<=i2) {
            nside= i2-i1+1;
            xside= new float[nside];
            yside= new float[nside];
            for (i=0; i<nside; i++) {
              xside[i]= float(i1++);
              yside[i]= gy1;
            }
          }
        } else if (n<n1) {
          j1= int(yprev+1.1);
          j2= int(ycurr-0.1);
          if (j1<=j2) {
            nside= j2-j1+1;
            xside= new float[nside];
            yside= new float[nside];
            for (i=0; i<nside; i++) {
              xside[i]= gx2;
              yside[i]= float(j1++);
            }
          }
        } else if (n<n2) {
          i1= int(xprev-0.1);
          i2= int(xcurr+1.1);
          if (i1>=i2) {
            nside= i1-i2+1;
            xside= new float[nside];
            yside= new float[nside];
            for (i=0; i<nside; i++) {
              xside[i]= float(i1--);
              yside[i]= gy2;
            }
          }
        } else {
          j1= int(yprev-0.1);
          j2= int(ycurr+1.1);
          if (j1>=j2) {
            nside= j1-j2+1;
            xside= new float[nside];
            yside= new float[nside];
            for (i=0; i<nside; i++) {
              xside[i]= gx1;
              yside[i]= float(j1--);
            }
          }
        }
        if (nside==0) {
          clprev->connect[prevend][0]= currcode;
          clcurr->connect[currend][1]= prevcode;
        } else {
          clprev->connect[prevend][0]= ncl;
          clcurr->connect[currend][1]= ncl + maxLines;
          ContourLine *cl= new ContourLine();
          cl->original=   true;
          cl->ivalue=     -300;
          cl->value=      -888.;
          cl->highLeft=   false;
          cl->closed=     false;
          cl->izleft=     -1;
          cl->izright=    -1;
          cl->npos=       nside;
          cl->xpos=       xside;
          cl->ypos=       yside;
          cl->undefLine=  false;
          cl->isoLine=    false;
          cl->undefTouch= false;
          cl->undefLeft=  false;
          cl->undefRight= true;
          cl->outer=      false;
          cl->inner=      false;
          cl->direction=  0;     // not determined yet
          cl->highInside= false; // not determined yet
          cl->corner=     true;
          cl->connect[0][0]= prevcode;
          cl->connect[0][1]= prevcode;
          cl->connect[1][0]= currcode;
          cl->connect[1][1]= currcode;
          contourlines.push_back(cl);
          ncl++;
        }
        prevcode= currcode;
        prevend=  currend;
        prevline= currline;
        clprev= clcurr;
        xprev= xcurr;
        yprev= ycurr;
      }
    }

    ncl= contourlines.size();
  }


  horAttach.clear();
  verAttach.clear();

  if (shading && !contourlines.empty()) {

    int nclfirst= contourlines.size();

    joinContours(contourlines,idraw,drawBorders,iconv);

    // pointreduction.................................................
    if (drawBorders && iconv<2) {
      // remove points on straight lines (faster shading)
      ncl= contourlines.size();
      for (j=nclfirst; j<ncl; j++) {
        if (contourlines[j]->npos > 5) {
          ContourLine* cl= contourlines[j];
          float* clx= cl->xpos;
          float* cly= cl->ypos;
          npos= cl->npos;
          float dx1, dx2=clx[1]-clx[0];
          float dy1, dy2=cly[1]-cly[0];
          np=1;
          for (i=1; i<npos-1; ++i) {
            dx1= dx2;
            dy1= dy2;
            dx2=clx[i+1]-clx[i];
            dy2=cly[i+1]-cly[i];
            if (fabsf(dx2-dx1)>0.1f || fabsf(dy2-dy1)>0.1f) {
              if (!((fabsf(dx1)<0.1f && fabsf(dx2)<0.1f) ||
                  (fabsf(dy1)<0.1f && fabsf(dy2)<0.1f))) {
                if (np!=i) {
                  clx[np]= clx[i];
                  cly[np]= cly[i];
                }
                np++;
              }
            }
          }
          if (np!=npos-1) {
            clx[np]= clx[npos-1];
            cly[np]= cly[npos-1];
          }
          np++;
          cl->npos= np;
        }
      }
    }
    // ...............................................................

    fillContours(gl, contourlines, nx, ny, z,
        iconv, cxy, xz, yz, idraw, poptions, drawBorders,
        fieldArea,zrange,zstep,zoff,fieldUndef);
  }

  if (contourlines.size()>0) {
    int ncl= contourlines.size();
    for (j=0; j<ncl; j++) {
      delete[] contourlines[j]->xpos;
      delete[] contourlines[j]->ypos;
      delete contourlines[j];
    }
    contourlines.clear();
  }

  if (undefReplaced) {
    delete[] zrep;
    zrep= 0;
    z= zOriginal;
    zOriginal= 0;
  }
  if (zOriginal) delete[] zOriginal;

  delete[] undefSquare;
  delete[] undefAllSquare;

  return true;
}

namespace {

int smoothline(int npos, float x[], float y[], int nfirst, int nlast,
    int ismooth, float xsmooth[], float ysmooth[])
{
  // Smooth line, make and return spline through points.
  //
  //  input:
  //     x(n),y(n), n=1,npos:   x and y in "window" coordinates
  //     x(nfrst),y(nfsrt):     first point
  //     x(nlast),y(nlast):     last  point
  //     ismooth:               number of points spline-interpolated
  //                            between each pair of input points
  //
  //  method: 'hermit interpolation'
  //     nfirst=0:      starting condition for spline = relaxed
  //     nfirst>0:      starting condition for spline = clamped
  //     nlast<npos-1:  ending   condition for spline = clamped
  //     nlast=npos-1:  ending   condition for spline = relaxed
  //        relaxed  -  second derivative is zero
  //        clamped  -  derivatives computed from nearest points

  int   ndivs, n, ns, i;
  float rdivs, xl1, yl1, s1, xl2, yl2, s2, dx1, dy1, dx2, dy2;
  float c32, c42, c31, c41, fx1, fx2, fx3, fx4, fy1, fy2, fy3, fy4;
  float tstep, t, t2, t3;

  bool contains_hugeval = false;
  for (int j = 0; j<npos; j++) {
    if(x[j] == HUGE_VAL || y[j] == HUGE_VAL) {
      contains_hugeval = true;
    }
  }

  if (npos<3 || nfirst<0 || nfirst>=nlast
      || nlast>npos-1 || ismooth<1 || contains_hugeval) {
    nfirst = (nfirst>0)     ? nfirst : 0;
    nlast  = (nlast<npos-1) ? nlast  : npos-1;
    ns = 0;
    for (n=nfirst; n<=nlast; ++n) {
      xsmooth[ns] = x[n];
      ysmooth[ns] = y[n];
      ++ns;
    }
    return ns;
  }

  ndivs = ismooth;
  rdivs = 1./float(ismooth+1);

  n = nfirst;
  if (n > 0)
  {
    xl1 = x[n]-x[n-1];
    yl1 = y[n]-y[n-1];
    s1 = miutil::absval(xl1, yl1);
    xl2 = x[n+1]-x[n];
    yl2 = y[n+1]-y[n];
    s2 = miutil::absval(xl2, yl2);
    dx2 = (xl1*(s2/s1)+xl2*(s1/s2))/(s1+s2);
    dy2 = (yl1*(s2/s1)+yl2*(s1/s2))/(s1+s2);
  }
  else
  {
    xl2 = x[n+1]-x[n];
    yl2 = y[n+1]-y[n];
    s2 = miutil::absval(xl2, yl2);
    dx2 = xl2/s2;
    dy2 = yl2/s2;
  }

  xsmooth[0] = x[nfirst];
  ysmooth[0] = y[nfirst];
  ns = 0;

  for (n=nfirst+1; n<=nlast; ++n)
  {
    xl1 = xl2;
    yl1 = yl2;
    s1  = s2;
    dx1 = dx2;
    dy1 = dy2;

    if (n < npos-1) {
      xl2 = x[n+1]-x[n];
      yl2 = y[n+1]-y[n];
      s2 = miutil::absval(xl2, yl2);
      dx2 = (xl1*(s2/s1)+xl2*(s1/s2))/(s1+s2);
      dy2 = (yl1*(s2/s1)+yl2*(s1/s2))/(s1+s2);
    }
    else {
      dx2 = xl1/s1;
      dy2 = yl1/s1;
    }

    // four spline coefficients for x and y
    c32 =  1./s1;
    c42 =  c32*c32;
    c31 =  c42*3.;
    c41 =  c42*c32*2.;
    fx1 =  x[n-1];
    fx2 =  dx1;
    fx3 =  c31*xl1-c32*(2.*dx1+dx2);
    fx4 = -c41*xl1+c42*(dx1+dx2);
    fy1 =  y[n-1];
    fy2 =  dy1;
    fy3 =  c31*yl1-c32*(2.*dy1+dy2);
    fy4 = -c41*yl1+c42*(dy1+dy2);

    // make 'ismooth' straight lines, from point 'n-1' to point 'n'

    tstep = s1*rdivs;
    t = 0.;

    for (i=0; i<ndivs; ++i) {
      t += tstep;
      t2 = t*t;
      t3 = t2*t;
      ns++;
      xsmooth[ns] = fx1 + fx2*t + fx3*t2 + fx4*t3;
      ysmooth[ns] = fy1 + fy2*t + fy3*t2 + fy4*t3;
    }

    ns++;
    xsmooth[ns] = x[n];
    ysmooth[ns] = y[n];
  }

  ns++;

  return ns;
}

int sortlines(float zvmin, float zvmax, int mvalue, int nvalue,
    float zvalue[], int ivalue[],
    int icolour[], int linetype[], int linewidth[],
    int idraw, float zrange[], float zstep, float zoff,
    int nlines, float rlines[],
    int ncol, int icol[], int ntyp, int ityp[],
    int nwid, int iwid[], int nlim, float rlim[],
    const float& fieldUndef)
{
  // make list of lines to draw, and colour, linetype and width to use.
  // possibly called twice, for primary and secondary plot options.
  //
  //-----------------------------------------------------------------
  //  DNMI/FoU  05.05.1999  Anstein Foss ... dfelt
  //  DNMI/FoU  15.08.1999  Anstein Foss ... C/C++
  //-----------------------------------------------------------------

  int   nvalue1, ivzero, nfirst, nzmin, nzmax, ivz, n, nn, i;
  int   icolx, ltypx, iwidx, n1, n2, nv;
  float zmin, zmax, zv, zlim, dz1, dz2;

  int *ivsort, *ihelp;
  float *fhelp;

  zmin = zvmin;
  zmax = zvmax;
  nvalue1 = nvalue;
  ivzero = -1;
  nfirst = -1;

  nvalue--;

  if (idraw==1 || idraw==2)
  {
    if (zrange[0]<=zrange[1]) {
      if (zmin<zrange[0]) zmin = zrange[0];
      if (zmax>zrange[1]) zmax = zrange[1];
    }
    // adjust first and last value
    nzmin = int((zmin-zoff)/zstep);
    nzmax = int((zmax-zoff)/zstep);
    if (zoff+nzmin*zstep < zmin-zstep*0.001) nzmin++;
    if (zoff+nzmax*zstep > zmax+zstep*0.001) nzmax--;
    if (nvalue1==0) nzmin--;
    int mval= mvalue-nvalue1;
    if (nzmax-nzmin+1 > mval) {
      if (fabsf(zmin) > fabsf(zmax)) nzmin = nzmax-mval+1;
      else                           nzmax = nzmin+mval-1;
    }
    ivz = nzmax+1;
    if (idraw==2) ivz = 0;
    for (n=nzmin; n<=nzmax; ++n) {
      if (n != ivz) {
        zvalue[++nvalue] = zoff+zstep*float(n);
        ivalue[nvalue]= n;
        if (n>ivz) ivalue[nvalue]--;
      }
      if (n==0) ivzero = nvalue;
    }
    if (nzmax<0) ivzero = nvalue+1;
    else if (idraw==2) ivzero++;
  }
  else if (idraw==3)
  {
    //changed by LB 2007-03-05
    //PROBLEM: both zmin and zmax between rline[n] and rline[n+1]
    //-> no colour shading. This seems to solve the problem.
    for (n=0; n<nlines; ++n) {
      //         if (rlines[n]>=zmin &&
      //       	    rlines[n]<=zmax && nvalue<mvalue-1) {
      if (rlines[n]<=zmax &&
          nvalue<mvalue-1) {
        zvalue[++nvalue] = rlines[n];
        ivalue[nvalue]= n;
        if (nfirst<0) nfirst = n;
      }
    }
  }
  else if (idraw==4)
  {
    nn = 0;
    zv = rlines[0];
    while (zv<=zmax && nvalue<mvalue-1) {
      for (n=0; n<nlines; ++n) {
        zv = rlines[n]*powf(10.,float(nn));
        if (zv>=zmin && zv<=zmax && nvalue<mvalue-1) {
          zvalue[++nvalue] = zv;
          ivalue[nvalue]= nn*nlines + n;
          if (nfirst<0) nfirst = n;
        }
      }
      zv = rlines[0]*powf(10.,float(++nn));
    }
  }
  else return -1;

  nvalue++;

  if (nvalue==nvalue1) return nvalue;

  if (ncol<1 || ntyp<1 || nwid<1 || (ncol==1 && ntyp==1 && nwid==1))
  {
    icolx = -1;
    ltypx = -1;
    iwidx = -1;
    if (nvalue1>0) {
      icolx =   icolour[nvalue1-1];
      ltypx =  linetype[nvalue1-1];
      iwidx = linewidth[nvalue1-1];
    }
    if (ncol>0) icolx = icol[0];
    if (ntyp>0) ltypx = ityp[0];
    if (nwid>0) iwidx = iwid[0];
    for (n=nvalue1; n<nvalue; ++n) {
      icolour[n] = icolx;
      linetype[n] = ltypx;
      linewidth[n] = iwidx;
    }
  }
  else if (nlim>1)
  {
    zv = zvalue[nvalue1];
    n = nvalue1;
    for (i=0; i<nlim; ++i) {
      icolx = (i<ncol) ? icol[i] : icol[ncol-1];
      ltypx = (i<ntyp) ? ityp[i] : ityp[ntyp-1];
      iwidx = (i<nwid) ? iwid[i] : iwid[nwid-1];
      if (i<nlim-1) zlim = rlim[i+1];
      else          zlim = fieldUndef;
      while (zv<zlim && n<nvalue) {
        icolour[n] = icolx;
        linetype[n] = ltypx;
        linewidth[n] = iwidx;
        n++;
        if (n<nvalue) zv = zvalue[n];
      }
    }
  }
  else if (idraw==1 || idraw==2)
  {
    for (i=0; i<3; ++i) {
      icolx = (i<ncol) ? icol[i] : icol[ncol-1];
      ltypx = (i<ntyp) ? ityp[i] : ityp[ntyp-1];
      iwidx = (i<nwid) ? iwid[i] : iwid[nwid-1];
      if (i==0 && ivzero>=0) {
        n1 = nvalue1;
        n2 = ivzero;
      }
      else if (i==1 && idraw==1 && ivzero>=0 && ivzero<nvalue) {
        n1 = ivzero;
        n2 = ivzero+1;
      }
      else if (i==2) {
        n1 = ivzero+1;
        n2 = nvalue;
        if (idraw==2) n1--;
      }
      else {
        n1 = -1;
        n2 = -1;
      }
      for (n=n1; n<n2; ++n) {
        icolour[n] = icolx;
        linetype[n] = ltypx;
        linewidth[n] = iwidx;
      }
    }
  }
  else if (idraw==3 || idraw==4)
  {
    nn=nfirst-1;
    for (n=nvalue1; n<nvalue; ++n) {
      ++nn;
      // nn relooping for idraw==4
      if (nn>=nlines) nn = 0;
      icolour[n] = (nn<ncol) ? icol[nn] : icol[ncol-1];
      linetype[n] = (nn<ntyp) ? ityp[nn] : ityp[ntyp-1];
      linewidth[n] = (nn<nwid) ? iwid[nn] : iwid[nwid-1];
    }
  }
  else return -1;

  if (nvalue1<1) return nvalue;

  // merge secondary options with previous (primary) options,
  // prefer secondary options

  ivsort = new int[nvalue];

  n1 = 0;
  n2 = nvalue1;
  nv = -1;
  while (n1<nvalue1 && n2<nvalue)
  {
    if (n1>0 && n1<nvalue1-1) dz1 = zvalue[n1+1]-zvalue[n1-1];
    else if (n1>0)            dz1 = zvalue[n1]  -zvalue[n1-1];
    else if (n1<nvalue1-1)    dz1 = zvalue[n1+1]-zvalue[n1];
    else                      dz1 = 0.;
    if (n2>nvalue1 && n2<nvalue-1) dz2 = zvalue[n2+1]-zvalue[n2-1];
    else if (n2>nvalue1)           dz2 = zvalue[n2]  -zvalue[n2-1];
    else if (n2<nvalue-1)          dz2 = zvalue[n2+1]-zvalue[n2];
    else                           dz2 = 0.;
    nv++;
    if (fabsf(zvalue[n1]-zvalue[n2]) <= (dz1+dz2)*0.01) {
      ivsort[nv] = n2;
      n1++;
      n2++;
    }
    else if (zvalue[n1] < zvalue[n2]) {
      ivsort[nv] = n1;
      n1++;
    }
    else {
      ivsort[nv] = n2;
      n2++;
    }
  }

  if (n1<nvalue1-1) {
    for (n=n1; n<nvalue1; ++n) ivsort[++nv] = n;
  }
  else if (n2<nvalue-1) {
    for (n=n2; n<nvalue; ++n) ivsort[++nv] = n;
  }
  nv++;

  // sort output arrays

  ihelp  = new   int[nv];
  fhelp  = new float[nv];

  for (n=0; n<nv; ++n) fhelp[n] = zvalue[ivsort[n]];
  for (n=0; n<nv; ++n) zvalue[n] = fhelp[n];

  for (n=0; n<nv; ++n) ihelp[n] = ivalue[ivsort[n]];
  for (n=0; n<nv; ++n) ivalue[n] = ihelp[n];

  for (n=0; n<nv; ++n) ihelp[n] = icolour[ivsort[n]];
  for (n=0; n<nv; ++n) icolour[n] = ihelp[n];

  for (n=0; n<nv; ++n) ihelp[n] = linetype[ivsort[n]];
  for (n=0; n<nv; ++n) linetype[n] = ihelp[n];

  for (n=0; n<nv; ++n) ihelp[n] = linewidth[ivsort[n]];
  for (n=0; n<nv; ++n) linewidth[n] = ihelp[n];

  delete []ivsort;
  delete []ihelp;
  delete []fhelp;

  return nv;
}


void posConvert(int npos, float *x, float *y, float *cxy)
{
  float xh,yh;

  for (int n=0; n<npos; n++) {
    xh= x[n];
    yh= y[n];
    x[n]= cxy[0]+cxy[1]*xh+cxy[2]*yh;
    y[n]= cxy[3]+cxy[4]*xh+cxy[5]*yh;
  }
}


void posConvert(int npos, float *x, float *y, int nx, int ny, const float *xz, const float *yz)
{
  int i,j,ij;
  float dx,dy,a,b,c,d;

  for (int n=0; n<npos; n++) {
    i= int(x[n]);  if (i>nx-2) i=nx-2;
    j= int(y[n]);  if (j>ny-2) j=ny-2;
    dx= x[n] - float(i);
    dy= y[n] - float(j);
    a= (1.-dy)*(1.-dx);
    b= (1.-dy)*dx;
    c= dy*(1.-dx);
    d= dy*dx;
    ij= j*nx + i;
    if(xz[ij]==HUGE_VAL ||xz[ij+1]==HUGE_VAL ||xz[ij+nx]==HUGE_VAL || xz[ij+nx+1]==HUGE_VAL ||
        yz[ij]==HUGE_VAL ||yz[ij+1]==HUGE_VAL ||yz[ij+nx]==HUGE_VAL || yz[ij+nx+1]==HUGE_VAL){
      x[n]=HUGE_VAL;
      y[n]=HUGE_VAL;
    } else {
      x[n] = a*xz[ij] + b*xz[ij+1] + c*xz[ij+nx] + d*xz[ij+nx+1];
      y[n] = a*yz[ij] + b*yz[ij+1] + c*yz[ij+nx] + d*yz[ij+nx+1];
    }
  }
}

void joinContours(std::vector<ContourLine*>& contourlines, int idraw, bool drawBorders, int iconv)
{
  int ncl= contourlines.size();

  ContourLine *cl;
  int n,nstop;
  int npos,i,ntot,nend;
  int nc,lr1=0,lr2=0,adduse[2];

  for (nc=0; nc<ncl; nc++) {
    cl= contourlines[nc];
    cl->undefLeft=  false;
    cl->undefRight= false;
    if (cl->closed || cl->corner)
      cl->use= 3;
    else if (cl->undefLine)
      cl->use= (cl->highLeft) ? 1 : 2;
    else
      cl->use= 0;
    if (cl->npos==1 &&
        cl->connect[0][0]<0 &&
        cl->connect[0][1]<0 &&
        cl->connect[1][0]<0 &&
        cl->connect[1][1]<0) cl->use= 9;
  }

  for (nc=0; nc<ncl; nc++) {
    if (contourlines[nc]->use<3) {
      if (contourlines[nc]->use==0) {
        lr1= 0;
        lr2= 1;
      } else if (contourlines[nc]->use==1) {
        lr1= 1;
        lr2= 1;
      } else if (contourlines[nc]->use==2) {
        lr1= 0;
        lr2= 0;
      }

      // lr: 0=left 1=right

      for (int lr=lr1; lr<=lr2; lr++) {

        if (lr==0) {
          adduse[0]= 1;
          adduse[1]= 2;
        } else {
          adduse[0]= 2;
          adduse[1]= 1;
        }
        std::vector<int> joined;
        joined.push_back(nc);

        nstop= nc;
        npos=  contourlines[nc]->npos;

        n= contourlines[nc]->connect[1][lr];

        bool undefCheck= true;
        bool isoLine=    contourlines[nc]->isoLine;
        bool undefTouch= contourlines[nc]->undefTouch;
        bool undefLeft=  false;
        bool undefRight= false;
        bool corner=     contourlines[nc]->corner;
        int izleft=      contourlines[nc]->izleft;
        int izright=     contourlines[nc]->izright;

        int looptest=0;

        while (n!=nstop && looptest<ncl && n>=0) {
          looptest++;
          nend= n/maxLines;
          n=    n%maxLines;
          joined.push_back(n);

          if (undefCheck && contourlines[n]->undefLine) {
            undefCheck= false;
            if (nend==0) {
              undefLeft=   contourlines[n]->highLeft;
              undefRight= !contourlines[n]->highLeft;
            } else {
              undefLeft=  !contourlines[n]->highLeft;
              undefRight=  contourlines[n]->highLeft;
            }
          }

          if (contourlines[n]->isoLine) isoLine= true;
          if (contourlines[n]->undefTouch) undefTouch= true;
          if (contourlines[n]->corner) corner= true;

          if (!contourlines[n]->undefLine) {
            if (nend==0) {
              if (contourlines[n]->izleft>=0)  izleft=  contourlines[n]->izleft;
              if (contourlines[n]->izright>=0) izright= contourlines[n]->izright;
            } else {
              if (contourlines[n]->izright>=0) izleft=  contourlines[n]->izright;
              if (contourlines[n]->izleft>=0)  izright= contourlines[n]->izleft;
            }
          }

          npos+=contourlines[n]->npos;
          contourlines[n]->use+=adduse[nend];
          nend=(nend+1)%2;
          n= contourlines[n]->connect[nend][lr];
        }

        bool joinerror= false;

        if (looptest==ncl || n<0) {

          METLIBS_LOG_WARN("CONTOUR joinContours ERROR  nc,lr,nstop,n: "
          <<nc<<" "<<lr<<" "<<nstop<<" "<<n);

          joinerror= true;
        }

        npos++;
        float *xpos= new float[npos];
        float *ypos= new float[npos];

        ntot= 0;

        cl= contourlines[nc];
        npos= cl->npos;
        float *clx= cl->xpos;
        float *cly= cl->ypos;

        for (i=0; i<npos; i++) {
          xpos[i]= clx[i];
          ypos[i]= cly[i];
        }
        ntot= npos;

        n= cl->connect[1][lr];

        looptest=0;

        while (n!=nstop && looptest<ncl && n>=0) {
          looptest++;
          nend= n/maxLines;
          n=    n%maxLines;

	  	  if (n>=int(contourlines.size()))
            METLIBS_LOG_WARN("CONTOUR ERROR. n,contourlines.size(): "
            <<n<<" "<<contourlines.size());

          cl= contourlines[n];
          npos= cl->npos;
          clx= cl->xpos;
          cly= cl->ypos;

          if (npos<1)
            METLIBS_LOG_WARN("CONTOUR ERROR. npos: "<<npos);

          if (nend==0) {
            i=0;
            if (drawBorders) {
              float dx= clx[i] - xpos[ntot-1];
              float dy= cly[i] - ypos[ntot-1];
              if (dx*dx+dy*dy<0.01f) i=1;
            }
            for (; i<npos; i++) {
              xpos[ntot]=   clx[i];
              ypos[ntot++]= cly[i];
            }
          } else {
            i=npos-1;
            if (drawBorders) {
              float dx= clx[i] - xpos[ntot-1];
              float dy= cly[i] - ypos[ntot-1];
              if (dx*dx+dy*dy<0.01f) i--;
            }
            for (; i>=0; i--) {
              xpos[ntot]=   clx[i];
              ypos[ntot++]= cly[i];
            }
          }

          nend=(nend+1)%2;
          n= cl->connect[nend][lr];
        }

        float dx= xpos[ntot-1]-xpos[0];
        float dy= ypos[ntot-1]-ypos[0];
        if (dx*dx+dy*dy>0.0) {
          xpos[ntot]=   xpos[0]; // close line
          ypos[ntot++]= ypos[0];
        }

        ContourLine *cl= new ContourLine();
        cl->original=   false;
        cl->ivalue=     contourlines[nc]->ivalue;
		//cl->value=      -999.; // dummy (one or two values)
		cl->value=      contourlines[nc]->value; // value is used by Shape option
        cl->highInside= false;  // but not determined yet
        cl->highLeft=   contourlines[nc]->highLeft;
        cl->closed=     true;
        cl->izleft=     izleft;
        cl->izright=    izright;
        cl->direction=  0;
        cl->npos=       ntot;
        cl->xpos=       xpos;
        cl->ypos=       ypos;
        cl->corner=     corner;
        cl->undefLine=  false;
        cl->outer=      false;
        cl->inner=      false;
        cl->isoLine=    isoLine;
        cl->undefTouch= undefTouch;
        cl->undefLeft=  undefLeft;
        cl->undefRight= undefRight;

        if (undefTouch) {
          cl->undefLeft=  (lr==1);
          cl->undefRight= (lr==0);
        }

        cl->connect[0][0]= -1;
        cl->connect[0][1]= -1;
        cl->connect[1][0]= -1;
        cl->connect[1][1]= -1;
        cl->use= 3;
        cl->joined= joined;

        if (joinerror) {
          cl->npos*=-1;
          cl->closed= false;
        }

        contourlines.push_back(cl);

      }

      contourlines[nc]->use= 3;
    }
  }

  // check if corners used

  if (!contourlines[0]->undefLine && contourlines[0]->use==3) {
    std::vector<int> joined;
    npos=  1;
    nstop= 0;
    n= contourlines[0]->connect[1][0];
    int looptest= 0;
    joined.push_back(0);

    while (n!=nstop && looptest<ncl && n>=0) {
      looptest++;
      joined.push_back(n);
      nend= n/maxLines;
      n=    n%maxLines;

      if (!contourlines[n]->corner)
        METLIBS_LOG_WARN("CONTOUR joinContours ERROR.  NOT corner/side");

      if (nend!=0)
        METLIBS_LOG_WARN("CONTOUR joinContours ERROR.  nend!=0");

      npos+=contourlines[n]->npos;
      nend=(nend+1)%2;
      n= contourlines[n]->connect[nend][0];
    }
    npos++;

    if (looptest==ncl || n<0)
      METLIBS_LOG_WARN("CONTOUR joinContours ERROR at corners/sides");

    float *xpos= new float[npos];
    float *ypos= new float[npos];

    xpos[0]= contourlines[0]->xpos[0];
    ypos[0]= contourlines[0]->ypos[0];
    ntot= 1;
    n= contourlines[0]->connect[1][0];
    looptest= 0;

    while (n!=nstop && looptest<ncl && n>=0) {
      looptest++;
      nend= n/maxLines;
      n=    n%maxLines;
      npos= contourlines[n]->npos;
      if (nend==0) {
        for (i=0; i<npos; i++) {
          xpos[ntot]=   contourlines[n]->xpos[i];
          ypos[ntot++]= contourlines[n]->ypos[i];
        }
      } else {
        for (i=npos-1; i>=0; i--) {
          xpos[ntot]=   contourlines[n]->xpos[i];
          ypos[ntot++]= contourlines[n]->ypos[i];
        }
      }
      nend=(nend+1)%2;
      n= contourlines[n]->connect[nend][0];
    }

    float dx= xpos[ntot-1]-xpos[0];
    float dy= ypos[ntot-1]-ypos[0];
    if (dx*dx+dy*dy>0.0) {
      xpos[ntot]=   xpos[0]; // close line
      ypos[ntot++]= ypos[0];
    }

    ContourLine *cl= new ContourLine();
    cl->original=   false;
    cl->ivalue=     contourlines[0]->ivalue;
    //cl->value=      -999.; // dummy (one or two values)
    cl->value=      contourlines[0]->value; // value is used by Shape option
    cl->highInside= false;  // but not determined yet
    cl->highLeft=   contourlines[0]->highLeft;
    cl->closed=     true;
    cl->izleft=     contourlines[0]->izleft;
    cl->izright=    contourlines[0]->izright;
    cl->direction=  0;
    cl->npos=       ntot;
    cl->xpos=       xpos;
    cl->ypos=       ypos;
    cl->corner=     true;
    cl->undefLine=  false;
    cl->isoLine=    false;
    cl->undefTouch= false;
    cl->undefLeft=  false;
    cl->undefRight= false;
    cl->outer=      false;
    cl->inner=      false;
    cl->connect[0][0]= -1;
    cl->connect[0][1]= -1;
    cl->connect[1][0]= -1;
    cl->connect[1][1]= -1;
    cl->use= 3;

    contourlines.push_back(cl);
  }

  ncl= contourlines.size();

  int j,jc;

  float xmin,xmax,ymin,ymax,xlr,ylr;
  int   m1,m2,m3;
  double ax,bx,cx,ay,by,cy,area2;

  for (j=0; j<ncl; j++) {
    cl= contourlines[j];
    cl->direction= 0;

    if (cl->closed && cl->npos>3) {
      npos= cl->npos;
      xmin= xmax= xlr= cl->xpos[0];
      ymin= ymax= ylr= cl->ypos[0];
      m1= 0;
      for (i=1; i<npos; i++) {
        if (xmin > cl->xpos[i]) xmin= cl->xpos[i];
        if (xmax < cl->xpos[i]) xmax= cl->xpos[i];
        if (ymin > cl->ypos[i]) ymin= cl->ypos[i];
        if (ymax < cl->ypos[i]) ymax= cl->ypos[i];
        if (ylr > cl->ypos[i] ||
            (ylr == cl->ypos[i] && xlr > cl->xpos[i])) {
          xlr= cl->xpos[i];
          ylr= cl->ypos[i];
          m1= i;
        }
      }
      cl->xmin= xmin;
      cl->xmax= xmax;
      cl->ymin= ymin;
      cl->ymax= ymax;

      npos--;

      // clockwise or counterclockwise direction
      m2= (m1-1+npos) % npos;
      m3= (m1+1) % npos;
      ax= cl->xpos[m2];
      bx= cl->xpos[m1];
      cx= cl->xpos[m3];
      ay= cl->ypos[m2];
      by= cl->ypos[m1];
      cy= cl->ypos[m3];
      area2 = ax * by - ay * bx +
      ay * cx - ax * cy +
      bx * cy - cx * by;
      if (area2 > 0.0)
        cl->direction=  1;  // counterclockwise
      else if (area2 < 0.0)
        cl->direction= -1;  // clockwise
      else {
        METLIBS_LOG_WARN("CONTOUR DIRECTION ERROR  nc,npos: "<<j<<" "<<npos);
      }
      if (cl->direction!=0) {
        if (( cl->highLeft && cl->direction==1) ||
            (!cl->highLeft && cl->direction==-1))
          cl->highInside= true;
        if (cl->undefLeft  && cl->direction==1)  cl->undefLine= true;
        if (cl->undefRight && cl->direction==-1) cl->undefLine= true;
        if (drawBorders) {
          if (cl->direction==1) cl->ivalue= cl->izleft;
          else                  cl->ivalue= cl->izright;
          if (!cl->undefLine) cl->highInside= true;
        }
      }
    }

    if (cl->direction==0) cl->closed= false;
  }

  for (jc=0; jc<ncl; jc++) {
    if (contourlines[jc]->closed) {
      cl= contourlines[jc];
      cl->outer= true;
      cl->inner= true;
      if (cl->original && cl->undefLine) {
        cl->undefLeft=   cl->highLeft;
        cl->undefRight= !cl->highLeft;
      }
      if (cl->undefLeft  && cl->direction==1)  cl->outer= false;
      if (cl->undefRight && cl->direction==-1) cl->outer= false;
      if (cl->undefLeft  && cl->direction==-1) cl->inner= false;
      if (cl->undefRight && cl->direction==1)  cl->inner= false;

      if (cl->corner) cl->inner= false;

      int ivalue= cl->ivalue;
      if (!cl->highInside) ivalue--;
      //changed by LB 2007-03-05
      //PROBLEM: both zmin and zmax between rline[n] and rline[n+1]
      //-> no colour shading. This seems to solve the problem.
      if (ivalue<0 && (idraw==3 || idraw==4))
        cl->outer= false;

      if (idraw==2) {
        if (cl->ivalue==0  && !cl->highInside) cl->outer= false;
        if (cl->ivalue==-1 &&  cl->highInside) cl->outer= false;
      }

      if (!cl->inner && !cl->outer) cl->closed= false;
    }
  }

}

/**
 * Returns a vector of x positions indicating where the y values in
 * the y array cross the value of ycross.
 */
std::vector<float> findCrossing(float ycross, int n, float* x, float* y)
{
  std::vector<float> xcross;
  std::multiset<float> xset;

  float px,py,xc;
  px= x[n-1];
  py= y[n-1];
  for (int i=0; i<n; i++) {
    if ((py <  ycross && y[i] >= ycross) ||
        (py >= ycross && y[i] <  ycross)) {
      xc= px + (x[i] - px) * (ycross - py) / (y[i] - py);
      xset.insert(xc);
    }
    px= x[i];
    py= y[i];
  }
  std::multiset<float>::iterator p = xset.begin(), pend = xset.end();
  while (p!=pend) {
    xcross.push_back(*p);
    p++;
  }
  return xcross;
}

void fillContours(DiGLPainter* gl, std::vector<ContourLine*>& contourlines, int nx, int ny, float z[], int iconv, float* cxy, const float* xz, const float* yz,
                  int idraw, const PlotOptions& poptions, bool drawBorders, const Area& fieldArea, float zrange[], float zstep, float zoff,
                  const float& fieldUndef)
{
  std::vector<Colour> colours;
  int ncolours= poptions.palettecolours.size();
  colours= poptions.palettecolours;

  std::vector<Colour> colours_cold;
  int ncolours_cold= poptions.palettecolours_cold.size();
  colours_cold = poptions.palettecolours_cold;

  int npatterns= poptions.patterns.size();

  // set alpha shading
  if(poptions.alpha<255){
    for(int i=0;i<ncolours;i++)
      colours[i].set(Colour::alpha,(unsigned char)poptions.alpha);
    for(int i=0;i<ncolours_cold;i++)
      colours_cold[i].set(Colour::alpha,(unsigned char)poptions.alpha);
  }

  int ncl= contourlines.size();
  if (ncl<1) return;

  ContourLine *cl;
  ContourLine *cl2;
  int i,jc,n,ncontours;

  std::vector<std::vector<int>> clindex(ncl);
  // NEW function for countourline indexes to use, separated from the other code,
  // to be used in other methods, such as writing shape files
  getCLindex(contourlines, clindex, poptions, drawBorders, fieldUndef);

  const std::vector<float> classValues = diutil::parseClassValues(poptions);
  const int nclass= classValues.size();

  if (iconv==1) {
    for (jc=0; jc<ncl; jc++) {
      cl= contourlines[jc];
      if (cl->closed) posConvert(cl->npos,cl->xpos,cl->ypos,cxy);
    }
  } else if (iconv==2) {
    for (jc=0; jc<ncl; jc++) {
      cl= contourlines[jc];
      if (cl->closed) posConvert(cl->npos,cl->xpos,cl->ypos,nx,ny,xz,yz);
    }
  }

  gl->ShadeModel(DiGLPainter::gl_FLAT);
  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK,DiGLPainter::gl_FILL);
  gl->Enable(DiGLPainter::gl_BLEND);
  gl->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);

  for (jc=0; jc<ncl; jc++) {

    cl= contourlines[jc];
    if (cl->outer && cl->closed && cl->ivalue!=-300 &&
        (!cl->undefLine || (!cl->highInside && cl->joined.size()<2))) {

      ncontours= clindex[jc].size();
      int nposis=0;
      for (i=0; i<ncontours; i++) {
        cl2= contourlines[clindex[jc][i]];
        nposis+=(cl2->npos - 1);
      }

      if (nposis>2) {

        // colour index
        int ivalue= cl->ivalue;
        if (cl->isoLine && !cl->highInside)
          ivalue--;
        if (ivalue>=0 && idraw==2) ivalue++;
        if(idraw >2 || zrange[0]>zrange[1] ||
            ( ivalue<(zrange[1]-zoff)/zstep
                && ivalue>((zrange[0]-zoff)/zstep)-1)) {

          if (poptions.discontinuous) {

            //discontinuous (classes)
            int ii;
            if (nclass>0) {
              ii=0;
              while (ii<nclass && classValues[ii]!=ivalue) ii++;
              if (ii==nclass) i=-1;
            } else {
              ii=ivalue%ncolours;
              if (ii<0) ii+=ncolours;
            }
            if (ii>=0) {
              if (ii<ncolours)
                gl->setColour(colours[ii]);
              if (ii<npatterns) {
                ImageGallery ig;
                gl->Enable(DiGLPainter::gl_POLYGON_STIPPLE);
                GLubyte* p = ig.getPattern(poptions.patterns[ii]);
                if(p==0)
                  gl->PolygonStipple(solid);
                else
                  gl->PolygonStipple(p);
              }
            } else {
              continue;
            }

          } else {

            //continuous
            if(ncolours_cold>0 && ivalue<0) {
              if (!poptions.repeat && ivalue < -1 * (ncolours_cold)) {
                i=ncolours_cold-1;
              } else {
                i= (ncolours_cold-ivalue-1)%ncolours_cold;
                if (i<0) i+=ncolours_cold;
              }
              gl->setColour(colours_cold[i]);
            } else if(ncolours>0) {
              if (!poptions.repeat) {
                if(ivalue>ncolours-1){
                  i=ncolours-1;
                } else if(ivalue<0){
                  i=0;
                } else {
                  i= ivalue%ncolours;
                  if (i<0) i+=ncolours;
                }
              } else {
                i= ivalue%ncolours;
                if (i<0) i+=ncolours;
              }
              gl->setColour(colours[i]);
            } else { //no colours
              gl->setColour(poptions.fillcolour);
            }

            if(npatterns>0){
              ImageGallery ig;
              gl->Enable(DiGLPainter::gl_POLYGON_STIPPLE);
              i= ivalue%npatterns;
              if (i<0) i+=npatterns;
              GLubyte* p = ig.getPattern(poptions.patterns[i]);
              if(p==0)
                gl->PolygonStipple(solid);
              else
                gl->PolygonStipple(p);
            }
          }

          QList<QPolygonF> contours;
          for (n=0; n<ncontours; n++) {
            cl= contourlines[clindex[jc][n]];
            const int npos = cl->npos-1;
            int i;
            QPolygonF polygon;
            for (i=0; i<npos; i++) {
              if (cl->xpos[i]==HUGE_VAL || cl->ypos[i]==HUGE_VAL) {
                continue;
              }
              polygon << QPointF(cl->xpos[i], cl->ypos[i]);
            }
            if (polygon.size() >= 3)
              contours << polygon;
          }
          gl->drawPolygons(contours);
          gl->Disable(DiGLPainter::gl_POLYGON_STIPPLE);
        }
      }
    }
  }

  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK,DiGLPainter::gl_LINE);
  gl->Disable(DiGLPainter::gl_BLEND);
  gl->ShadeModel(DiGLPainter::gl_FLAT);
  gl->EdgeFlag(DiGLPainter::gl_TRUE);
}

void getCLindex(std::vector<ContourLine*>& contourlines, std::vector<std::vector<int>>& clind, const PlotOptions& poptions, bool drawBorders,
                const float& fieldUndef)
{

  int ncl = contourlines.size();

  ContourLine *cl;
  ContourLine *cl2;
  int i,j,k,ic,jc,nc,n;//,ncontours;

  float ycross,xtest;
  std::vector<float> xcross;
  std::vector<float> xcross2;

  std::vector<std::vector<int>> insiders(ncl);

  // START FINDING clindex

  for (jc=0; jc<ncl; jc++) {
    if (contourlines[jc]->closed) {
      cl= contourlines[jc];

      ycross= fieldUndef;

      int lowIndex=  cl->ivalue;
      int highIndex= cl->ivalue;
      if (cl->highInside)
        highIndex++;
      else
        lowIndex--;

      if (drawBorders) {
        lowIndex=  -999999;
        highIndex= +999999;
      }

      for (ic=0; ic<ncl; ic++) {
        if (ic!=jc && contourlines[ic]->inner) {
          cl2= contourlines[ic];
          if ((cl2->ivalue >= lowIndex && cl2->ivalue <= highIndex) ||
               cl2->undefLine || cl->undefLine) {
            if (cl2->xmin >= cl->xmin && cl2->xmax <= cl->xmax &&
                cl2->ymin >= cl->ymin && cl2->ymax <= cl->ymax) {
              if (ycross <= cl2->ymin || ycross >= cl2->ymax) {
                ycross= cl2->ymin + (cl2->ymax - cl2->ymin) * 0.55;
                xcross= findCrossing(ycross, cl->npos, cl->xpos, cl->ypos);
              }
              nc= xcross.size();
              if (nc>1) {
                if (cl2->xmax > xcross[0] && cl2->xmin < xcross[nc-1]) {
                  xcross2= findCrossing(ycross, cl2->npos, cl2->xpos, cl2->ypos);
                  int nc2= xcross2.size();
                  if (nc2>1) {
                    bool search=true, inside=false, alleq=true;
                    int c1=0, c2=0;
                    while (search && c2<nc2) {
                      if (c1<nc) {
                        if (xcross[c1]<xcross2[c2]) {
                          c1++;
                        } else if (xcross[c1]>xcross2[c2]) {
                          if (c1%2==0) {
                            inside= false;
                            search= false;
                          } else {
                            inside= true;
                            search= false;
                          }
                          c2++;
                          alleq=false;
                        } else {
                          if (c1%2 != c2%2) {
                            inside= false;
                            search= false;
                          }
                          c1++;
                          c2++;
                        }
                      } else {
                        c2++;
                        if (c1%2 == c2%2) {
                          inside= false;
                          search= false;
                        }
                      }
                    }
                    if (search && alleq) {
                      if (cl->ymin<cl2->ymin || cl->ymax>cl2->ymax ||
                          cl->xmin<cl2->xmin || cl->xmax>cl2->xmax) {
                        inside= true;
                      } else {
                        inside= true;
                        int j1,nj1= cl->joined.size();
                        int j2,nj2= cl2->joined.size();

                        j2= 0;
                        while (search && j2<nj2) {
                          bool ok= true;
                          for (int j1=0; j1<nj1; j1++)
                            if (cl->joined[j1]==cl2->joined[j2]) ok= false;
                          if (ok) {
                            int kc= cl2->joined[j2];
                            int nk= contourlines[kc]->npos;
                            float xcp= contourlines[kc]->xpos[nk/2];
                            float ycp= contourlines[kc]->ypos[nk/2];
                            std::vector<float> xcross1;
                            xcross1=  findCrossing(ycp, cl->npos, cl->xpos, cl->ypos);
                            int nc1= xcross1.size();
                            if (nc1>1) {
                              i=0;
                              while (i<nc1 && xcross1[i]<xcp) i++;
                              if (i==nc1 || (i<nc1 && xcross1[i]!=xcp)) {
                                inside=(i%2==1);
                                search= false;
                              }
                            }
                          }
                          j2++;
                        }
                        j1= 0;
                        while (search && j1<nj1) {
                          bool ok= true;
                          for (j2=0; j2<nj2; j2++)
                            if (cl->joined[j1]==cl2->joined[j2]) ok= false;
                          if (ok) {
                            int kc= cl->joined[j1];
                            int nk= contourlines[kc]->npos;
                            float xcp= contourlines[kc]->xpos[nk/2];
                            float ycp= contourlines[kc]->ypos[nk/2];
                            std::vector<float> xcross1;
                            xcross1=  findCrossing(ycp, cl2->npos, cl2->xpos, cl2->ypos);
                            int nc1= xcross1.size();
                            if (nc1>1) {
                              i=0;
                              while (i<nc1 && xcross1[i]<xcp) i++;
                              if (i==nc1 || (i<nc1 && xcross1[i]!=xcp)) {
                                inside=(i%2==0);
                                search= false;
                              }
                            }
                          }
                          j1++;
                        }
                      }
                    }
                    if (inside) insiders[jc].push_back(ic);

                  } else {

                    if (!drawBorders && xcross2.size()>1)
                      xtest= (xcross2[0]+xcross2[1])*0.5;
                    else if (xcross2.size()>0)
                      xtest= xcross2[0];
                    else
                      xtest= fieldUndef;
                    n=0;
                    while (n<nc && xcross[n]<=xtest) n++;
                    if (n>0 && fabsf(xcross[n-1]-xtest)<0.0001) {
                      if (!cl->undefLine && cl2->undefLine) n= 0;
                    }
                    if (n%2 == 1) {
                      insiders[jc].push_back(ic);
                    }
                  }
                }
              }
			}
		  }
        }
	  }
	}
  }
  for (jc=0; jc<ncl; jc++) {

	if (contourlines[jc]->outer) {
	  clind[jc].push_back(jc);
	  if (insiders[jc].size()>0) {
	cl= contourlines[jc];
	int ninside= insiders[jc].size();
        std::vector<int> inside;
        for (j=0; j<ninside; j++) {
	  ic= insiders[jc][j];
	  if (contourlines[ic]->inner) {
		bool ok= true;
		for (i=0; i<ninside; i++) {
		  int kc= insiders[jc][i];
		  int nk= insiders[kc].size();
		  for (k=0; k<nk; k++)
		if (ic==insiders[kc][k]) ok= false;
		}
		if (ok) clind[jc].push_back(ic);
	  }
	}
	int nj1= cl->joined.size();
	ninside= clind[jc].size();
	bool ok= true;
	j=1;
	while (ok && j<ninside) {
	  ic= clind[jc][j];
	  cl2= contourlines[ic];
	  int nj2= cl2->joined.size();
	  for (int j1=0; j1<nj1; j1++)
		for (int j2=0; j2<nj2; j2++)
		  if (cl->joined[j1]==cl2->joined[j2]) ok= false;
	  j++;
	}
	if (!ok) {
	  cl->outer= false;
	  clind[jc].clear();
	}
	  }
	}
  }
  // END FINDING clindex

}

void replaceUndefinedValues(int nx, int ny, float *f, bool fillAll,
    const float& fieldUndef)
{

  const int nloop= 20;

  float *ftmp= new float[nx*ny];

  float fmin=  fieldUndef;
  float fmax= -fieldUndef;

  for (int ij=0; ij<nx*ny; ij++) {
    if (f[ij]!=fieldUndef) {
      if (f[ij]<fmin) fmin= f[ij];
      if (f[ij]>fmax) fmax= f[ij];
    }
  }

  for (int ij=0; ij<nx*ny; ij++) {
    ftmp[ij]= f[ij];
    if (f[ij]==fieldUndef) {
      float sum= 0.;
      int   num= 0;
      int i=ij%nx;
      int j=ij/nx;

      float sum2= 0.;
      int   num2= 0;
      if (i>0 && f[ij-1]!=fieldUndef) {
        sum+= f[ij-1];
        num++;
        if (i>1 && f[ij-2]!=fieldUndef) {
          sum2+= f[ij-1]-f[ij-2];
          num2++;
        }
      }
      if (i<nx-1 && f[ij+1]!=fieldUndef) {
        sum+= f[ij+1];
        num++;
        if (i<nx-2 && f[ij+2]!=fieldUndef) {
          sum2+= f[ij+1]-f[ij+2];
          num2++;
        }
      }
      if (j>0 && f[ij-nx]!=fieldUndef) {
        sum+= f[ij-nx];
        num++;
        if (j>1 && f[ij-2*nx]!=fieldUndef) {
          sum2+= f[ij-nx]-f[ij-2*nx];
          num2++;
        }
      }
      if (j<ny-1 && f[ij+nx]!=fieldUndef) {
        sum+= f[ij+nx];
        num++;
        if (j<ny-2 && f[ij+2*nx]!=fieldUndef) {
          sum2+= f[ij+nx]-f[ij+2*nx];
          num2++;
        }
      }
      if (i>0 && j>0 && f[ij-nx-1]!=fieldUndef) {
        sum+= f[ij-nx-1];
        num++;
        if (i>1 && j>1 && f[ij-2*nx-2]!=fieldUndef) {
          //	  sum2+= 0.5*(f[ij-nx-1]-f[ij-2*nx-2]);
          sum2+= f[ij-nx-1]-f[ij-2*nx-2];
          num2++;
        }
      }
      if (i>0 && j<ny-1 && f[ij+nx-1]!=fieldUndef) {
        sum+= f[ij+nx-1];
        num++;
        if (i>1 && j<ny-2 && f[ij+2*nx-2]!=fieldUndef) {
          //	  sum2+= 0.5*(f[ij+nx-1]-f[ij+2*nx-2]);
          sum2+= f[ij+nx-1]-f[ij+2*nx-2];
          num2++;
        }
      }
      if (i<nx-1 && j>0 && f[ij-nx+1]!=fieldUndef) {
        sum+= f[ij-nx+1];
        num++;
        if (i<nx-2 && j>1 && f[ij-2*nx+2]!=fieldUndef) {
          //	  sum2+= 0.5*(f[ij-nx+1]-f[ij-2*nx+2]);
          sum2+= f[ij-nx+1]-f[ij-2*nx+2];
          num2++;
        }
      }
      if (i<nx-1 && j<ny-1 && f[ij+nx+1]!=fieldUndef) {
        sum+= f[ij+nx+1];
        num++;
        if (i<nx-2 && j<ny-2 && f[ij+2*nx+2]!=fieldUndef) {
          //	  sum2+= 0.5*(f[ij+nx+1]-f[ij+2*nx+2]);
          sum2+= f[ij+nx+1]-f[ij+2*nx+2];
          num2++;
        }
      }
      if (num2>1) sum+=sum2;

      if (num>0) {
        ftmp[ij]= sum/float(num);
        if (ftmp[ij]<fmin) ftmp[ij]= fmin;
        if (ftmp[ij]>fmax) ftmp[ij]= fmax;
      }
    }
  }
  for (int ij=0; ij<nx*ny; ij++) f[ij]= ftmp[ij];
  delete[] ftmp;
  if (!fillAll) return;

  int nundef= 0;
  float avg= 0.0;
  for (int i=0; i<nx*ny; i++) {
    if (f[i]!=fieldUndef)
      avg+=f[i];
    else
      nundef++;
  }
  if (nundef>0 && nundef<nx*ny) {
    avg/=float(nx*ny-nundef);
    int *indx= new int[nundef];
    int n1a,n1b,n2a,n2b,n3a,n3b,n4a,n4b,n5a,n5b;
    int i,j,ij,n;
    n=0;
    n1a=n;
    for (j=1; j<ny-1; j++)
      for (i=1; i<nx-1; i++)
        if (f[j*nx+i]==fieldUndef) indx[n++]= j*nx+i;
    n1b=n;
    n2a=n;
    i=0;
    for (j=1; j<ny-1; j++)
      if (f[j*nx+i]==fieldUndef) indx[n++]= j*nx+i;
    n2b=n;
    n3a=n;
    i=nx-1;
    for (j=1; j<ny-1; j++)
      if (f[j*nx+i]==fieldUndef) indx[n++]= j*nx+i;
    n3b=n;
    n4a=n;
    j=0;
    for (i=0; i<nx; i++)
      if (f[j*nx+i]==fieldUndef) indx[n++]= j*nx+i;
    n4b=n;
    n5a=n;
    j=ny-1;
    for (i=0; i<nx; i++)
      if (f[j*nx+i]==fieldUndef) indx[n++]= j*nx+i;
    n5b=n;

    for (i=0; i<nx*ny; i++)
      if (f[i]==fieldUndef) f[i]= avg;

    float cor= 1.6;
    float error;

    // much faster than in the DNMI ecom3d ocean model,
    // where method was found to fill undefined values with
    // rather sensible values...

    for (int loop=0; loop<nloop; loop++) {
      for (n=n1a; n<n1b; n++) {
        ij=indx[n];
        error= (f[ij-nx]+f[ij-1]+f[ij+1]+f[ij+nx])*0.25-f[ij];
        f[ij]+=(error*cor);
      }
      for (n=n2a; n<n2b; n++) {
        ij=indx[n];
        f[ij]=f[ij+1];
      }
      for (n=n3a; n<n3b; n++) {
        ij=indx[n];
        f[ij]=f[ij-1];
      }
      for (n=n4a; n<n4b; n++) {
        ij=indx[n];
        f[ij]=f[ij+nx];
      }
      for (n=n5a; n<n5b; n++) {
        ij=indx[n];
        f[ij]=f[ij-nx];
      }
    }

    delete[] indx;
  }
}

void drawLine(DiPainter* gl, int start, int stop, float* x, float* y)
{
  //split line if position is undefined
  QPolygonF points;
  for (int i=start; i<stop+1; ++i) {
    if (x[i]!=HUGE_VAL && y[i]!=HUGE_VAL) {
      points << QPointF(x[i], y[i]);
    } else {
      gl->drawPolyline(points);
      points.clear();
      while(x[i]==HUGE_VAL || y[i]==HUGE_VAL ) ++i;
      --i;
    }
  }
  gl->drawPolyline(points);
}

} // namespace
