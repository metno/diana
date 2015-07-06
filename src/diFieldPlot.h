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
#ifndef diFieldPlot_h
#define diFieldPlot_h

#include "diPlot.h"

#include "diCommonTypes.h"
#include "diRasterPlot.h"

#include <diField/diField.h>
#include <puTools/miTime.h>

#include <QPolygonF>

#include <vector>

class DiGLPainter;

/**
  \brief Plots one field

  Holds the (scalar) field(s) after reading and any computations.
  Contains all field plotting methodes except contouring.
*/
class FieldPlot : public Plot, protected RasterPlot {

public:
  FieldPlot();
  ~FieldPlot();

  bool getDataAnnotations(std::vector<std::string>& anno);
  int getLevel() const;

  void plot(DiGLPainter* gl, PlotOrder zorder);

  bool updateNeeded(std::string&) const;
  bool prepare(const std::string& fname, const std::string&);
  bool setData(const std::vector<Field*>&, const miutil::miTime&);
  const Area& getFieldArea() const;
  bool getRealFieldArea(Area&) const;
  bool getShadePlot() const { return (pshade || poptions.plot_under); }
  void getFieldAnnotation(std::string&, Colour&) const;
  const std::vector<Field*>& getFields() const {return fields; }
  const miutil::miTime& getTime() const {return ftime;}
  const miutil::miTime& getAnalysisTime() const {return analysisTime;}
  bool plotUndefined(DiGLPainter* gl);
  bool plotNumbers(DiGLPainter* gl);
  std::string getModelName();
  std::string getTrajectoryFieldName();
  bool fieldsOK();
  void clearFields();

private:
  std::vector<Field*> fields; // fields, stored elsewhere
  std::vector<Field*> tmpfields; // tmp fields, stored here
  miutil::miTime ftime;          // current field time
  miutil::miTime analysisTime;   // time of model analysis

  // plotting parameters
  std::string plottype;       // plot-method to use
  bool pshade;          // shaded (true) or line drawing (false)

  // from plotting routines to annotations
  float    vectorAnnotationSize;
  std::string vectorAnnotationText;

  bool getTableAnnotations(std::vector<std::string>& anno);

  typedef std::vector<float*> (FieldPlot::*prepare_vectors_t)(float* x, float* y);

  std::vector<float*> prepareVectors(float* x, float* y);
  std::vector<float*> prepareDirectionVectors(float* x, float* y);

  void setAutoStep(float* x, float* y, int& ix1, int ix2, int& iy1, int iy2,
      int maxElementsX, int& step, float& dist);
  int xAutoStep(float* x, float* y, int& ix1, int ix2, int iy, float sdist);

  // plotting methods
  bool plotMe(DiGLPainter* gl);
  bool plotWind(DiGLPainter* gl);
  bool plotWindAndValue(DiGLPainter* gl, bool flightlevelChart=false);
  bool plotValue(DiGLPainter* gl);
  bool plotValues(DiGLPainter* gl);

  bool plotVector(DiGLPainter* gl);
  bool plotDirection(DiGLPainter* gl);
  bool plotArrows(DiGLPainter* gl, prepare_vectors_t pre_vec,
      const float* colourfield, float& arrowlength);

  bool plotContour(DiGLPainter* gl);
  bool plotContour2(DiGLPainter* gl);

  bool plotRaster(DiGLPainter* gl);
  const Colour* colourForValue(float value) const;

  bool plotFrameOnly(DiGLPainter* gl);
  void plotFrame(DiGLPainter* gl, int nx, int ny, const float *x, const float *y);
  void plotFrameStencil(DiGLPainter* gl, int nx, int ny, const float *x, const float *y);
  QPolygonF makeFramePolygon(int nx, int ny, const float *x, const float *y) const;
  bool markExtreme(DiGLPainter* gl);
  bool plotGridLines(DiGLPainter* gl);

  int resamplingFactor(int nx, int ny) const;

  /** Return true if fields 0..count-1 are non-0 and have data.
   *  If count == 0, check that at least one field exists an that all fields have data.
   */
  bool checkFields(size_t count) const;

  bool getGridPoints(float* &x, float* &y, int& ix1, int& ix2, int& iy1, int& iy2, int factor=1, bool boxes=false) const;
  bool getPoints(int n, float* x, float* y) const;

  bool centerOnGridpoint() const;

protected:
  StaticPlot* rasterStaticPlot() Q_DECL_OVERRIDE
    { return getStaticPlot(); }
  const GridArea& rasterArea() Q_DECL_OVERRIDE
    { return fields[0]->area; }

  //! create image for alpha_shade, fill_cell, or alarm_box
  QImage rasterScaledImage(const GridArea&, int scale) Q_DECL_OVERRIDE;
};

#endif
