/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2020 met.no

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

#ifndef diFieldRenderer_h
#define diFieldRenderer_h

#include "diFieldRendererBase.h"

#include "diColour.h"
#include "diPlotOptions.h"

#include <QPolygonF>

/**
  \brief Plots one field

  Holds the (scalar) field(s) after reading and any computations.
  Contains all field plotting methodes except contouring.
*/
class FieldRenderer : public FieldRendererBase
{
public:
  FieldRenderer();
  ~FieldRenderer();

  void setData(const Field_pv& fields, const miutil::miTime& time) override;
  void clearData() override;

  void setGridConverter(GridConverter* gc) override;
  void setBackgroundColour(const Colour& bg) override;
  void setMap(const PlotArea& pa) override;
  void setPlotOptions(const PlotOptions& po) override;

  std::set<PlotOrder> plotLayers() override;
  void plot(DiGLPainter* gl, PlotOrder zorder) override;
  bool plotNumbers(DiGLPainter* gl) override;

  bool hasVectorAnnotation() override;
  void getVectorAnnotation(float& size, std::string& text) override;

private:

  typedef std::vector<float*> (FieldRenderer::*prepare_vectors_t)(const float* x, const float* y);

  std::vector<float*> prepareVectors(const float* x, const float* y);
  std::vector<float*> prepareDirectionVectors(const float* x, const float* y);
  std::vector<float*> doPrepareVectors(const float* x, const float* y, bool direction);

  void setAutoStep(const float* x, const float* y, int& ix1, int ix2, int& iy1, int iy2, int maxElementsX, int& step, float& dist);
  int xAutoStep(const float* x, const float* y, int& ix1, int ix2, int iy, float sdist);

  // plotting methods
  bool plotMe(DiGLPainter* gl, PlotOrder zorder);
  bool plotWind(DiGLPainter* gl);
  bool plotWindAndValue(DiGLPainter* gl, bool flightlevelChart = false);
  bool plotValue(DiGLPainter* gl);
  bool plotValues(DiGLPainter* gl);

  bool plotVector(DiGLPainter* gl);
  bool plotDirection(DiGLPainter* gl);
  bool plotArrows(DiGLPainter* gl, prepare_vectors_t pre_vec, const float* colourfield, float& arrowlength);

  bool plotContour(DiGLPainter* gl);
  bool plotContour2(DiGLPainter* gl, PlotOrder zorder);

  bool plotRaster(DiGLPainter* gl);

  void plotUndefined(DiGLPainter* gl);

  bool plotFrameOnly(DiGLPainter* gl);
  void plotFrame(DiGLPainter* gl, int nx, int ny, const float* x, const float* y);
  void plotFrameStencil(DiGLPainter* gl, int nx, int ny, const float* x, const float* y);
  QPolygonF makeFramePolygon(int nx, int ny, const float* x, const float* y) const;
  bool markExtreme(DiGLPainter* gl);
  bool plotGridLines(DiGLPainter* gl);

  bool isShadePlot() const;

  /** Return true if fields 0..count-1 are non-0 and have data.
   *  If count == 0, check that at least one field exists an that all fields have data.
   */
  bool checkFields(size_t count) const;

  bool getGridPoints(float*& x, float*& y, int& ix1, int& ix2, int& iy1, int& iy2, int factor = 1, bool boxes = false) const;
  bool getPoints(int n, float* x, float* y) const;

  const std::string& plottype() const { return poptions_.plottype; }

private:
  Field_pv fields_;                // fields, stored elsewhere
  Field_pv tmpfields_;             // tmp fields, stored here
  miutil::miTime ftime_;           // current field time

  // plotting parameters
  bool is_raster_; //!< true iff raster plot
  bool is_shade_;  //!< shaded (true) or line drawing (false)

  GridConverter* gc_;
  Colour backgroundcolour_;
  PlotArea pa_;
  PlotOptions poptions_;
};

#endif
