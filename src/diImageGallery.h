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

#include <GL/gl.h> // for GLubyte

#include <map>
#include <string>
#include <vector>

class StaticPlot;

/**
   \brief image cache and OpenGl plotting

   the image gallery is a cache of images used in Diana. Inherits
   Plot, so is also the plotting engine for generic images
*/
class ImageGallery {
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
    std::vector<float>x;
    std::vector<float>y;
  };

  /// Image data (binary)
  struct image {
    std::string name;
    std::string filename;
    std::string markerFilename;
    bool alpha;
    int width;
    int height;
    unsigned char* data;
    std::vector<Line> line;
    int type;
    bool read_error;
    image();
    ~image();
    void erase();
  };

  /// Pattern data (binary)
  struct pattern {
    std::string name;
    std::string filename;
    GLubyte* pattern_data;
    bool read_error;
    pattern();
    ~pattern();
    void erase();
  };

private:
  static std::map<std::string,image> Images;
  static std::map<std::string,pattern> Patterns;
  static std::map< int, std::vector<std::string> > Type;

  bool plotImage_(StaticPlot* sp, const std::string name,         // OpenGL-plotting
      const float& gx, const float& gy,
      const float scalex,
      const float scaley,
      const int alpha);

  bool plotMarker_(StaticPlot* sp, const std::string name,         // OpenGL-plotting
      const float& x, const float& y,
      const float scale);

  bool readImage(const std::string& name); //read data from file once
  bool readPattern(const std::string& name); //read data from file once

  void addImageName(const std::string& filename, int type);

  bool readFile(const std::string name, const std::string filename);

  bool addImage(const image& im);    // add image


public:
  ImageGallery();
  static void clear();               ///< clear all images

  /// add image
  bool addImage(const std::string& name,
      const int w,
      const int h,
      const unsigned char* d,
      const bool a);
  /// add pattern
  bool addPattern(const std::string& name,
      const unsigned char* d);
  /// remove imega
  bool delImage(const std::string& name);
  /// remove pattern
  bool delPattern(const std::string& name);

  /// image width in GL coor
  float width_(const std::string& name);
  /// image height in GL coor
  float height_(const std::string& name);
  /// image width in pixels
  int   widthp(const std::string& name);
  /// image height in pixels
  int   heightp(const std::string& name);

  /// plot one image at gl-pos
  bool plotImage(StaticPlot* sp, const std::string& name,
      const float& x, const float& y,
      const bool center = true,
      const float scale = 1.0,
      const int alpha = 255);
  /// plot several images at gl-positions (different images)
  bool plotImages(StaticPlot* sp, const int n,
      const std::vector<std::string>& vn,
      const float* x, const float* y,
      const bool center = true,
      const float scale = 1.0,
      const int alpha = 255);
  /// plot several images at gl-positions (same image)
  bool plotImages(StaticPlot* sp, const int n,
      const std::string& name,
      const float* x, const float* y,
      const bool center = true,
      const float scale = 1.0,
      const int alpha = 255);
  /// plot one image at pixel-pos
  bool plotImageAtPixel(StaticPlot* sp, const std::string& name,
      const float& x, const float& y,
      const bool center = true,
      const float scale = 1.0,
      const int alpha = 255);
  void printInfo() const;
  /// return all image-names of one type
  void ImageNames(std::vector<std::string>& vnames, int type) const;
  /// return filename
  std::string getFilename(const std::string& name, bool pattern=false);
  /// return binary pattern by name
  GLubyte* getPattern(std::string name);
  /// parse the images section of the setup file
  bool parseSetup();
};

#endif
