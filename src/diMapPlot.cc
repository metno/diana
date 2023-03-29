/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2023 met.no

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

#include "diMapPlot.h"

#include "diKVListPlotCommand.h"
#include "diMapManager.h"
#include "diStaticPlot.h"

#include <puTools/miStringFunctions.h>

#include <cfloat>
#include <fstream>
#include <sstream>

#define MILOGGER_CATEGORY "diana.MapPlot"
#include <miLogger/miLogging.h>

using namespace miutil;

namespace {

bool calculateGeogridParameters(const Projection& p, const Rectangle& maprect, float & lonmin,
    float & lonmax, float & latmin, float & latmax)
{
  if (!p.adjustedLatLonBoundingBox(maprect, lonmin, lonmax, latmin, latmax))
    return false;

  const float LIMIT = 89.95f;
  if (latmin < -LIMIT && !p.isLegal(0.0, -90.0))
    latmin = -LIMIT;
  if (latmax > LIMIT && !p.isLegal(0.0, 90.0))
    latmax = LIMIT;

  return true;
}

} // namespace

// static members
std::map<std::string, FilledMap> MapPlot::filledmapObjects;
std::map<std::string, int> MapPlot::filledmapRefCounts;

std::map<std::string,ShapeObject> MapPlot::shapemaps;
std::map<std::string,Area> MapPlot::shapeareas;

MapPlot::MapPlot()
  : mapchanged(true)
  , haspanned(false)
  , mCanvas(0)
{
  METLIBS_LOG_SCOPE();
  drawlist[0]=0;
  drawlist[1]=0;
  drawlist[2]=0;
  isactive[0]= false;
  isactive[1]= false;
  isactive[2]= false;
}

MapPlot::~MapPlot()
{
  METLIBS_LOG_SCOPE();
  dereferenceFilledMaps(mapinfo);
}

void MapPlot::setCanvas(DiCanvas* c)
{
  if (mCanvas && mCanvas->supportsDrawLists()) {
    for (int i=0; i<3; ++i) {
      if (mCanvas->IsList(drawlist[i]))
        mCanvas->DeleteLists(drawlist[i], 1);
      drawlist[i] = 0;
    }
  }
  mCanvas = dynamic_cast<DiGLCanvas*>(c);
}

std::string MapPlot::getEnabledStateKey() const
{
  return std::string();
}

/*
 Extract plotting-parameters from PlotInfo.
 */
bool MapPlot::prepare(const PlotCommand_cp& pc, bool ifequal)
{
  METLIBS_LOG_SCOPE();

  MapManager mapm;

  KVListPlotCommand_cp cmd = std::dynamic_pointer_cast<const KVListPlotCommand>(pc);
  if (!cmd)
    return false;

  MapInfo tmpinfo;
  for (const KeyValue& kv : cmd->all()) {
    if (kv.key() == "map") {
      mapm.getMapInfoByName(kv.value(), tmpinfo);
    } else if (kv.key() == "backcolour") {
      bgcolourname_ = kv.value();
    }
  }

  bool equal= (tmpinfo.name == mapinfo.name);

  if (ifequal && !equal) // check if essential mapinfo remains the same
    return false;

  // first reference the new, then dereference the old
  referenceFilledMaps(tmpinfo);
  dereferenceFilledMaps(mapinfo);

  mapinfo= tmpinfo;

  if (!bgcolourname_.empty() && cmd->size() == 1) {
    //just background colour, no map. No reason to make MapPlot object
    return false;
  }

  // fill in new options for mapinfo and make proper PlotOptions
  // the different map-elements
  mapm.fillMapInfo(cmd->all(), mapinfo, contopts, landopts, lonopts, latopts, ffopts);

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

// static
void MapPlot::referenceFilledMaps(const MapInfo& mi)
{
  METLIBS_LOG_SCOPE(LOGVAL(mi.type));
  if (mi.type != "triangles")
    return;

  for (size_t i=0; i<mi.mapfiles.size(); i++) {
    const std::string& fn = mi.mapfiles[i].fname;
    METLIBS_LOG_DEBUG(LOGVAL(fn));
    fmRefCounts_t::iterator it = filledmapRefCounts.find(fn);
    if (it != filledmapRefCounts.end()) {
      METLIBS_LOG_DEBUG(LOGVAL(it->second) << " += 1");
      it->second += 1;
    } else {
      METLIBS_LOG_DEBUG("insert");
      filledmapRefCounts.insert(std::make_pair(fn, 1));
      // do not insert an object into filledmapObjects yet
    }
  }
}

// static
void MapPlot::dereferenceFilledMaps(const MapInfo& mi)
{
  METLIBS_LOG_SCOPE(LOGVAL(mi.type));
  if (mi.type != "triangles")
    return;

  for (size_t i=0; i<mi.mapfiles.size(); i++) {
    const std::string& fn = mi.mapfiles[i].fname;
    METLIBS_LOG_DEBUG(LOGVAL(fn));
    fmRefCounts_t::iterator it = filledmapRefCounts.find(fn);
    if (it != filledmapRefCounts.end()) {
      METLIBS_LOG_DEBUG(LOGVAL(it->second) << " -= 1");
      it->second -= 1;
      if (it->second == 0) {
        METLIBS_LOG_DEBUG("erase '" << fn << "'");
        filledmapObjects.erase(fn);
        filledmapRefCounts.erase(fn);
      }
    } else {
      METLIBS_LOG_ERROR("decreasing refcount for filled map file '" << fn << "' wich has not been referenced");
    }
  }
}

// static
FilledMap* MapPlot::fetchFilledMap(const std::string& filename)
{
  METLIBS_LOG_SCOPE(LOGVAL(filename));
  fmObjects_t::iterator it = filledmapObjects.find(filename);
  if (it != filledmapObjects.end())
    return &(it->second);

#if 1 || defined(DEBUG_FETCHFILLEDMAP)
  if (filledmapRefCounts.find(filename) == filledmapRefCounts.end()) {
    METLIBS_LOG_ERROR("refusing to access filled map file '" << filename
        << "' wich has not been referenced");
    return 0;
  }
#endif

  METLIBS_LOG_DEBUG("insert new filledmap '" << filename << "'");
  return &(filledmapObjects.insert(std::make_pair(filename, FilledMap(filename))).first->second);
}

void MapPlot::changeProjection(const Area& /*mapArea*/, const Rectangle& /*plotSize*/, const diutil::PointI& /*physSize*/)
{
  mapchanged = true;
}

/*
 Plot one layer of the map
 */
void MapPlot::plot(DiGLPainter* gl, PlotOrder porder)
{
  METLIBS_LOG_SCOPE();

  int zorder;
  if (porder == PO_BACKGROUND)
    zorder = 0;
  else if (porder == PO_LINES_BACKGROUND)
    zorder = 1;
  else if (porder == PO_OVERLAY_TOP)
    zorder = 2;
  else
    return;
  METLIBS_LOG_DEBUG(LOGVAL(zorder));

  if (!isEnabled() || !isactive[zorder])
    return;

  if (getStaticPlot()->isPanning()) {
    haspanned = true;
  } else if (haspanned) {
    mapchanged= true;
    haspanned= false;
  }

  if (!mapinfo.name.empty())
    plotMap(gl,zorder);

  // plot latlon
  bool plot_lon = mapinfo.lon.ison && mapinfo.lon.zorder == zorder;
  bool plot_lat = mapinfo.lat.ison && mapinfo.lat.zorder == zorder;
  if (plot_lon || plot_lat) {

    if (mapinfo.lon.showvalue || mapinfo.lat.showvalue)
      gl->setFont(diutil::BITMAPFONT);

    plotGeoGrid(gl, mapinfo, plot_lon, plot_lat);
  }

  // plot frame
  bool frameok = getStaticPlot()->getRequestedarea().P().isDefined();
  if (frameok && mapinfo.frame.ison && mapinfo.frame.zorder==zorder) {
    //    METLIBS_LOG_DEBUG("Plotting frame for layer:" << zorder);
    const Rectangle& reqr = getStaticPlot()->getRequestedarea().R();
    const Colour& c = getStaticPlot()->notBackgroundColour(ffopts.linecolour);
    gl->setLineStyle(c, ffopts.linewidth, ffopts.linetype);
    gl->drawRect(false, reqr);
  }

  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);
}

void MapPlot::plotMap(DiGLPainter* gl, int zorder)
{
  bool makenew= false;
  bool makelist= false;

  if (!mCanvas || !mCanvas->supportsDrawLists()) {
    // do not use display lists: always make a new plot from scratch
    makenew = true;
  } else if (mapchanged || !mCanvas->IsList(drawlist[zorder])) {
    // Making new map drawlist for this zorder
    makelist=true;
    if (mCanvas->IsList(drawlist[zorder]))
      mCanvas->DeleteLists(drawlist[zorder], 1);
    drawlist[zorder] = mCanvas->GenLists(1);
    if (drawlist[zorder] != 0) {
      gl->NewList(drawlist[zorder], DiGLPainter::gl_COMPILE_AND_EXECUTE);
    } else {
      METLIBS_LOG_WARN("Unable to create new displaylist, gl->GenLists(1) returns 0");
      makelist = false;
    }

    makenew= true;

    if (mapchanged) {
      if ((zorder==2) || (zorder==1 && !isactive[2]) || (zorder==0 && !isactive[1] && !isactive[2]))
        mapchanged= false;
    }
    // make new plot anyway during panning
  }

  METLIBS_LOG_DEBUG(LOGVAL(mapinfo.type) << LOGVAL(makenew));
  if (makenew) {
    std::string mapfile;
    // diagonal in pixels
    const float physdiag= getStaticPlot()->getPhysDiagonal();
    // map resolution i km/pixel
    float mapres= (physdiag > 0 ? getStaticPlot()->getGcd()/(physdiag*1000) : 0);

    // find correct mapfile
    size_t n= mapinfo.mapfiles.size();
    if (n==1) {
      mapfile= mapinfo.mapfiles[0].fname;
    } else if (n>1) {
      size_t fnum= 0;
      for (; fnum<n; fnum++) {
        if (mapres > mapinfo.mapfiles[fnum].sizelimit)
          break;
      }
      if (fnum==n)
        fnum=n-1;
      mapfile= mapinfo.mapfiles[fnum].fname;
    }

    const Colour& c = getStaticPlot()->notBackgroundColour(contopts.linecolour);

    //Plot map
    if (mapinfo.type=="normal" || mapinfo.type=="pland") {
      // check contours
      if (mapinfo.contour.ison && mapinfo.contour.zorder==zorder) {
        const Rectangle& ms = getStaticPlot()->getMapSize();
        const float xylim[4]= { ms.x1, ms.x2, ms.y1, ms.y2 };
        if (!plotMapLand4(gl, mapfile, xylim, contopts.linetype, contopts.linewidth, c))
          METLIBS_LOG_ERROR("ERROR OPEN/READ " << mapfile);
      }

    } else if (mapinfo.type=="triangles") {
      bool land= mapinfo.land.ison && mapinfo.land.zorder==zorder;
      bool cont= mapinfo.contour.ison && mapinfo.contour.zorder==zorder;

      if (land || cont) {
        if (FilledMap* fm = fetchFilledMap(mapfile)) {
          Area fullarea(getStaticPlot()->getMapArea().P(), getStaticPlot()->getPlotSize());
          fm->plot(gl, fullarea, getStaticPlot()->getMapSize(), getStaticPlot()->getGcd(), land, cont, !cont && mapinfo.contour.ison, contopts.linetype.bmap,
                   contopts.linewidth, c, landopts.fillcolour, getStaticPlot()->getBackgroundColour());
        }
      }

    } else if (mapinfo.type=="lines_simple_text") {
      // check contours
      if (mapinfo.contour.ison && mapinfo.contour.zorder==zorder) {
        if (!plotLinesSimpleText(gl, mapfile))
          METLIBS_LOG_ERROR("ERROR OPEN/READ " << mapfile);
      }

    } else if (mapinfo.type=="shape") {
      METLIBS_LOG_DEBUG(LOGVAL(mapinfo.land.ison) << LOGVAL(mapinfo.land.zorder)
          << LOGVAL(mapinfo.contour.ison) << LOGVAL(mapinfo.contour.zorder));
      const bool land= mapinfo.land.ison && mapinfo.land.zorder==zorder;
      const bool cont= mapinfo.contour.ison && mapinfo.contour.zorder==zorder;

      if (shapemaps.count(mapfile) == 0) {
        METLIBS_LOG_DEBUG("Creating new shapeObject for map: " << mapfile);
        shapemaps[mapfile] = ShapeObject();
        shapemaps[mapfile].read(mapfile);
        shapeareas[mapfile] = getStaticPlot()->getMapArea();
      }
      if (shapeareas[mapfile].P() != getStaticPlot()->getMapArea().P()) {
        METLIBS_LOG_DEBUG("reprojecting shape map from file '" << mapfile << "'");
        if (!shapemaps[mapfile].changeProj()) {
          shapemaps[mapfile] = ShapeObject();
          shapemaps[mapfile].read(mapfile);
        }
        shapeareas[mapfile] = getStaticPlot()->getMapArea();
      }
      METLIBS_LOG_DEBUG("shape plot");
      const Area fullarea(getStaticPlot()->getMapArea().P(), getStaticPlot()->getPlotSize());
      shapemaps[mapfile].plot(gl, fullarea, getStaticPlot()->getGcd(), land, cont,
          mapinfo.special, mapinfo.symbol,
          contopts.linetype, contopts.linewidth, contopts.linecolour,
          landopts.fillcolour, getStaticPlot()->getBackgroundColour());
    } else {
      METLIBS_LOG_WARN("Unknown maptype for map " << mapinfo.name << " = "
          << mapinfo.type);
    }

    if (makelist)
      gl->EndList();

  } else {
    // execute old display list
    if (mCanvas->IsList(drawlist[zorder]))
      gl->CallList(drawlist[zorder]);
  }

}

bool MapPlot::plotMapLand4(DiGLPainter* gl, const std::string& filename, const float xylim[],
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
  gl->setLineStyle(colour, linewidth, linetype);

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
          if (!projection.convertFromGeographic(np,x,y)) {
            METLIBS_LOG_WARN("plotMapLand4(0), getPoints returned false");
          }

          clipPrimitiveLines(gl, np,x,y,xylim,jumplimit);
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
    if (!projection.convertToGeographic(nn,x,y)) {
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
                  if (!projection.convertFromGeographic(np,x,y)) {
                    METLIBS_LOG_WARN("plotMapLand4(2), getPoints returned false");
                  }
                  clipPrimitiveLines(gl, np, x, y, xylim, jumplimit);

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

  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);
  fclose(pfile);

  return true;
}



bool MapPlot::plotGeoGrid(DiGLPainter* gl, const MapInfo& mapinfo, bool plot_lon, bool plot_lat, int plotResolution)
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

  const float jumplimit = p.getMapLinesJumpLimit();
  float lonmin=FLT_MAX, lonmax=-FLT_MAX, latmin=FLT_MAX, latmax=-FLT_MAX;
  if (!calculateGeogridParameters(p, getStaticPlot()->getMapSize(), lonmin, lonmax, latmin, latmax))
    return false;

  int n, j;
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

  auto t_geo2map = p.transformationFrom(Projection::geographic());

  // draw longitude lines.....................................

  if (plot_lon && ilon1<=ilon2) {

    const Colour& c = getStaticPlot()->notBackgroundColour(lonopts.linecolour);
    gl->setLineStyle(c, lonopts.linewidth, lonopts.linetype);

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
      std::unique_ptr<float[]> x(new float[nlat]);
      std::unique_ptr<float[]> y(new float[nlat]);
      for (int ilon = ilon1; ilon <= ilon2; ilon++) {
        glon = longitudeStep * float(ilon);
        std::ostringstream ost;
        ost << fabsf(glon) << " " << (glon < 0 ? "W" : "E");
        std::string plotstr = ost.str();
        for (n = 0; n < nlat; n++) {
          x[n] = glon;
          y[n] = glat + dlat * float(n);
        }
        if (t_geo2map->forward(nlat, x.get(), y.get())) {
          const Rectangle& ms = getStaticPlot()->getMapSize();
          const float xylim[4]= { ms.x1, ms.x2, ms.y1, ms.y2 };
          clipPrimitiveLines(gl, nlat, x.get(), y.get(), xylim, jumplimit, lon_values, lon_valuepos, plotstr);
        } else {
          geo2xyError = true;
        }
      }
    }
  }
  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);

  if (value_annotations.size() > 0) {
    gl->setFontSize(lon_fontsize);
    n = value_annotations.size();
    Rectangle prevr;
    for (j = 0; j < n; j++) {
      float x = value_annotations[j].x;
      float y = value_annotations[j].y;
      if (prevr.isinside(x,y)){
        continue;
      }
      gl->drawText(value_annotations[j].t, x, y, 0);
      float w,h;
      gl->getTextSize(value_annotations[j].t,w,h);
      prevr = Rectangle(x,y,x+w,y+h);
    }
    value_annotations.clear();
  }

  // draw latitude lines......................................

  if (plot_lat && ilat1<=ilat2) {
    const Colour& c = getStaticPlot()->notBackgroundColour(latopts.linecolour);
    gl->setLineStyle(c, latopts.linewidth, latopts.linetype);

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
      std::unique_ptr<float[]> x(new float[nlon]);
      std::unique_ptr<float[]> y(new float[nlon]);
      for (int ilat = ilat1; ilat <= ilat2; ilat++) {
        glat = latitudeStep * float(ilat);
        std::ostringstream ost;
        ost << fabsf(glat) << " " << (glat < 0 ? "S" : "N");
        std::string plotstr = ost.str();
        for (n = 0; n < nlon; n++) {
          x[n] = glon + dlon * float(n);
          y[n] = glat;
        }
        if (t_geo2map->forward(nlon, x.get(), y.get())) {
          const Rectangle& ms = getStaticPlot()->getMapSize();
          const float xylim[4]= { ms.x1, ms.x2, ms.y1, ms.y2 };
          clipPrimitiveLines(gl, nlon, x.get(), y.get(), xylim, jumplimit, lat_values, lat_valuepos, plotstr);
        } else {
          geo2xyError = true;
        }
      }
    }
  }

  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);

  if (value_annotations.size() > 0) {
    gl->setFontSize(lat_fontsize);
    n = value_annotations.size();
    Rectangle prevr;
    for (j = 0; j < n; j++) {
      float x = value_annotations[j].x;
      float y = value_annotations[j].y;
      if (prevr.isinside(x,y)){
        continue;
      }
      gl->drawText(value_annotations[j].t, x, y, 0);
      float w,h;
      gl->getTextSize(value_annotations[j].t,w,h);
      prevr = Rectangle(x,y,x+w,y+h);
    }
    value_annotations.clear();
  }

  /*  }*/

  if (geo2xyError) {
    METLIBS_LOG_ERROR("MapPlot::plotGeoGrid ERROR: gc.geo2xy failure(s)");
  }

  return true;
}

bool MapPlot::plotLinesSimpleText(DiGLPainter* gl, const std::string& filename)
{
  // plot lines from very simple text file,
  //  each line in file: latitude(desimal,float) longitude(desimal,float)
  //  end_of_line: "----"

  std::ifstream file;
  file.open(filename.c_str());
  if (file.bad())
    return false;

  float xylim[4]= { getStaticPlot()->getMapSize().x1, getStaticPlot()->getMapSize().x2,
                    getStaticPlot()->getMapSize().y1, getStaticPlot()->getMapSize().y2 };

  const Colour& c = getStaticPlot()->notBackgroundColour(contopts.linecolour);
  gl->setLineStyle(c, contopts.linewidth, contopts.linetype);

  const int nmax= 2000;

  float x[nmax];
  float y[nmax];

  std::string str;
  std::vector<std::string> coords;
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
        clipPrimitiveLines(gl, n, x, y, xylim, jumplimit);
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

  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);

  return (nlines>0);
}

void MapPlot::clipPrimitiveLines(DiGLPainter* gl, int npos, float *x, float *y, const float xylim[4],
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
    diutil::xyclip(n - i, &x[i], &y[i], xylim, anno_position, anno, value_annotations, gl);
  }
}
