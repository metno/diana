/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017 met.no

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

#ifndef IMAGESINK_H
#define IMAGESINK_H

class QPainter;

class ImageSink
{
public:
  virtual ~ImageSink();
  virtual bool isPrinting() = 0;

  //! begin a page, possibly adding a new page
  virtual bool beginPage() = 0;
  virtual QPainter& paintPage() = 0;
  //! close page
  virtual bool endPage() = 0;
  virtual bool finish();
};

#endif // IMAGESINK_H
