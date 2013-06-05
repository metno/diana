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
#ifndef diVcrossPlot_h
#define diVcrossPlot_h

#include "diColour.h"
#include "diFontManager.h"
#include "diPlotOptions.h"
#include "diPrintOptions.h"

#include <diField/diGridConverter.h>
#include <diField/diField.h>
#include <diField/diFieldFunctions.h>
#include <puTools/miTime.h>

#include <GL/gl.h>

#include <vector>
#include <map>

const int numVcrossFunctions= 42;

const std::string vcrossFunctionDefs[numVcrossFunctions]= {
  "add(a,b)",
  "subtract(a,b)",
  "multiply(a,b)",
  "divide(a,b)",
  "tc_from_tk(tk)",
  "tk_from_tc(tc)",
  "tc_from_th(th)",
  "tk_from_th(th)",
  "th_from_tk(tk)",
  "thesat_from_tk(tk)",
  "thesat_from_th(th)",
  "the_from_tk_q(tk,q)",
  "the_from_th_q(th,q)",
  "rh_from_tk_q(tk,q)",
  "rh_from_th_q(th,q)",
  "q_from_tk_rh(tk,rh)",
  "q_from_th_rh(th,rh)",
  "tdc_from_tk_q(tk,q)",
  "tdc_from_th_q(th,q)",
  "tdc_from_tk_rh(tk,rh)",
  "tdc_from_th_rh(th,rh)",
  "tdk_from_tk_q(tk,q)",
  "tdk_from_th_q(th,q)",
  "tdk_from_tk_rh(tk,rh)",
  "tdk_from_th_rh(th,rh)",
  "ducting_from_tk_q(tk,q)",
  "ducting_from_th_q(th,q)",
  "ducting_from_tk_rh(tk,rh)",
  "ducting_from_th_rh(th,rh)",
  "d_ducting_dz_from_tk_q(tk,q)",
  "d_ducting_dz_from_th_q(th,q)",
  "d_ducting_dz_from_tk_rh(tk,rh)",
  "d_ducting_dz_from_th_rh(th,rh)",
  "ff_total(u,v)",
  "ff_normal(u,v)",
  "ff_tangential(u,v)",
  "ff_north_south(u,v)",
  "ff_east_west(u,v)",
  "ff_knots_from_ms(ffms)",
  "momentum_vn_fs(vn)",
  "height_above_msl_from_th(th)",
  "height_above_surface_from_th(th)"
};

/// functions for computations of vertical crossection data
enum VcrossFunction {
  vcf_add,
  vcf_subtract,
  vcf_multiply,
  vcf_divide,
  vcf_tc_from_tk,
  vcf_tk_from_tc,
  vcf_tc_from_th,
  vcf_tk_from_th,
  vcf_th_from_tk,
  vcf_thesat_from_tk,
  vcf_thesat_from_th,
  vcf_the_from_tk_q,
  vcf_the_from_th_q,
  vcf_rh_from_tk_q,
  vcf_rh_from_th_q,
  vcf_q_from_tk_rh,
  vcf_q_from_th_rh,
  vcf_tdc_from_tk_q,
  vcf_tdc_from_th_q,
  vcf_tdc_from_tk_rh,
  vcf_tdc_from_th_rh,
  vcf_tdk_from_tk_q,
  vcf_tdk_from_th_q,
  vcf_tdk_from_tk_rh,
  vcf_tdk_from_th_rh,
  vcf_ducting_from_tk_q,
  vcf_ducting_from_th_q,
  vcf_ducting_from_tk_rh,
  vcf_ducting_from_th_rh,
  vcf_d_ducting_dz_from_tk_q,
  vcf_d_ducting_dz_from_th_q,
  vcf_d_ducting_dz_from_tk_rh,
  vcf_d_ducting_dz_from_th_rh,
  vcf_ff_total,
  vcf_ff_normal,
  vcf_ff_tangential,
  vcf_ff_north_south,
  vcf_ff_east_west,
  vcf_knots_from_ms,
  vcf_momentum_vn_fs,
  vcf_height_above_msl_from_th,
  vcf_height_above_surface_from_th,
  vcf_no_function
};

//----------------------------------------------------
// enums and structs used in static data and static functions

const int numVcrossPlotTypes= 5;

const std::string vcrossPlotDefs[numVcrossPlotTypes]= {
  "contour(f)",
  "wind(u,v)",
  "vector(u,v)",
  "vt+omega(vt,omega)",
  "vt+w(vt,w)"
};

enum VcrossPlotType {
  vcpt_contour,
  vcpt_wind,
  vcpt_vector,
  vcpt_vt_omega,
  vcpt_vt_w,
  vcpt_no_plot
};

enum VcrossZoomType {
  vczoom_keep,
  vczoom_standard,
  vczoom_decrease,
  vczoom_increase,
  vczoom_move
};

/// Verical Crossection computations from setup file
struct vcFunction {
  VcrossFunction function;
  std::vector<std::string> vars;
};

/// Vertical Crossection plottable fields from setup file
struct vcField {
  std::string name;
  std::vector<std::string> vars;
  VcrossPlotType plotType;
  std::string       plotOpts;
};

/// Vertical Crossection data contents incl. possible computations
struct FileContents {
  std::vector<std::string> fieldNames;
  std::map<std::string,vcFunction> useFunctions;
};
//----------------------------------------------------

/// Posssible vertical types for Vertical Crossection plots
enum VcrossVertical {
  vcv_exner,
  vcv_pressure,
  vcv_height,
  vcv_none
};

/// Annotation info for Vertical Crossections
struct VcrossText {
  bool     timeGraph;
  Colour   colour;
  std::string modelName;
  std::string crossectionName;
  std::string fieldName;
  int      forecastHour;
  miutil::miTime   validTime;
  std::string extremeValueString;
};


class VcrossOptions;
class GLPfile;


/**
   \brief Plots (prognostic) Vertical Crossection data

   Data preparation and plotting. Many computations can be done.
   Uses the standard (field) Contouring for scalar field plots.
   An object holds data for one crossection.
*/
class VcrossPlot
{

  friend class VcrossFile;
  friend class VcrossField;

public:
  VcrossPlot();
  ~VcrossPlot();

  static bool parseSetup();
  static void makeContents(const std::string& fileName,
			   const std::vector<int>& iparam, int vcoord);
  static void deleteContents(const std::string& fileName);
  static std::vector<std::string> getFieldNames(const std::string& fileName);
  static std::map<std::string,std::string> getAllFieldOptions();
  static std::string getFieldOptions(const std::string& fieldname);

  static void setPlotWindow(int w, int h);

  static void getPlotSize(float& x1, float& y1, float& x2, float& y2,
			  Colour& rubberbandColour);
  static void decreasePart(int px1, int py1, int px2, int py2);
  static void increasePart();
  static void movePart(int pxmove, int pymove);
  static void standardPart();

  static void startPlot(int numplots, VcrossOptions *vcoptions);
  static void plotText();

  // postscript output
  static bool startPSoutput(const printOptions& po);
  static void addHCStencil(const int& size,
			   const float* x, const float* y);
  static void addHCScissor(const int x0, const int y0,
			   const int  w, const int  h);
  static void removeHCClipping();
  static void UpdateOutput();
  static void resetPage();
  static bool startPSnewpage();
  static bool endPSoutput();

  bool prepareData(const std::string& fileName);
  bool plot(VcrossOptions *vcoptions,
	    const std::string& fieldname,
	    PlotOptions& poptions);
  bool plotBackground(const std::vector<std::string>& labels);
  void plotFrame();
  void plotAnnotations(const std::vector<std::string>& labels);
  void plotVerticalMarkerLines();
  void plotMarkerLines();
  void plotVerticalGridLines();
  void plotSurfacePressure();
  void plotLevels();
  void plotXLabels();
  void plotTitle();
  void plotMax(float x, float y, float scale);
  void plotMin(float x, float y, float scale);
  int getHorizontalPosNum() { return horizontalPosNum; }
  int getNearestPos(int px);

protected:
  // PostScript module
  static GLPfile* psoutput;  // PostScript module
  static bool hardcopy;      // producing postscript

private:
  static FontManager*  fp;  // fontpack
  static GridConverter gc;  // gridconverter class
  FieldFunctions ffunc;     // Container for Field Functions

  //----------------------------------------------------
  static std::map<std::string,int> vcParName;    // name -> number
  static std::map<int,std::string> vcParNumber;  // number -> name

  static std::multimap<std::string,vcFunction> vcFunctions;

  static std::map<std::string,vcField> vcFields;
  static std::vector<std::string> vcFieldNames;  // setup/dialog sequence

  static std::map<std::string,FileContents> fileContents;
  //----------------------------------------------------

  static int plotw;
  static int ploth;
  static int numplot;
  static int nplot;

  static float fontsize;
  static float fontscale;
  static float chxdef;
  static float chydef;
  static float xFullWindowmin;
  static float xFullWindowmax;
  static float yFullWindowmin;
  static float yFullWindowmax;
  static float xWindowmin;
  static float xWindowmax;
  static float yWindowmin;
  static float yWindowmax;
  static float xPlotmin;
  static float xPlotmax;
  static float yPlotmin;
  static float yPlotmax;
  static Colour backColour;
  static Colour contrastColour;

  static VcrossZoomType zoomType;
  static int            zoomSpec[4];

  static miutil::miTime timeGraphReference;
  static float  timeGraphMinuteStep;

  static bool bottomStencil;

  static std::vector<VcrossText> vcText;

  //---------------------------------

  VcrossOptions* vcopt;

  std::string modelName;
  std::string crossectionName;
  miutil::miTime   validTime;
  int      forecastHour;
  std::vector<miutil::miTime> validTimeSeries;
  std::vector<int>    forecastHourSeries;
  bool     timeGraph;

  int horizontalPosNum;

  int vcoord;
  int nPoint;
  int numLev;
  int nTotal;
  int iundef;

  std::vector<int> idPar1d; //parm number of corresponding entry in cdata1d
  std::vector<int> idPar2d; //parm number of corresponding entry in cdata2d

  std::vector<float*> cdata1d;
  std::vector<float*> cdata2d;

  std::vector<float> alevel;
  std::vector<float> blevel;

  float vrangemin,vrangemax;
  float tgdx,tgdy,horizontalLength;

  //----------------------------------

  float tgxpos,tgypos;

  // data pointers (cdata1d, single level data)
  int nps,nxg,nyg,nxs,nsin,ncos,npy1,npy2,nlat,nlon;
  int ngdir1,ngdir2,npss,npsb,ncor,nxds,ntopo;

  // data pointers (cdata2d, multilevel data)
  int npp,nzz,npi,nx,ny,nwork;

  // plottable 2d parameters (fields), access by name
  // std::map<name,cdata2d_index>
  std::map<std::string,int> params;

  std::map<std::string,vcFunction> useFunctions;

  //----------------------------------

  VcrossVertical vcoordPlot;
  std::string       verticalAxis;;
  float v2hRatio;
  float yconst,yscale;

  float xDatamin,xDatamax,yDatamin,yDatamax;
  float pmin,pmax,pimin,pimax;

  std::vector<float> vlimitmin;
  std::vector<float> vlimitmax;

  //----------------------------------

  float refPosition;
  std::vector<std::string> markName;
  std::vector<float>    markNamePosMin;
  std::vector<float>    markNamePosMax;

  //----------------------------------

  void prepareVertical();
  int addPar1d(int param, float *pdata = 0);
  int addPar2d(int param, float *pdata = 0);

  bool computeSize();
  bool plotData(const std::string& fieldname,
	        PlotOptions& poptions);
  int findParam(const std::string& var);
  int computer(const std::string& var, VcrossFunction vcfunc,
	       std::vector<int> parloc);

  bool plotWind(float *u, float *v, float *x, float *y,
		int *part, float *ylim,
                float size, int hstepAuto, bool *uselevel,
		PlotOptions& poptions);
  bool vector2fixedLevel(float *u, float *v, float *x, float *y,
			 int ix1, int ix2, float *ylim,
			 int hstep, const std::vector<float> yfixed,
			 int &kf1, int &kf2,
			 float *uint, float *vint, float *xint, float *yint);
  bool vcMovement(float *vt, float *om, float *p, float *x, float *y,
		  int *part, float *ylim, float hours, int hstepAuto,
		  bool *uselevel, PlotOptions& poptions);
  bool plotVector(float *u, float *v, float *x, float *y,
		  int *part, float *ylim,
                  float size, int hstepAuto, bool *uselevel,
		  PlotOptions& poptions);
  void replaceUndefinedValues(int nx, int ny, float *f,
			      bool fillAll);
  void xyclip(int npos, float *x, float *y, float xylim[4]);
  std::vector <std::string> split(const std::string,const char,const char);
  void plotArrow(const float& x0,
		 const float& y0,
		 const float& dx,
		 const float& dy,
		 bool thinArrows);
};

#endif
