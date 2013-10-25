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
#ifndef _qtImageGallery_h
#define _qtImageGallery_h

#include <qimage.h>
#include <map>

/**
   \brief the Qt::image cache

   the Qt::image gallery is a cache of Qt::images
*/
class QtImageGallery {
private:
  static std::map<std::string, QImage> Images;
public:
  QtImageGallery();

  static void clear();                             ///< clear all images
  bool delImage(const std::string& name);          ///< remove image

  /// add a new QImage with name to gallery
  bool addImageToGallery(const std::string& name, const QImage& image);

  bool addImageToGallery(const std::string& name, std::string& imageStr);

  void addImagesInDirectory(const std::string& dir);

  void ImageNames(std::vector<std::string>& vnames) const; ///< return all image-names
  bool Image(const std::string& name, QImage& image); ///< return QImage
};

#endif // _qtImageGallery_h
