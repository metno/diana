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

#ifndef diPolyContouring_hh
#define diPolyContouring_hh 1

#include "diPlotOptions.h"

#include <map>
#include <string>
#include <vector>

#include "poly_contouring.hh"

class DiGLPainter;

class DianaLevels {
public:
  virtual ~DianaLevels() { }
  virtual contouring::level_t level_for_value(float value) const = 0;
  virtual float value_for_level(contouring::level_t l) const = 0;
  enum { UNDEF_LEVEL = -10000000 };
};
typedef boost::shared_ptr<DianaLevels> DianaLevels_p;

DianaLevels_p dianaLevelsForPlotOptions(const PlotOptions& mPoptions, float fieldUndef);

//------------------------------------------------------------------------

class DianaLevelList : public DianaLevels {
public:
  DianaLevelList(const std::vector<float>& levels);
  DianaLevelList(float lstart, float lstop, float lstep);
  virtual contouring::level_t level_for_value(float value) const;
  virtual float value_for_level(contouring::level_t l) const;
  size_t nlevels() const
    { return mLevels.size(); }
protected:
  DianaLevelList();
protected:
  std::vector<float> mLevels;
};

// ------------------------------------------------------------------------

class DianaLevelList10 : public DianaLevelList {
public:
  DianaLevelList10(const std::vector<float>& levels, size_t count);
  enum { BASE = 10 };
};

// ------------------------------------------------------------------------

class DianaLevelLog : public DianaLevelList {
public:
  DianaLevelLog(const std::vector<float>& levels);
  virtual contouring::level_t level_for_value(float value) const;
  virtual float value_for_level(contouring::level_t l) const;
  enum { BASE = 10 };
};

//------------------------------------------------------------------------

class DianaLevelStep : public DianaLevels {
public:
  DianaLevelStep(float step, float off);
  void set_limits(float mini, float maxi);
  virtual contouring::level_t level_for_value(float value) const;
  virtual float value_for_level(contouring::level_t l) const;
protected:
  float mStep, mOff, mMin, mMax;
  bool mHaveMin, mHaveMax;
};

// ########################################################################

class DianaPositions {
public:
  virtual ~DianaPositions() { }
  virtual contouring::point_t position(size_t ix, size_t iy) const = 0;
};

typedef boost::shared_ptr<DianaPositions> DianaPositions_p;

// ########################################################################

class DianaFieldBase : public contouring::field_t {
public:
  DianaFieldBase(const DianaLevels& levels, const DianaPositions& positions)
    : mLevels(levels), mPositions(positions) { }
  
  virtual contouring::level_t grid_level(size_t ix, size_t iy) const
    { return mLevels.level_for_value(value(ix, iy)); }
  virtual contouring::point_t line_point(contouring::level_t level, size_t x0, size_t y0, size_t x1, size_t y1) const;
  virtual contouring::point_t grid_point(size_t x, size_t y) const
    { return position(x, y); }

  contouring::level_t undefined_level() const
    { return DianaLevels::UNDEF_LEVEL; }
  
protected:
  virtual float value(size_t ix, size_t iy) const = 0;

  contouring::point_t position(size_t ix, size_t iy) const
    { return mPositions.position(ix, iy); }

  const DianaPositions& positions() const
    { return mPositions; }

private:
  const DianaLevels& mLevels;
  const DianaPositions& mPositions;
};

// ########################################################################

class DianaLines : public contouring::lines_t {
public:
  DianaLines(const PlotOptions& poptions, const DianaLevels& levels);
  virtual ~DianaLines();

  void add_contour_line(contouring::level_t level, const contouring::points_t& points, bool closed);
  void add_contour_polygon(contouring::level_t level, const contouring::points_t& points);

  virtual void paint();

protected:
  typedef std::vector<contouring::point_t> point_v;
  typedef std::vector<point_v> point_vv;
  typedef std::map<contouring::level_t, point_vv> level_points_m;

protected:
  virtual void paint_polygons();
  virtual void paint_lines();
  virtual void paint_labels();

  virtual void setLine(const Colour& colour, const Linetype& linetype, int linewidth) = 0;
  virtual void setFillColour(const Colour& colour) = 0;
  virtual void drawLine(const point_v& lines) = 0;
  virtual void drawPolygons(const point_vv& polygons) = 0;
  virtual void drawLabels(const point_v& points, contouring::level_t li) = 0;

protected:
  const PlotOptions& mPlotOptions;
  const DianaLevels& mLevels;

private:
  level_points_m m_lines;
  level_points_m m_polygons;
};

// ########################################################################

int find_index(bool repeat, int available, int i);

bool poly_contour(int nx, int ny, int ix0, int iy0, int ix1, int iy1,
    const float z[], const float xz[], const float yz[],
    DiGLPainter* gl, const PlotOptions& poptions, float fieldUndef);

#endif // diPolyContouring_hh
