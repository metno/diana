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
#include <puTools/miString.h>
#include <vector>

using namespace std;


/**

  \brief Plots one field

  Holds the (scalar) field(s) after reading and any computations.
  Contains all field plotting methodes except contouring.
*/
class FieldPlot : public Plot {

public:
  // Constructor
  FieldPlot();
  // Destructor
  ~FieldPlot();

  ///plot in overlay buffer
  bool overlayBuffer(){return overlay;}

  bool getAnnotations(vector<miutil::miString>& anno);
  bool getDataAnnotations(vector<miutil::miString>& anno);
  bool plot();
  bool plot(const int){return false;}
  bool updateNeeded(miutil::miString&);
  // check if current has same level
  bool updatePinNeeded(const miutil::miString pin);
  bool prepare(const miutil::miString& fname, const miutil::miString&);
  bool setData(const vector<Field*>&, const miutil::miTime&);
  Area& getFieldArea();
  bool getRealFieldArea(Area&);
  bool getShadePlot() const { return pshade; }
  bool getUndefinedPlot() const { return pundefined; }
  void getFieldAnnotation(miutil::miString&, Colour&);
  vector<Field*> getFields() {return fields; }
  miutil::miTime getTime() const {return ftime;}
  miutil::miTime getAnalysisTime() const {return analysisTime;}
  bool plotUndefined();
  bool plotNumbers();
  miutil::miString getModelName();
  miutil::miString getTrajectoryFieldName();
  bool obs_mslp(ObsPositions& obsPositions);
  bool fieldsOK();
  void clearFields();

private:
  vector<Field*> fields; // fields, stored elsewhere
  vector<Field*> tmpfields; // tmp fields, stored here
  miutil::miTime ftime;          // current field time
  miutil::miTime analysisTime;   // time of model analysis

  bool overlay; //plot in overlay;

  // plotting parameters
  miutil::miString plottype;       // plot-method to use
  bool pshade;          // shaded (true) or line drawing (false)
  bool pundefined;      // mark undefined areas/values

  // from plotting routines to annotations
  float    vectorAnnotationSize;
  miutil::miString vectorAnnotationText;

  vector<float*> prepareVectors(float* x, float* y, bool rotateVectors);
  vector<float*> prepareDirectionVectors(float* x, float* y, bool rotateVectors);
  void setAutoStep(float* x, float* y, int& ix1, int ix2, int& iy1, int iy2,
		   int maxElementsX, int& step, float& dist);
  int xAutoStep(float* x, float* y, int& ix1, int ix2, int iy, float sdist);

  // plotting methods
  bool plotWind();
  bool plotWindAndValue(bool flightlevelChart=false);
  bool plotVector();
  bool plotValue();
  bool plotValues();
  bool plotVectorColour();
  bool plotDirection();
  bool plotDirectionColour();
  bool plotContour();
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

  bool plotAlpha_shade();
  void plotFrame(const int nx, const int ny,
  		 float *x, float *y,
		 const int mapconvert,
		 float *cvfield2map);
  bool markExtreme();
  bool plotGridLines();
};

#endif
