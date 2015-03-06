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
#ifndef CONTOURING_H
#define CONTOURING_H

#include <diField/diArea.h>
#include <vector>
#include <string>

#if !defined(USE_PAINTGL)
#include <glp/glpfile.h>
#endif

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

/// the field contouring routine
bool contour(int nx, int ny, float z[], float xz[], float yz[],
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
             FontManager* fp, const PlotOptions& poptions, GLPfile* psoutput,
	     const Area& fieldArea, const float& fieldUndef,
	     const std::string& modelName = "", const std::string& paramName = "",
	     const int& fhour = 0);

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
		int nx, int ny, float *xz, float *yz);
/// joins contour parts (also along undefined areas) to closed contours
void joinContours(std::vector<ContourLine*>& contourlines, int idraw,
		  bool drawBorders, int iconv, int valuecut);
/// find points where a horizontal line crosses a closed contour
std::vector<float> findCrossing(float ycross, int n, float *x, float *y);

/// uses OpenGL tesselation (triangulation) to shade between isolines
void fillContours(std::vector<ContourLine*>& contourlines,
		  int nx, int ny, float z[],
		  int iconv, float *cxy, float *xz, float *yz, int idraw,
		  const PlotOptions& poptions, bool drawBorders,
                  GLPfile* psoutput, const Area& fieldArea,
		  float zrange[], float zstep, float zoff,
		  const float& fieldUndef);

/// replaces undefined values with relatively sensible values
void replaceUndefinedValues(int nx, int ny, float *f, bool fillAll,
			    const float& fieldUndef);

/// draw part of contour line
void drawLine(int start, int stop, float* x, float* y);
/// write shapefile
void writeShapefile(std::vector<ContourLine*>& contourlines,
		    int nx, int ny,
		    int iconv, float *cxy,
		    float *xz, float *yz,
		    int idraw,
		    const PlotOptions& poptions,
		    bool drawBorders,
		    const Area& fieldArea,
		    float zrange[],
		    float zstep,
		    float zoff,
		    const float& fieldUndef,
		    const std::string& modelName,
		    const std::string& paramName,
		    const int& fhour);

/// get CL index
void getCLindex(std::vector<ContourLine*>& contourlines, std::vector< std::vector<int> >& clind,
		const PlotOptions& poptions, bool drawBorders, const float& fieldUndef);

#endif
