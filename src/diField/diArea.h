// -*- c++ -*-
/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013 met.no

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
#ifndef diArea_h
#define diArea_h

#include "diRectangle.h"
#include "diProjection.h"

#include <iosfwd>

/**
  \brief Rectangular area with Projection

  Specification of rectangular area in a specific map projection.
*/

class Area {
private:
  Projection proj;  ///> The map projection
  Rectangle rect;   ///> The rectangular area
  std::string name;    ///> name of this Area

  /// Copy members
  void memberCopy(const Area& rhs);

public:
  Area();
  Area(const Projection&, const Rectangle&);
  Area(const std::string&, const Projection&, const Rectangle&);
  ~Area();

  Area(const Area &rhs);
  Area& operator=(const Area &rhs);

  bool operator==(const Area &rhs) const;
  bool operator!=(const Area &rhs) const;

  friend std::ostream& operator<<(std::ostream& output, const Area& a);

  const Projection& P() const {return proj; }
  void setP(const Projection& p) {proj= p; }
  const Rectangle& R() const {return rect; }
  void setR(const Rectangle& r) {rect= r; }
  const std::string& Name() const { return name; }
  void setName(const std::string& n) { name= n; }
  ///set default values of projection and rectangle
  void setDefault();
  ///set area from string (name=name proj4string="+proj..." rectangle=x1:y1:x2:y2)
  bool setAreaFromString(const std::string& areaString);
  /// get string (name=name proj4string="+proj..." rectangle=x1:y1:x2:y2)
  std::string getAreaString() const;

  double getDiagonalInMeters() const;
};

class GridArea : public Area {
public:
  GridArea()
    : nx(0), ny(0), resolutionX(1), resolutionY(1) { }

  explicit GridArea(const Area& rhs)
    : Area(rhs), nx(0), ny(0), resolutionX(1), resolutionY(1) { }

  explicit GridArea(const Area& rhs, int nX, int nY, float rX, float rY)
    : Area(rhs), nx(nX), ny(nY), resolutionX(rX), resolutionY(rY) { }

  int gridSize() const
    { return nx*ny; }

  GridArea scaled(int factor) const;

public:
  int nx;
  int ny;
  float resolutionX;
  float resolutionY;
};

#endif
