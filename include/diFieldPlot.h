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

#include <diPlot.h>
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

  void setDifference(const miString& str1, const miString& str2)
  	{ fieldDiff1=str1; fieldDiff2=str2; difference=true; }
  bool isDifference() const
  	{ return difference; }
  void getDifference(miString& str1, miString& str2, int& vectorIdx) const
  	{ str1=fieldDiff1; str2=fieldDiff2; vectorIdx=vectorIndex; }
  ///plot in overlay buffer
  bool overlayBuffer(){return overlay;}

  bool getAnnotations(vector<miString>& anno);
  bool getDataAnnotations(vector<miString>& anno);
  bool plot();
  bool plot(const int){return false;}
  bool updateNeeded(miString&);
  bool updateLevelNeeded(const miString& levelSpec, miString& pin);
  bool updateIdnumNeeded(const miString& idnumSpec, miString& pin);
  bool prepare(const miString&);
  bool setData(const vector<Field*>&, const miTime&);
  Area& getFieldArea();
  bool getRealFieldArea(Area&);
  bool getShadePlot() const { return pshade; }
  bool getUndefinedPlot() const { return pundefined; }
  void getFieldAnnotation(miString&, Colour&);
  vector<Field*> getFields() {return fields; }
  miTime getTime() const {return ftime;}
  miTime getAnalysisTime() const {return analysisTime;}
  bool plotUndefined();
  bool plotNumbers();
  miString getModelName();
  miString getTrajectoryFieldName();
  miString getRadarEchoFieldName();
  bool obs_mslp(ObsPositions& obsPositions);
  bool fieldsOK();
  void clearFields();

private:
  vector<Field*> fields; // fields, stored elsewhere
  vector<Field*> tmpfields; // tmp fields, stored here
  miTime ftime;          // current field time
  miTime analysisTime;   // time of model analysis

  bool overlay; //plot in overlay;
  bool difference;
  miString fieldDiff1, fieldDiff2;
  int vectorIndex;

  // plotting parameters
  miString ptype;       // plot-method to use
  bool pshade;          // shaded (true) or line drawing (false)
  bool pundefined;      // mark undefined areas/values

  // from plotting routines to annotations
  float    vectorAnnotationSize;
  miString vectorAnnotationText;

  vector<float*> prepareVectors(int nfields, float* x, float* y);
  vector<float*> prepareDirectionVectors(int nfields, float* x, float* y);
  void setAutoStep(float* x, float* y, int ix1, int ix2, int iy1, int iy2,
		   int maxElementsX, int& step, float& dist, bool& xStepComp);
  int xAutoStep(float* x, float* y, int ix1, int ix2, int iy, float sdist);

  // plotting methods
  bool plotWind();
  bool plotWindColour();
  bool plotWindTempFL();
  bool plotVector();
  bool plotVectorColour();
  bool plotDirection();
  bool plotDirectionColour();
  bool plotContour();
  bool plotBox_pattern();
  bool plotBox_alpha_shade();
  bool plotAlarmBox();
  bool plotAlpha_shade();
  void plotFrame(const int nx, const int ny,
  		 float *x, float *y,
		 const int mapconvert,
		 float *cvfield2map);
  bool markExtreme();
  bool plotGridLines();
};

#endif
