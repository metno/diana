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
#ifndef diSatPlot_h
#define diSatPlot_h

#include <diPlot.h>
#include <diSat.h>

#include <vector>

/**
  \brief Plot satellite and radar images
*/
class SatPlot : public Plot {
private:

  float xmin, xmax, ymin, ymax; //corners in maprect coordinates
  SatPlot(const SatPlot &rhs){}

  unsigned char * imagedata; // dataarray for resampling
  int previrs;               // previous resampling coeff.

  bool plotPixmap();
  bool plotFillcell();
  unsigned char * resampleImage(int& currwid, int& currhei,
      int& bmStartx, int& bmStarty,
      float& scales, float& scaley, int& nx, int& ny);

  GLuint texture;
  bool hasTexture;

public:
  SatPlot();
  ~SatPlot();

  Sat *satdata;

  void plot(PlotOrder zorder);
  void setData(Sat *);
  void clearData();
  Area& getSatArea(void){ return satdata->area;}
  double getGridResolutionX(){ return satdata->gridResolutionX;}
  double getGridResolutionY() {return satdata->gridResolutionY;}
  void getSatAnnotation(std::string &, Colour &);
  void getSatName(std::string &);
  void getCalibChannels(std::vector<std::string>& channels );
  ///get pixel value
  void values(float x,float y, std::vector<SatValues>& satval);
  ///get legend
  bool getAnnotations(std::vector<std::string>& anno);
  void setSatAuto(bool, const std::string&, const std::string&);
};

#endif
