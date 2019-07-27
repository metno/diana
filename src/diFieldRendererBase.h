/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018 met.no

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

#ifndef diFieldRendererBase_h
#define diFieldRendererBase_h

#include "diPlotArea.h"
#include "diPlotOrder.h"

#include <puTools/miTime.h>

#include <memory>
#include <set>
#include <vector>

class Field;
class GridConverter;
class DiGLPainter;
class Colour;
class PlotOptions;

class FieldRendererBase
{
public:
  virtual ~FieldRendererBase();

  virtual void setData(const std::vector<Field*>& vf, const miutil::miTime& time) = 0;
  virtual void clearData() = 0;

  virtual void setGridConverter(GridConverter* gc) = 0;
  virtual void setBackgroundColour(const Colour& bg) = 0;
  virtual void setMap(const PlotArea& pa) = 0;
  virtual void setPlotOptions(const PlotOptions& po) = 0;

  //! identify which layers will actually be filled
  virtual std::set<PlotOrder> plotLayers() = 0;

  virtual void plot(DiGLPainter* gl, PlotOrder zorder) = 0;
  virtual bool plotNumbers(DiGLPainter* gl) = 0;

  virtual bool hasVectorAnnotation() = 0;
  virtual void getVectorAnnotation(float& size, std::string& text) = 0;
};

typedef std::shared_ptr<FieldRendererBase> FieldRendererBase_p;

#endif // diFieldRendererBase_h
