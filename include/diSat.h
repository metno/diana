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

#ifndef diSat_h
#define diSat_h

//#ifndef DEBUGPRINT
//  #define DEBUGPRINT
//#endif

#include <diField/diArea.h>
#include <puTools/miString.h>
#include <puTools/miTime.h>
#include <diCommonTypes.h>


using namespace std;

/**

  \brief Satellite and radar data
    
  Contains data neded to plot and manage satellite and radar images
*/
class Sat {
public:
  // set from SatManager::init()
  bool approved;        ///< approved for plotting
  miString satellite;   ///< main product name
  miString filetype;    ///< subproduct
  miString formatType;  ///< filetype (mitiff or hdf5)
  miString metadata;
  miString channelInfo;
  miString paletteinfo;
  int hdf5type;
  vector<miString> vch; ///< name of channels selected
  miString filename;    ///< explicit selection of file
  miString actualfile;  ///< actual filename used
  bool autoFile;        ///< filename from plot time 
  float cut;            ///< image cut/stretch factor
  int alphacut;          ///< alpha-blending cutoff value
  int alpha;             ///< alpha-blending value
  int maxDiff;          ///< max allowed timedifference in minutes
  bool classtable;      ///< use classtable
  int nx;               ///< horizontal size of image
  int ny;               ///< vertical size of image
  Area area;            ///< Satellite area/projection
  miTime time;          ///< valid time
  miString annotation;  ///< annotation string
  miString plotname;    ///< unique plotname
  bool palette;         ///< palette sat-file
  miString plotChannels;///< channelname for annotation
  bool mosaic;          ///< make mosaic plot 
  miTime firstMosaicFileTime; ///< time start for mosaic image generation
  miTime lastMosaicFileTime;  ///< time stop for mosaic image generation
  miString satellite_name; ///< name of satellite from file
  bool commonColourStretch;    /// other images can use stretch from this image

  //grid
  float TrueLat; ///< grid true latitude
  float GridRot; ///< grid rotation
  float Ax;      ///< grid parameter
  float Ay;      ///< grid parameter
  float Bx;      ///< grid parameter
  float By;      ///< grid parameter

  // calibration
  //Strings from file header
  miString cal_vis;            /// calibration info visible channel
  miString cal_ir;             /// calibration info ir channel
  vector<miString> cal_table;  /// calibration info 

  struct table_cal{
    miString channel;
    vector<miString> val;
    float a,b;
  };
  map<int,table_cal> calibrationTable; /// calibration of current channels
  vector<miString> cal_channels;  /// name++ of current channels

  /// colour palette info 
  struct Palette {
    miString name;           ///< name of palette
    int noofcl;              ///< number of colours
    vector<miString> clname; ///< names of colour classes
    int cmap[3][256];        ///< rgb value map
  };

  Palette paletteInfo;       ///< Palette info

  enum { maxch=16 };         ///< maximum number of channels

  unsigned char *image; ///< Image ready for plot
  int index[maxch];     ///< index of plotted channels
  int no;               ///< no of plotted channels
  int rgbindex[3];      ///< channelindex for rgb-operations
  unsigned char* rawimage[maxch]; ///< raw image
  float* origimage[3]; ///< original image for temperature display images
  int rawchannels[maxch];         ///< raw images channel numbers
  
  int calibidx;         ///< channel to use in values routine
  /// calibration coefficients for channel values
  struct calib{
    miString channel;  ///< GUI text
    float a,b; ///< calibration coeff.
  };
  calib cal[maxch]; ///< calibration data for all channels
  
  /// any images defined
  bool noimages(){return !(image && rawimage);}
  
  // flags to indicate changes in parameters
  bool channelschanged;    ///< changes in selected channels
  bool rgboperchanged;     ///< changes in rgb-operation parameters
  bool alphaoperchanged;   ///< changes in alpha-operation params
  bool mosaicchanged;      ///< changes in mosaic parameters

  vector<int> hideColor;   ///< colour values to hide

  /// set default values from a SatDialogInfo
  static void setDefaultValues(const SatDialogInfo &); 

private:
  // Copy members
  void memberCopy(const Sat& rhs);
  static float defaultCut;
  static int defaultAlphacut;
  static int defaultAlpha;
  static int defaultTimediff;
  static bool defaultClasstable;

public:
  // Constructors
  Sat();
  Sat(const Sat &rhs);
  Sat(const miString & pin);
  // Destructor
  ~Sat();

  // Assignment operator
  Sat& operator=(const Sat &rhs);
  // Equality operator
  bool operator==(const Sat &rhs) const;

  void cleanup();
  void values(int x,int y,vector<SatValues>& satval);
  void setCalibration();
  void setAnnotation();
  void setPlotName();
  void setArea();

};

#endif
