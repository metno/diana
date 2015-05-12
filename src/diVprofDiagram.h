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
#ifndef VPROFDIAGRAM_H
#define VPROFDIAGRAM_H

#include "diVprofTables.h"
#include "diVprofOptions.h"
#include "diGLPainter.h"

#include <vector>

class VprofOptions;

/**
   \brief Plots the Vertical Profile diagram background without data
*/
class VprofDiagram : public VprofTables
{
public:
  VprofDiagram(VprofOptions *vpop, DiGLPainter* gl);
  ~VprofDiagram();
  void changeOptions(VprofOptions *vpop);
  void changeNumber(int ntemp, int nprog);
  void plot();
  void plotText();
  void setPlotWindow(int w, int h);

private:
  void setDefaults();
  void prepare();
  void condensationtrails();
  void plotDiagram();
  void fpInitStr(const std::string& str,
      float x, float y, float z,
      float size,
      Colour c,
      std::string format="",
      std::string font="");
  void fpDrawStr(bool first=false);

  //-----------------------------------------------

  DiGLPainter* gl;
  VprofOptions *vpopt;

  bool diagramInList;
  DiGLPainter::GLuint drawlist;    // openGL drawlist

  int plotw, ploth;
  int plotwDiagram, plothDiagram;

  int numtemp, numprog;

  bool newdiagram; // if new diagram (background) needed

  float pmin,pmax;
  int kpmin,kpmax;

  // cotrails : lines for evaluation of possibility for condensation trails
  //            (kondensstriper fra fly)
  //                 cotrails[0][k] : t(rw=0%)
  //                 cotrails[1][k] : t(ri=40%)
  //                 cotrails[2][k] : t(ri=100%)
  //                 cotrails[3][k] : t(rw=100%)
  float cotrails[4][mptab];

  bool  init_tables;
  bool  init_cotrails;
  int   last_diagramtype;
  float last_tangle;
  float last_rsvaxis;
  float last_pmin;
  float last_pmax;

  struct fpStrInfo{
    std::string str;
    float x,y,z;
    float size;
    Colour c;
    std::string format;
    std::string font;
  };

  std::vector<fpStrInfo> fpStr;
};

#endif
