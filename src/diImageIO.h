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
#ifndef _diImageIO_h
#define _diImageIO_h

#include <string>

namespace imageIO {

  /**
     \brief Image data for IO
  */

  struct Image_data {
    std::string filename;        // source filename
    int width;                // width of image
    int height;               // height of image
    int nchannels;            // 3=RGB, 4=RGBA
    unsigned char* data;      // image-data
    bool pattern;             // read image as a pattern
    Image_data()
      :filename(""),width(0),height(0),nchannels(0),data(0){}
    Image_data(const std::string& f)
      :filename(f),width(0),height(0),nchannels(0),data(0){}
    ~Image_data(){if (data) delete[] data;}
  };

  
  /**
     Generic image read, calls read_png or read_xpm
  */
  bool read_image(Image_data& img);
		  
  /**
     PNG routines
  */
  bool read_png(Image_data& img);
  bool write_png(const Image_data& img);
  
  
  /**
     XPM - routines
  */
  // image directly from xpm-data
  bool imageFromXpmdata(const char** xd, Image_data& img);
  bool patternFromXpmdata(const char** xd, Image_data& img);
  // from file
  bool read_xpm(Image_data& img);
  
} // namespace imageIO

#endif
