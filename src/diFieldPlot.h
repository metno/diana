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
#ifndef diFieldPlot_h
#define diFieldPlot_h

#include "diPlot.h"
#include <diCommonTypes.h>
#include <diField/diField.h>
#include <puTools/miTime.h>

#include <vector>

/**
  \brief Plots one field

  Holds the (scalar) field(s) after reading and any computations.
  Contains all field plotting methodes except contouring.
*/
class FieldPlot : public Plot {

public:
  FieldPlot();
  ~FieldPlot();

  ///plot in overlay buffer
  bool overlayBuffer(){return overlay;}
  void setOverlayBuffer(bool overlay_){overlay=overlay_;}

  bool getDataAnnotations(std::vector<std::string>& anno);

  void plot(PlotOrder zorder);

  bool updateNeeded(std::string&);
  // check if current has same level
  bool updatePinNeeded(const std::string& pin);
  bool prepare(const std::string& fname, const std::string&);
  bool setData(const std::vector<Field*>&, const miutil::miTime&);
  const Area& getFieldArea();
  bool getRealFieldArea(Area&);
  bool getShadePlot() const { return (pshade || poptions.plot_under); }
  void getFieldAnnotation(std::string&, Colour&);
  std::vector<Field*> getFields() {return fields; }
  miutil::miTime getTime() const {return ftime;}
  miutil::miTime getAnalysisTime() const {return analysisTime;}
  bool plotUndefined();
  bool plotNumbers();
  std::string getModelName();
  std::string getTrajectoryFieldName();
  bool fieldsOK();
  void clearFields();

private:
  std::vector<Field*> fields; // fields, stored elsewhere
  std::vector<Field*> tmpfields; // tmp fields, stored here
  miutil::miTime ftime;          // current field time
  miutil::miTime analysisTime;   // time of model analysis

  bool overlay; //plot in overlay;

  // plotting parameters
  std::string plottype;       // plot-method to use
  bool pshade;          // shaded (true) or line drawing (false)

  // from plotting routines to annotations
  float    vectorAnnotationSize;
  std::string vectorAnnotationText;

  bool getTableAnnotations(std::vector<std::string>& anno);
  std::vector<float*> prepareVectors(float* x, float* y, bool rotateVectors);
  std::vector<float*> prepareDirectionVectors(float* x, float* y);
  void setAutoStep(float* x, float* y, int& ix1, int ix2, int& iy1, int iy2,
		   int maxElementsX, int& step, float& dist);
  int xAutoStep(float* x, float* y, int& ix1, int ix2, int iy, float sdist);

  // plotting methods
  bool plotMe();
  bool plotWind();
  bool plotWindAndValue(bool flightlevelChart=false);
  bool plotVector();
  bool plotValue();
  bool plotValues();
  bool plotVectorColour();
  bool plotDirection();
  bool plotDirectionColour();
  bool plotContour();
  bool plotContour2();
  bool plotBox_pattern();
  bool plotBox_alpha_shade();
  bool plotAlarmBox();

  /**
   * Fills grid cells with a color, according to an input table and the grid cells values.
   * NOTE: Define "values" and "palettecolors" of equal size in the setup-file
   * ("values" are then mapped to "palettecolors")
   * @return Returns true on success
   */
  bool plotFillCell();
  bool plotFillCellExt();
  unsigned char * createRGBAImage(Field * field);
  unsigned char * resampleImage(int& currwid, int& currhei,
    int& bmStartx, int& bmStarty,
    float& scalex, float& scaley,int& nx, int& ny);
  bool plotPixmap();
  unsigned char * imagedata; // dataarray for resampling and transforming field to RGBA image
  int previrs;               // previous resampling coeff.

  bool plotAlpha_shade();
  bool plotFrameOnly();
  void plotFrame(const int nx, const int ny,
      float *x, float *y);
  void plotFrameStencil(const int nx, const int ny, float *x, float *y);
  void plotFilledFrame(const int nx, const int ny, float *x, float *y);
  bool markExtreme();
  bool plotGridLines();

  int resamplingFactor(int nx, int ny) const;

  /** Return true if fields 0..count-1 are non-0 and have data.
   *  If count == 0, check that at least one field exists an that all fields have data.
   */
  bool checkFields(size_t count) const;

  bool getGridPoints(float* &x, float* &y, int& ix1, int& ix2, int& iy1, int& iy2, int factor=1, bool boxes=false, bool cached=true) const;
  bool getPoints(int n, float* x, float* y) const;
};

#endif
