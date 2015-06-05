/*
 Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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
#ifndef _diShapeObject_h
#define _diShapeObject_h

#include "diObjectPlot.h"
#include "diGLPainter.h"

#include <QPolygonF>

#include <shapefil.h>

#include <map>
#include <vector>

/**
 \brief Plots generic shape data
 */
class ShapeObject : public ObjectPlot {
private:
  std::vector <Colour> colours;
  std::string fname; //field name to colour by
  std::map <std::string,Colour> stringcolourmap; //descr, colour
  std::map <int,Colour> intcolourmap; //descr, colour
  std::map <double,Colour> doublecolourmap; //descr, colour
  bool colourmapMade; //true if colour map made
  bool stringcolourmapMade;
  bool intcolourmapMade;
  bool doublecolourmapMade;

  std::vector<std::string> dbfIntName;
  std::vector<std::string> dbfDoubleName;
  std::vector<std::string> dbfStringName;
  std::vector< std::vector<int> > dbfIntDesc;
  std::vector< std::vector<double> > dbfDoubleDesc;
  std::vector< std::vector<std::string> > dbfStringDesc;

  int readDBFfile(const std::string& shpfilename);
  void makeColourmap();
  bool readProjection(const std::string& shpfilename);

  Projection projection;
  XY mReductionScale;

  typedef boost::shared_ptr<SHPObject> SHPObject_p;
  struct ShpData {
    SHPObject_p shape;
    int type() const
      { return shape->nSHPType; }
    int id() const
      { return shape->nShapeId; }
    int nvertices() const
      { return shape->nVertices; }
    int nparts() const
      { return shape->nParts; }
    int pbegin(int part) const
      { return shape->panPartStart[part]; }
    int pend(int part) const
      { return (part < nparts()-1) ? pbegin(part+1) : nvertices(); }

    ShpData(SHPObject_p s)
      : shape(s) { }

    Colour colour; //!< as read from dbase file

    QList<QPolygonF> contours; //!< reprojected to map
    Rectangle rect; //<! bounding box in map coordinates
    std::vector<Rectangle> partRects;  //<! bounding boxes of parts in map coordinates

    QList<QPolygonF> reduced; //!< reprojected to map, and reduced for map scale
  };
  typedef std::vector<ShpData> ShpData_v;
  ShpData_v shapes;

  //! reduce polygons by dropping segments that would be inside one pixel
  void reduceForScale();

public:
  ShapeObject();
  ~ShapeObject();

  bool changeProj();
  bool read(const std::string& filename);

  void plot(DiGLPainter* gl, PlotOrder zorder);

  bool plot(DiGLPainter* gl,
      const Area& area, // current area
      double gcd, // size of plotarea in m
      bool land, // plot triangles
      bool cont, // plot contour-lines
      bool special, // special case, when plotting symbol instead of a point
      int symbol, // symbol number to be plottet
      const Linetype& linetype, // contour line type
      float linewidth, // contour linewidth
      const Colour& lcolour, // contour linecolour
      const Colour& fcolour, // triangles fill colour
      const Colour& bcolour);

  virtual int getXYZsize() const;
  virtual XY getXY(int idx) const;
  virtual std::vector<XY> getXY() const;

  virtual void setXY(const std::vector<float>& x, const std::vector<float>& y);
  virtual bool getAnnoTable(std::string& str);
};

#endif
