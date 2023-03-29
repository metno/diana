/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2020 met.no

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

/*
  DESCRIPTION:    Plots classification table for classified data
 */

#include "diana_config.h"

#include "diGLPainter.h"
#include "diImageGallery.h"
#include "diLegendPlot.h"
#include "util/string_util.h"

#include "mi_fieldcalc/math_util.h"

#include <puTools/miStringFunctions.h>

#include <cmath>

#include "polyStipMasks.h"

#define MILOGGER_CATEGORY "diana.LegendPlot"
#include <miLogger/miLogging.h>

using namespace miutil;

namespace {
static void getStringSize(DiGLPainter* gl, const std::string& str, float& width, float& height)
{
  gl->getTextSize(str, width, height);
  height *= 1.2; // FIXME
}
} // namespace

LegendPlot::LegendPlot(const std::string& str)
{
  METLIBS_LOG_SCOPE();

  std::string sstr(str);
  miutil::replace(sstr, '"',' ');
  miutil::trim(sstr);
  std::vector<std::string> vstr = miutil::split(sstr, "=");
  if(vstr.size()==2){
    std::vector<std::string> tokens = miutil::split(vstr[1], ";", false);
    int n=tokens.size();
    if(n>0){
      if (poptions.tableHeader)
        titlestring = tokens[0];
      for (int i = 1; i + 2 < n; i += 3) {
        ColourCode cc;
        cc.colour = Colour(tokens[i]);
        cc.pattern = tokens[i+1];
        cc.colourstr = tokens[i+2];
        // if string starts with '|', do not plot colour/pattern box
        if (diutil::startswith(cc.colourstr, "|")) {
          cc.plotBox = false;
          miutil::remove(cc.colourstr, '|');
        } else {
          cc.plotBox = true;
        }
        colourcodes.push_back(cc);
      }
    }
  }
}

LegendPlot::~LegendPlot()
{
}

void LegendPlot::calculateSizes(DiGLPainter* gl, float& xborder, float& yborder, float& tablewidth, float& titlewidth, float& maxheight,
                                std::vector<std::string>& vtitlestring)
{
  gl->setFont(poptions.fontname, poptions.fontface, poptions.fontsize);
  getStringSize(gl, "c", xborder, yborder);
  xborder /= 2;
  yborder /= 2;

  tablewidth = titlewidth = maxheight = 0;
  vtitlestring.clear();

  if (colourcodes.empty())
    return;

  //colour code strings
  for (const auto& c : colourcodes) {
    float width, height;
    getStringSize(gl, c.colourstr + suffix, width, height);
    miutil::maximize(tablewidth, width);
    miutil::maximize(maxheight, height);
  }

  //title
  if (!titlestring.empty()) {
    float width, height;
    getStringSize(gl, titlestring, width, height);
    if (width > tablewidth) {
      const std::vector<std::string> vs = miutil::split(titlestring, " ");
      if (vs.size()>=5) {
        // handle field difference...
        std::string smove;
        int l, n= vs.size();
        for (int i=0; i<n; i++) {
          l= vs[i].length();
          if (l==1 && vs[i]=="(" && i<n-1) {
            smove="( ";
          } else if (l==1 && vs[i]=="-" && i<n-1) {
            smove="- ";
          } else if (l==1 && vs[i]==")" && vtitlestring.size()>0)  {
            int j= vtitlestring.size();
            vtitlestring[j-1] += " )";
          } else if (!smove.empty()) {
            vtitlestring.push_back(smove + vs[i]);
            smove.clear();
          } else {
            vtitlestring.push_back(vs[i]);
          }
        }
      } else {
        vtitlestring = vs;
      }
    } else {
      vtitlestring.push_back(titlestring);
    }
    for (const auto& t : vtitlestring) {
      getStringSize(gl, t, width, height);
      miutil::maximize(titlewidth, width);
      miutil::maximize(maxheight, height);
    }
    titlewidth += 2 * xborder;
  }

  tablewidth += 7 * xborder;
  miutil::maximize(titlewidth, tablewidth);
}

bool LegendPlot::plotLegend(DiGLPainter* gl, float x, float y)
{
  METLIBS_LOG_SCOPE();
  // fill the table with colours and textstrings from palette information

  float xborder, yborder, tablewidth, titlewidth, maxheight;
  std::vector<std::string> vtitlestring;
  calculateSizes(gl, xborder, yborder, tablewidth, titlewidth, maxheight, vtitlestring);

  // position table
  const float titleheight = maxheight * vtitlestring.size();
  const float tableheight = maxheight * colourcodes.size() + 2 * yborder;

  float x1title = x;
  float x2title = x + titlewidth;
  float y2title = y;
  float y1title = y - titleheight;

  float x1table = 0.0, x2table = 0.0;
  if(poptions.h_align==align_right){
    x1table = x2title-tablewidth;
  } else if(poptions.h_align==align_left){
    x1table = x1title;
  } else if(poptions.h_align==align_center){
    x1table = (x2title+x1title)/2.- tablewidth/2.;
  }
  x2table = x1table + tablewidth;
  float y1table = y1title-tableheight;

  //draw title background
  if (!vtitlestring.empty()) {
    gl->setColour(poptions.fillcolour, false);
    gl->drawRect(true, x1title, y2title, x2title, y1title);

    //draw title
    gl->setColour(poptions.textcolour);
    float titley1 = y2title-yborder-maxheight/2;
    for (const auto& t : vtitlestring) {
      gl->drawText(t, (x1title + xborder), titley1);
      titley1 -= maxheight;
    }
  }

  // draw table background
  gl->Enable(DiGLPainter::gl_BLEND);
  gl->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);
  gl->setColour(poptions.fillcolour);
  gl->drawRect(true, x1table, y1table, x2table, y1title);

  // draw table
  float x1box = x1table + xborder;
  float x2box = x1box + 4 * xborder;
  float y2box = y1title - yborder;
  float y1box = y2box - maxheight;
  ImageGallery ig;
  gl->Enable(DiGLPainter::gl_POLYGON_STIPPLE);
  for (const auto& c : colourcodes) {
    if (c.plotBox) {
      // draw colour/pattern box
      // draw background of colour/pattern boxes
      gl->setColour(poptions.fillcolour, false);
#if 1 // FIXME this is a strange painting call
      gl->Begin(DiGLPainter::gl_POLYGON);
      gl->Vertex2f(x1box, y1box);
      gl->Vertex2f(x1box, y2box);
      gl->Vertex2f(x2box, y2box);
      gl->Vertex2f(x2box, y1box);
      gl->End();
#else
      gl->drawRect(true, x1box, y1box, x2box, y2box);
#endif
      if (!c.pattern.empty()) {
        DiGLPainter::GLubyte* p = ig.getPattern(c.pattern);
        if (p == 0)
          gl->PolygonStipple(solid);
        else
          gl->PolygonStipple(p);
      } else {
        gl->PolygonStipple(solid);
      }
      gl->setColour(c.colour);
      gl->drawRect(true, x1box, y1box, x2box, y2box);

      // draw border of colour/pattern box
      gl->drawRect(false, x1box, y1box, x2box, y2box);
    }

    // draw textstring
    gl->setColour(poptions.textcolour);
    gl->drawText(c.colourstr + suffix, (x2box + xborder), (y1box + 0.8 * yborder));
    y2box -= maxheight;
    y1box -= maxheight;
  }
  gl->Disable(DiGLPainter::gl_POLYGON_STIPPLE);
  gl->Disable(DiGLPainter::gl_BLEND);

  return true;
}

float LegendPlot::height(DiGLPainter* gl)
{
  float xborder, yborder, table_width, title_width, row_height;
  std::vector<std::string> vtitlestring;
  calculateSizes(gl, xborder, yborder, table_width, title_width, row_height, vtitlestring);

  const float titleheight = row_height * vtitlestring.size();
  const float tableheight = row_height * colourcodes.size() + yborder;

  return (tableheight + titleheight);
}

float LegendPlot::width(DiGLPainter* gl)
{
  float xborder, yborder, tablewidth, titlewidth, maxheight;
  std::vector<std::string> vtitlestring;
  calculateSizes(gl, xborder, yborder, tablewidth, titlewidth, maxheight, vtitlestring);

  return titlewidth;
}
