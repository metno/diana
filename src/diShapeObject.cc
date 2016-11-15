/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2015 met.no

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

#include "diShapeObject.h"

#include "diColourShading.h"
#include "diGLPainter.h"
#include "diPoint.h"
#include "util/polygon_util.h"
#include "util/subprocess.h"

#include <diField/VcrossUtil.h> // minimize + maximize
#include <puTools/miStringFunctions.h>

#include <QFile>
#include <QRegExp>

#include <cfloat>
#include <sstream>

#define MILOGGER_CATEGORY "diana.ShapeObject"
#include <miLogger/miLogging.h>

/* Created at Wed Oct 12 15:31:16 2005 */

using namespace std;
using namespace miutil;

namespace /* anonymous */ {

inline XY fromQ(const QPointF& p) { return XY(p.x(), p.y()); }

inline bool encloses(const Rectangle& outer, const Rectangle& inner)
{
  return outer.isinside(inner.x1, inner.y1)
      && outer.isinside(inner.x2, inner.y2);
}

// anything > 1 will cause small painting errors; anything > 0 might
// cause problems if the hardware has sub-pixel resolution (i.e. the
// painting is scaled)
const int MIN_PIXELS = 1;

// factor by which to extend the map area
const float AREA_EXTRA = 0.02;

// parts/polygons smaller than this fraction of the map are regarded
// as invisible
const float AREA_VISIBLE = 0.005;

const bool SKIP_SMALL = false;


bool extractParameterFromWKT(const QString& wkt, const QString& key, float& value)
{
  QRegExp r_parameter("PARAMETER\\[\""+key+"\", *([0-9.Ee+-]+)\\]");
  if (r_parameter.indexIn(wkt) >= 0) {
    value = r_parameter.cap(1).toFloat();
    return true;
  } else {
    return false;
  }
}

bool setProjectionViaGdalSRSInfo(Projection& p, QString wkt)
{
  const QString PROJECTION_LCC     = "PROJECTION[\"Lambert_Conformal_Conic\"]";
  const QString PROJECTION_LCC_2SP = "PROJECTION[\"Lambert_Conformal_Conic_2SP\"]";
  if (wkt.contains(PROJECTION_LCC)
      && wkt.contains("PARAMETER[\"standard_parallel_1\",")
      && wkt.contains("PARAMETER[\"standard_parallel_2\","))
  {
    wkt.replace(PROJECTION_LCC, PROJECTION_LCC_2SP);
    METLIBS_LOG_INFO("trying to fix wkt for gdalsrsinfo, new wkt='" << wkt.toStdString() << "'");
  }

  QByteArray gdalStdOut;
  if (diutil::execute("gdalsrsinfo", QStringList() << "-o" << "proj4" << wkt, &gdalStdOut) != 0)
    return false;

  QString proj4(gdalStdOut);

  // gdalsrsinfo puts its proj4 output in quotes and adds some whitespace
  proj4 = proj4.trimmed();
  if (proj4.startsWith("'"))
    proj4 = proj4.mid(1);
  if (proj4.endsWith("'"))
    proj4 = proj4.mid(0, proj4.length()-1);
  proj4 = proj4.trimmed();

  if (proj4.isEmpty())
    return false;
  METLIBS_LOG_DEBUG("gdalsrsinfo wkt='" << wkt.toStdString() << "', proj4='" << proj4.toStdString() << "'");
  return p.set_proj_definition(proj4.toStdString());
}

bool guessProjectionFromWKT(Projection& p, const QString& wkt)
{
  if (!wkt.startsWith("PROJCS[\""))
    return false;

  const QRegExp r_utm("\"WGS_1984_UTM_Zone_(\\d+).\"");
  if (r_utm.indexIn(wkt, 0) != -1) {
    const QString proj4 = "+proj=utm +zone=" + r_utm.cap(1)
        + " +ellps=WGS84 +datum=WGS84 +units=m";
    METLIBS_LOG_INFO("guessing utm from wkt='" << wkt.toStdString() << "', proj4='" << proj4.toStdString() << "'");
    p.set_proj_definition(proj4.toStdString());
    return true;
  }

  if (wkt.contains("PROJECTION[\"Lambert_Conformal_Conic\"]")) {
    QString proj4 = QString("+proj=lcc");
    float lat_1, lat_2, lon_0=0, lat_0, x_0, y_0;
    if (extractParameterFromWKT(wkt, "standard_parallel_1", lat_1)
        && extractParameterFromWKT(wkt, "standard_parallel_2", lat_2))
    {
      proj4 += QString(" +lat_1=%1 +lat_2=%2").arg(lat_1).arg(lat_2);
    }
    if (extractParameterFromWKT(wkt, "central_meridian", lon_0) && lon_0 != 0) {
      proj4 += QString(" +lon_0=%1").arg(lon_0);
    }
    if (extractParameterFromWKT(wkt, "latitude_of_origin", lat_0)) {
      proj4 += QString(" +lat_0=%1").arg(lat_0);
    }
    if (extractParameterFromWKT(wkt, "false_easting", x_0) && x_0 != 0) {
      proj4 += QString(" +lat_0=%1").arg(x_0);
    }
    if (extractParameterFromWKT(wkt, "false_northing", y_0) && y_0 != 0) {
      proj4 += QString(" +y_0=%1").arg(y_0);
    }
    if (wkt.contains("GEOGCS[\"GCS_WGS_1984\"")) {
      proj4 += " +ellps=WGS84";
    } else {
      return false;
    }
    if (wkt.contains("UNIT[\"Meter\",1]")) {
      proj4 += " +units=m";
    } else {
      return false;
    }
    METLIBS_LOG_DEBUG("guessing lcc from wkt='" << wkt.toStdString() << "', proj4='" << proj4.toStdString() << "'");
    p.set_proj_definition(proj4.toStdString());
    return true;
  }

  if (wkt.startsWith("GEOGCS[\"GCS_WGS_1984\"")) {
    METLIBS_LOG_INFO("wild guessing geographic from wkt '" << wkt.toStdString() << "'");
    p = Projection::geographic();
    return true;
  }

  return false;
}

} // anonymous namespace

ShapeObject::ShapeObject()
  : mReductionScale(0, 0) // invalid
{
  projection = Projection::geographic();

  typeOfObject = ShapeXXX;

  ColourShading cs("standard");
  colours=cs.getColourShading();
  colourmapMade=false;
}

ShapeObject::~ShapeObject()
{
}

bool ShapeObject::changeProj()
{
  METLIBS_LOG_SCOPE();

  int npoints = 0;
  float *tx = 0, *ty = 0;

  bool success = true;
  for (ShpData_v::iterator s = shapes.begin(); s != shapes.end(); ++s) {
    if (npoints < s->nvertices()) {
      npoints = s->nvertices();
      delete[] tx;
      delete[] ty;
      tx = new float[npoints];
      ty = new float[npoints];
    }

    for (int j=0; j<s->nvertices(); j++) {
      tx[j] = s->shape->padfX[j];
      ty[j] = s->shape->padfY[j];

      if (projection.isGeographic()) {
        // an ugly fix to avoid problem with -180.0, 180.0
        const float DEG_LIM = 179.9;
        if (tx[j] < -DEG_LIM)
          tx[j] = -DEG_LIM;
        else if (tx[j] > DEG_LIM)
          tx[j] = DEG_LIM;
        tx[j] *= DEG_TO_RAD;
        ty[j] *= DEG_TO_RAD;
      }
    }
    success &= getStaticPlot()->ProjToMap(projection, s->nvertices(), tx, ty);

    s->contours.clear();
    s->rect = Rectangle(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
    s->partRects.clear();

    const int np = s->nparts();
    const bool single_point = (np == 0 && s->type() == SHPT_POINT && s->nvertices() == 1);
    for (int p=0; (single_point && p==0) || p<np; ++p) {
      const int pb = single_point ? 0 : s->pbegin(p);
      const int pe = single_point ? 0 : s->pend(p);

      QPolygonF polygon;
      polygon << QPointF(tx[pb], ty[pb]);

      s->partRects.push_back(Rectangle(tx[pb], ty[pb], tx[pb], ty[pb]));
      Rectangle& pr = s->partRects.back();

      for (int v=pb+1; v<pe; v++) {
        polygon << QPointF(tx[v], ty[v]);
        vcross::util::minimaximize(pr.x1, pr.x2, tx[v]);
        vcross::util::minimaximize(pr.y1, pr.y2, ty[v]);
      }

      s->contours << polygon;

      vcross::util::minimize(s->rect.x1, pr.x1);
      vcross::util::minimize(s->rect.y1, pr.y1);
      vcross::util::maximize(s->rect.x2, pr.x2);
      vcross::util::maximize(s->rect.y2, pr.y2);
    }
  }

  delete[] tx;
  delete[] ty;

  mReductionScale = XY(0, 0); // invalid

  return success;
}

bool ShapeObject::read(const std::string& filename)
{
  METLIBS_LOG_TIME(filename);

  SHPHandle hSHP = SHPOpen(filename.c_str(), "rb");
  if (hSHP == NULL) {
    METLIBS_LOG_ERROR("Unable to read shp file '" << filename << "'");
    return false;
  }

  int nShapeType, nEntities;
  double adfMinBound[4], adfMaxBound[4];
  SHPGetInfo(hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound);

  shapes.reserve(nEntities);
  for (int i = 0; i < nEntities; i++) {
    SHPObject_p shp(SHPReadObject(hSHP, i), SHPDestroyObject);
    const int type = shp->nSHPType;
    if (type == SHPT_POLYGON || type == SHPT_ARC || type == SHPT_POINT)
      shapes.push_back(ShpData(shp));
  }

  SHPClose(hSHP);

  bool dbf_ok = (readDBFfile(filename) == 0);
  if (!readProjection(filename))
    projection = Projection::geographic();

  colourmapMade=false;
  stringcolourmapMade=false;
  doublecolourmapMade=false;
  intcolourmapMade=false;

  changeProj();

  return dbf_ok;
}

// FIXME this might turn nice polygons into self-intersecting polygons
void ShapeObject::reduceForScale()
{
  if (mReductionScale == getStaticPlot()->getPhysToMapScale())
    return;
  mReductionScale = getStaticPlot()->getPhysToMapScale();

  METLIBS_LOG_TIME();
  for (ShpData_v::iterator s = shapes.begin(); s != shapes.end(); ++s) {
    if (s->type() == SHPT_POINT) {
      // cannot reduce point data
      continue;
    }

    s->reduced.clear();
    for (int p=0; p < s->nparts(); p++) {
      const QPolygonF& pp = s->contours.at(p);
      QPolygonF ppr;
      XY pp0 = getStaticPlot()->MapToPhys(fromQ(pp.at(0)));
      ppr << pp.at(0);
      for (int k = 1; k < pp.size()-1; k++) {
        const XY ppk = getStaticPlot()->MapToPhys(fromQ(pp.at(k)));
        const bool dx = (abs(pp0.x() - ppk.x()) >= MIN_PIXELS);
        const bool dy = (abs(pp0.y() - ppk.y()) >= MIN_PIXELS);
        if (dx || dy) {
          ppr << pp.at(k);
          pp0 = ppk;
        }
      }
      ppr << pp.at(pp.size()-1);
      s->reduced << ppr;
    }
  }
}

void ShapeObject::plot(DiGLPainter* gl, PlotOrder porder)
{
  METLIBS_LOG_TIME(LOGVAL(shapes.size()));
  makeColourmap();
  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
  for (ShpData_v::const_iterator s = shapes.begin(); s != shapes.end(); ++s) {
    if (s->type() != SHPT_POLYGON)
      continue;

    gl->setLineStyle(s->colour, 2);
    gl->drawPolygons(s->contours); // TODO optimize like in the other plot(...) function
  }
}

bool ShapeObject::plot(DiGLPainter* gl,
    const Area& area, // current area
    double gcd, // size of plotarea in m
    bool land, // plot triangles
    bool cont, // plot contour-lines
    bool special, // special case, when plotting symbol instead of a point
    int symbol, // symbol number to be plotted
    const Linetype& linetype, // contour line type
    float linewidth, // contour linewidth
    const Colour& lcolour, // contour linecolour
    const Colour& fcolour, // triangles fill colour
    const Colour& bcolour)
{
  METLIBS_LOG_TIME(LOGVAL(shapes.size()) << LOGVAL(land) << LOGVAL(cont));

  reduceForScale();

  //also scale according to windowheight and width (standard is 500)
  const float scalefactor = sqrtf(getStaticPlot()->getPhysHeight()*getStaticPlot()->getPhysHeight()
      +getStaticPlot()->getPhysWidth()*getStaticPlot()->getPhysWidth());
  const int fontSizeToPlot = int(2*7000000/gcd * scalefactor/500);
  const float symbol_rad = symbol/scalefactor;
  const float point_rad = linewidth*2/scalefactor;

  const Rectangle areaX = diutil::adjustedRectangle(area.R(),
      AREA_EXTRA*area.R().width()  + 2*linewidth * getStaticPlot()->getPhysToMapScaleX(),
      AREA_EXTRA*area.R().height() + 2*linewidth * getStaticPlot()->getPhysToMapScaleY());
  const float visibleW = areaX.width()  * AREA_VISIBLE;
  const float visibleH = areaX.height() * AREA_VISIBLE;

  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);

  size_t item_count = 0;
  const size_t item_limit = 0;
  for (ShpData_v::const_iterator s = shapes.begin(); s != shapes.end(); ++s) {
    if (s->contours.isEmpty())
      continue;

    if (!areaX.intersects(s->rect))
      continue;

    if (item_limit > 0 && ++item_count >= item_limit) {
      METLIBS_LOG_WARN("stop plotting after " << item_limit << " items");
      break;
    }

    if (s->type() == SHPT_POINT) {
      const QPolygonF& p = s->contours.at(0);
      if (s->nparts() == 0 && special==true && s->nvertices()==1) {
        // METLIBS_LOG_DEBUG("plotting a special SHPT_POINT");
        gl->setFont(poptions.fontname, fontSizeToPlot, DiCanvas::F_NORMAL);
        gl->drawText("SSS", s->shape->padfX[0], s->shape->padfY[0], 0.0);
        gl->setLineStyle(lcolour, 2);
        gl->drawCircle(true, p.at(0).x(), p.at(0).y(), symbol_rad);
      } else {
        // METLIBS_LOG_DEBUG("plotting SHPT_POINT(s)");
        gl->setColour(lcolour);
        for (int k = 0; k < p.size(); k++) {
          const QPointF& ppk = p.at(k);
          if (areaX.isinside(ppk.x(), ppk.y()))
            gl->drawCircle(true, ppk.x(), ppk.y(), point_rad);
        }
      }
      continue;
    }

    // now it is either a polyline (arc) or a polygon; data are in s->reduced
    if (!land && !cont)
      continue;

    if (SKIP_SMALL && fabs(s->rect.width()) < visibleW && fabs(s->rect.height()) < visibleH)
      continue;

    QList<QPolygonF> lines, fills;
    for (int p=0; p < s->nparts(); p++) {
      const QPolygonF& part =s->reduced.at(p);
      if (part.size() < 2 || (!cont && land && part.size() < 3))
        continue;

      const Rectangle& pr = s->partRects[p];
      if (!areaX.intersects(pr))
        continue;

      if (SKIP_SMALL) {
        const bool part_too_small = (pr.width() < visibleW && pr.height() < visibleH);
        if (part_too_small)
          continue;
      }

      // FIXME: not checking nparts == 1 might cause problems if polygons have holes
      if (/*s->nparts() == 1 &&*/ part.size() >= 16 && !encloses(areaX, pr)) {
        const QPolygonF trimmed = diutil::trimToRectangle(areaX, part);
        METLIBS_LOG_DEBUG(LOGVAL(part.size()) << LOGVAL(trimmed.size()));
        if (trimmed.size() >= 3)
          fills << trimmed;
        else if (cont && trimmed.size() == 2)
          lines << trimmed;
      } else {
        fills << part;
      }
    }

    if (!fills.isEmpty() && land && s->type() == SHPT_POLYGON) {
      gl->setColour(fcolour);
      gl->drawPolygons(fills);
    }
    if (cont && !(fills.isEmpty() && lines.isEmpty())) {
      gl->setLineStyle(lcolour, linewidth, linetype);
      for (int i=0; i<fills.size(); ++i)
        gl->drawPolyline(fills.at(i));
      for (int i=0; i<lines.size(); ++i)
        gl->drawPolyline(lines.at(i));
    }
  }
  return true;
}

static void writeColour(std::ostream& rs, const Colour& col)
{
  rs << ";" << (int) col.R() << ":" << (int) col.G() << ":"
     << (int) col.B() <<";;";
}

bool ShapeObject::getAnnoTable(std::string& str)
{
  makeColourmap();
  std::ostringstream rs;
  rs << "table=\"" << fname;
  if (stringcolourmapMade) {
    map <std::string,Colour>::iterator q=stringcolourmap.begin();
    for (; q!=stringcolourmap.end(); q++) {
      writeColour(rs, q->second);
      rs << q->first;
    }
  } else if (intcolourmapMade) {
    map <int,Colour>::iterator q=intcolourmap.begin();
    for (; q!=intcolourmap.end(); q++) {
      writeColour(rs, q->second);
      rs << q->first;
    }
  } else if (doublecolourmapMade) {
    map <double,Colour>::iterator q=doublecolourmap.begin();
    for (; q!=doublecolourmap.end(); q++) {
      writeColour(rs, q->second);
      rs << q->first;
    }
  }
  str = rs.str();
  return true;
}

void ShapeObject::makeColourmap()
{
  if (colourmapMade)
    return;
  colourmapMade=true;

  if (poptions.palettecolours.size())
    colours= poptions.palettecolours;

  if (not poptions.fname.empty())
    fname=poptions.fname;
  else if (dbfStringName.size())
    fname=dbfStringName[0];
  else if (dbfDoubleName.size())
    fname=dbfDoubleName[0];
  else if (dbfIntName.size())
    fname=dbfIntName[0];

  const int ncolours = colours.size();

  // strings, first find which vector of strings to use, dbfStringDescr
  for (size_t idsn=0; idsn < dbfStringName.size(); idsn++) {
    if (dbfStringName[idsn]==fname) {
      stringcolourmapMade = true;
      const std::vector<std::string>& dbfStringDescr = dbfStringDesc[idsn];
      for (size_t i=0, ii=0; i<shapes.size(); i++) {
        const std::string& descr = dbfStringDescr[shapes[i].id()];
        if (stringcolourmap.find(descr)==stringcolourmap.end())
          stringcolourmap[descr] = colours[(ii++) % ncolours];
        shapes[i].colour = stringcolourmap[descr];
      }
      return;
    }
  }

  // double, first find which vector of double to use, dbfDoubleDescr
  for (size_t iddn=0; iddn < dbfDoubleName.size(); iddn++) {
    if (dbfDoubleName[iddn]==fname) {
      const std::vector<double>& dbfDoubleDescr = dbfDoubleDesc[iddn];
      for (size_t i=0, ii=0; i<shapes.size(); i++) {
        double descr=dbfDoubleDescr[shapes[i].id()];
        if (doublecolourmap.find(descr)==doublecolourmap.end())
          doublecolourmap[descr] = colours[(ii++) % ncolours];
        shapes[i].colour = doublecolourmap[descr];
      }
      return;
    }
  }

  // int, first find which vector of double to use, dbfIntDescr
  for (size_t idin=0; idin < dbfIntName.size(); idin++) {
    if (dbfIntName[idin]==fname) {
      const std::vector<int>& dbfIntDescr = dbfIntDesc[idin];
      intcolourmapMade=true;
      for (size_t i=0, ii=0; i<shapes.size(); i++) {
        int descr = dbfIntDescr[shapes[i].id()];
        if (intcolourmap.find(descr)==intcolourmap.end())
          intcolourmap[descr] = colours[(ii++) % ncolours];
        shapes[i].colour = intcolourmap[descr];
      }
      return;
    }
  }
}

int ShapeObject::getXYZsize() const
{
  int size=0;
  for (ShpData_v::const_iterator s = shapes.begin(); s != shapes.end(); ++s)
    size += s->nvertices();
  return size;
}

XY ShapeObject::getXY(int idx) const
{
  int s0 = 0;
  for (ShpData_v::const_iterator s = shapes.begin(); s != shapes.end(); ++s) {
    for (int p=0; p<s->contours.size(); ++p) {
      const int s1 = s0 + s->contours.at(p).size();
      if (idx >= s0 && idx < s1)
        return fromQ(s->contours.at(p).at(idx - s0));
      s0 = s1;
    }
  }
  // index out of bounds
  return XY(0, 0);
}

std::vector<XY> ShapeObject::getXY() const
{
  std::vector<XY> xy;
  xy.reserve(getXYZsize());
  for (ShpData_v::const_iterator s = shapes.begin(); s != shapes.end(); ++s) {
    for (int p=0; p<s->contours.size(); ++p)
      for (int v=0; v<s->contours.at(p).size(); ++v)
        xy.push_back(fromQ(s->contours.at(p).at(v)));
  }
  return xy;
}

void ShapeObject::setXY(const std::vector<float>& x, const std::vector<float>& y)
{
  const size_t n = getXYZsize();
  if (x.size() != n || y.size() != n) {
    METLIBS_LOG_ERROR("invalid setXY");
    return;
  }
  int m=0;
  for (size_t i=0; i<shapes.size(); i++) {
    double *sx = shapes[i].shape->padfX, *sy = shapes[i].shape->padfY;
    for (int j = 0; j < shapes[i].shape->nVertices; j++) {
      sx[j] = x[m];
      sy[j] = y[m];
      m++;
    }
  }
}

int ShapeObject::readDBFfile(const std::string& filename)
{
  METLIBS_LOG_SCOPE(LOGVAL(filename));

  DBFHandle hDBF = DBFOpen(filename.c_str(), "rb");
  if (hDBF == NULL) {
    METLIBS_LOG_ERROR("DBFOpen "<<filename<<" failed");
    return 2;
  }

  const int nFieldCount = DBFGetFieldCount(hDBF);
  const int nRecordCount = DBFGetRecordCount(hDBF);

  if (nFieldCount == 0) {
    METLIBS_LOG_ERROR("There are no fields in this table!");
    DBFClose(hDBF);
    return 3;
  }

  std::vector<int> indexInt, indexDouble, indexString;

  const std::vector<int> dummyint;
  const std::vector<double> dummydouble;
  const std::vector<std::string> dummystring;

  /* -------------------------------------------------------------------- */
  /*	Check header definitions.					*/
  /* -------------------------------------------------------------------- */
  for (int i = 0; i < nFieldCount; i++) {
    int nWidth, nDecimals;
    char szTitle[12];
    DBFFieldType eType = DBFGetFieldInfo(hDBF, i, szTitle, &nWidth, &nDecimals);
    METLIBS_LOG_DEBUG("---> '" << szTitle << "'");

    const std::string name = miutil::to_upper(szTitle);
    if (eType == FTInteger) {
      indexInt.push_back(i);
      dbfIntName.push_back(name);
      dbfIntDesc.push_back(dummyint);
    } else if (eType == FTDouble) {
      indexDouble.push_back(i);
      dbfDoubleName.push_back(name);
      dbfDoubleDesc.push_back(dummydouble);
    } else if (eType == FTString) {
      indexString.push_back(i);
      dbfStringName.push_back(name);
      dbfStringDesc.push_back(dummystring);
    }
  }

  for (size_t n=0; n<dbfIntName.size(); n++) {
    int i = indexInt[n];
    std::vector<int>& values = dbfIntDesc[n];
    METLIBS_LOG_DEBUG("Int    description:  "<<indexInt[n]<<"  "<<dbfIntName[n]);
    for (int iRecord=0; iRecord<nRecordCount; iRecord++) {
      int att = 0;
      if (!DBFIsAttributeNULL(hDBF, iRecord, i))
        att = DBFReadIntegerAttribute(hDBF, iRecord, i);
      values.push_back(att);
    }
  }

  for (size_t n=0; n<dbfDoubleName.size(); n++) {
    int i= indexDouble[n];
    std::vector<double>& values = dbfDoubleDesc[n];
    METLIBS_LOG_DEBUG("Double description:  "<<indexDouble[n]<<"  "<<dbfDoubleName[n]);
    for (int iRecord=0; iRecord<nRecordCount; iRecord++) {
      double att = 0.0;
      if (!DBFIsAttributeNULL(hDBF, iRecord, i))
        att = DBFReadDoubleAttribute(hDBF, iRecord, i);
      values.push_back(att);
    }
  }

  for (size_t n=0; n<dbfStringName.size(); n++) {
    int i= indexString[n];
    std::vector<std::string>& values = dbfStringDesc[n];
    METLIBS_LOG_DEBUG("String description:  "<<indexString[n]<<"  "<<dbfStringName[n]);
    for (int iRecord=0; iRecord<nRecordCount; iRecord++) {
      if (!DBFIsAttributeNULL(hDBF, iRecord, i))
        values.push_back(DBFReadStringAttribute(hDBF, iRecord, i));
      else
        values.push_back("-");
    }
  }

  DBFClose(hDBF);

  return 0;
}

bool ShapeObject::readProjection(const std::string& shpfilename)
{
  METLIBS_LOG_SCOPE();
  QString prjfilename = QString::fromStdString(shpfilename);
  if (!prjfilename.endsWith(".shp")) {
    METLIBS_LOG_WARN("shp filename '" << shpfilename << "' does not end in '.shp'");
    return false;
  }

  prjfilename.replace(prjfilename.size()-3, 3, "prj");
  QFile prjfile(prjfilename);
  METLIBS_LOG_DEBUG(LOGVAL(prjfile.size()));

  prjfile.open(QIODevice::ReadOnly);
  const QString prj(prjfile.readAll());
  prjfile.close();

  METLIBS_LOG_DEBUG(LOGVAL(prj.toStdString()));
  if (prj.isEmpty())
    return false;

  if (setProjectionViaGdalSRSInfo(projection, prj))
    return true;

  if (guessProjectionFromWKT(projection, prj))
    return true;

  METLIBS_LOG_WARN("shapefile prj not understood: " << prj.toStdString());
  return false;
}
