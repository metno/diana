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

#include <diMapPlot.h>

#include <diMapManager.h>
#include <diFontManager.h>

#include <puTools/miStringFunctions.h>

#include <cfloat>
#include <fstream>

//#define DEBUGPRINT
#define MILOGGER_CATEGORY "diana.MapPlot"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

// static members
map<std::string,FilledMap> MapPlot::filledmaps;
set<std::string> MapPlot::usedFilledmaps;
map<std::string,ShapeObject> MapPlot::shapemaps;
map<std::string,Area> MapPlot::shapeareas;

MapPlot::MapPlot()
  : mapchanged(true)
  , haspanned(false),
#if defined(USE_PAINTGL)
              usedrawlists(false)
#else
              usedrawlists(true)
#endif
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  drawlist[0]=0;
  drawlist[1]=0;
  drawlist[2]=0;
  isactive[0]= false;
  isactive[1]= false;
  isactive[2]= false;
}

MapPlot::~MapPlot()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
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
bool MapPlot::prepare(const std::string& pinfo, const Area& rarea, bool ifequal)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE(pinfo);
#endif

  Area newarea;
  MapManager mapm;

  reqarea = rarea; //get requested area from previous MapPlot

  // split on blank, preserve ""
  vector<std::string> tokens= miutil::split_protected(pinfo, '"','"'," ",true);
  int n= tokens.size();
  if (n == 0) {
    return false;
  }

  //new syntax
  //pinfo AREA defines area (projection,rectangle)
  //pinfo MAP defines map (which map, colours, lat,lon etc)
  //obsolete
  //pinfo MAP contains "area=..."


  std::string bgcolourname;
  MapInfo tmpinfo;
  bool areadef=false;
  for (int i=0; i<n; i++) {
    vector<std::string> stokens= miutil::split(tokens[i], 0, "=");
    if (stokens.size()==2) {
      if (miutil::to_upper(stokens[0])=="MAP") {
        mapm.getMapInfoByName(stokens[1], tmpinfo);
      } else if (miutil::to_upper(stokens[0])=="BACKCOLOUR") {
        bgcolourname= stokens[1];
      } else if (miutil::to_upper(stokens[0])=="AREA") { //obsolete
        mapm.getMapAreaByName(stokens[1], newarea);
        areadef= true;
      } else if (miutil::to_upper(stokens[0]) == "PROJECTION") { //obsolete
        newarea.setArea(stokens[1]);
        getStaticPlot()->xyLimit.clear();
        areadef = true;
      } else if (miutil::to_upper(stokens[0])=="XYLIMIT") { //todo: add warning and show new syntax
        vector<std::string> vstr= miutil::split(stokens[1], 0, ",");
        if (vstr.size()>=4) {
          getStaticPlot()->xyLimit.clear();
          getStaticPlot()->xyLimit.push_back((atof(vstr[0].c_str()) - 1.0)*newarea.P().getGridResolutionX());
          getStaticPlot()->xyLimit.push_back((atof(vstr[1].c_str()) - 1.0)*newarea.P().getGridResolutionX());
          getStaticPlot()->xyLimit.push_back((atof(vstr[2].c_str()) - 1.0)*newarea.P().getGridResolutionY());
          getStaticPlot()->xyLimit.push_back((atof(vstr[3].c_str()) - 1.0)*newarea.P().getGridResolutionY());
          METLIBS_LOG_WARN("WARNING: using obsolete syntax xylimit");
          METLIBS_LOG_WARN("New syntax:");
          METLIBS_LOG_WARN("AREA "<<newarea.P()<<" rectangle="<<getStaticPlot()->xyLimit[0]<<":"<<getStaticPlot()->xyLimit[1]<<":"<<getStaticPlot()->xyLimit[2]<<":"<<getStaticPlot()->xyLimit[3]);

          if (getStaticPlot()->xyLimit[0]>=getStaticPlot()->xyLimit[1] || getStaticPlot()->xyLimit[2]>=getStaticPlot()->xyLimit[3])
            getStaticPlot()->xyLimit.clear();
        }
      } else if (miutil::to_upper(stokens[0])=="XYPART") {//obsolete
        vector<std::string> vstr= miutil::split(stokens[1], 0, ",");
        if (vstr.size()>=4) {
          getStaticPlot()->xyPart.clear();
          for (int j=0; j<4; j++)
            getStaticPlot()->xyPart.push_back(atof(vstr[j].c_str()) * 0.01);
          if (getStaticPlot()->xyPart[0]>=getStaticPlot()->xyPart[1] || getStaticPlot()->xyPart[2]>=getStaticPlot()->xyPart[3])
            getStaticPlot()->xyPart.clear();
        }
      }
    }
  }

  bool equal= (tmpinfo.name == mapinfo.name);

  if (ifequal && !equal) // check if essential mapinfo remains the same
    return false;

  mapinfo= tmpinfo;

  //Background colour
  if ((not bgcolourname.empty())) {
    getStaticPlot()->setBgColour(bgcolourname); // static Plot member
    //just background colour, no map. No reason to make MapPlot object
    if ( n==2 ) {
      return false;
    }
  }

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
    set<std::string>::iterator uend= usedFilledmaps.end();
    map<std::string,FilledMap>::iterator p= filledmaps.begin();
    vector<std::string> erasemaps;
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
void MapPlot::plot(PlotOrder porder)
{
  METLIBS_LOG_SCOPE();

  int zorder;
  if (porder == BACKGROUND)
    zorder = 0;
  else if (porder == LINES_BACKGROUND)
    zorder = 1;
  else if (porder == OVERLAY)
    zorder = 2;
  else
    return;

  if (!isEnabled() || !isactive[zorder])
    return;

  if (getStaticPlot()->isPanning()) {
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
  } else if ((getStaticPlot()->getDirty() && !getStaticPlot()->isPanning()) || mapchanged || !glIsList(drawlist[zorder])) {
    // Making new map drawlist for this zorder
    makelist=true;
    if (glIsList(drawlist[zorder]))
      glDeleteLists(drawlist[zorder], 1);
    drawlist[zorder] = glGenLists(1);
    if (drawlist[zorder] != 0) {
      glNewList(drawlist[zorder], GL_COMPILE_AND_EXECUTE);
    } else {
      METLIBS_LOG_WARN("Unable to create new displaylist, glGenLists(1) returns 0");
      makelist = false;
    }

    makenew= true;

    if (mapchanged) {
      if ((zorder==2) || (zorder==1 && !isactive[2]) || (zorder==0
          && !isactive[1] && !isactive[2]))
        mapchanged= false;
    }
    // make new plot anyway during panning
  } else if (getStaticPlot()->getDirty()) { // && mapinfo.type!="triangles"
    makenew= true;
  }

  if (makenew) {
    std::string mapfile;
    // diagonal in pixels
    float physdiag= sqrt(getStaticPlot()->getPhysWidth()*getStaticPlot()->getPhysWidth()+getStaticPlot()->getPhysHeight()*getStaticPlot()->getPhysHeight());
    // map resolution i km/pixel
    float mapres= (physdiag > 0.0 ? getStaticPlot()->getGcd()/(physdiag*1000) : 0.0);

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
    if (c==getStaticPlot()->getBackgroundColour())
      c= getStaticPlot()->getBackContrastColour();

    if (mapinfo.type=="normal" || mapinfo.type=="pland") {
      // check contours
      if (mapinfo.contour.ison && mapinfo.contour.zorder==zorder) {
        float xylim[4]= { getStaticPlot()->getMapSize().x1, getStaticPlot()->getMapSize().x2, getStaticPlot()->getMapSize().y1, getStaticPlot()->getMapSize().y2 };
        if (!plotMapLand4(mapfile, xylim, contopts.linetype,
            contopts.linewidth, c))
          METLIBS_LOG_ERROR("ERROR OPEN/READ " << mapfile);
      }

    } else if (mapinfo.type=="triangles") {
      bool land= mapinfo.land.ison && mapinfo.land.zorder==zorder;
      bool cont= mapinfo.contour.ison && mapinfo.contour.zorder==zorder;

      if (land || cont) {
        if (filledmaps.count(mapfile)==0) {
          filledmaps[mapfile]= FilledMap(mapfile);
        }
        Area fullarea(getStaticPlot()->getMapArea().P(), getStaticPlot()->getPlotSize());
        filledmaps[mapfile].plot(fullarea, getStaticPlot()->getMapSize(), getStaticPlot()->getGcd(), land, cont, !cont
            && mapinfo.contour.ison, contopts.linetype.bmap,
            contopts.linewidth, c.RGBA(), landopts.fillcolour.RGBA(),
            getStaticPlot()->getBackgroundColour().RGBA());
      }

    } else if (mapinfo.type=="lines_simple_text") {
      // check contours
      if (mapinfo.contour.ison && mapinfo.contour.zorder==zorder) {
        if (!plotLinesSimpleText(mapfile))
          METLIBS_LOG_ERROR("ERROR OPEN/READ " << mapfile);
      }

    } else if (mapinfo.type=="shape") {
      bool land= mapinfo.land.ison && mapinfo.land.zorder==zorder;
      bool cont= mapinfo.contour.ison && mapinfo.contour.zorder==zorder;

      Colour c= contopts.linecolour;

      if (shapemaps.count(mapfile) == 0) {
#ifdef DEBUGPRINT
        METLIBS_LOG_DEBUG("Creating new shapeObject for map: " << mapfile);
#endif
        shapemaps[mapfile] = ShapeObject();
        shapemaps[mapfile].read(mapfile,true);
        shapeareas[mapfile] = Area(getStaticPlot()->getMapArea());
      }
      if (shapeareas[mapfile].P() != getStaticPlot()->getMapArea().P()) {
#ifdef DEBUGPRINT
        METLIBS_LOG_DEBUG("Projection wrong for: " << mapfile);
#endif
        bool success = shapemaps[mapfile].changeProj(shapeareas[mapfile]);

        // Reread file if unsuccessful
        if(!success) {
          shapemaps[mapfile] = ShapeObject();
          shapemaps[mapfile].read(mapfile,true);
        }
        shapeareas[mapfile] = Area(getStaticPlot()->getMapArea());
      }
      Area fullarea(getStaticPlot()->getMapArea().P(), getStaticPlot()->getPlotSize());
      shapemaps[mapfile].plot(fullarea, getStaticPlot()->getGcd(), land, cont, !cont && mapinfo.contour.ison,
          mapinfo.special,mapinfo.symbol,mapinfo.dbfcol,
          contopts.linetype.bmap, contopts.linewidth, c.RGBA(),
          landopts.fillcolour.RGBA(), getStaticPlot()->getBackgroundColour().RGBA());
    } else {
      METLIBS_LOG_WARN("Unknown maptype for map " << mapinfo.name << " = "
          << mapinfo.type);
    }

    if (makelist)
      glEndList();
    getStaticPlot()->UpdateOutput();

  } else {
    // execute old display list
    if (glIsList(drawlist[zorder]))
      glCallList(drawlist[zorder]);
    getStaticPlot()->UpdateOutput();
  }

  // check latlon
  bool plot_lon = mapinfo.lon.ison && mapinfo.lon.zorder == zorder;
  bool plot_lat = mapinfo.lat.ison && mapinfo.lat.zorder == zorder;
  if (plot_lon || plot_lat) {

    if (mapinfo.lon.showvalue || mapinfo.lat.showvalue)
      getStaticPlot()->getFontPack()->setFont("BITMAPFONT");

    plotGeoGrid(mapinfo, plot_lon, plot_lat);
  }

  // plot frame
  if (frameok && mapinfo.frame.ison && mapinfo.frame.zorder==zorder) {
    //    METLIBS_LOG_DEBUG("Plotting frame for layer:" << zorder);
    Rectangle reqr= reqarea.R();
    Colour c= ffopts.linecolour;
    if (c==getStaticPlot()->getBackgroundColour())
      c= getStaticPlot()->getBackContrastColour();
    glColor4ubv(c.RGBA());
    //       glColor3f(0.0,0.0,0.0);
    glLineWidth(ffopts.linewidth);
    if (ffopts.linetype.stipple) {
      glLineStipple(1, ffopts.linetype.bmap);
      glEnable(GL_LINE_STIPPLE);
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    if (reqarea.P()==getStaticPlot()->getMapArea().P()) {
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
      getStaticPlot()->ProjToMap(reqarea.P(), npos, x, y);

      glBegin(GL_LINE_LOOP);
      for (int i=0; i<npos; i++) {
        glVertex2f(x[i], y[i]);
      }
      glEnd();

      delete[] x;
      delete[] y;
    }
    getStaticPlot()->UpdateOutput();
  }

  getStaticPlot()->UpdateOutput();
  glDisable(GL_LINE_STIPPLE);
}


bool MapPlot::plotMapLand4(const std::string& filename, float xylim[],
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

  Projection geoProj;
  geoProj.setGeographic();
  const Projection& projection = getStaticPlot()->getMapArea().P();

  const unsigned int nwrec = 1024;
  const unsigned int maxpos = 2000;
  const int mlevel1 = 36 * 18;
  const int mlevel2 = 10 * 10;

  short indata[nwrec];
  float x[maxpos], y[maxpos];

  unsigned int nw, npi, n, npp, npos;
  int np, irec, jrec, nd, err, nwdesc, version;
  int iscale2 = 1, nlevel1 = 0, nlevel2 = 0, i, j;
  int n1, n2, nn, nwx1, nwx2, nlines, nl;

  float scale, slat2, slon2, x1, y1, x2, y2, dx, dy;
  float glon, glat, glonmin, glonmax, glatmin, glatmax, reflon, reflat;

  float box1[4], box2[4];

  short int ilevel1[mlevel1][5];
  short int ilevel2[mlevel2][5];

  float jumplimit = projection.getMapLinesJumpLimit();

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

  //  METLIBS_LOG_DEBUG("plotMapLand4 file=" << filename << " version=" << version);

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

  bool illegal_southpole = !projection.isLegal(0.0, -90.0);
  bool illegal_northpole = !projection.isLegal(0.0, 90.0);

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
          int b = projection.convertFromGeographic(np,x,y,geoProj);
          if (b!=0){
            METLIBS_LOG_WARN("plotMapLand4(0), getPoints returned false");
          }

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
    int b = projection.convertToGeographic(nn,x,y,geoProj);
    if (b!=0){
      METLIBS_LOG_WARN("plotMapLand4(1), getPoints returned false");
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

    //projection.adjustGeographicExtension(glonmin,glonmax,glatmin,glatmax);

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
                  int b = projection.convertFromGeographic(np,x,y, geoProj);
                  if (b!=0){
                    METLIBS_LOG_WARN("plotMapLand4(2), getPoints returned false");
                  }
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
  diutil::MapValuePosition lon_valuepos = diutil::mapValuePositionFromText(mapinfo.lon.value_pos);
  float lon_fontsize = mapinfo.lon.fontsize;

  float latitudeStep = mapinfo.lat.density;
  bool lat_values = mapinfo.lat.showvalue;
  diutil::MapValuePosition lat_valuepos = diutil::mapValuePositionFromText(mapinfo.lat.value_pos);
  float lat_fontsize = mapinfo.lat.fontsize;

  /*
  METLIBS_LOG_DEBUG((lon_values ? "lon_values=ON" : "lon_values=OFF") << " "
  << (lat_values ? "lat_values=ON" : "lat_values=OFF") << " lon_valuepos="
  << lon_valuepos << " lat_valuepos=" << lat_valuepos);
   */


  const Projection& p = getStaticPlot()->getMapArea().P();
  if (!p.isDefined()) {
    METLIBS_LOG_ERROR("MapPlot::plotGeoGrid ERROR: undefined projection");
    return false;
  }

  if (latitudeStep<0.001 || longitudeStep<0.001) {
    METLIBS_LOG_ERROR("MapPlot::plotGeoGrid ERROR: latitude/longitude step");
    return false;
  }

  if (latitudeStep>180.)
    latitudeStep= 180.;
  if (longitudeStep>180.)
    longitudeStep= 180.;
  if (plotResolution<1)
    plotResolution= 100;


  float xylim[4]= { getStaticPlot()->getMapSize().x1, getStaticPlot()->getMapSize().x2, getStaticPlot()->getMapSize().y1, getStaticPlot()->getMapSize().y2 };

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
  //METLIBS_LOG_DEBUG("longitudeStep,latitudeStep:  "<<longitudeStep<<" "<<latitudeStep);
  //METLIBS_LOG_DEBUG("ilon1,ilon2,ilat1,ilat2:     "<<ilon1<<" "<<ilon2<<" "<<ilat1<<" "<<ilat2);
  //METLIBS_LOG_DEBUG("glon1,glon2,glat1,glat2:     "<<glon1<<" "<<glon2<<" "<<glat1<<" "<<glat2);
  //METLIBS_LOG_DEBUG("lonmin,lonmax,latmin,latmax: "<<lonmin<<" "<<lonmax<<" "<<latmin<<" "<<latmax);
  //METLIBS_LOG_DEBUG("maprect x1,x2,y1,y2:         "<<xylim[0]<<" "<<xylim[1]<<" "<<xylim[2]<<" "<<xylim[3]);
  //########################################################################

  n= (ilat2-ilat1+1)*(ilon2-ilon1+1);
  if (n>1200) {
    float reduction= float(n)/1200.;
    n= int(float(plotResolution)/reduction + 0.5);
    if (n<2)
      n=2;
    //########################################################################
    //METLIBS_LOG_DEBUG("geoGrid: plotResolution,n: "<<plotResolution<<" "<<n);
    //########################################################################
    if (plotResolution>n)
      plotResolution= n;
  }

  bool geo2xyError= false;


  // draw longitude lines.....................................

  if (plot_lon && ilon1<=ilon2) {

    Colour c= lonopts.linecolour;
    if (c==getStaticPlot()->getBackgroundColour())
      c= getStaticPlot()->getBackContrastColour();
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
      METLIBS_LOG_ERROR("** MapPlot::plotGeoGrid ERROR in Curved longitude lines, nlat="
      << nlat);
    else {
      float *x = new float[nlat];
      float *y = new float[nlat];
      for (int ilon = ilon1; ilon <= ilon2; ilon++) {
        glon = longitudeStep * float(ilon);
        ostringstream ost;
        ost << fabsf(glon) << " " << (glon < 0 ? "W" : "E");
        std::string plotstr = ost.str();
        for (n = 0; n < nlat; n++) {
          x[n] = glon;
          y[n] = glat + dlat * float(n);
        }
        if (getStaticPlot()->GeoToMap(nlat, x, y)) {
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
  getStaticPlot()->UpdateOutput();
  glDisable(GL_LINE_STIPPLE);

  if (value_annotations.size() > 0) {
    getStaticPlot()->getFontPack()->setFontSize(lon_fontsize);
    n = value_annotations.size();
    Rectangle prevr;
    for (j = 0; j < n; j++) {
      float x = value_annotations[j].x;
      float y = value_annotations[j].y;
      if (prevr.isinside(x,y)){
        continue;
      }
      getStaticPlot()->getFontPack()->drawStr(value_annotations[j].t.c_str(), x, y, 0);
      float w,h;
      getStaticPlot()->getFontPack()->getStringSize(value_annotations[j].t.c_str(),w,h);
      prevr = Rectangle(x,y,x+w,y+h);
    }
    value_annotations.clear();
    getStaticPlot()->UpdateOutput();
  }

  // draw latitude lines......................................

  if (plot_lat && ilat1<=ilat2) {
    Colour c= latopts.linecolour;
    if (c==getStaticPlot()->getBackgroundColour())
      c= getStaticPlot()->getBackContrastColour();
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
      METLIBS_LOG_ERROR("** MapPlot::plotGeoGrid ERROR in Curved Latitude lines, nlon="
          << nlon);
      METLIBS_LOG_ERROR("lonmin,lonmax=" << lonmin << "," << lonmax);
    } else {
      float *x = new float[nlon];
      float *y = new float[nlon];
      for (int ilat = ilat1; ilat <= ilat2; ilat++) {
        glat = latitudeStep * float(ilat);
        ostringstream ost;
        ost << fabsf(glat) << " " << (glat < 0 ? "S" : "N");
        std::string plotstr = ost.str();
        for (n = 0; n < nlon; n++) {
          x[n] = glon + dlon * float(n);
          y[n] = glat;
        }
        if (getStaticPlot()->GeoToMap(nlon, x, y)) {
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

  getStaticPlot()->UpdateOutput();
  glDisable(GL_LINE_STIPPLE);

  if (value_annotations.size() > 0) {
    getStaticPlot()->getFontPack()->setFontSize(lat_fontsize);
    n = value_annotations.size();
    Rectangle prevr;
    for (j = 0; j < n; j++) {
      float x = value_annotations[j].x;
      float y = value_annotations[j].y;
      if (prevr.isinside(x,y)){
        continue;
      }
      getStaticPlot()->getFontPack()->drawStr(value_annotations[j].t.c_str(), x, y, 0);
      float w,h;
      getStaticPlot()->getFontPack()->getStringSize(value_annotations[j].t.c_str(),w,h);
      prevr = Rectangle(x,y,x+w,y+h);
    }
    value_annotations.clear();
    getStaticPlot()->UpdateOutput();
  }

  /*  }*/

  if (geo2xyError) {
    METLIBS_LOG_ERROR("MapPlot::plotGeoGrid ERROR: gc.geo2xy failure(s)");
  }

  return true;
}

bool MapPlot::plotLinesSimpleText(const std::string& filename)
{
  // plot lines from very simple text file,
  //  each line in file: latitude(desimal,float) longitude(desimal,float)
  //  end_of_line: "----"

  ifstream file;
  file.open(filename.c_str());
  if (file.bad())
    return false;

  float xylim[4]= { getStaticPlot()->getMapSize().x1, getStaticPlot()->getMapSize().x2, getStaticPlot()->getMapSize().y1, getStaticPlot()->getMapSize().y2 };

  Colour c= contopts.linecolour;
  if (c==getStaticPlot()->getBackgroundColour())
    c= getStaticPlot()->getBackContrastColour();
  glColor4ubv(c.RGBA());
  glLineWidth(contopts.linewidth+0.1);
  if (contopts.linetype.bmap!=0xFFFF) {
    glLineStipple(1, contopts.linetype.bmap);
    glEnable(GL_LINE_STIPPLE);
  }

  const int nmax= 2000;

  float x[nmax];
  float y[nmax];

  std::string str;
  vector<std::string> coords;
  bool endfile= false;
  bool endline;
  int nlines= 0;
  int n= 0;
  float jumplimit = getStaticPlot()->getMapArea().P().getMapLinesJumpLimit();

  while (!endfile) {

    endline= false;

    while (!endline && n<nmax && getline(file, str)) {
      coords= miutil::split(str, " ", true);
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

    if (endline) {
      n--;
    } else if (n<nmax) {
      endfile= true;
    }

    if (n>1) {
      float xn= x[n-1];
      float yn= y[n-1];
      if (getStaticPlot()->GeoToMap(n, x, y)) {
        clipPrimitiveLines(n, x, y, xylim, jumplimit);
        nlines++;
      } else {
        METLIBS_LOG_ERROR("MapPlot::plotLinesSimpleText  gc.geo2xy ERROR");
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

  getStaticPlot()->UpdateOutput();

  glDisable(GL_LINE_STIPPLE);

  return (nlines>0);
}

void MapPlot::clipPrimitiveLines(int npos, float *x, float *y, float xylim[4],
    float jumplimit, bool plotanno, diutil::MapValuePosition anno_position, const std::string& anno)
{
  int i, n = 0;
  while (n < npos) {
    i = n++;
    while (n < npos
        && fabsf(x[n - 1] - x[n]) < jumplimit
        && fabsf(y[n - 1] - y[n]) < jumplimit)
    {
      n++;
    }
    if (not plotanno)
      anno_position = diutil::map_none;
    diutil::xyclip(n - i, &x[i], &y[i], xylim, anno_position, anno, value_annotations);
  }
}
