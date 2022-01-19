/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2021 met.no

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
#ifndef CONTOURING_H
#define CONTOURING_H

#include <string>

class PlotOptions;
class Area;
class DiGLPainter;

/// the field contouring routine
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
             const Area& fieldArea, const float& fieldUndef);

#endif
