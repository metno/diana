/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "diField/diPoint.h"
#include "diGLPainter.h"
#include "diVprofOptions.h"
#include "diVprofValues.h"

#include <QSize>

#include <vector>

class VprofOptions;

// water vapour saturation pressure table
//        vanndampens metningstrykk, ewt(41).
//        t i grader celsius: -100,-95,-90,...,90,95,100.
//            tc=...    x=(tc+105)*0.2    l=x
//            et=ewt(l)+(ewt(l+1)-ewt(l))*(x-l)
// => fetched from MetNo::Constants

/**
   \brief Plots the Vertical Profile diagram background without data
*/
class VprofDiagram
{
  enum Alignment { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT, ALIGN_DUCTINGMIN, ALIGN_DUCTINGMAX };
  enum Font { FONT_DEFAULT, FONT_SCALED };

public:
  // vertical table resolution in hPa
  static const int idptab = 5;

  // length of vertical tables (0-1300 hPa)
  static const int mptab = 261;

  void plotValues(int nplot, const VprofValues& values, bool isSelectedRealization);

private:
  // transforms (p, t) pair to (x,y) pair in screen coordinates
  diutil::PointF transformPT(float p, float t) const;

  diutil::PointF to_pixel(const diutil::PointF& xy) const;
  diutil::PointF to_pixel(float x, float y) const { return to_pixel(diutil::PointF(x, y)); }

  float p_to_y(float p) const;
  float dx_from_p(float p) const;
  float dx_from_y(float x) const;
  float t_from_xp(float x, float p) const;

  // transforms p to x in screen coordinates
  float transformP(float p) const;

  void drawPT(const std::vector<diutil::PointF>& pt);
  void drawPT(const std::vector<float>& p, const std::vector<float>& t);

  void drawPX(const std::vector<float>& pp, const std::vector<float>& v, float x0, float scale, float xlim0, float xlim1);

  void drawLine(const diutil::PointF& p1, const diutil::PointF& p2);
  void drawLine(float x1, float y1, float x2, float y2) { drawLine(diutil::PointF(x1, y1), diutil::PointF(x2, y2)); }

  void drawRect(bool fill, float x1, float y1, float x2, float y2);
  void drawText(const std::string& text, float x, float y, float angle = 0);

protected:
  float getTextWidth(const std::string& text) const;
  void setFontsize(float chy);

  //---------------------------------------------------------------

  float chxbas, chybas; // standard character size
  float chxlab, chylab; // labels etc. character size
  float chxtxt, chytxt; // text (name,time etc.) character size

  enum { XMIN, XMAX, YMIN, YMAX, NXYMINMAX };
  enum {
    BOX_TOTAL,    // n=0 : total
    BOX_PT_AXES,  // n=1 : p-t diagram
    BOX_PT,       // n=2 : p-t diagram with labels
    BOX_FL,       // n=3 : flight levels
    BOX_SIG_WIND, // n=4 : significant wind levels
    BOX_WIND,     // n=5 : wind (first if multiple)
    BOX_VER_WIND, // n=6 : vertical wind
    BOX_REL_HUM,  // n=7 : relative humidity
    BOX_DUCTING,  // n=8 : ducting
    BOX_TEXT,     // n=9 : text
    NBOXES
  };
  float xysize[NBOXES][NXYMINMAX]; // position of diagram parts
  std::vector<VprofText> vptext;   // info for text plotting

public:
  VprofDiagram(VprofOptions *vpop, DiGLPainter* gl);
  ~VprofDiagram();
  void changeNumber(int nprog);
  void plot();
  void plotText();
  void setPlotWindow(const QSize& size);

private:
  void setDefaults();
  void prepare();
  void condensationtrails();
  void plotDiagram();
  void fpInitStr(const std::string& str, float x, float y, float angle, float size, const Colour& c, Alignment format = ALIGN_LEFT, Font font = FONT_DEFAULT);
  void fpDrawStr(bool first=false);

  //-----------------------------------------------

  DiGLPainter* gl;
  VprofOptions *vpopt;

  bool diagramInList;
  DiGLPainter::GLuint drawlist;    // openGL drawlist

  int plotw, ploth;
  int plotwDiagram, plothDiagram;

  int numprog;

  bool newdiagram; // if new diagram (background) needed

  float pmin,pmax;
  float y1000, idy_1000_100, tan_tangle;
  Rectangle full;

  // cotrails : lines for evaluation of possibility for condensation trails
  //            (kondensstriper fra fly)
  //                 cotrails[0][k] : t(rw=0%)
  //                 cotrails[1][k] : t(ri=40%)
  //                 cotrails[2][k] : t(ri=100%)
  //                 cotrails[3][k] : t(rw=100%)
  float cotrails[4][mptab];
  bool  init_cotrails;

  struct fpStrInfo{
    std::string str;
    float x, y, angle;
    float size;
    Colour c;
    Alignment format;
    Font font;
  };

  std::vector<fpStrInfo> fpStr;
};

#endif
