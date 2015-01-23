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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diImageIO.h>
#include <puTools/miStringFunctions.h>
#include <png.h>
#include <map>
#include <fstream>
#include <string.h>

#define MILOGGER_CATEGORY "diana.ImageIO"
#include <miLogger/miLogging.h>

using namespace imageIO;
using namespace::miutil;

bool imageIO::read_image(Image_data& img)
{
  if (miutil::contains(img.filename, ".png"))
    return read_png(img);
  else if (miutil::contains(img.filename, ".xpm"))
    return read_xpm(img);

  return false;
}




/*
  PNG routines
*/


bool imageIO::read_png(Image_data& img){
  METLIBS_LOG_INFO("--------- read_png: " << img.filename);

  FILE *fp = fopen(img.filename.c_str(), "rb");
  if (!fp){
    METLIBS_LOG_ERROR("read_png ERROR can't open file:" << img.filename);
    return false;
  }

  png_structp png_ptr = png_create_read_struct
    (PNG_LIBPNG_VER_STRING,
     (png_voidp)0,//user_error_ptr,
     0,//user_error_fn,
     0);//user_warning_fn);
  if (!png_ptr){
    METLIBS_LOG_ERROR("read_png ERROR creating png_struct");
    fclose(fp);
    return false;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr){
    png_destroy_read_struct(&png_ptr,
			    (png_infopp)NULL, (png_infopp)NULL);
    METLIBS_LOG_ERROR("read_png ERROR creating info_struct");
    fclose(fp);
    return false;
  }

  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info){
    png_destroy_read_struct(&png_ptr, &info_ptr,
			    (png_infopp)NULL);
    METLIBS_LOG_ERROR("read_png ERROR creating end_info_struct");
    fclose(fp);
    return false;
  }

     if (setjmp(png_jmpbuf(png_ptr))){
  //if (setjmp(png_ptr->jmpbuf)){
    png_destroy_read_struct(&png_ptr, &info_ptr,
			    &end_info);
    METLIBS_LOG_ERROR("read_png ERROR longjmp out of process");
    fclose(fp);
    return false;
  }

  png_init_io(png_ptr, fp);


  // do the read
  const int png_transforms = 0;
  png_read_png(png_ptr, info_ptr, png_transforms, NULL);

  png_uint_32 uwidth, uheight;
  int color_type;
  int bit_depth;
  int interlace_type;//=   PNG_INTERLACE_NONE;
  int compression_type;//= PNG_COMPRESSION_TYPE_DEFAULT;
  int filter_type;//=      PNG_FILTER_TYPE_DEFAULT;

  png_get_IHDR(png_ptr, info_ptr, &uwidth, &uheight,
	       &bit_depth, &color_type, &interlace_type,
	       &compression_type, &filter_type);
  img.width= uwidth;
  img.height= uheight;

  //   METLIBS_LOG_DEBUG("image width:" << img.width);
  //   METLIBS_LOG_DEBUG("image height:" << img.height);
  //   METLIBS_LOG_DEBUG("image bit_depth:" << bit_depth);
  //   METLIBS_LOG_DEBUG("image color_type:" << color_type);

  img.nchannels=4;
  if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
    img.nchannels= 4;
  else if (color_type == PNG_COLOR_TYPE_RGB)
    img.nchannels= 3;
  else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    img.nchannels= 4;
  else if (color_type == PNG_COLOR_TYPE_GRAY)
    img.nchannels= 3;
  else if (color_type == PNG_COLOR_TYPE_PALETTE){
    METLIBS_LOG_ERROR("PNG_COLOR_TYPE_PALETTE"
	 << " ..exiting");
    return false;
  } else {
    METLIBS_LOG_ERROR("Unknown color_type:" << color_type
	 << " ..exiting");
    return false;
  }

  //METLIBS_LOG_INFO("image nchannels:" << img.nchannels);


  png_bytep *row_pointers;
  row_pointers = png_get_rows(png_ptr, info_ptr);

  //png_read_image(png_ptr, row_pointers);
  //png_read_end(png_ptr, end_info);

  // unpack image from row-based structure
  // New 2004-02-03: force alpha-channel!
  if (img.nchannels != 4){
    img.data= new unsigned char [img.width*img.height*4];
    int bp=0;
    for (int i=img.height-1; i>=0; i--){
      for (int j=0, k=0; j<img.width*img.nchannels; j++,k++,bp++){
	img.data[bp] = row_pointers[i][j];
	if (k==2){
	  bp++;
	  img.data[bp] = 255;
	  k=-1;
	}
      }
    }
    img.nchannels = 4;

  } else {
    img.data= new unsigned char [img.width*img.height*img.nchannels];
    int bp=0;
    for (int i=img.height-1; i>=0; i--){
      for (int j=0; j<img.width*img.nchannels; j++,bp++){
	img.data[bp] = row_pointers[i][j];
      }
    }
  }

  // clean up
  png_destroy_read_struct(&png_ptr, &info_ptr,
			  &end_info);
  return true;
}



bool imageIO::write_png(const Image_data& img){
  METLIBS_LOG_INFO("--------- write_png: " << img.filename);

  // open file for write
  FILE *fp = fopen(img.filename.c_str(), "wb");
  if (!fp) {
    METLIBS_LOG_ERROR("write_png ERROR can't open file:" << img.filename);
    return false;
  }

  // create png struct (private)
  png_structp png_ptr = png_create_write_struct
    (PNG_LIBPNG_VER_STRING, (png_voidp)0, 0, 0);

  if (!png_ptr){
    METLIBS_LOG_ERROR("write_png ERROR creating png_struct");
    fclose(fp);
    return false;
  }

  // create info struct
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_write_struct(&png_ptr,
			     (png_infopp)NULL);
    METLIBS_LOG_ERROR("write_png ERROR creating info_struct");
    fclose(fp);
    return false;
  }

#ifdef linux
  if (setjmp(png_jmpbuf(png_ptr))) {
#else
  if (setjmp(png_ptr->jmpbuf)) {
#endif
    png_destroy_write_struct(&png_ptr, &info_ptr);
    METLIBS_LOG_ERROR("write_png ERROR longjmp out of process");
    fclose(fp);
    return false;
  }

  png_init_io(png_ptr, fp);

  const int color_type=       (img.nchannels==4 ?
			       PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB);
  const int bit_depth=        8;
#ifdef linux
  const int interlace_type=   PNG_INTERLACE_NONE;
  const int compression_type= PNG_COMPRESSION_TYPE_DEFAULT;
  const int filter_type=      PNG_FILTER_TYPE_DEFAULT;
#else
  const int interlace_type=   0;
  const int compression_type= 0;
  const int filter_type=      0;
#endif

  // set all info
  png_set_IHDR(png_ptr, info_ptr, img.width, img.height,
	       bit_depth, color_type, interlace_type,
	       compression_type, filter_type);


  // pack image into row-based structure
  png_byte **row_pointers;
  row_pointers= new png_byte*[img.height];
  int bp=0;
  for (int i=img.height-1; i>=0; i--){
    row_pointers[i]= new png_byte[img.width*img.nchannels];
    for (int j=0; j<img.width*img.nchannels; j++,bp++){
      row_pointers[i][j]= img.data[bp];
    }
  }

//#ifdef THEHARDWAY
  // write info to file
  png_write_info(png_ptr, info_ptr);

//   png_write_IHDR(png_ptr, width, height, bit_depth,
// 		 color_type, compression_type, filter_type,
// 		 interlace_type);


  // write image to file
  png_write_image(png_ptr, row_pointers);

  // write any trailing info-data
  png_write_end(png_ptr, info_ptr);

//#else
//
//  int png_transforms=0;
//  png_set_rows(png_ptr, info_ptr, row_pointers);
//  png_write_png(png_ptr, info_ptr, png_transforms, png_voidp_NULL);
//
//#endif

  // clean up
  png_destroy_write_struct(&png_ptr, &info_ptr);

  for (int i=0; i<img.height; i++)
    delete[] row_pointers[i];
  delete[] row_pointers;

  fclose(fp);
  return true;
}




/*
  XPM - routines
*/


// convert a HEX-character (0-9,A-F) to int
int chartoint_(const char c)
{
  const int zv= int('0');
  const int nv= int('9');
  const int av= int('A');
  const int fv= int('F');

  int v= int(c);

  if (v >= zv && v<= nv)
    return v-zv;
  else if (v >= av && v <= fv)
    return v-av+10;

  // illegal character
  return 0;
}

int hexToInt(const std::string& p)
{
  int l= p.length(), res=0, fact=1;
  for (int i=l-1; i>=0; i--,fact*=15)
    res += chartoint_(p[i])*fact;

  return res;
}


bool imageIO::imageFromXpmdata(const char** xd, Image_data& img)
{
  const std::vector<std::string> vs = miutil::split(xd[0], 0, " ");
  if (vs.size() < 4) {
    METLIBS_LOG_ERROR("imageFromXpmdata ERROR too few elements:" << xd[0]);
    return false;
  }
  const int xsize = miutil::to_int(vs[0]);
  const int ysize = miutil::to_int(vs[1]);
  const int ncols = miutil::to_int(vs[2]);
  const int nchar = miutil::to_int(vs[3]);

  if (xsize < 1 || ysize < 1 || ncols < 1 || nchar < 1){
    METLIBS_LOG_ERROR("imageFromXpmdata ERROR Illegal numbers "
	 << " xsize:" << xsize << " ysize:" << ysize
	 << " ncols:" << ncols << " nchar:" << nchar
	);
    return false;
  }

  std::map<std::string, int> redmap, greenmap, bluemap, alphamap;

  for (int i=0; i<ncols; i++){
    const std::string buf = xd[1+i];
    int j= buf.find_last_of("c");
    if (j < 0){
      METLIBS_LOG_ERROR("imageFromXpmdata ERROR Illegal colourdefinition:"
	   << buf);
      return false;
    }
    const std::string key = buf.substr(0,nchar);
    std::string colour = miutil::trimmed(buf.substr(j+1,buf.length()-j-1));
    if (colour == "None"){
      redmap[key]  = 255;
      greenmap[key]= 255;
      bluemap[key] = 255;
      alphamap[key]= 0;
    } else {
      if (colour.size() < 2){
	METLIBS_LOG_ERROR("imageFromXpmdata ERROR Illegal colourdefinition:"
	     << buf);
	return false;
      }
      colour = colour.substr(1,colour.length()-1);
      int numcomp = colour.length()/3;
      redmap[key]  = hexToInt(colour.substr(0,numcomp));
      greenmap[key]= hexToInt(colour.substr(numcomp,numcomp));
      bluemap[key] = hexToInt(colour.substr(numcomp*2,numcomp));
      alphamap[key]= 255;
    }
  }

  // data
  img.width = xsize;
  img.height= ysize;
  img.nchannels= 4;
  img.data= new unsigned char [img.width*img.height*img.nchannels];
  int pp= 0;
  for (int y=ysize-1; y>=0; y--){
    const std::string line = xd[y+ncols+1];
    for (int x=0; x<xsize*nchar; x+=nchar){
      const std::string pixel = line.substr(x,nchar);
      img.data[pp+0]= redmap[pixel];
      img.data[pp+1]= greenmap[pixel];
      img.data[pp+2]= bluemap[pixel];
      img.data[pp+3]= alphamap[pixel];
      pp+= 4;
    }
  }

  return true;
}

bool imageIO::patternFromXpmdata(const char** xd, Image_data& img)
{
  const std::vector<std::string> vs = miutil::split(xd[0], 0, " ");
  if (vs.size() < 4) {
    METLIBS_LOG_ERROR("imageFromXpmdata ERROR too few elements:" << xd[0]);
    return false;
  }
  int xsize = miutil::to_int(vs[0]);
  int ysize = miutil::to_int(vs[1]);
  const int ncols = miutil::to_int(vs[2]);
  const int nchar = miutil::to_int(vs[3]);

  if (xsize < 32 || ysize < 32 || ncols < 2 || nchar < 1){
    METLIBS_LOG_ERROR("patternFromXpmdata ERROR Illegal numbers "
	 << " xsize:" << xsize << " ysize:" << ysize
	 << " ncols:" << ncols << " nchar:" << nchar
	);
    return false;
  }

  // if size > 32x32, truncate
  xsize=ysize=32;

  //Colour
  //  noneKey is first colour key, or key with colour "None" -> bit=0
  // all other keys -> bit=1
  std::string noneKey;
  for (int i=0; i<ncols; i++){
    const std::string buf = xd[1+i];
    int j= buf.find_last_of("c");
    if (j < 0){
      METLIBS_LOG_ERROR("imageFromXpmdata ERROR Illegal colourdefinition:"
	   << buf);
      return false;
    }
    const std::string colour = miutil::trimmed(buf.substr(j+1,buf.length()-j-1));
    if (colour == "None"){
      noneKey = buf.substr(0, nchar);
      break;
    }
    if (i==0)
      noneKey = buf.substr(0, nchar);
  }

  // data
  img.width = xsize;
  img.height = ysize;
  img.data = new unsigned char [128];

  for (int i=0; i<128; i++) img.data[i]=0;

  int ii=0;
  for (int y=31; y>=0; y--) {
    const std::string line = xd[y+ncols+1];
    for (int x=0; x<32*nchar;) {
      for (int i=0; i<8; i++, x+=nchar) {
	std::string pixel = line.substr(x, nchar);
	if (pixel != noneKey) {
	  ii=(31-y)*4 + x/8;
	  img.data[ii] = img.data[ii] | (0x01 << (7-i));
	}
      }
    }
  }

  return true;
}




bool imageIO::read_xpm(Image_data& img)
{
  METLIBS_LOG_INFO("--------- read_xpm: " << img.filename);

  std::ifstream file(img.filename.c_str());

  if (!file) {
    METLIBS_LOG_ERROR("readXpmFile ERROR: Unable to open file:" << img.filename);
    return false;
  }

  std::string buf;
  std::vector<std::string> vs;

  while(getline(file,buf)){
    miutil::trim(buf);
    if (buf.length() == 0)
      continue;
    if (buf[0]!='\"')
      continue;
    int i= buf.find_last_of("\"");
    buf= buf.substr(1,i-1);
    vs.push_back(buf);
  }
  if (vs.empty())
    return false;

  //   METLIBS_LOG_DEBUG("RESULTING DATA:");
  char **data = new char*[vs.size()];
  for (unsigned int i=0; i<vs.size(); i++){
    data[i]= strdup(vs[i].c_str());
    //     METLIBS_LOG_DEBUG(data[i]);
  }

  bool res;
  if(img.pattern)
    res=  patternFromXpmdata(const_cast<const char**>(data),img);
  else
    res=  imageFromXpmdata(const_cast<const char**>(data),img);

  // OBS: free !!!!!!!!!!!!!!!!!!!!!!!!

  delete[] data;

  return res;
}
