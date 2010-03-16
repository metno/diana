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

#include <fstream>
#include <diMapManager.h>
#include <diMapPlot.h>
#include <milib/milib.h>
#include <iostream>
#include <stdio.h>
#include <diFontManager.h>

#include <float.h>
#include <math.h>
#include <values.h>

using namespace std; using namespace miutil;

// static members
map<miString,FilledMap> MapPlot::filledmaps;
set<miString> MapPlot::usedFilledmaps;
map<miString,ShapeObject> MapPlot::shapemaps;
map<miString,Area> MapPlot::shapeareas;

// Default constructor
MapPlot::MapPlot() :
  Plot(), mapchanged(true), haspanned(false), usedrawlists(true)
{
#ifdef DEBUGPRINT
  cerr << "++ MapPlot::Default Constructor" << endl;
#endif
  drawlist[0]=0;
  drawlist[1]=0;
  drawlist[2]=0;
  isactive[0]= false;
  isactive[1]= false;
  isactive[2]= false;
}

// Destructor
MapPlot::~MapPlot()
{
#ifdef DEBUGPRINT
  cerr << "++ MapPlot::Destructor" << endl;
#endif
  if (glIsList(drawlist[0]))
    glDeleteLists(drawlist[0], 1);
  if (glIsList(drawlist[1]))
    glDeleteLists(drawlist[1], 1);
  if (glIsList(drawlist[2]))
    glDeleteLists(drawlist[2], 1);
}

/*
 Extract plotting-parameters from PlotInfo.
 */
bool MapPlot::prepare(const miString& pinfo, bool ifequal)
{
  miString bgcolourname;
  Area newarea;
  bool areadef=false;
  MapInfo tmpinfo;
  MapManager mapm;

  vector<miString> tokens= pinfo.split('"', '"'), stokens;
  int n= tokens.size();
  for (int i=0; i<n; i++) {
    stokens= tokens[i].split('=');
    if (stokens.size()==2) {
      if (stokens[0].upcase()=="MAP") {
        mapm.getMapInfoByName(stokens[1], tmpinfo);
      } else if (stokens[0].upcase()=="BACKCOLOUR") {
        bgcolourname= stokens[1];
      } else if (stokens[0].upcase()=="AREA") {
        mapm.getMapAreaByName(stokens[1], newarea);
        areadef= true;
      } else if (stokens[0].upcase() == "PROJECTION") {
        newarea.setArea(stokens[1]);
        xyLimit.clear();
        areadef = true;
      } else if (stokens[0].upcase()=="XYLIMIT") {
        vector<miString> vstr= stokens[1].split(',');
        if (vstr.size()>=4) {
          xyLimit.clear();
          for (int j=0; j<4; j++)
            xyLimit.push_back(atof(vstr[j].cStr()) - 1.0);
          if (xyLimit[0]>=xyLimit[1] || xyLimit[2]>=xyLimit[3])
            xyLimit.clear();
        }
      } else if (stokens[0].upcase()=="XYPART") {
        vector<miString> vstr= stokens[1].split(',');
        if (vstr.size()>=4) {
          xyPart.clear();
          for (int j=0; j<4; j++)
            xyPart.push_back(atof(vstr[j].cStr()) * 0.01);
          if (xyPart[0]>=xyPart[1] || xyPart[2]>=xyPart[3])
            xyPart.clear();
        }
      }
    }
  }

  bool equal= (tmpinfo.name == mapinfo.name);

  if (ifequal && !equal) // check if essential mapinfo remains the same
    return false;

  mapinfo= tmpinfo;

  if (bgcolourname.exists())
    bgcolour= bgcolourname; // static Plot member
  if (areadef) {
    reqarea= newarea;
  }
  areadefined= areadef;

  // fill in new options for mapinfo and make proper PlotOptions
  // the different map-elements
  mapm.fillMapInfo(pinfo, mapinfo, contopts, landopts, lonopts, latopts, ffopts);

  // set active zorder layer
  for (int i = 0; i < 3; i++) {
    isactive[i] = ((mapinfo.contour.ison && mapinfo.contour.zorder == i)
        || (mapinfo.land.ison && mapinfo.land.zorder == i)
        || (mapinfo.lon.ison && mapinfo.lon.zorder == i)
        || (mapinfo.lat.ison && mapinfo.lat.zorder == i)
        || (mapinfo.frame.ison && mapinfo.frame.zorder == i));
  }

  mapchanged= true;
  return true;
}

// return requested area
bool MapPlot::requestedArea(Area& rarea)
{
  if (areadefined) {
    rarea= reqarea;
  }
  return areadefined;
}

// static function
bool MapPlot::checkFiles(bool first)
{
  if (first) {
    if (filledmaps.size()==0)
      return false;
  } else {
    // erase the filledmaps not used
    set<miString>::iterator uend= usedFilledmaps.end();
    map<miString,FilledMap>::iterator p= filledmaps.begin();
    vector<miString> erasemaps;
    while (p!=filledmaps.end()) {
      if (usedFilledmaps.find(p->first)==uend)
        erasemaps.push_back(p->first);
      p++;
    }
    int n= erasemaps.size();
    for (int i=0; i<n; i++)
      filledmaps.erase(erasemaps[i]);
  }
  usedFilledmaps.clear();
  return true;
}

void MapPlot::markFiles()
{
  if (mapinfo.type=="triangles") {
    int n= mapinfo.mapfiles.size();
    for (int i=0; i<n; i++)
      usedFilledmaps.insert(mapinfo.mapfiles[i].fname);
  }
}

/*
 Plot one layer of the map
 */
bool MapPlot::plot(const int zorder)
{
#ifdef DEBUGPRINT
  cerr << "++ MapPlot::plot() ++" << endl;
#endif
  if (!enabled || !isactive[zorder])
    return false;

  if (panning) {
    haspanned = true;
  } else if (haspanned) {
    mapchanged= true;
    haspanned= false;
  }

  bool frameok = (reqarea.P().isDefined());
  bool makenew= false;
  bool makelist= false;

  if ( !usedrawlists) {
    // do not use display lists: always make a new plot from scratch
    makenew = true;
  } else if ((dirty && !panning) || mapchanged || !glIsList(drawlist[zorder])) {
    // Making new map drawlist for this zorder
    makelist=true;
    if (glIsList(drawlist[zorder]))
      glDeleteLists(drawlist[zorder], 1);
    drawlist[zorder] = glGenLists(1);
    glNewList(drawlist[zorder], GL_COMPILE_AND_EXECUTE);
    makenew= true;

    if (mapchanged) {
      if ((zorder==2) || (zorder==1 && !isactive[2]) || (zorder==0
          && !isactive[1] && !isactive[2]))
        mapchanged= false;
    }
    // make new plot anyway during panning
  } else if (dirty) { // && mapinfo.type!="triangles"
    makenew= true;
  }

  if (makenew) {
    miString mapfile;
    // diagonal in pixels
    float physdiag= sqrt(pwidth*pwidth+pheight*pheight);
    // map resolution i km/pixel
    float mapres= (physdiag > 0.0 ? gcd/(physdiag*1000) : 0.0);

    // find correct mapfile
    int n= mapinfo.mapfiles.size();
    if (n==1) {
      mapfile= mapinfo.mapfiles[0].fname;
    } else if (n>1) {
      int fnum= 0;
      for (fnum=0; fnum<n; fnum++) {
        if (mapres > mapinfo.mapfiles[fnum].sizelimit)
          break;
      }
      if (fnum==n)
        fnum=n-1;
      mapfile= mapinfo.mapfiles[fnum].fname;
    }

    Colour c= contopts.linecolour;
    if (c==backgroundColour)
      c= backContrastColour;

    if (mapinfo.type=="normal") {
      // check contours
      if (mapinfo.contour.ison && mapinfo.contour.zorder==zorder) {
        float xylim[4]= { maprect.x1, maprect.x2, maprect.y1, maprect.y2 };
        if (!plotMapLand4(mapfile, xylim, contopts.linetype,
            contopts.linewidth, c))
          cerr << "ERROR OPEN/READ " << mapfile << endl;
      }

    } else if (mapinfo.type=="triangles") {
      bool land= mapinfo.land.ison && mapinfo.land.zorder==zorder;
      bool cont= mapinfo.contour.ison && mapinfo.contour.zorder==zorder;

      if (land || cont) {
        if (filledmaps.count(mapfile)==0) {
          filledmaps[mapfile]= FilledMap(mapfile);
        }
        Area fullarea(area.P(), fullrect);
        filledmaps[mapfile].plot(fullarea, maprect, gcd, land, cont, !cont
            && mapinfo.contour.ison, contopts.linetype.bmap,
            contopts.linewidth, c.RGBA(), landopts.fillcolour.RGBA(),
            backgroundColour.RGBA());
      }

    } else if (mapinfo.type=="lines_simple_text") {
      // check contours
      if (mapinfo.contour.ison && mapinfo.contour.zorder==zorder) {
        if (!plotLinesSimpleText(mapfile))
          cerr << "ERROR OPEN/READ " << mapfile << endl;
      }

    } else if (mapinfo.type=="shape") {
      bool land= mapinfo.land.ison && mapinfo.land.zorder==zorder;
      bool cont= mapinfo.contour.ison && mapinfo.contour.zorder==zorder;

      Colour c= contopts.linecolour;

      if (shapemaps.count(mapfile) == 0) {
#ifdef DEBUGPRINT
        cerr << "Creating new shapeObject for map: " << mapfile << endl;
#endif
        shapemaps[mapfile] = ShapeObject();
        shapemaps[mapfile].read(mapfile,true);
        shapeareas[mapfile] = Area(area);
      }
      if (shapeareas[mapfile].P() != area.P()) {
#ifdef DEBUGPRINT
        cerr << "Projection wrong for: " << mapfile << endl;
#endif
        bool success = shapemaps[mapfile].changeProj(shapeareas[mapfile]);

        // Reread file if unsuccessful
        if(!success) {
          shapemaps[mapfile] = ShapeObject();
          shapemaps[mapfile].read(mapfile,true);
        }
        shapeareas[mapfile] = Area(area);
      }
      Area fullarea(area.P(), fullrect);
      shapemaps[mapfile].plot(fullarea, gcd, land, cont, !cont && mapinfo.contour.ison,
          contopts.linetype.bmap, contopts.linewidth, c.RGBA(),
          landopts.fillcolour.RGBA(), backgroundColour.RGBA());
    } else {
      cerr << "Unknown maptype for map " << mapinfo.name << " = "
          << mapinfo.type << endl;
    }

    if (makelist)
      glEndList();
    UpdateOutput();

  } else {
    // execute old display list
    if (glIsList(drawlist[zorder]))
      glCallList(drawlist[zorder]);
    UpdateOutput();
  }

  // check latlon
  bool plot_lon = mapinfo.lon.ison && mapinfo.lon.zorder == zorder;
  bool plot_lat = mapinfo.lat.ison && mapinfo.lat.zorder == zorder;
  if (plot_lon || plot_lat) {

    if (mapinfo.lon.showvalue || mapinfo.lat.showvalue)
      fp->setFont("BITMAPFONT");

    plotGeoGrid(mapinfo, plot_lon, plot_lat);
  }

  // plot frame
  if (frameok && mapinfo.frame.ison && mapinfo.frame.zorder==zorder) {
    //cerr << "Plotting frame for layer:" << zorder << endl;
    Rectangle reqr= reqarea.R();
    Colour c= ffopts.linecolour;
    if (c==backgroundColour)
      c= backContrastColour;
    glColor4ubv(c.RGBA());
    //       glColor3f(0.0,0.0,0.0);
    glLineWidth(ffopts.linewidth);
    if (ffopts.linetype.stipple) {
      glLineStipple(1, ffopts.linetype.bmap);
      glEnable(GL_LINE_STIPPLE);
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    if (reqarea.P()==area.P()) {
      glBegin(GL_LINE_LOOP);
      glVertex2f(reqr.x1, reqr.y1);
      glVertex2f(reqr.x2, reqr.y1);
      glVertex2f(reqr.x2, reqr.y2);
      glVertex2f(reqr.x1, reqr.y2);
      glEnd();
    } else {
      // frame belongs to a different projection
      // ..plot it in the current projection

      // first check if difference only in translation/scaling
      bool similarAreas=false;
//      gc.checkAreaSimilarity(reqarea, area, similarAreas);
      // number of subdivisions for each frame-side
      int nsub = (similarAreas ? 1 : 20);
      int npos = 4*nsub;
      float *x= new float[npos];
      float *y= new float[npos];
      float dx= (reqr.x2 - reqr.x1)/float(nsub);
      float dy= (reqr.y2 - reqr.y1)/float(nsub);
      float px, py;
      int i;
      // fill float-arrays with x,y in original projection
      x[0]=x[3*nsub]= reqr.x1;
      x[nsub]=x[2*nsub]= reqr.x2;
      y[0]=y[nsub]= reqr.y1;
      y[2*nsub]=y[3*nsub]= reqr.y2;
      for (i=1, px= reqr.x1+dx; i<nsub; px+=dx, i++) {
        x[i]= x[3*nsub-i]= px;
        y[i]= reqr.y1;
        y[3*nsub-i]= reqr.y2;
      }
      for (i=nsub+1, py= reqr.y1+dy; i<2*nsub; py+=dy, i++) {
        y[i]= y[5*nsub-i]= py;
        x[i]= reqr.x2;
        x[5*nsub-i]= reqr.x1;
      }
      // convert points to current projection
      gc.getPoints(reqarea, area, npos, x, y);

      glBegin(GL_LINE_LOOP);
      for (int i=0; i<npos; i++) {
        glVertex2f(x[i], y[i]);
      }
      glEnd();

      delete[] x;
      delete[] y;
    }
    UpdateOutput();
  }

  UpdateOutput();
  glDisable(GL_LINE_STIPPLE);

  return true;
}


bool MapPlot::plotMapLand4(const miString& filename, float xylim[],
    const Linetype& linetype, float linewidth, const Colour& colour)
{
  //
  //       plot land.  data from 'standard' file, type 4.
  //
  //   handling version 1 and 2 files.
  //   (version 1 with unsorted lines, precision usually 1/100 degree,
  //    version 2 with sorted lines (two level box system) with
  //    best possible precision approx. 2m with 10x10 and 1x1 degree
  //    boxes).
  //
  //       graphigs: OpenGL
  //
  //     input:
  //     ------
  //        filename:  file name
  //        xylim[0-3]: x1,x2,y1,y2
  //        linetype:   line type bitmask
  //        linewidth:  line width
  //        colour:     colour-components rgba
  //
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //  standard file structure:
  //  ------------------------
  //  - record format: access='direct',form='unformatted'
  //  - record length: 2048 bytes
  //  - file header (first part of record 1), integer*2 words:
  //      1 - identifier = 104
  //      2 - version    = 1
  //      3 - record length in unit bytes = 2048
  //      4 - length of data description (words)
  //  - data description (second part of record 1, minimum 6 words),
  //    integer*2 words:
  //      1 - data type:   1=lines      2=polygons/lines
  //      2 - data format: 1=integer*2
  //      3 - no. of words describing each line (in front of each line)
  //      4 - coordinate type: 1=latitude,longitude
  //      5 - scaling of coordinates (real_value = 10.**scale * file_value)
  //      6 - 0 (not used)
  //  - data follows after file header and data description.
  //  - each line has a number of words describing it in front of the
  //    coordinate pairs.
  //  - line description (minimum 1 word):
  //      1 - no. of positions (coordinate pairs) on the line.
  //          0 = end_of_data
  //      2 - possible description of the line (for plot selection):
  //            1=land/sea  2=borders  3=lakes  4=rivers (values 1-100).
  //      3 - line type:  1=line  2=polygon
  //      restriction: a line desciption is never split into two records
  //                   (i.e. possibly unused word(s) at end_of_record)
  //  - line coordinates:
  //          latitude,longitude, latitude,longitude, ......
  //      restriction: a coordinate pair is never split into two records
  //                   (i.e. possibly one unused word at end_of_record)
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  //  current version is not plotting polygons (only plotting the borders)
  //
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  //---------------------------------------------------------------------
  //  DNMI/FoU  29.07.1992  Anstein Foss
  //  DNMI/FoU  29.08.1995  Anstein Foss
  //  DNMI/FoU  24.09.1995  Anstein Foss ... utskrift for UNIRAS (ascii)
  //  DNMI/FoU  30.01.1996  Anstein Foss ... version 2 file (sorted)
  //  DNMI/FoU  09.08.1999  Anstein Foss ... C/C++
  //  met.no    25.05.2009  Audun Christoffersen ... independent on met.no projections
  //---------------------------------------------------------------------

  Area geoArea;
  Projection geoProj;
  geoProj.setGeographic();
  geoArea.setP(geoProj);

  const unsigned int nwrec = 1024;
  const unsigned int maxpos = 2000;
  const int mlevel1 = 36 * 18;
  const int mlevel2 = 10 * 10;

  short indata[nwrec];
  float x[maxpos], y[maxpos];

  unsigned int nw, npi, n, npp, npos;
  int np, irec, jrec, nd, err, nwdesc, version;
  int iscale2, nlevel1, nlevel2, i, j;//, ibgn, ierror;
  int n1, n2, nn, nwx1, nwx2, nlines, nl;

  float scale, slat2, slon2, x1, y1, x2, y2, dx, dy, dxbad;
  float glon, glat, glonmin, glonmax, glatmin, glatmax, reflon, reflat;

  float box1[4], box2[4];

  short int ilevel1[mlevel1][5];
  short int ilevel2[mlevel2][5];

  float jumplimit = area.P().getMapLinesJumpLimit();

  // colour, linetype and -width
  glColor4ubv(colour.RGBA());
  glLineWidth(linewidth);
  if (linetype.stipple) {
    glLineStipple(1, linetype.bmap);
    glEnable(GL_LINE_STIPPLE);
  }

  FILE *pfile;
  long offset;

  if ((pfile = fopen(filename.c_str(), "rb")) == NULL)
    return false;

  irec = 1;
  if (fread(indata, 2, nwrec, pfile) != nwrec) {
    fclose(pfile);
    return false;
  }

  // check file header
  err = 0;
  if (indata[0] != 104)
    err = 1;
  if (indata[1] != 1 && indata[1] != 2)
    err = 1;
  if (indata[2] != 2048)
    err = 1;
  if (indata[3] < 6)
    err = 1;
  version = indata[1];
  if (version == 2 && indata[3] < 18)
    err = 1;
  nw = 3;
  nd = indata[3];
  if (nw + nd >= nwrec)
    err = 1;
  // data description
  if (indata[nw + 1] != 1 && indata[nw + 1] != 2)
    err = 1;
  if (indata[nw + 2] != 1)
    err = 1;
  nwdesc = indata[nw + 3];
  if (nwdesc < 1)
    err = 1;
  if (indata[nw + 4] != 1)
    err = 1;

  if (err != 0) {
    fclose(pfile);
    return false;
  }

//  cerr << "plotMapLand4 file=" << filename << " version=" << version << endl;

  // for version 1 this is the scaling of all values (lat,long)
  // for version 2 this is the scaling of reference values (lat,long)
  scale = indata[nw + 5];
  scale = powf(10., scale);
  if (version == 2) {
    iscale2 = indata[nw + 6];
    nlevel1 = indata[nw + 7];
    for (i = 0; i < 4; i++)
      box1[i] = indata[nw + 11 + i] * scale;
    for (i = 0; i < 4; i++)
      box2[i] = indata[nw + 15 + i] * scale;
  }

  nw += nd;

  dxbad = 1.e+35;
  // TODO: remove this?
  // polar stereographic
/*
  if (gridtype != 1 && gridtype != 4) {
    float tx[2] = { -170., 170. };
    float ty[2] = { 60., 60. };
    xyconvert(2, tx, ty, igeogrid, geogrid, gridtype, gridparam, &ierror);
    dxbad = fabsf(tx[0] - tx[1]);
  } else {
    dxbad = 1.e+35;
  }
*/

  bool illegal_southpole = !area.P().isLegal(0.0, -90.0);
  bool illegal_northpole = !area.P().isLegal(0.0, 90.0);

  if (version == 1) {

    // version 1 (not sorted) ........................................

    npos = 1;

    while (npos > 0) {

      // get line description
      if (nw + nwdesc >= nwrec) {
        irec++;
        if (fread(indata, 2, nwrec, pfile) != nwrec) {
          fclose(pfile);
          return false;
        }
        nw = -1;
      }
      npos = indata[nw + 1];

      // current version: not using more than first word of line description
      nw += nwdesc;
      np = npp = 0;

      while (npp < npos) {
        if (nw + 2 >= nwrec) {
          irec++;
          if (fread(indata, 2, nwrec, pfile) != nwrec) {
            fclose(pfile);
            return false;
          }
          nw = -1;
        }
        npi = npos - npp;
        if (npi * 2 > nwrec - nw - 1)
          npi = (nwrec - nw - 1) / 2;
        if (npi > maxpos - np)
          npi = maxpos - np;

        for (n = 0; n < npi; n++, nw += 2) {
          //  y[] = latitude  = scale * indata[i]
          //  x[] = longitude = scale * indata[i+1]
          y[np + n] = scale * indata[nw + 1];
          x[np + n] = scale * indata[nw + 2];
        }
        npp += npi;
        np += npi;

        if ((npp == npos || (unsigned int)np == maxpos) && np > 1) {
          if (illegal_southpole || illegal_northpole){
          /*
          if (gridtype == 5 || gridtype == 6) {
            // mercator/lambert, avoid latitudes +90 and -90
             */
            for (i = 0; i < np; ++i) {
              if (illegal_northpole && y[i] > +89.95)
                y[i] = +89.95;
              if (illegal_southpole && y[i] < -89.95)
                y[i] = -89.95;
            }
          }
          x1 = x[np - 1];
          y1 = y[np - 1];
          // convert coordinates from longitude,latitude to x,y
          bool b = gc.getPoints(geoArea,area,np,x,y);
          if (!b){
            cerr << "plotMapLand4(0), getPoints returned false" << endl;
          }

          //xyconvert(np, x, y, igeogrid, geogrid, gridtype, gridparam, &ierror);
/*
 * TODO: what about this?
          if (gridtype == 1 || gridtype == 4) {
            xyclip(np, &x[0], &y[0], &xylim[0], jumplimit);
          } else {
            // some problems handled here (but not good, may loose lines)
            // skip lines crossing the 180 degrees meridian
            ibgn = 0;
            for (i = 1; i < np; ++i) {
              if (fabsf(x[i - 1] - x[i]) > dxbad) {
                // plot (part of line inside xylim area)
                if (i - ibgn > 1)
                  xyclip(i - ibgn, &x[ibgn], &y[ibgn], &xylim[0], jumplimit);
                ibgn = i;
              }
            }
            // plot (part of line inside xylim area)
            if (np - ibgn > 1)
              xyclip(np - ibgn, &x[ibgn], &y[ibgn], &xylim[0], jumplimit);
          }
          */
          clipPrimitiveLines(np,x,y,xylim,jumplimit);
          x[0] = x1;
          y[0] = y1;
          np = 1;
        }
      }
    }

  } else {

    // version 2 (two level sorted) ................................

    if (nlevel1 > mlevel1) {
      fclose(pfile);
      return false;
    }

    if (box2[0] > box2[1])
      slat2 = box2[0] / float(iscale2);
    else
      slat2 = box2[1] / float(iscale2);
    if (box2[2] > box2[3])
      slon2 = box2[2] / float(iscale2);
    else
      slon2 = box2[3] / float(iscale2);

    for (n1 = 0; n1 < nlevel1; ++n1) {
      for (i = 0; i < 5; ++i) {
        nw++;
        if (nw >= nwrec) {
          irec++;
          if (fread(indata, 2, nwrec, pfile) != nwrec) {
            fclose(pfile);
            return false;
          }
          nw = 0;
        }
        ilevel1[n1][i] = indata[nw];
      }
    }

    // first simple attempt
    nn = int(sqrtf(float(maxpos)));
    if (nn > 16)
      nn = 16;
    x1 = xylim[0];
    x2 = xylim[1];
    y1 = xylim[2];
    y2 = xylim[3];
    dx = (x2 - x1) / float(nn - 1);
    dy = (y2 - y1) / float(nn - 1);
    n = 0;
    for (j = 0; j < nn; ++j) {
      for (i = 0; i < nn; ++i, ++n) {
        x[n] = x1 + i * dx;
        y[n] = y1 + j * dy;
      }
    }
    nn = n;
    bool b = gc.getPoints(area,geoArea,nn,x,y);
    if (!b){
      cerr << "plotMapLand4(1), getPoints returned false" << endl;
    }
    glonmin = glonmax = x[0];
    glatmin = glatmax = y[0];
    for (n = 1; n < (unsigned int)nn; ++n) {
      if (glonmin > x[n])
        glonmin = x[n];
      if (glonmax < x[n])
        glonmax = x[n];
      if (glatmin > y[n])
        glatmin = y[n];
      if (glatmax < y[n])
        glatmax = y[n];
    }
    glonmin -= 1.;
    glonmax += 1.;
    glatmin -= 1.;
    glatmax += 1.;

    //area.P().adjustGeographicExtension(glonmin,glonmax,glatmin,glatmax);

    for (n1 = 0; n1 < nlevel1; ++n1) {

      glat = ilevel1[n1][0] * scale;
      glon = ilevel1[n1][1] * scale;
      nlevel2 = ilevel1[n1][2];

      if (nlevel2 > mlevel2) {
        fclose(pfile);
        return false;
      }

      if (glat + box1[1] >= glatmin && glat + box1[0] <= glatmax && glon
          + box1[3] >= glonmin && glon + box1[2] <= glonmax && nlevel2 > 0) {
        jrec = irec;
        nwx1 = ilevel1[n1][3];
        nwx2 = ilevel1[n1][4];
        irec = (nwx1 + nwx2 * 32767 + nwrec - 1) / nwrec;
        nw = (nwx1 + nwx2 * 32767) - (irec - 1) * nwrec - 2;
        if (irec != jrec) {
          offset = (irec - 1) * nwrec * 2;
          if (fseek(pfile, offset, SEEK_SET) != 0) {
            fclose(pfile);
            return false;
          }
          if (fread(indata, 2, nwrec, pfile) != nwrec) {
            fclose(pfile);
            return false;
          }
        }

        for (n2 = 0; n2 < nlevel2; ++n2) {
          for (i = 0; i < 5; ++i) {
            nw++;
            if (nw >= nwrec) {
              irec++;
              if (fread(indata, 2, nwrec, pfile) != nwrec) {
                fclose(pfile);
                return false;
              }
              nw = 0;
            }
            ilevel2[n2][i] = indata[nw];
          }
        }

        for (n2 = 0; n2 < nlevel2; ++n2) {

          reflat = ilevel2[n2][0] * scale;
          reflon = ilevel2[n2][1] * scale;
          nlines = ilevel2[n2][2];

          if (reflat + box2[1] >= glatmin && reflat + box2[0] <= glatmax
              && reflon + box2[3] >= glonmin && reflon + box2[2] <= glonmax
              && nlines > 0) {
            jrec = irec;
            nwx1 = ilevel2[n2][3];
            nwx2 = ilevel2[n2][4];
            irec = (nwx1 + nwx2 * 32767 + nwrec - 1) / nwrec;
            nw = (nwx1 + nwx2 * 32767) - (irec - 1) * nwrec - 2;

            if (irec != jrec) {
              offset = (irec - 1) * nwrec * 2;
              if (fseek(pfile, offset, SEEK_SET) != 0) {
                fclose(pfile);
                return false;
              }
              if (fread(indata, 2, nwrec, pfile) != nwrec) {
                fclose(pfile);
                return false;
              }
            }

            for (nl = 0; nl < nlines; ++nl) {
              // get line description
              if (nw + nwdesc >= nwrec) {
                irec++;
                if (fread(indata, 2, nwrec, pfile) != nwrec) {
                  fclose(pfile);
                  return false;
                }
                nw = -1;
              }
              npos = indata[nw + 1];

              // current version:
              // not using more than first word of line description
              nw += nwdesc;
              np = npp = 0;

              while (npp < npos) {
                if (nw + 2 >= nwrec) {
                  irec++;
                  if (fread(indata, 2, nwrec, pfile) != nwrec) {
                    fclose(pfile);
                    return false;
                  }
                  nw = -1;
                }
                npi = npos - npp;
                if (npi * 2 > nwrec - nw - 1)
                  npi = (nwrec - nw - 1) / 2;
                if (npi > maxpos - np)
                  npi = maxpos - np;

                for (n = 0; n < npi; ++n, nw += 2) {
                  // y[] = latitude
                  // x[] = longitude
                  y[np + n] = reflat + slat2 * indata[nw + 1];
                  x[np + n] = reflon + slon2 * indata[nw + 2];
                }
                npp += npi;
                np += npi;

                if ((npp == npos || (unsigned int)np == maxpos) && np > 1) {
                  if (illegal_southpole || illegal_northpole){
                    for (i = 0; i < np; ++i) {
                      if (illegal_northpole && y[i] > +89.95)
                        y[i] = +89.95;
                      if (illegal_southpole && y[i] < -89.95)
                        y[i] = -89.95;
                    }
                  }
                  x1 = x[np - 1];
                  y1 = y[np - 1];
                  // convert coordinates from longitude,latitude to x,y
                  bool b = gc.getPoints(geoArea,area,np,x,y);
                  if (!b){
                    cerr << "plotMapLand4(2), getPoints returned false"<< endl;
                  }
/*
TODO: what about this?
                  if (gridtype == 1 || gridtype == 4) {
                    xyclip(np, &x[0], &y[0], &xylim[0], jumplimit);
                  } else {
                    // some problems handled here (but not good, may loose lines)
                    // skip lines crossing the 180 degrees meridian
                    ibgn = 0;
                    for (i = 1; i < np; ++i) {
                      if (fabsf(x[i - 1] - x[i]) > dxbad) {
                        // plot (part of line inside xylim area)
                        if (i - ibgn > 1)
                          xyclip(i - ibgn, &x[ibgn], &y[ibgn], &xylim[0],
                              jumplimit);
                        ibgn = i;
                      }
                    }
                    // plot (part of line inside xylim area)
                    if (np - ibgn > 1)
                      xyclip(np - ibgn, &x[ibgn], &y[ibgn], &xylim[0],
                          jumplimit);
                  }
*/
                  clipPrimitiveLines(np, x, y, xylim, jumplimit);

                  x[0] = x1;
                  y[0] = y1;
                  np = 1;
                }
              }
            }
          }
        }
      }
    }
  }

  glDisable(GL_LINE_STIPPLE);
  fclose(pfile);

  return true;
}



bool MapPlot::plotGeoGrid(const MapInfo& mapinfo, bool plot_lon, bool plot_lat, int plotResolution)
{
  float longitudeStep = mapinfo.lon.density;
  bool lon_values     = mapinfo.lon.showvalue;
  int lon_valuepos = convertLatLonPos(mapinfo.lon.value_pos);
  float lon_fontsize = mapinfo.lon.fontsize;

  float latitudeStep = mapinfo.lat.density;
  bool lat_values = mapinfo.lat.showvalue;
  int lat_valuepos = convertLatLonPos(mapinfo.lat.value_pos);
  float lat_fontsize = mapinfo.lat.fontsize;

/*
  cerr << (lon_values ? "lon_values=ON" : "lon_values=OFF") << " "
  << (lat_values ? "lat_values=ON" : "lat_values=OFF") << " lon_valuepos="
  << lon_valuepos << " lat_valuepos=" << lat_valuepos << endl;
*/


  Projection p= area.P();
  if (!p.isDefined()) {
    cerr<<"MapPlot::plotGeoGrid ERROR: undefined projection"<<endl;
    return false;
  }

  if (latitudeStep<0.001 || longitudeStep<0.001) {
    cerr<<"MapPlot::plotGeoGrid ERROR: latitude/longitude step"<<endl;
    return false;
  }

  if (latitudeStep>180.)
    latitudeStep= 180.;
  if (longitudeStep>180.)
    longitudeStep= 180.;
  if (plotResolution<1)
    plotResolution= 10;
  if((latitudeStep>30 || longitudeStep>30) && plotResolution == 10) {
    plotResolution = 100;
  }

  float xylim[4]= { maprect.x1, maprect.x2, maprect.y1, maprect.y2 };

  int n, j;
  float lonmin=FLT_MAX, lonmax=-FLT_MAX, latmin=FLT_MAX, latmax=-FLT_MAX;
  float jumplimit;

  bool b = p.calculateGeogridParameters(xylim, lonmin, lonmax, latmin, latmax,
      jumplimit);

  if (!b) return false;

  int ilon1= int(lonmin/longitudeStep);
  int ilon2= int(lonmax/longitudeStep);
  int ilat1= int(latmin/latitudeStep);
  int ilat2= int(latmax/latitudeStep);
  if (lonmin>0.0)
    ilon1++;
  if (lonmax<0.0)
    ilon2--;
  if (latmin>0.0)
    ilat1++;
  if (latmax<0.0)
    ilat2--;

  float glon1= ilon1*longitudeStep;
  float glon2= ilon2*longitudeStep;
  float glat1= ilat1*latitudeStep;
  float glat2= ilat2*latitudeStep;
  float glon, glat;


//########################################################################
//cerr<<"longitudeStep,latitudeStep:  "<<longitudeStep<<" "<<latitudeStep<<endl;
//cerr<<"ilon1,ilon2,ilat1,ilat2:     "<<ilon1<<" "<<ilon2<<" "<<ilat1<<" "<<ilat2<<endl;
//cerr<<"glon1,glon2,glat1,glat2:     "<<glon1<<" "<<glon2<<" "<<glat1<<" "<<glat2<<endl;
//cerr<<"lonmin,lonmax,latmin,latmax: "<<lonmin<<" "<<lonmax<<" "<<latmin<<" "<<latmax<<endl;
//cerr<<"maprect x1,x2,y1,y2:         "<<xylim[0]<<" "<<xylim[1]<<" "<<xylim[2]<<" "<<xylim[3]<<endl;
//########################################################################

  n= (ilat2-ilat1+1)*(ilon2-ilon1+1);
  if (n>1200) {
    float reduction= float(n)/1200.;
    n= int(float(plotResolution)/reduction + 0.5);
    if (n<2)
      n=2;
    //########################################################################
    //cerr<<"geoGrid: plotResolution,n: "<<plotResolution<<" "<<n<<endl;
    //########################################################################
    if (plotResolution>n)
      plotResolution= n;
  }

  bool geo2xyError= false;


  // draw longitude lines.....................................

  if (plot_lon && ilon1<=ilon2) {

    Colour c= lonopts.linecolour;
    if (c==backgroundColour)
      c= backContrastColour;
    glColor4ubv(c.RGBA());
    glLineWidth(lonopts.linewidth+0.1);
    if (lonopts.linetype.bmap!=0xFFFF) {
      glLineStipple(1, lonopts.linetype.bmap);
      glEnable(GL_LINE_STIPPLE);
    }

    // curved longitude lines

    float dlat = latitudeStep / float(plotResolution);
    int nlat = (ilat2 - ilat1) * plotResolution + 1;
    int n1 = 0, n2 = 0;
    while (glat1 - dlat * float(n1 + 1) >= latmin)
      n1++;
    while (glat2 + dlat * float(n2 + 1) <= latmax)
      n2++;
    glat = glat1 - dlat * float(n1);
    nlat += (n1 + n2);
    if (nlat < 2)
      cerr << "** MapPlot::plotGeoGrid ERROR in Curved longitude lines, nlat="
          << nlat << endl;
    else {
      float *x = new float[nlat];
      float *y = new float[nlat];
      for (int ilon = ilon1; ilon <= ilon2; ilon++) {
        glon = longitudeStep * float(ilon);
        ostringstream ost;
        ost << fabsf(glon) << " " << (glon < 0 ? "W" : "E");
        miString plotstr = ost.str();
        for (n = 0; n < nlat; n++) {
          x[n] = glon;
          y[n] = glat + dlat * float(n);
        }
        if (gc.geo2xy(area, nlat, x, y)) {
          clipPrimitiveLines(nlat, x, y, xylim, jumplimit, lon_values,
              lon_valuepos, plotstr);
        } else {
          geo2xyError = true;
        }
      }
      delete[] x;
      delete[] y;
    }
  }
  UpdateOutput();
  glDisable(GL_LINE_STIPPLE);

  if (value_annotations.size() > 0) {
    fp->setFontSize(lon_fontsize);
    n = value_annotations.size();
    Rectangle prevr;
    for (j = 0; j < n; j++) {
      float x = value_annotations[j].x;
      float y = value_annotations[j].y;
      if (prevr.isinside(x,y)){
        continue;
      }
      fp->drawStr(value_annotations[j].t.cStr(), x, y, 0);
      float w,h;
      fp->getStringSize(value_annotations[j].t.cStr(),w,h);
      prevr = Rectangle(x,y,x+w,y+h);
    }
    value_annotations.clear();
    UpdateOutput();
  }

  // draw latitude lines......................................

  if (plot_lat && ilat1<=ilat2) {
    Colour c= latopts.linecolour;
    if (c==backgroundColour)
      c= backContrastColour;
    glColor4ubv(c.RGBA());
    glLineWidth(latopts.linewidth+0.1);
    if (latopts.linetype.bmap!=0xFFFF) {
      glLineStipple(1, latopts.linetype.bmap);
      glEnable(GL_LINE_STIPPLE);
    }

    // curved latitude lines

    float dlon = longitudeStep / float(plotResolution);
    int nlon = (ilon2 - ilon1) * plotResolution + 1;
    int n1 = 0, n2 = 0;
    while (glon1 - dlon * float(n1 + 1) >= lonmin)
      n1++;
    while (glon2 + dlon * float(n2 + 1) <= lonmax)
      n2++;
    glon = glon1 - dlon * float(n1);
    nlon += (n1 + n2);
    if (nlon < 1) {
      cerr << "** MapPlot::plotGeoGrid ERROR in Curved Latitude lines, nlon="
          << nlon << endl;
      cerr << "lonmin,lonmax=" << lonmin << "," << lonmax << endl;
    } else {
      float *x = new float[nlon];
      float *y = new float[nlon];
      for (int ilat = ilat1; ilat <= ilat2; ilat++) {
        glat = latitudeStep * float(ilat);
        ostringstream ost;
        ost << fabsf(glat) << " " << (glat < 0 ? "S" : "N");
        miString plotstr = ost.str();
        for (n = 0; n < nlon; n++) {
          x[n] = glon + dlon * float(n);
          y[n] = glat;
        }
        if (gc.geo2xy(area, nlon, x, y)) {
          clipPrimitiveLines(nlon, x, y, xylim, jumplimit, lat_values,
              lat_valuepos, plotstr);
        } else {
          geo2xyError = true;
        }
      }
      delete[] x;
      delete[] y;
    }
  }

  UpdateOutput();
  glDisable(GL_LINE_STIPPLE);

  if (value_annotations.size() > 0) {
    fp->setFontSize(lat_fontsize);
    n = value_annotations.size();
    Rectangle prevr;
    for (j = 0; j < n; j++) {
      float x = value_annotations[j].x;
      float y = value_annotations[j].y;
      if (prevr.isinside(x,y)){
        continue;
      }
      fp->drawStr(value_annotations[j].t.cStr(), x, y, 0);
      float w,h;
      fp->getStringSize(value_annotations[j].t.cStr(),w,h);
      prevr = Rectangle(x,y,x+w,y+h);
    }
    value_annotations.clear();
    UpdateOutput();
  }

/*  }*/

  if (geo2xyError) {
    cerr<<"MapPlot::plotGeoGrid ERROR: gc.geo2xy failure(s)"<<endl;
  }

  return true;
}

bool MapPlot::plotLinesSimpleText(const miString& filename)
{
  // plot lines from very simple text file,
  //  each line in file: latitude(desimal,float) longitude(desimal,float)
  //  end_of_line: "----"

  ifstream file;
  file.open(filename.c_str());
  if (file.bad())
    return false;

  float xylim[4]= { maprect.x1, maprect.x2, maprect.y1, maprect.y2 };

  Colour c= contopts.linecolour;
  if (c==backgroundColour)
    c= backContrastColour;
  glColor4ubv(c.RGBA());
  glLineWidth(contopts.linewidth+0.1);
  if (contopts.linetype.bmap!=0xFFFF) {
    glLineStipple(1, contopts.linetype.bmap);
    glEnable(GL_LINE_STIPPLE);
  }

  const int nmax= 2000;

  float x[nmax];
  float y[nmax];

  miString str;
  vector<miString> coords;
  bool endfile= false;
  bool endline;
  int nlines= 0;
  int n= 0;
  float jumplimit = area.P().getMapLinesJumpLimit();

  while (!endfile) {

    endline= false;

    while (!endline && n<nmax && getline(file, str)) {
      coords= str.split(' ', true);
      if (coords.size()>=2) {
        y[n]= atof(coords[0].c_str()); // latitude
        x[n]= atof(coords[1].c_str()); // longitude
        endline= (y[n]< -90.01f || y[n]> +90.01f || x[n]<-360.01f || x[n]
            >+360.01f);
      } else {
        endline= true;
      }
      n++;
    }

    if (endline)
      n--;

    else if (n<nmax)
      endfile= true;

    if (n>1) {
      float xn= x[n-1];
      float yn= y[n-1];
      if (gc.geo2xy(area, n, x, y)) {
        clipPrimitiveLines(n, x, y, xylim, jumplimit);
        nlines++;
      } else {
        cerr<<"MapPlot::plotLinesSimpleText  gc.geo2xy ERROR" << endl;
        endfile= true;
      }
      if (!endline && !endfile) {
        x[0]= xn;
        y[0]= yn;
        n= 1;
      } else {
        n= 0;
      }
    }

  }

  file.close();

  UpdateOutput();

  glDisable(GL_LINE_STIPPLE);

  return (nlines>0);
}

void MapPlot::clipPrimitiveLines(int npos, float *x, float *y, float xylim[4],
    float jumplimit, bool plotanno, int anno_position, miString anno)
{
  int i, n = 0;
  while (n < npos) {
    i = n++;
    while (n < npos && fabsf(x[n - 1] - x[n]) < jumplimit && fabsf(y[n - 1]
        - y[n]) < jumplimit) {
      n++;
    }
    xyclip(n - i, &x[i], &y[i], xylim, plotanno, anno_position, anno);
  }
}

void MapPlot::xyclip(int npos, float *x, float *y, float xylim[4],
    bool plotanno, int anno_position, miString anno)
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
  float xa, xb, ya, yb, xint, yint, x1, x2, y1, y2;
  float xc[4], yc[4];

  if (npos < 2)
    return;

  xa = xylim[0];
  xb = xylim[1];
  ya = xylim[2];
  yb = xylim[3];

  float xoffset = (xb - xa) / 200.0;
  float yoffset = (yb - ya) / 200.0;

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
      if (plotanno) {
        if ((anno_position == map_all) ||
            (anno_position == map_left && fabsf(xint - xa) < 0.001) ||
            (anno_position == map_bottom && fabsf(yint - ya) < 0.001)) {
          ValueAnno va;
          va.t = anno;
          va.x = xint + xoffset;
          va.y = yint + yoffset;
          value_annotations.push_back(va);
        }
      }
    } else if (k1 == 1) {
      // siste punkt paa linjestykke innenfor
      glBegin(GL_LINE_STRIP);
      glVertex2f(xint, yint);
      for (i = nint + 1; i < n; i++) {
        glVertex2f(x[i], y[i]);
      }
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
    for (i = nint + 1; i < npos; i++) {
      glVertex2f(x[i], y[i]);
    }
    glEnd();
  }
}

int MapPlot::convertLatLonPos(miutil::miString pos)
{
  pos = pos.downcase();
  if(pos == "left") {
    return map_left;
  }
  if(pos == "bottom") {
    return map_bottom;
  }
  if(pos == "right") {
    return map_right;
  }
  if(pos == "top") {
    return map_top;
  }
  if(pos == "both") {
    return map_all;
  }

  //obsolete syntax
  return atoi(pos.c_str());

}
