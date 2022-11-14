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

#include "diana_config.h"

#include "qtImageGallery.h"
#include "diImageGallery.h"

#include "diUtilities.h"
#include <puTools/miStringFunctions.h>

#include <QDataStream>
#include <QFileInfo>

#define MILOGGER_CATEGORY "diana.ImageGallery"
#include <miLogger/miLogging.h>


std::map<std::string, QImage> QtImageGallery::Images;

QtImageGallery::QtImageGallery()
{
}

bool QtImageGallery::addImageToGallery(const std::string& name, const QImage& image)
{
  if (image.isNull()) {
    METLIBS_LOG_ERROR("qtImageGallery::addImageToGallery ERROR:" << " invalid image:" << name);
    return false;
  }
  ImageGallery ig;
  unsigned char* data= 0;
  int width=  image.width();
  int height= image.height();
  int numb= width*height*4;
  int k=0;
  bool addok= false;
  if (numb==0){
    METLIBS_LOG_ERROR("qtImageGallery::addImageToGallery ERROR:"
	   << " zero bytes in image:" << name);
    return false;
  } else {
    data= new unsigned char [numb];
    for (int j=height-1; j>=0; j--){
      for (int i=0; i<width; i++){
	QRgb p= image.pixel(i,j);
	data[k+0]= qRed(p);
	data[k+1]= qGreen(p);
	data[k+2]= qBlue(p);
	data[k+3]= qAlpha(p);
	k+= 4;
      }
    }

    addok= ig.addImage(name,width,height,
		       data,true);
    delete[] data;
  }

  if (!addok) return false;

  // add image to local list
  Images[name] = image;

  return true;
}

bool QtImageGallery::addImageToGallery(const std::string& name, const std::string& imageStr)
{
  std::vector<std::string> vs = miutil::split(imageStr," ");
  int n=vs.size();
  QByteArray a(n,' ');
  for (int i=0; i<n; i++)
    a[i]= char(atoi(vs[i].c_str()));

  // qt4 fix: Using pointer to a as arg
  QDataStream s( &a, QIODevice::ReadOnly );  // open on a's data
  QImage image;
  s >> image;                       // read raw bindata

  return addImageToGallery(name, image);

}

void QtImageGallery::addImagesInDirectory(const std::string& dir)
{
//  METLIBS_LOG_DEBUG("============= globbing in:" << dir);
/* Image support in Qt
BMP Windows Bitmap Read/write 
GIF Graphic Interchange Format (optional) Read 
JPG Joint Photographic Experts Group Read/write 
JPEG Joint Photographic Experts Group Read/write 
PNG Portable Network Graphics Read/write 
PBM Portable Bitmap Read 
PGM Portable Graymap Read 
PPM Portable Pixmap Read/write 
XBM X11 Bitmap Read/write 
XPM X11 Pixmap Read/write
*/


  const diutil::string_v matches = diutil::glob(dir);
  for (diutil::string_v::const_iterator it = matches.begin(); it != matches.end(); ++it) {
    const std::string& fname = *it;
    if (not miutil::contains(fname, "~")) {
      QString filename = QString::fromStdString(fname);
      QFileInfo fileinfo(filename);
      QString name = fileinfo.baseName();
	  std::string format;
	  // sometimes Qt doesnt understand the format
	  if(miutil::contains(fname, ".xpm"))
		  format = "XPM";
	  else if(miutil::contains(fname, ".png"))
		  format = "PNG";
	  else if(miutil::contains(fname, ".jpg"))
		  format = "JPG";
	  else if(miutil::contains(fname, ".jpeg"))
		  format = "JPEG";
	  else if(miutil::contains(fname, ".gif"))
		  format = "GIF";
	  else if(miutil::contains(fname, ".pbm"))
		  format = "PBM";
	  else if(miutil::contains(fname, ".pgm"))
		  format = "PGM";
	  else if(miutil::contains(fname, ".ppm"))
		  format = "PPM";
	  else if(miutil::contains(fname, ".xbm"))
		  format = "XBM";
	  else if(miutil::contains(fname, ".bmp"))
		  format = "BMP";
	  QImage image;
	  if (!format.empty())
		  image.load(fname.c_str(),format.c_str());
	  else
		  image.load(fname.c_str(),NULL);
	  if (image.isNull())
	  {
		  METLIBS_LOG_ERROR("QtImageGallery::addImagesInDirectory: problem loading image: " << fname);
		  continue;
	  }
	  addImageToGallery(name.toStdString(),image);
    }
  }
}

void QtImageGallery::clear()
{
  ImageGallery ig;
  ig.clear();

  Images.clear();
}


bool QtImageGallery::delImage(const std::string& name)
{
  ImageGallery ig;

  if ((Images.count(name) == 0) || (!ig.delImage(name)))
    return false;

  Images.erase(name);

  return true;
}

void QtImageGallery::ImageNames(std::vector<std::string>& vnames) const
{
  vnames.clear();

  std::map<std::string, QImage>::const_iterator p = Images.begin();

  for (; p!=Images.end(); p++)
    vnames.push_back(p->first);
}

bool QtImageGallery::Image(const std::string& name, QImage& image) // return QImage
{
  if (Images.count(name) == 0)
    return false;

  image= Images[name];
  return true;
}
