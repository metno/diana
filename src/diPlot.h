/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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
#ifndef diPlot_h
#define diPlot_h

#include <diColour.h>
#include <diPlotOptions.h>
#include <diPrintOptions.h>

#include <diField/diArea.h>
#include <diField/diGridConverter.h>
#include <puTools/miTime.h>

#include <GL/gl.h>

#include <vector>

class GLPfile;
class FontManager;


/**
   \brief Ancestor of all map plotting classes

   Plot keeps all static data shared by the various plotting classes.
   - postscript generation initiated here
*/

class Plot {
protected:
  // static members
  static Area area;          // Projection and size of current grid
  static Area requestedarea; // Projection and size of requested grid
  static Rectangle maprect;  // Size of map plot area
  static Rectangle fullrect; // Size of full plot area
  static GridConverter gc;   // gridconverter class
  static miutil::miTime ctime;       // current time
  static float pwidth;       // physical size of plotarea
  static float pheight;      // --- " ---
  static FontManager* fp;    // master fontpack
  static bool dirty;         // plotarea has changed
  static GLPfile* psoutput;  // PostScript module
  static bool hardcopy;      // producing postscript
  static int pressureLevel;          // current pressure level
  static int oceandepth;       // current ocean depth
  static std::string bgcolour;  // name of background colour
  static Colour backgroundColour;   // background colour
  static Colour backContrastColour; // suitable contrast colour
  static float gcd;          // great circle distance
  static bool panning;       // panning in progress
  static std::vector<float> xyLimit; // MAP ... xyLimit=x1,x2,y1,y2
  static std::vector<float> xyPart;  // MAP ... xyPart=x1%,x2%,y1%,y2%

  bool enabled;              // plot enabled
  bool datachanged;          // plotdata has changed
  bool rgbmode;              // rgb or colour-index mode
  std::string pinfo;            // plotinfo
  PlotOptions poptions;      // plotoptions
  printerManager printman;   // printer manager
  std::string plotname;         // name of plot

  void psAddImage(const GLvoid*,GLint,GLint,GLint, // pixels,size,nx,ny
		  GLfloat,GLfloat,GLfloat,GLfloat, // x,y,sx,sy
		  GLint,GLint,GLint,GLint,   // start,stop
		  GLenum,GLenum);  // format, type

public:
  // Constructors
  Plot();
  virtual ~Plot() {}

  // Equality operator
  bool operator==(const Plot &rhs) const;

  /// init fonts
  static void initFontManager();

  /// kill current FontManager, start new and init
  static void restartFontManager();

  /// plot
  virtual bool plot(){return false; }
  /// plot for specified layer
  virtual bool plot(const int){return false; }

  /// enable this plot object
  void enable(const bool f= true);
  /// is this plot object enabled
  bool Enabled() const {return enabled;}

  /// return current area on map
  Area& getMapArea(){return area;}
  /// set area, possibly trying to keep the current physical area
  bool setMapArea(const Area&, bool keepcurrentarea);
  /// with a new projection: find the best matching physical area with the current one
  Area findBestMatch(const Area&);
  /// this is the area we really want
  void setRequestedarea(const Area &a){requestedarea=a;}
  /// this is the area we really wanted
  Area& getRequestedarea(){return requestedarea;}

  /// this is the full size of the plot in the current projection
  Rectangle& getPlotSize(){return fullrect;}
  /// set the  full size of the plot in the current projection
  void setPlotSize(const Rectangle& r);

  /// this is size of the data grid
  Rectangle& getMapSize(){return maprect;}
  /// set the size of the data grid
  void setMapSize(const Rectangle& r);

  /// set the physical size of the map in pixels
  void setPhysSize(const float, const float);
  /// this is  the physical size of the map in pixels
  void getPhysSize(float&, float&);

  /// set the current data time
  void setTime(const miutil::miTime& t){ctime= t; }
  /// return the current data time
  miutil::miTime getTime(){return ctime;}

  /// set current pressure level
  void setPressureLevel(int l){pressureLevel= l; }
  /// this is the current pressure level
  int getPressureLevel(){return pressureLevel;}

  /// set current ocean depth
  void setOceanDepth(int depth){oceandepth= depth; }
  /// this is the current ocean depth
  int getOceanDepth(){return oceandepth;}

  /// set name of background colour
  void setBgColour(const std::string& cn){bgcolour= cn;}
  /// return the name of the current background colour
  const std::string& getBgColour() const {return bgcolour;}

  /// set background colour
  void setBackgroundColour(const Colour& c){backgroundColour= c;}
  /// set colour with good contrast to background
  void setBackContrastColour(const Colour& c){backContrastColour= c;}

  /// return the current background colour
  const Colour& getBackgroundColour() const { return backgroundColour; }
  /// return colour with good contrast to background
  const Colour& getBackContrastColour() const { return backContrastColour; }

  /// set the plot info string
  void setPlotInfo(const std::string& pin);

  /// return n elements of the current plot info string
  std::string getPlotInfo(int n=0);

  /// return the elements given
  std::string getPlotInfo(std::string str);

  /// return true if right plot string
  bool plotInfoOK(const std::string& pin){return (pinfo == pin);}

  /// return the current PlotOptions
  const PlotOptions& getPlotOptions() const { return poptions; }

  /// return pointer to the FontManager
  FontManager* getFontPack() const {return fp;}

  /// mark this as 'redraw needed'
  void setDirty(const bool =true);
  /// is redraw needed
  bool getDirty(){return dirty;}

  /// set colour mode (rgb or color-index)
  void setColourMode(const bool isrgb =true);
  /// return current colourmode
  bool getColourMode() {return rgbmode; }

  /// clear clipping variables
  void xyClear();

  /// set name of this plot object
  void setPlotName(const std::string& name){plotname= name;}
  /// return name of this plot object
  virtual void getPlotName(std::string& name){name= plotname;}

  // hardcopy routines
  /// start postscript output
  bool startPSoutput(const printOptions& po);
  /// add a stencil as x,y arrays (postscript only)
  void addHCStencil(const int& size, const float* x, const float* y);
  /// add a scissor in GL coordinates (postscript only)
  void addHCScissor(const double x0, const double y0, // GL scissor
		    const double  w, const double  h);
  /// add a scissor in pixel coordinates (postscript only)
  void addHCScissor(const int x0, const int y0, // Pixel scissor
		    const int  w, const int  h);
  /// remove all clipping (postscript only)
  void removeHCClipping();
  /// for postscript output - resample state vectors
  void UpdateOutput();
  /// start new page in postscript
  bool startPSnewpage();
  /// for postscript output - reset state vectors
  void resetPage();
  /// end postscript output
  bool endPSoutput();

  /// set great circle distance
  void setGcd(const float dist);
  /// toggle panning
  void panPlot(const bool);
};

#endif
