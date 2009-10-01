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
#include <fstream>
#include <iostream>
#include <diFilledMap.h>
#include <stdio.h>

#include <math.h>
#include <values.h>
#include <sys/types.h>
#include <sys/stat.h>

#define DEG_TO_RAD  .0174532925199432958

/* Created at Wed Aug  8 09:54:14 2001 */

const float bignum = FLT_MAX;

const int nwrec = 1024; // recordsize in 2-byte integers

struct map_boundbox {
  float east;
  float west;
  float north;
  float south;
  float size()
  {
    return ((east - west) * (north - south));
  }
};

inline void int16to32(const short a, const short b, int& i)
{
  i = int((a << 16) | (b & 65535));
}

FilledMap::FilledMap() :
  filename(""), timestamp(0), scale(1.0), tscale(1.0), numGroups(0), groups(0),
  numPolytiles(0), polydata(0), opened(false), contexist(false)
{
}

FilledMap::FilledMap(const miString fn) :
  filename(fn), timestamp(0), scale(1.0), tscale(1.0), numGroups(0), groups(0),
  numPolytiles(0), polydata(0), opened(false), contexist(false)
{
}

FilledMap::~FilledMap()
{
  clearPolys();
  if (polydata)
    delete[] polydata;
  polydata = 0;
  clearGroups();
}

void FilledMap::clearGroups()
{
  for (int i = 0; i < numGroups; i++) {
    if (groups[i].numtiles > 0) {
      if (groups[i].tilex)
        delete[] groups[i].tilex;
      if (groups[i].tiley)
        delete[] groups[i].tiley;
      if (groups[i].midlat)
        delete[] groups[i].midlat;
      if (groups[i].midlon)
        delete[] groups[i].midlon;
      if (groups[i].mmx)
        delete[] groups[i].mmx;
      if (groups[i].mmy)
        delete[] groups[i].mmy;
      if (groups[i].crecnr)
        delete[] groups[i].crecnr;
      if (groups[i].cwp)
        delete[] groups[i].cwp;
      if (groups[i].use)
        delete[] groups[i].use;
      if (groups[i].tiletype)
        delete[] groups[i].tiletype;
      groups[i].numtiles = 0;
    }
  }
  numGroups = 0;
  if (groups)
    delete[] groups;
  groups = 0;
}

void FilledMap::clearPolys()
{
  for (int i = 0; i < numPolytiles; i++) {
    if (polydata[i].polyverx)
      delete[] polydata[i].polyverx;
    if (polydata[i].polyvery)
      delete[] polydata[i].polyvery;
    polydata[i].polysize.clear();
    polydata[i].np = 0;
  }
  numPolytiles = 0;
  contexist = false;
}

long FilledMap::gettimestamp()
{
  struct stat buf;
  const char *path = filename.cStr();
  if (!stat(path, &buf)) {
    return long(buf.st_ctime);
  }
  return long(0);
}

bool FilledMap::readheader()
{
  FILE *pfile;
  if ((pfile = fopen(filename.cStr(), "rb")) == NULL) {
    cerr << "Could not open file for read:" << filename << endl;
    return false;
  }

  // get timestamp
  timestamp = gettimestamp();

  short iscale1, iscale2, itscale; // scales
  short indata[nwrec]; // a record

  short recnr = 0;
  short wp = 0;
  int nwr;

  nwr = fread(indata, 2, nwrec, pfile);
  if (nwr != nwrec) {
    fclose(pfile);
    cerr << "Reading error 1" << endl;
    fclose(pfile);
    return false;
  }

  // clean up first
  clearGroups();
  clearPolys();
  if (polydata)
    delete[] polydata;
  polydata = 0;

  // check header
  iscale1 = indata[3];
  iscale2 = indata[4];
  itscale = indata[5];
  scale = 1.0 / (iscale1 * pow(float(10), iscale2));
  tscale = 1.0 / pow(double(10), itscale);

  numGroups = indata[6];
  groups = new tile_group[numGroups];

  wp = 7;

  for (int k = 0; k < numGroups; k++) {
    if (wp + 4 >= nwrec) {
      nwr = fread(indata, 2, nwrec, pfile);
      if (nwr != nwrec) {
        fclose(pfile);
        cerr << "Reading error 1.5" << endl;
        fclose(pfile);
        return false;
      }
      recnr++;
      wp = 0;
    }
    int numt = indata[wp + 0];
    groups[k].numtiles = numt;
    groups[k].tilex = new float[(numt + 1) * 4];
    groups[k].tiley = new float[(numt + 1) * 4];
    groups[k].mmx = new float[(numt + 1) * 2];
    groups[k].mmy = new float[(numt + 1) * 2];
    groups[k].midlat = new float[numt];
    groups[k].midlon = new float[numt];
    groups[k].crecnr = new int[numt];
    groups[k].cwp = new int[numt];
    groups[k].use = new bool[numt];
    groups[k].tiletype = new int[numt];

    int gidx = numt * 4;

    //     groups[k].tilex[gidx+0]= indata[wp+1]*tscale; // south-west
    //     groups[k].tiley[gidx+0]= indata[wp+3]*tscale;
    //     groups[k].tilex[gidx+1]= indata[wp+2]*tscale; // north-east
    //     groups[k].tiley[gidx+1]= indata[wp+4]*tscale;
    //     groups[k].tilex[gidx+2]= groups[k].tilex[gidx+0]; // north-west
    //     groups[k].tiley[gidx+2]= groups[k].tiley[gidx+1];
    //     groups[k].tilex[gidx+3]= groups[k].tilex[gidx+1]; // south-east
    //     groups[k].tiley[gidx+3]= groups[k].tiley[gidx+0];

    //     cerr << "SouthWest(" << groups[k].tiley[gidx+0] << ","
    // 	 << groups[k].tilex[gidx+0]
    // 	 << ") NorthEast(" << groups[k].tiley[gidx+1] << ","
    // 	 << groups[k].tilex[gidx+1]
    // 	 << ") NorthWest(" << groups[k].tiley[gidx+2] << ","
    // 	 << groups[k].tilex[gidx+2]
    // 	 << ") SouthEast(" << groups[k].tiley[gidx+3] << ","
    // 	 << groups[k].tilex[gidx+3] << ")" << endl;

    float minx = bignum, miny = bignum, maxx = -bignum, maxy = -bignum;

    wp += 5;

    int bidx = 0;
    for (int i = 0; i < groups[k].numtiles; i++, bidx += 4) {
      numPolytiles++;
      if (wp + 5 >= nwrec) {
        nwr = fread(indata, 2, nwrec, pfile);
        if (nwr != nwrec) {
          fclose(pfile);
          cerr << "Reading error 2" << endl;
          fclose(pfile);
          return false;
        }
        recnr++;
        wp = 0;
      }

      groups[k].crecnr[i] = indata[wp + 0];
      groups[k].cwp[i] = indata[wp + 1];

      float west = indata[wp + 2] * tscale;
      float east = indata[wp + 3] * tscale;
      float south = indata[wp + 4] * tscale;
      float north = indata[wp + 5] * tscale;

      // mark tiles near the poles
      if (south < -70) // 70
        groups[k].tiletype[i] = 2;
      else if (north > 85) // 70
        groups[k].tiletype[i] = 1;
      else
        groups[k].tiletype[i] = 0;

      if (west < minx)
        minx = west;
      if (east > maxx)
        maxx = east;
      if (south < miny)
        miny = south;
      if (north > maxy)
        maxy = north;

      groups[k].tilex[bidx + 0] = west; // south-west
      groups[k].tiley[bidx + 0] = south;
      groups[k].tilex[bidx + 1] = east; // north-east
      groups[k].tiley[bidx + 1] = north;
      groups[k].tilex[bidx + 2] = west; // north-west
      groups[k].tiley[bidx + 2] = north;
      groups[k].tilex[bidx + 3] = east; // south-east
      groups[k].tiley[bidx + 3] = south;

      groups[k].midlon[i] = (east + west) / 2.0;
      groups[k].midlat[i] = (north + south) / 2.0;
      wp += 6;
    } // tile loop

    groups[k].tilex[gidx + 0] = minx; // south-west
    groups[k].tiley[gidx + 0] = miny;
    groups[k].tilex[gidx + 1] = maxx; // north-east
    groups[k].tiley[gidx + 1] = maxy;
    groups[k].tilex[gidx + 2] = minx; // north-west
    groups[k].tiley[gidx + 2] = maxy;
    groups[k].tilex[gidx + 3] = maxx; // south-east
    groups[k].tiley[gidx + 3] = miny;

  } // group loop

  if (numPolytiles > 0) {
    polydata = new tile_data[numPolytiles];
  }
  numPolytiles = 0;

  fclose(pfile);
  return true;
}

bool FilledMap::plot(Area area, // current area
    Rectangle maprect, // the visible rectangle
    double gcd, // size of plotarea in m
    bool land, // plot triangles
    bool cont, // plot contour-lines
    bool keepcont, // keep contourlines for later
    GLushort linetype, // contour line type
    float linewidth, // contour linewidth
    const uchar_t* lcolour, // contour linecolour
    const uchar_t* fcolour, // triangles fill colour
    const uchar_t* bcolour)
{ // background color


  bool startfresh = false;

  float xylim[4]= { maprect.x1, maprect.x2, maprect.y1, maprect.y2 };
  float jumplimit = area.P().getMapLinesJumpLimit();

  // check if mapfile has been altered since header was read
  bool filechanged = false;
  if (opened) {
    long ctimestamp = gettimestamp();
    filechanged = (ctimestamp != timestamp);
    if (filechanged) {
      cerr << "MAPFile changed on disk:" << filename << " Current timestamp:"
          << ctimestamp << " Old timestamp:" << timestamp << endl;
    }
  }

  startfresh = (!opened || filechanged);

  if (startfresh) {
    if (!readheader()) {
      cerr << "Returned false" << endl;
      return false;
    }
    opened = true;
  }

  //cerr << "FilledMap::plot():" << filename << endl;
  glLineWidth(linewidth);
  if (linetype != 0xFFFF) {
    glLineStipple(1, linetype);
    glEnable(GL_LINE_STIPPLE);
  } else {
    glDisable(GL_LINE_STIPPLE);
  }

  if (cont && contexist) {
    glColor4ubv(lcolour);
    for (int psize = 0; psize < numPolytiles; psize++) {
      int numpo = polydata[psize].polysize.size();
      int id1 = 0, id2;
      for (int ipp = 0; ipp < numpo; ipp++) {
        id2 = id1 + polydata[psize].polysize[ipp];

        clipPrimitiveLines(id1, id2-1, polydata[psize].polyverx,
            polydata[psize].polyvery,xylim,jumplimit);
/*
        glBegin(GL_LINE_STRIP);
        for (int iv = id1; iv < id2; iv++) {
          glVertex2f(polydata[psize].polyverx[iv], polydata[psize].polyvery[iv]);
        }
        glEnd();
*/
        id1 = id2;
      }
    }
    clearPolys();
    return true;
  }

  FILE *pfile;
  if ((pfile = fopen(filename.cStr(), "rb")) == NULL) {
    cerr << "Could not open file for read:" << filename << endl;
    return false;
  }

  if (!startfresh)
    clearPolys();

  int nwr;
  short recnr = -1;
  short wp = 0;
  short indata[nwrec]; // a record

  float geomin; // minimum size of polygon in geo degrees
  geomin = gcd / 20000000;
  geomin = geomin * geomin;

  Projection srcProj;
  srcProj.setGeographic();

  //Projection srcProj("+proj=lonlat +ellps=WGS84 +datum=WGS84", DEG_TO_RAD, DEG_TO_RAD);
  //Projection srcProj("+proj=lonlat +a=6371000.0 +b=6371000.0",DEG_TO_RAD,DEG_TO_RAD);
  //Projection srcProj("+proj=lonlat +ellps=WGS84 ",DEG_TO_RAD,DEG_TO_RAD);
  //+to_meter=.0174532925199432958


  if (area.P() != proj || startfresh) {
    bool cutsouth = false;//!area.P().isLegal(0.0,-90.0);
    bool cutnorth = false;//!area.P().isLegal(0.0,90.0);
    area.P().filledMapCutparameters(cutnorth,cutsouth);
    cutnorth = false; // no dangerous tiles at north pole

    // convert all borders to correct projection
    for (int i = 0; i < numGroups; i++) {
      int num = groups[i].numtiles;
      int num4 = num * 4;
      int num5 = num * 5;
      float *ctx = new float[num5];
      float *cty = new float[num5];
      for (int m = 0; m < num4; m++) {
        ctx[m] = groups[i].tilex[m];
        cty[m] = groups[i].tiley[m];
      }
      for (int m = 0; m < num; m++) {
        ctx[num4 + m] = groups[i].midlon[m];
        cty[num4 + m] = groups[i].midlat[m];
      }

      area.P().convertPoints(srcProj, num5, ctx, cty);
      //gc.geo2xy(area, num5, ctx, cty);
      float minx = bignum, maxx = -bignum, miny = bignum, maxy = -bignum;
      // find min-max corners
      for (int j = 0; j < num; j++) {
        int bidx = 4 * j;
        int cidx = 2 * j;
        groups[i].mmx[cidx + 0] = bignum;
        groups[i].mmx[cidx + 1] = -bignum;
        groups[i].mmy[cidx + 0] = bignum;
        groups[i].mmy[cidx + 1] = -bignum;

        groups[i].use[j] = true;
        if ((cutsouth && groups[i].tiletype[j] == 2) || (cutnorth
            && groups[i].tiletype[j] == 1)) {
          groups[i].use[j] = false;
          continue;
        }
        // if discontinuity in tile-borders: drop tile
        if (((ctx[num4 + j] > ctx[bidx + 0]) && (ctx[num4 + j] > ctx[bidx + 1])
            && (ctx[num4 + j] > ctx[bidx + 2]) && (ctx[num4 + j]
            > ctx[bidx + 3])) ||

        ((ctx[num4 + j] < ctx[bidx + 0]) && (ctx[num4 + j] < ctx[bidx + 1])
            && (ctx[num4 + j] < ctx[bidx + 2]) && (ctx[num4 + j]
            < ctx[bidx + 3])) ||

        ((cty[num4 + j] > cty[bidx + 0]) && (cty[num4 + j] > cty[bidx + 1])
            && (cty[num4 + j] > cty[bidx + 2]) && (cty[num4 + j]
            > cty[bidx + 3])) ||

        ((cty[num4 + j] < cty[bidx + 0]) && (cty[num4 + j] < cty[bidx + 1])
            && (cty[num4 + j] < cty[bidx + 2]) && (cty[num4 + j]
            < cty[bidx + 3]))) {
          groups[i].use[j] = false;
          continue;
        }

        for (int l = 0; l < 4; l++) {
          if (ctx[bidx + l] > groups[i].mmx[cidx + 1])
            groups[i].mmx[cidx + 1] = ctx[bidx + l];
          if (ctx[bidx + l] < groups[i].mmx[cidx + 0])
            groups[i].mmx[cidx + 0] = ctx[bidx + l];
          if (cty[bidx + l] > groups[i].mmy[cidx + 1])
            groups[i].mmy[cidx + 1] = cty[bidx + l];
          if (cty[bidx + l] < groups[i].mmy[cidx + 0])
            groups[i].mmy[cidx + 0] = cty[bidx + l];
        }
        if (groups[i].mmx[cidx + 0] < minx)
          minx = groups[i].mmx[cidx + 0];
        if (groups[i].mmx[cidx + 1] > maxx)
          maxx = groups[i].mmx[cidx + 1];
        if (groups[i].mmy[cidx + 0] < miny)
          miny = groups[i].mmy[cidx + 0];
        if (groups[i].mmy[cidx + 1] > maxy)
          maxy = groups[i].mmy[cidx + 1];
      }
      delete[] ctx;
      delete[] cty;
      groups[i].mmx[2 * num + 0] = minx;
      groups[i].mmx[2 * num + 1] = maxx;
      groups[i].mmy[2 * num + 0] = miny;
      groups[i].mmy[2 * num + 1] = maxy;
    }
    proj = area.P();
  }

  float x1, y1, x2, y2;
  x1 = area.R().x1;
  x2 = area.R().x2;
  y1 = area.R().y1;
  y2 = area.R().y2;

  // start group loop
  for (int g = 0; g < numGroups; g++) {
    int ngt = groups[g].numtiles;

    // check if tilegroup inside area
    if ((groups[g].mmx[ngt * 2 + 0] > x2) || (groups[g].mmx[ngt * 2 + 1] < x1)
        || (groups[g].mmy[ngt * 2 + 0] > y2) || (groups[g].mmy[ngt * 2 + 1]
        < y1)) {
      continue;
    }
    // start tile loop
    for (int i = 0; i < ngt; i++) {
      // check if legal tile for this projection
      if (!groups[g].use[i])
        continue;

      // check if tile inside area
      if ((groups[g].mmx[i * 2 + 0] > x2) || (groups[g].mmx[i * 2 + 1] < x1)
          || (groups[g].mmy[i * 2 + 0] > y2) || (groups[g].mmy[i * 2 + 1] < y1)) {
        continue;
      }
      // check if correct record number
      if (recnr != groups[g].crecnr[i]) {
        // position filepointer to correct record
        int offset = groups[g].crecnr[i] * nwrec * 2;
        if (fseek(pfile, offset, 0) != 0) {
          fclose(pfile);
          return false;
        }
        // read record
        nwr = fread(indata, 2, nwrec, pfile);
        if (nwr != nwrec) {
          fclose(pfile);
          cerr << "Reading error 3" << endl;
          return false;
        }
        recnr = groups[g].crecnr[i];
      }
      wp = groups[g].cwp[i];

      if (wp + 26 >= nwrec) {
        nwr = fread(indata, 2, nwrec, pfile);
        if (nwr != nwrec) {
          fclose(pfile);
          cerr << "Reading error 4" << endl;
          return false;
        }
        recnr++;
        wp = 0;
      }
      int nump; // number of polygons
      int16to32(indata[wp + 0], indata[wp + 1], nump);
      int npv; // total number of poly-vertices
      int16to32(indata[wp + 2], indata[wp + 3], npv);
      int ntv; // total number of triangle-vertices
      int16to32(indata[wp + 4], indata[wp + 5], ntv);
      ntv *= 3;
      short numtypes = indata[wp + 6]; // max polygontype

      if (nump == 0 || (npv == 0 && ntv == 0) || numtypes == 0)
        continue;

      float midlon = groups[g].midlon[i];
      float midlat = groups[g].midlat[i];

      short typerecnr[10], typewp[10];
      for (int j = 0; j < numtypes; j++) {
        typerecnr[j] = indata[wp + 7 + j * 2];
        typewp[j] = indata[wp + 7 + j * 2 + 1];
      }
      wp += 27;

      //
      int psize = numPolytiles;
      polydata[psize].np = 0;
      polydata[psize].polysize.clear();
      polydata[psize].polyverx = new float[npv];
      polydata[psize].polyvery = new float[npv];
      numPolytiles++;

      int pidx = 0; // polygon index

      float *triverx = 0, *trivery = 0;
      if (ntv > 0) {
        triverx = new float[ntv];
        trivery = new float[ntv];
      }

      // start polygon-type loop
      for (int type = 0; type < numtypes; type++) {
        // check if correct record number
        if (recnr != typerecnr[type]) {
          // position filepointer to correct record
          int offset = typerecnr[type] * nwrec * 2;
          if (fseek(pfile, offset, 0) != 0) {
            fclose(pfile);
            return false;
          }
          // read record
          nwr = fread(indata, 2, nwrec, pfile);
          if (nwr != nwrec) {
            fclose(pfile);
            cerr << "Reading error 4.2" << endl;
            return false;
          }
          recnr = typerecnr[type];
        }
        wp = typewp[type];

        if (wp + 0 >= nwrec) {
          nwr = fread(indata, 2, nwrec, pfile);
          if (nwr != nwrec) {
            fclose(pfile);
            cerr << "Reading error 4.3" << endl;
            return false;
          }
          recnr++;
          wp = 0;
        }
        short ntype = indata[wp + 0];
        wp++;

        int tidx = 0; // triangle index

        // start polygons for this tile
        for (int j = 0; j < ntype; j++) {
          // start polygons for this type
          // check if next record
          if (wp + 6 >= nwrec) {
            nwr = fread(indata, 2, nwrec, pfile);
            if (nwr != nwrec) {
              fclose(pfile);
              cerr << "Reading error 5" << endl;
              return false;
            }
            recnr++;
            wp = 0;
          }

          map_boundbox pbb;// polygon bounding box
          pbb.west = indata[wp + 0] * scale + midlon;
          pbb.east = indata[wp + 1] * scale + midlon;
          pbb.south = indata[wp + 2] * scale + midlat;
          pbb.north = indata[wp + 3] * scale + midlat;

          short nnp = indata[wp + 4];// number of semi-polygons
          int nt;// number of triangles
          int16to32(indata[wp + 5], indata[wp + 6], nt);
          wp += 7;

          // check if polygon to be used
          if (pbb.size() < geomin) {
            break;
          }

          for (int ij = 0; ij < nnp; ij++) {
            // read size of this polygon
            if (wp + 1 >= nwrec) {
              nwr = fread(indata, 2, nwrec, pfile);
              if (nwr != nwrec) {
                fclose(pfile);
                cerr << "Reading error 6" << endl;
                return false;
              }
              recnr++;
              wp = 0;
            }
            int np;// size of semipolygon
            int16to32(indata[wp + 0], indata[wp + 1], np);
            wp += 2;
            polydata[psize].polysize.push_back(np);
            polydata[psize].np += np;
            // polygon vertices
            for (int k = 0; k < np; k++, wp += 2) {
              if (wp + 1 >= nwrec) {
                nwr = fread(indata, 2, nwrec, pfile);
                if (nwr != nwrec) {
                  fclose(pfile);
                  cerr << "Reading error 7" << endl;
                  return false;
                }
                recnr++;
                wp = 0;
              }
              polydata[psize].polyverx[pidx] = indata[wp + 0] * scale + midlon;
              polydata[psize].polyvery[pidx] = indata[wp + 1] * scale + midlat;
              pidx += 1;
            } // polygon
          } // semi-polygon loop

          // triangle vertices
          for (int k = 0; k < nt; k++, wp += 6) {
            if (wp + 5 >= nwrec) {
              nwr = fread(indata, 2, nwrec, pfile);
              if (nwr != nwrec) {
                fclose(pfile);
                cerr << "Reading error 8" << endl;
                return false;
              }
              recnr++;
              wp = 0;
            }
            triverx[tidx + 0] = indata[wp + 0] * scale + midlon;
            trivery[tidx + 0] = indata[wp + 1] * scale + midlat;
            triverx[tidx + 1] = indata[wp + 2] * scale + midlon;
            trivery[tidx + 1] = indata[wp + 3] * scale + midlat;
            triverx[tidx + 2] = indata[wp + 4] * scale + midlon;
            trivery[tidx + 2] = indata[wp + 5] * scale + midlat;
            tidx += 3;
          }
        } // end polygon

        // draw triangles
        if (tidx > 0 && land) {
          if (type == 1)
            glColor4ubv(bcolour);
          else
            glColor4ubv(fcolour);

          glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

          // convert vertices
          area.P().convertPoints(srcProj, tidx, triverx, trivery);
          //gc.geo2xy(area, tidx, triverx, trivery);

          clipTriangles(0, tidx, triverx, trivery, xylim, jumplimit);
/*
          glBegin(GL_TRIANGLES);
          for (int iv = 0; iv < tidx; iv++) {
            glVertex2f(triverx[iv], trivery[iv]);
          }
          glEnd();
*/
        }
      }

      // draw coast-lines and land-boundaries
      int numpo = polydata[psize].polysize.size();
      if (numpo > 0 && (cont || keepcont)) {

        // convert vertices
        area.P().convertPoints(srcProj, polydata[psize].np, polydata[psize].polyverx, polydata[psize].polyvery);
        //gc.geo2xy(area, polydata[psize].np, polydata[psize].polyverx, polydata[psize].polyvery);

        if (cont) {
          glColor4ubv(lcolour);
          int id1 = 0, id2;
          for (int ipp = 0; ipp < numpo; ipp++) {
            id2 = id1 + polydata[psize].polysize[ipp];

            clipPrimitiveLines(id1, id2-1, polydata[psize].polyverx,
                polydata[psize].polyvery,xylim,jumplimit);
/*
            glBegin(GL_LINE_STRIP);
            for (int iv = id1; iv < id2; iv++) {
              glVertex2f(polydata[psize].polyverx[iv],
                  polydata[psize].polyvery[iv]);
            }
            glEnd();
            */
            id1 = id2;
          }
        }
      }
      delete[] triverx;
      delete[] trivery;
    }
  }

  if (keepcont)
    contexist = true;
  else
    clearPolys();

  //cerr << "+++ Finished" << endl;
  fclose(pfile);
  glDisable(GL_LINE_STIPPLE);
  return true;
}

void FilledMap::clipTriangles(int i1, int i2, float * x, float * y, float xylim[4],
    float jumplimit){

  const float bigjump = 1000000;
  glBegin(GL_TRIANGLES);
  for (int iv = i1; iv < i2; iv+=3) {
    float x1=x[iv], x2=x[iv+1], x3=x[iv+2];
    float y1=y[iv], y2=y[iv+1], y3=y[iv+2];

    if (jumplimit > bigjump || (fabsf(x1 - x2) < jumplimit && fabsf(x2 - x3) < jumplimit &&
        fabsf(x1 - x3) < jumplimit && fabsf(y1 - y2) < jumplimit &&
        fabsf(y2 - y3) < jumplimit && fabsf(y1 - y3) < jumplimit)){
      glVertex2f(x1, y1);
      glVertex2f(x2, y2);
      glVertex2f(x3, y3);
    }
/*
    glVertex2f(x[iv], y[iv]);
    glVertex2f(x[iv+1], y[iv+1]);
    glVertex2f(x[iv+2], y[iv+2]);
*/
  }
  glEnd();

}



void FilledMap::clipPrimitiveLines(int i1, int i2, float *x, float *y, float xylim[4],
    float jumplimit)
{
  int i, n = i1;
  while (n < i2) {
    i = n++;
    while (n <= i2 && fabsf(x[n - 1] - x[n]) < jumplimit && fabsf(y[n - 1]
        - y[n]) < jumplimit) {
      n++;
    }
    xyclip(n - i, &x[i], &y[i], xylim);
  }
}

void FilledMap::xyclip(int npos, float *x, float *y, float xylim[4])
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

