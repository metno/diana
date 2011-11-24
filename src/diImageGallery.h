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
#ifndef _diImageGallery_h
#define _diImageGallery_h


#include <diPlot.h>
#include <puTools/miString.h>
#include <vector>
#include <map>
#include <GL/gl.h>

using namespace std; 

/**
   \brief image cache and OpenGl plotting

   the image gallery is a cache of images used in Diana. Inherits Plot, so is also the plotting engine for generic images

*/

class ImageGallery : public Plot {
public:
  /// type of image
  enum type{
    basic   = 0,
    marker  = 1,
    fillpattern = 2
  };
  /// line data for simple vector/marker drawings
  struct Line {
    int width;
    bool fill;
    vector<float>x;
    vector<float>y;
  };

  /// Image data (binary)
  struct image {
    miutil::miString name;
    miutil::miString filename;
    miutil::miString markerFilename;
    bool alpha;
    int width;
    int height;
    unsigned char* data;
    vector<Line> line;
    int type;
    bool read_error;
    image();
    ~image();
    void erase();
  };

  /// Pattern data (binary)
  struct pattern {
    miutil::miString name;
    miutil::miString filename;
    GLubyte* pattern_data;
    bool read_error;
    pattern();
    ~pattern();
    void erase();
  };

private:
  static map<miutil::miString,image> Images;
  static map<miutil::miString,pattern> Patterns;
  static map< int, vector<miutil::miString> > Type;

  bool plotImage_(const miutil::miString name,         // OpenGL-plotting
		  const float& gx, const float& gy,
		  const float scalex,
		  const float scaley,
		  const int alpha);

  bool plotMarker_(const miutil::miString name,         // OpenGL-plotting
		   const float& x, const float& y,
		   const float scale);

  bool readImage(const miutil::miString& name); //read data from file once
  bool readPattern(const miutil::miString& name); //read data from file once

  void addImageName(const miutil::miString& filename, int type);

  bool readFile(const miutil::miString name, const miutil::miString filename);

  bool addImage(const image& im);    // add image


public:
  ImageGallery();
  static void clear();               ///< clear all images
  
  /// add image
  bool addImage(const miutil::miString& name,
		const int w,
		const int h,
		const unsigned char* d,
		const bool a);
  /// add pattern
  bool addPattern(const miutil::miString& name,
		  const unsigned char* d);
  /// remove imega
  bool delImage(const miutil::miString& name);
  /// remove pattern
  bool delPattern(const miutil::miString& name);

  /// image width in GL coor
  float width(const miutil::miString& name); 
  /// image height in GL coor
  float height(const miutil::miString& name);
  /// image width in pixels
  int   widthp(const miutil::miString& name);  
  /// image height in pixels
  int   heightp(const miutil::miString& name);

  /// plot one image at gl-pos
  bool plotImage(const miutil::miString& name,
		 const float& x, const float& y,
		 const bool center = true,
		 const float scale = 1.0,
		 const int alpha = 255);
  /// plot several images at gl-positions (different images)
  bool plotImages(const int n,
		  const vector<miutil::miString>& vn,
		  const float* x, const float* y,
		  const bool center = true,
		  const float scale = 1.0,
		  const int alpha = 255);
  /// plot several images at gl-positions (same image)
  bool plotImages(const int n,
		  const miutil::miString& name,
		  const float* x, const float* y,
		  const bool center = true,
		  const float scale = 1.0,
		  const int alpha = 255);
  /// plot one image at pixel-pos
  bool plotImageAtPixel(const miutil::miString& name,
			const float& x, const float& y,
			const bool center = true,
			const float scale = 1.0,
			const int alpha = 255);
  void printInfo() const;
  /// return all image-names of one type 
  void ImageNames(vector<miutil::miString>& vnames, int type) const; 
  /// return filename
  miutil::miString getFilename(const miutil::miString& name, bool pattern=false);
  /// return binary pattern by name
  GLubyte* getPattern(miutil::miString name);
  /// parse the images section of the setup file
  bool parseSetup();
};

#endif

