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
#ifndef _diCommonTypes_h
#define _diCommonTypes_h

#include "diColour.h"

#include <puDatatypes/miCoordinates.h>
#include <puTools/miTime.h>
#include <diField/diArea.h>

#include <set>
#include <vector>

/**
   \brief GUI slider data
*/
struct SliderValues{
   int minValue;
   int maxValue;
   int value;
   float scale;
};

//--------------------------------------------------
// image
//--------------------------------------------------

/**
   \brief GUI data for one geo image source
*/
struct SatFileInfo{
  std::string name;
  std::string formattype; //mitiff or hdf5
  std::string metadata;
  std::string proj4string;
  std::string channelinfo;
  std::string paletteinfo;
  int hdf5type;
  miutil::miTime time;
  miutil::miClock clock;
  int day;
  bool opened;
  std::vector<std::string> channel;
  std::string default_channel;
  bool palette; //palette or rgb file
  std::vector <Colour> col;    //vector of colours used
  std::string fileformat;
  SatFileInfo() :
    opened(false), palette(false)
    {
    }
};

/**
   \brief GUI data for geo images
*/
struct SatDialogInfo{

  /// image file info
  struct File {
    std::string name;            ///< filename
    std::vector<std::string> channel; ///< channels available
    std::string default_channel; ///< default channel
    std::string fileformat;      ///< file format
  };

  /// main image info
  struct Image{
    std::string name;           ///< main image name
    std::vector<File> file;       ///< files available
    std::string default_file;   ///< default filename
  };

  std::vector<Image> image;       ///< defined Images
  SliderValues cut;          ///< rgb cutoff value for histogram stretching
  SliderValues alphacut;     ///< rgb cutoff value for alpha blending
  SliderValues alpha;        ///< alpha blending value
  SliderValues timediff;     ///< max time difference
};


//--------------------------------------------------
//info about object files
//--------------------------------------------------

/**
   \brief GUI data for object file
*/
struct ObjFileInfo{
  std::string name; ///< name of the file
  miutil::miTime time;   ///< time of the file
};


//--------------------------------------------------
// Map structures
//--------------------------------------------------

/**
   \brief GUI data for one map file
*/
struct MapFileInfo {
  std::string fname;     ///< filename
  double sizelimit; ///< viewsize limit in km's
};

/**
   \brief GUI data for one map element
*/
struct MapElementOption {
  bool ison;           ///< element is on
  std::string linecolour; ///< line color
  std::string fillcolour; ///< fill color
  std::string linetype;   ///< line type
  std::string linewidth;  ///< line width
  int zorder;          ///< z-position on map
  float density;       ///< density in degrees (latlon)
  bool showvalue;      ///< plot value string (latlon)
  std::string value_pos;  ///< value position (0=left, 1=bottom, 2=both) (latlon)
  float fontsize;      ///< fontsize for value plotting (latlon)
  MapElementOption() : ison(false) {}
};

/**
   \brief GUI data for one map
*/
struct MapInfo {
  std::string name;             ///< name of map
  std::string type;             ///< type of mapsource(s)
  bool logok;                ///< ok to log
  std::vector<MapFileInfo> mapfiles; ///< the file(s)
  MapElementOption contour;  ///< contourline options
  MapElementOption land;     ///< filled land options
  MapElementOption lon;      ///< lon-lines options
  MapElementOption lat;      ///< lat-lines options
  MapElementOption frame;    ///< area-frame options
  bool special;        ///< plot a symbol instead of a point (special case)
  int symbol;          ///< symbol number in ttf file (special case)
  std::string dbfcol;     ///< column name in dbf file, values to be plottet 
                                ///< (special case)
};

/**
   \brief GUI data for all maps
*/
struct MapDialogInfo {
  std::vector<std::string> areas; ///< all defined areas (names)
  std::string default_area;  ///< default area-name
  std::vector<MapInfo> maps;   ///< all defined maps
  std::vector<std::string> default_maps; ///< list of default maps (names)
  std::string backcolour;    ///< background colour
};


//--------------------------------------------------
// Observation structures
//--------------------------------------------------

/**
   \brief GUI data for all observations
*/
struct ObsDialogInfo{

  /// list of prioritized stations
  struct PriorityList{
    std::string file;
    std::string name;
  };

  /// list of plotting criterias
  struct CriteriaList{
    std::string name;
    std::vector<std::string> criteria;
  };
  /// observation data type
  struct DataType {
    std::string name;
    std::vector<bool> active;///< same length as button
  };

  /// data button info for observation dialogue
  struct Button {
    std::string name;
    std::string tooltip;
    int high,low;
    bool Default;
  };

  /// observation plot type
  struct PlotType {
    std::string name;
    std::vector<DataType> datatype;
    std::vector<Button> button;
    std::string misc;
    std::vector<int> pressureLevels; ///<Only used for PlotType "Pressure levels"
    std::vector<CriteriaList> criteriaList;
  };

  std::vector<PlotType> plottype;
  std::vector<PriorityList> priority;
  SliderValues density, size, timediff;
  std::string defValues;
};


struct ObsPositions {
  Area obsArea;
  int numObs;
  float* xpos;
  float* ypos;
  float* values;
  bool convertToGrid;
  ObsPositions():numObs(0),xpos(0),ypos(0),values(0), convertToGrid(true){}
};

//--------------------------------------------------
// Station structures
//--------------------------------------------------

/**
   \brief GUI data for all stations
*/

struct stationInfo {
    std::string name;
    std::string url;
    float lat;
    float lon;
};

struct stationSetInfo {
    std::string name;
    std::string url;
    std::string image;
    bool showvalue;
};

struct stationDialogInfo {
  std::vector<stationSetInfo> sets;
  std::map<std::string, bool> chosen;
  std::string selected;
};

//--------------------------------------------------
// Objects/Edit
//--------------------------------------------------

/**
   \brief data for one edit tool
*/
struct editToolInfo {
  std::string name;
  int index;
  std::string colour;
  std::string borderColour;
  int sizeIncrement;
  bool spline;
  std::string linetype;
  std::string filltype;

};

/**
  \brief global define for the ComplexPressureText tool
*/

#ifndef TOOL_DECREASING
#define TOOL_DECREASING "Decreasing pressure"
#endif

#ifndef TOOL_INCREASING
#define TOOL_INCREASING "Increasing pressure"
#endif



/**
   \brief data for one edit mode (field-editing, object-drawing)
*/
struct editModeInfo {
  std::string editmode;
  std::vector<editToolInfo> edittools;
};

/**
   \brief data for one map mode (normal, editing)
*/
struct mapModeInfo {
  std::string mapmode;
  std::vector<editModeInfo> editmodeinfo;
};

/**
   \brief data for all edititing functions
*/
struct EditDialogInfo {
  std::vector<mapModeInfo> mapmodeinfo;
};


/**
   \brief metadata on one map area
*/
struct selectArea{
  std::string name; ///<name of area ie. VV, Hordaland etc.
  bool selected; ///<
  int id;        ///<id of group(areaObjects)
};

//--------------------------------------------------

/**
   \brief geo image data value in one position
*/
struct SatValues{
  std::string channel;
  float value; ///<rgb
  std::string text; ///<palette
};


/**
   \brief quick menu type
*/
struct QuickMenuDefs {
  std::string filename;
};


/**
   \brief metadata on one data layer (on map)
*/
struct PlotElement {
  std::string type;
  std::string str;
  std::string icon;
  bool enabled;
  PlotElement():enabled(false){}
  PlotElement(std::string t,std::string s,std::string i,bool e):
    type(t),str(s),icon(i),enabled(e){}
};

/**
   \brief text information sources
*/
struct InfoFile {
  std::string name;    ///< name of source
  std::string filename;///< name of file
  std::string doctype; ///< documenttype: auto,xml,html,text
  std::string fonttype;///< fonttype: auto, fixed
};

/**
   \brief front types and options
 */
enum frontType {
    Cold, Warm, Occluded, Stationary, TroughLine, ArrowLine, SquallLine, SigweatherFront, Line
};

/**
   \brief information about polylines
 */
struct PolyLineInfo {
  int id;
  std::string name;
  std::vector<LonLat> points;
  PolyLineInfo(int i, std::string n, std::vector<LonLat> p)
    : id(i), name(n), points(p) {}
};

#endif
