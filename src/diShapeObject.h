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

#include <diObjectPlot.h>
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
  std::vector<std::string> dbfStringDescr;
  std::vector<int> dbfIntDescr;
  std::vector<double> dbfDoubleDescr;
  GridConverter gc;

  void makeColourmap();
  void writeCoordinates();
  // Copy members
  void memberCopy(const ShapeObject& rhs);

  std::vector <SHPObject*> shapes;
  std::vector <SHPObject*> orig_shapes;
  std::vector<std::string> dbfIntName;
  std::vector<std::string> dbfDoubleName;
  std::vector<std::string> dbfStringName;
  std::vector< std::vector<int> > dbfIntDesc;
  std::vector< std::vector<double> > dbfDoubleDesc;
  std::vector< std::vector<std::string> > dbfStringDesc;
  std::map <std::string, std::vector<std::string> > dbfPlotDesc;
  int readDBFfile(const std::string& filename, std::vector<std::string>& dbfIntName,
      std::vector< std::vector<int> >& dbfIntDesc, std::vector<std::string>& dbfDoubleName,
      std::vector< std::vector<double> >& dbfDoubleDesc, std::vector<std::string>& dbfStringName,
      std::vector< std::vector<std::string> >& dbfStringDesc);

public:
  ShapeObject();
  ~ShapeObject();

  ShapeObject(const ShapeObject & rhs);
  ShapeObject& operator=(const ShapeObject &shpObj);

  bool changeProj(const Area& fromArea);
  bool read(std::string filename);
  bool read(std::string filename, bool convertFromGeo);

  void plot(PlotOrder zorder);

  bool plot(Area area, // current area
            double gcd, // size of plotarea in m
            bool land, // plot triangles
            bool cont, // plot contour-lines
            bool keepcont, // keep contourlines for later
            bool special, // special case, when plotting symbol instead of a point
            int symbol, // symbol number to be plottet
            std::string dbfcol, // text in dfb file to be plottet for that column
            GLushort linetype, // contour line type
            float linewidth, // contour linewidth
            const unsigned char* lcolour, // contour linecolour
            const unsigned char* fcolour, // triangles fill colour
            const unsigned char* bcolour);

  virtual int getXYZsize();
  virtual std::vector<float> getX();
  virtual std::vector<float> getY();
  virtual void setXY(std::vector<float> x, std::vector <float> y);
  virtual bool getAnnoTable(std::string & str);
};

#endif
