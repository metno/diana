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
#ifndef _diFieldEdit_h
#define _diFieldEdit_h

#include <diCommonTypes.h>
#include <diDrawingTypes.h>
#include <diMapMode.h>
#include <diFieldPlot.h>
#include <diEditSpec.h>

#include <diField/diField.h>
#include <diField/diGridConverter.h>

#include <vector>
#include <set>

class FieldPlotManager;

/// info about influence of field edit operations
struct FieldInfluence {
  float posx,posy;
  int   influencetype;
  float rcircle;
  float axellipse, ayellipse, ecellipse;
};

/// one field isoline and info used during "line" field edit operations
struct IsoLine {
  float value;
  float vleft;
  float vright;
  bool  closed;
  std::vector<float> x;
  std::vector<float> y;
};


/**
   \brief Performs field editing

   All field edit methodes.
   Reads and writes MetnoFieldFiles directly.
*/
class FieldEdit {

private:

  friend class EditManager;

  struct UndoField {
    editType editstate;
    int i1,i2,j1,j2;
    float *data[2];
    FieldInfluence influence;
  };

  // GUI settings (defaults) common to all FieldEdit objects
  // (possibly set before any objects created)
  static editType editstate;  //###### NOT GOOD when several editTools !!!!!!!!
  static int      def_influencetype;
  static float    def_ecellipse;
  static bool     drawExtraLines;

  GridConverter gc;   // gridconverter class

  FieldPlotManager* fieldPlotManager;

  Field*     workfield;
  Field*     editfield;
  FieldPlot* editfieldplot;

  bool showNumbers;
  bool numbersDisplayed;

  bool specset;
  Area areaspec;
  double gridResolutionX;
  double gridResolutionY;

  bool areaminimize;
  float minValue,maxValue; // used if not fieldUndef
  std::string inputFieldFormat;
  std::string inputFieldConfig;
  std::string plotName;
  std::string fieldUnit;
  std::string vcoord;
  std::string vlevel;
  std::string outputFilename;
  std::string lastFileWritten;

  std::vector<UndoField> undofields;
  int numundo;
  bool unsavedChanges;

  bool  active, editStarted, operationStarted;

  float def_rcircle,def_axellipse,def_ayellipse;

  Area  maparea;
  float posx,posy;
  int   influencetype;
  float rcircle,axellipse,ayellipse,ecellipse;
  float orcircle,oaxellipse,oayellipse,oecellipse;
  float rcirclePlot,axellipsePlot,ayellipsePlot;
  float xArrow,yArrow;
  bool  showArrow;

  float xfirst,yfirst,xrefpos,yrefpos,deltascale,xprev,yprev;
  float lineincrement;
  bool  convertpos;
  bool  justDoneUndoRedo;
  FieldInfluence firstInfluence, currentInfluence;
  float* odata;
  int   nx,ny,fsize;
  int   i1ed,i2ed,j1ed,j2ed, i1edp,i2edp,j1edp,j2edp;
  int   numUndefReplaced;
  std::vector<float> xline, yline;
  float fline,lineinterval;
  int   numsmooth;
  float brushValue;
  bool  brushReplaceUndef;
  float classLineValue;
  IsoLine isoline;
  std::set<float> lockedValue;
  bool discontinuous;
  bool drawIsoline;

  void cleanup();
  void makeWorkfield();
  void changeGrid();

  void setFieldInfluence(const FieldInfluence& fi, bool geo);
  FieldInfluence getFieldInfluence(bool geo);

  void editValue(float px, float py, float delta);
  void editMove(float px, float py);
  void editGradient(float px, float py, float deltax, float deltay);
  void editLine();
  void editLimitedLine();
  IsoLine findIsoLine(float xpos, float ypos, float value,
		      int nsmooth, int nx, int ny, float *z,
		      bool drawBorders);
  int smoothline(int npos, float x[], float y[],
	         int nfirst, int nlast, int ismooth,
                 float xsmooth[], float ysmooth[]);
  void editSmooth(float px, float py);
  void editBrush(float px, float py);
  void editClassLine();
  void replaceInsideLine(const std::vector<float>& vx,
			 const std::vector<float>& vy,
			 float replaceValue);
  void editWeight(float px, float py,
		  int i1, int i2, int j1, int j2, float *weight);
  void editExpWeight(float px, float py,
		     int i1, int i2, int j1, int j2, float *weight,
		     bool gradients);
  bool quicksmooth(int nsmooth, bool allDefined, int nx, int ny,
		   float *data, float *work,
	           float *worku1, float *worku2);

  void drawInfluence();

  StaticPlot* getStaticPlot() const;

public:
  FieldEdit(FieldPlotManager* fieldPlotManager);
  ~FieldEdit();
  // Assignment operator
  FieldEdit& operator=(const FieldEdit &rhs);

  void setSpec( EditProduct& ep, int fnum);

  void setData(const std::vector<Field*>& vf,
               const std::string& fieldname,
	       const miutil::miTime& tprod);

  bool notifyEditEvent(const EditEvent&);

  bool changedEditField();
  void getFieldSize(int &nx, int &ny);
  bool prepareEditFieldPlot(const std::string& fieldname,
			    const miutil::miTime& tprod);

  void setConstantValue(float value);
  bool readEditFieldFile(const std::string& filename,
                         const std::string& fieldname,
			 const miutil::miTime& tprod);
  bool readEditfield(const std::string& filename);
  bool writeEditFieldFile(const std::string& filename);
  void activate();
  void deactivate() { active= false; };
  bool activated() { return active; }
  void plot(Plot::PlotOrder porder, bool showinfluence);
  bool getAnnotations(std::vector<std::string>& anno);
};

#endif
