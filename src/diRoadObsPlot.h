/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2016 met.no

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

#ifndef DIANA_ROADOBSPLOT_H
#define DIANA_ROADOBSPLOT_H 1

#include "diObsPlot.h"

#ifdef ROADOBS
#ifdef NEWARK_INC
#include <newarkAPI/diStation.h>
#else
#include <roadAPI/diStation.h>
#endif
#endif // ROADOBS

class RoadObsPlot : public ObsPlot {
public:
  RoadObsPlot(const miutil::KeyValue_v& pin, ObsPlotType plottype);

protected:
  void plotIndex(DiGLPainter* gl, int index) override;

  void weather(DiGLPainter* gl, short int ww, float TTT, bool show_time_id, QPointF xy,
      float scale = 1, bool align_right = false) override;

  long findModificationTime(const std::string& fname) override;
  bool isFileUpdated(const std::string& fname, long now, long mod_time) override;

private:
  void plotRoadobs(DiGLPainter* gl, int index);
  void plotDBSynop(DiGLPainter* gl, int index);
  void plotDBMetar(DiGLPainter* gl, int index);

public:
  bool preparePlot();
};

#endif // DIANA_ROADOBSPLOT_H
