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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtImageGallery.h"
#include "diImageGallery.h"
#include <puCtools/glob.h>
#include <puCtools/glob_cache.h>
#include <QDataStream>
#include <QFileInfo>

map<miutil::miString,QImage> QtImageGallery::Images;

QtImageGallery::QtImageGallery()
{
}

bool QtImageGallery::addImageToGallery(const miutil::miString name,
				       const QImage& image)
{
  if (image.isNull()){
    cerr << "qtImageGallery::addImageToGallery ERROR:"
	 << " invalid image:" << name << endl;
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
    cerr << "qtImageGallery::addImageToGallery ERROR:"
	 << " zero bytes in image:" << name << endl;
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

bool QtImageGallery::addImageToGallery(const miutil::miString name,
				       miutil::miString& imageStr)
{
  vector<miutil::miString> vs= imageStr.split(" ");
  int n=vs.size();
  QByteArray a(n,' ');
  for (int i=0; i<n; i++)
    a[i]= char(atoi(vs[i].cStr()));

  // qt4 fix: Using pointer to a as arg
  QDataStream s( &a, QIODevice::ReadOnly );  // open on a's data
  QImage image;
  s >> image;                       // read raw bindata

  return addImageToGallery(name, image);

}

void QtImageGallery::addImagesInDirectory(const miutil::miString& dir){
//  cerr << "============= globbing in:" << dir << endl;
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


  glob_t globBuf;
  glob_cache(dir.c_str(),0,0,&globBuf);
  for( int k=0; k<globBuf.gl_pathc; k++) {
    miutil::miString fname = globBuf.gl_pathv[k];
    if( !fname.contains("~") ){
      QString filename = fname.c_str();
      QFileInfo fileinfo(filename);
      QString name = fileinfo.baseName();
	  miutil::miString format;
	  // sometimes Qt doesnt understand the format
	  if(fname.contains(".xpm"))
		  format = "XPM";
	  else if(fname.contains(".png"))
		  format = "PNG";
	  else if(fname.contains(".jpg"))
		  format = "JPG";
	  else if(fname.contains(".jpeg"))
		  format = "JPEG";
	  else if(fname.contains(".gif"))
		  format = "GIF";
	  else if(fname.contains(".pbm"))
		  format = "PBM";
	  else if(fname.contains(".pgm"))
		  format = "PGM";
	  else if(fname.contains(".ppm"))
		  format = "PPM";
	  else if(fname.contains(".xbm"))
		  format = "XBM";
	  else if(fname.contains(".bmp"))
		  format = "BMP";
	  QImage image;
	  if (!format.empty())
		  image.load(fname.c_str(),format.c_str());
	  else
		  image.load(fname.c_str(),NULL);
	  if (image.isNull())
	  {
		  cerr << "QtImageGallery::addImagesInDirectory: problem loading image: " << fname << endl;
		  continue;
	  }
	  addImageToGallery(name.toStdString(),image);
    }
  }
  globfree_cache(&globBuf);
}

void QtImageGallery::clear()
{
  ImageGallery ig;
  ig.clear();

  Images.clear();
}


bool QtImageGallery::delImage(const miutil::miString& name)
{
  ImageGallery ig;

  if ((Images.count(name) == 0) || (!ig.delImage(name)))
    return false;

  Images.erase(name);

  return true;
}

void QtImageGallery::ImageNames(vector<miutil::miString>& vnames) const
{
  vnames.clear();

  map<miutil::miString,QImage>::const_iterator p= Images.begin();

  for (; p!=Images.end(); p++)
    vnames.push_back(p->first);
}

bool QtImageGallery::Image(const miutil::miString& name,
			   QImage& image) // return QImage
{
  if (Images.count(name) == 0)
    return false;

  image= Images[name];
  return true;
}
