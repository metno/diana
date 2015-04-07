/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2014 met.no

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

#include "diAnnotationPlot.h"
#include <diLegendPlot.h>
#include <diFontManager.h>
#include <diImageGallery.h>
#include <GL/gl.h>

#include <puTools/miStringFunctions.h>

#include <cmath>

#define MILOGGER_CATEGORY "diana.AnnotationPlot"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

AnnotationPlot::AnnotationPlot()
{
  METLIBS_LOG_SCOPE();
  init();
}

AnnotationPlot::AnnotationPlot(const std::string& po)
{
  METLIBS_LOG_SCOPE(LOGVAL(po));
  init();
  prepare(po);
}

AnnotationPlot::~AnnotationPlot()
{
  for (Annotation_v::iterator ita = annotations.begin(); ita != annotations.end(); ++ita) {
    for (element_v::iterator ite = ita->annoElements.begin(); ite != ita->annoElements.end(); ++ite) {
      delete ite->classplot;
      ite->classplot = 0;
    }
  }
}

void AnnotationPlot::init()
{
  plotRequested = false;
  isMarked = false;
  labelstrings = std::string();
  productname = std::string();
  editable = false;
  useAnaTime = false;
}

const std::string AnnotationPlot::insertTime(const std::string& s, const miTime& time)
{
  bool norwegian = true;
  std::string es = s;
  if (miutil::contains(es, "$")) {
    if (miutil::contains(es, "$dayeng")) {
      miutil::replace(es, "$dayeng", "%A");
      norwegian = false;
    }
    if (miutil::contains(es, "$daynor")) {
      miutil::replace(es, "$daynor", "%A");
    }
    if (miutil::contains(es, "$day")) {
      miutil::replace(es, "$day", "%A");
    }
    miutil::replace(es, "$hour", "%H");
    miutil::replace(es, "$min", "%M");
    miutil::replace(es, "$sec", "%S");
    miutil::replace(es, "$auto", "$miniclock");
  }
  if (miutil::contains(es, "%")) {
    if (miutil::contains(es, "%anor")) {
      miutil::replace(es, "%anor", "%a");
    }
    if (miutil::contains(es, "%Anor")) {
      miutil::replace(es, "%Anor", "%A");
    }
    if (miutil::contains(es, "%bnor")) {
      miutil::replace(es, "%bnor", "%b");
    }
    if (miutil::contains(es, "%Bnor")) {
      miutil::replace(es, "%Bnor", "%B");
    }
    if (miutil::contains(es, "%aeng")) {
      miutil::replace(es, "%aeng", "%a");
      norwegian = false;
    }
    if (miutil::contains(es, "%Aeng")) {
      miutil::replace(es, "%Aeng", "%A");
      norwegian = false;
    }
    if (miutil::contains(es, "%beng")) {
      miutil::replace(es, "%beng", "%b");
      norwegian = false;
    }
    if (miutil::contains(es, "%Beng")) {
      miutil::replace(es, "%Beng", "%B");
      norwegian = false;
    }
  }
  if ((miutil::contains(es, "%") || miutil::contains(es, "$")) && !time.undef()) {
    if (!norwegian)
      es += " $lg=eng ";
    es = time.format(es);
  }
  return es;
}

const vector<std::string> AnnotationPlot::expanded(const vector<std::string>& vs)
{
  vector<std::string> evs;
  for (size_t j = 0; j < vs.size(); j++) {
    std::string es = insertTime(vs[j], getStaticPlot()->getTime());

    if (useAnaTime) {
      // only using the latest analysis time (yet...)
      miTime anaTime = miTime();
      int n = fieldAnaTime.size();
      for (int i = 0; i < n; i++) {
        if (!fieldAnaTime[i].undef()) {
          if (anaTime.undef() || anaTime < fieldAnaTime[i])
            anaTime = fieldAnaTime[i];
        }
      }
      miutil::replace(es, "@", "$");
      miutil::replace(es, "&", "%");
      es = insertTime(es, anaTime);
    }

    evs.push_back(es);
  }

  return evs;
}

void AnnotationPlot::setfillcolour(std::string colname)
{
  if (atype == anno_data) {
    Colour c(colname);
    poptions.fillcolour = c;
  }
}

bool AnnotationPlot::prepare(const std::string& pin)
{
  pinfo = pin;
  poptions.fontname = "BITMAPFONT"; //default
  poptions.textcolour = Colour("black");
  PlotOptions::parsePlotOption(pinfo, poptions);

  useAnaTime = miutil::contains(pinfo, "@") || miutil::contains(pinfo, "&");

  const vector<std::string> tokens = miutil::split_protected(pinfo, '"', '"');
  vector<std::string> stokens;
  std::string key, value;
  int i, n = tokens.size();
  if (n < 2)
    return false;

  cmargin = 0.007;
  cxoffset = 0.01;
  cyoffset = 0.01;
  cspacing = 0.2;
  clinewidth = 1;
  cxratio = 0.0;
  cyratio = 0.0;

  for (i = 1; i < n; i++) {
    std::string labeltype = tokens[i], LABELTYPE = miutil::to_upper(labeltype);
    stokens = miutil::split_protected(labeltype, '\"', '\"', "=", true);
    if (stokens.size() > 1) {
      key = miutil::to_lower(stokens[0]);
      value = stokens[1];
    }
    if (LABELTYPE == "DATA") {
      atype = anno_data;
    } else if (miutil::contains(LABELTYPE, "ANNO=")) {
      //complex annotation with <><>
      atype = anno_text;
      Annotation a;
      a.str = labeltype.substr(5, labeltype.length() - 5);
      if (a.str[0] == '"')
        a.str = a.str.substr(1, a.str.length() - 2);
      a.col = poptions.textcolour;
      a.spaceLine = false;
      annotations.push_back(a);
    } else if (miutil::contains(LABELTYPE, "TEXT=")) {
      atype = anno_text;
      if (stokens.size() > 1) {
        Annotation a;
        a.str = stokens[1];
        if (a.str[0] == '"')
          a.str = a.str.substr(1, a.str.length() - 2);
        a.col = poptions.textcolour;
        std::string s = a.str;
        miutil::trim(s);
        a.spaceLine = s.length() == 0;
        annotations.push_back(a);
      }
    } else if (key == "productname") {
      productname = value;
    } else {
      labelstrings += " " + tokens[i];
      if (key == "margin") {
        cmargin = atof(value.c_str());
      } else if (key == "xoffset") {
        cxoffset = atof(value.c_str());
      } else if (key == "yoffset") {
        cyoffset = atof(value.c_str());
      } else if (key == "xratio") {
        cxratio = atof(value.c_str());
      } else if (key == "yratio") {
        cyratio = atof(value.c_str());
      } else if (key == "clinewidth") {
        clinewidth = atoi(value.c_str());
      } else if (key == "plotrequested") {
        if (value == "true")
          plotRequested = true;
        else
          plotRequested = false;
      }

    }
  }
  splitAnnotations();
  putElements();

  return true;
}

bool AnnotationPlot::setData(const vector<Annotation>& a,
    const vector<miTime>& fieldAnalysisTime)
{
  if (atype != anno_text)
    annotations = a;
  if (useAnaTime)
    fieldAnaTime = fieldAnalysisTime;
  splitAnnotations();
  putElements();

  return true;
}

void AnnotationPlot::splitAnnotations()
{
  for (Annotation_v::iterator ita = annotations.begin(); ita != annotations.end(); ++ita) {
    ita->oldFormat = false;
    ita->vstr = split(ita->str, '<', '>');
    //use the whole string if no <> delimiters found - old text format
    if (ita->vstr.empty()) {
      ita->vstr.push_back(ita->str);
      ita->oldFormat = true;
    }
  }
  orig_annotations = annotations;
}

bool AnnotationPlot::putElements()
{
  METLIBS_LOG_DEBUG("AnnotationPlot::putElements");
  //decode strings, put into elements...
  vector<std::string> tokens, elementstrings;
  vector<Annotation> anew;
  nothingToDo = true;

  for (Annotation_v::iterator ita = annotations.begin(); ita != annotations.end(); ++ita) {
    for (element_v::iterator ite = ita->annoElements.begin(); ite != ita->annoElements.end(); ++ite)
      delete ite->classplot;
    ita->annoElements.clear();
    ita->hAlign = align_left;//default
    ita->polystyle = poly_none;//default
    ita->spaceLine = false;
    ita->wid = 0;
    ita->hei = 0;
    if (ita->str.empty())
      continue;
    int subel = 0;
    const vector<std::string> elementstrings = expanded(ita->vstr);
    for (size_t l = 0; l < elementstrings.size(); l++) {
      //      METLIBS_LOG_DEBUG("  elementstrings[l]:"<<elementstrings[l]);
      if (miutil::contains(elementstrings[l], "horalign=")) {
        //sets alignment for the whole annotation !
        const vector<std::string> stokens = miutil::split_protected(elementstrings[l], '\"', '\"', ",", true);
        for (size_t k = 0; k < stokens.size(); k++) {
          const vector<std::string> subtokens = miutil::split(stokens[k], 0, "=");
          if (subtokens.size() != 2)
            continue;
          if (subtokens[0] == "horalign") {
            if (subtokens[1] == "center")
              ita->hAlign = align_center;
            if (subtokens[1] == "right")
              ita->hAlign = align_right;
            if (subtokens[1] == "left")
              ita->hAlign = align_left;
          }
        }
      } else if (miutil::contains(elementstrings[l], "bcolour=")) {
        vector<std::string> stokens = miutil::split(elementstrings[l], 0, "=");
        if (stokens.size() != 2)
          continue;
        Colour c = Colour(stokens[1]);
        ita->bordercolour = c;
      } else if (miutil::contains(elementstrings[l], "polystyle=")) {
        vector<std::string> stokens = miutil::split(elementstrings[l], 0, "=");
        if (stokens.size() != 2)
          continue;
        if (stokens[1] == "fill")
          ita->polystyle = poly_fill;
        else if (stokens[1] == "border")
          ita->polystyle = poly_border;
        else if (stokens[1] == "both")
          ita->polystyle = poly_both;
        else if (stokens[1] == "none")
          ita->polystyle = poly_none;

      } else if (elementstrings[l] == "\\box") {
        subel--;
      } else {
        element e;
        if (ita->oldFormat) {
          e.eSize = 1.0;
          e.eFace = poptions.fontface;
          e.eHalign = align_left;
          e.isInside = false;
          e.inEdit = false;
          e.x1 = e.x2 = e.y1 = e.y2 = 0;
          e.itsCursor = 0;
          e.eAlpha = 255;
          e.eType = text;
          e.eText = elementstrings[l];
          e.eFace = poptions.fontface;
          e.horizontal = true;
          e.polystyle = poly_none;
          e.arrowFeather = false;
          //	  e.textcolour="black";
          nothingToDo = false;
          ita->annoElements.push_back(e);
        } else {
          if (decodeElement(elementstrings[l], e)) {
            nothingToDo = false;
            addElement2Vector(ita->annoElements, e, subel);
          }
          if (elementstrings[l].compare(0, 3, "box") == 0)
            subel++;
        }
      }
    }
    anew.push_back(*ita);
  }
  annotations = anew;

  return true;
}

void AnnotationPlot::addElement2Vector(vector<element>& v_e, const element& e,
    int index)
{
  if (index > 0) {
    index--;
    int n = v_e.size();
    if (n > 0)
      addElement2Vector(v_e[n - 1].subelement, e, index);
    return;
  }

  v_e.push_back(e);
}

bool AnnotationPlot::decodeElement(std::string elementstring, element& e)
{
  METLIBS_LOG_SCOPE(LOGVAL(elementstring));

  e.eSize = 1.0;
  e.eFace = poptions.fontface;
  e.eHalign = align_left;
  e.isInside = false;
  e.inEdit = false;
  e.x1 = e.x2 = e.y1 = e.y2 = 0;
  e.itsCursor = 0;
  e.eAlpha = 255;
  e.horizontal = true;
  e.polystyle = poly_none;
  e.arrowFeather = false;
  e.textcolour = "black";
  if (miutil::contains(elementstring, "symbol=")) {
    e.eType = symbol;
  } else if (miutil::contains(elementstring, "arrow=")) {
    e.eType = arrow;
  } else if (miutil::contains(elementstring, "image=")) {
    e.eType = image;
  } else if (miutil::contains(elementstring, "text=") && !miutil::contains(elementstring,"$")) {
    e.eType = text;
  } else if (miutil::contains(elementstring, "input=")) {
    e.eType = input;
    editable = true;
  } else if (miutil::contains(elementstring, "table=") && miutil::contains(elementstring, ";")) {
    e.eType = table;
    vector<std::string> stokens = miutil::split_protected(elementstring, '\"', '\"', ",", true);
    e.classplot = new LegendPlot(stokens[0]);
    e.classplot->setPlotOptions(poptions);
    e.classplot->setStaticPlot(getStaticPlot());
  } else if (miutil::contains(elementstring, "box")) {
    e.eType = box;
  } else {
    return false;
  }

  //element options
  vector<std::string> stokens = miutil::split_protected(elementstring, '\"', '\"', ",", true);
  int mtokens = stokens.size();
  for (int k = 0; k < mtokens; k++) {
    if (stokens[k] == "vertical") {
      e.horizontal = false;
    } else {
      vector<std::string> subtokens = miutil::split_protected(stokens[k], '\"', '\"', "=", true);
      if (subtokens.size() != 2)
        continue;
      if (subtokens[0] == "text" || subtokens[0] == "input") {
        e.eText = subtokens[1];
        editable = true;
        if (e.eText[0] == '"')
          e.eText = e.eText.substr(1, e.eText.length() - 2);
      }
      if (subtokens[0] == "title" && e.eType == table) {
        std::string title = subtokens[1];
        if (subtokens[1][0] == '"')
          subtokens[1] = subtokens[1].substr(1, subtokens[1].length() - 2);
        e.classplot->setTitle(subtokens[1]);
      } else if (subtokens[0] == "image")
        e.eImage = subtokens[1];
      else if (subtokens[0] == "arrow")
        e.arrowLength = atof(subtokens[1].c_str());
      else if (subtokens[0] == "feather")
        e.arrowFeather = (subtokens[1] == "true");
      else if (subtokens[0] == "symbol")
        e.eCharacter = atoi(subtokens[1].c_str());
      else if (subtokens[0] == "alpha")
        e.eAlpha = int(atof(subtokens[1].c_str()) * 255);
      else if (subtokens[0] == "font")
        e.eFont = subtokens[1];
      else if (subtokens[0] == "face")
        e.eFace = subtokens[1];
      else if (subtokens[0] == "tcolour")
        e.textcolour = subtokens[1];
      else if (subtokens[0] == "size")
        e.eSize = atof(subtokens[1].c_str());
      else if (subtokens[0] == "name") {
        e.eName = subtokens[1];
        inputText[e.eName] = e.eText;
        editable = true;
      } else if (subtokens[0] == "hal") {
        if (subtokens[1] == "center")
          e.eHalign = align_center;
        else if (subtokens[1] == "right")
          e.eHalign = align_right;
        else if (subtokens[1] == "left")
          e.eHalign = align_left;
      } else if (subtokens[0] == "pstyle") {
        if (subtokens[1] == "fill")
          e.polystyle = poly_fill;
        else if (subtokens[1] == "border")
          e.polystyle = poly_border;
        else if (subtokens[1] == "both")
          e.polystyle = poly_both;
        else if (subtokens[1] == "none")
          e.polystyle = poly_none;
      } else if (e.eType == table && subtokens[0] == "fcolour") {
        e.classplot->setBackgroundColour(Colour(subtokens[1]));
      } else if (e.eType == table && subtokens[0] == "suffix") {
        miutil::remove(subtokens[1], '"');
        e.classplot->setSuffix(subtokens[1]);
      }
    }
  }
  return true;
}

void AnnotationPlot::plot(PlotOrder zorder)
{
  METLIBS_LOG_SCOPE();
  if (zorder != LINES && zorder != OVERLAY)
    return;

  if (!isEnabled() || annotations.empty() || nothingToDo)
    return;

  borderline.clear();
  scaleAnno = false;
  plotAnno = true;

  Rectangle window;
  if (plotRequested && getStaticPlot()->getRequestedarea().P() == getStaticPlot()->getMapArea().P()) {
    window = getStaticPlot()->getRequestedarea().R();
    if (window.x1 == window.x2 || window.y1 == window.y2)
      window = getStaticPlot()->getPlotSize();
    if (cxratio > 0 && cyratio > 0)
      scaleAnno = true;
  } else
    window = getStaticPlot()->getPlotSize();

  fontsizeToPlot = int(poptions.fontsize);

  scaleFactor = 1.0;

  //border and offset of annotations
  border = cmargin * window.width();
  //get dimensions of annotations and of box around annotations(xbox,ybox)
  bbox = window;

  //check if annotations should be scaled, get new dimensions
  getXYBox();

  int n = annotations.size();

  // draw filled area
  Colour fc = poptions.fillcolour;
  if (fc.A() < Colour::maxv && getColourMode()) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  if (poptions.polystyle != poly_border && poptions.polystyle != poly_none) {
    if (getColourMode())
      glColor4ubv(fc.RGBA());
    else
      glIndexi(fc.Index());
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBegin(GL_POLYGON);
    glVertex2f(bbox.x1, bbox.y1);
    glVertex2f(bbox.x1, bbox.y2);
    glVertex2f(bbox.x2, bbox.y2);
    glVertex2f(bbox.x2, bbox.y1);
    glEnd();
  }
  glDisable(GL_BLEND);


  getStaticPlot()->UpdateOutput();

  //plotAnno could be false if annotations too big for box
  // return here after plotted box only
  if (!plotAnno)
    return;

  // draw the annotations
  for (int i = 0; i < n; i++) {
    Annotation & anno = annotations[i];
    //draw one annotation - one line
    Colour c = anno.col;
    if (c == getStaticPlot()->getBackgroundColour())
      c = getStaticPlot()->getBackContrastColour();
    currentColour = c;
    if (getColourMode())
      glColor4ubv(c.RGBA());
    else
      glIndexi(c.Index());
    plotElements(annotations[i].annoElements,
                 anno.rect.x1, anno.rect.y1, annotations[i].hei);
  }
  getStaticPlot()->UpdateOutput();

  // draw outline
  if (poptions.polystyle != poly_fill && poptions.polystyle != poly_none) {
    if (getColourMode())
      glColor4ubv(poptions.bordercolour.RGBA());
    else
      glIndexi(poptions.bordercolour.Index());

    glLineWidth(clinewidth);
    glBegin(GL_LINE_LOOP);
    glVertex2f(bbox.x1, bbox.y1);
    glVertex2f(bbox.x1, bbox.y2);
    glVertex2f(bbox.x2, bbox.y2);
    glVertex2f(bbox.x2, bbox.y1);
    glEnd();
  }

  //draw borders
  for (int i = 0; i < n; i++) {
    if (annotations[i].polystyle == poly_border || annotations[i].polystyle
        == poly_both) {
      glColor4ubv(annotations[i].bordercolour.RGBA());
      glBegin(GL_POLYGON);
      glVertex2f(annotations[i].rect.x1, annotations[i].rect.y1);
      glVertex2f(annotations[i].rect.x1, annotations[i].rect.y2);
      glVertex2f(annotations[i].rect.x2, annotations[i].rect.y2);
      glVertex2f(annotations[i].rect.x2, annotations[i].rect.y1);
      glEnd();
    }
    plotBorders();
  }

  getStaticPlot()->UpdateOutput();
}

bool AnnotationPlot::plotElements(vector<element>& annoEl, float& x, float& y,
    float annoHeight, bool horizontal)
{
  METLIBS_LOG_SCOPE(LOGVAL(annoEl.size()));

  float fontsizeScale;
  float wid, hei;
  if (getStaticPlot()->hardcopy)
    fontsizeScale = getStaticPlot()->getFontPack()->getSizeDiv();
  else
    fontsizeScale = 1.0;

  //    METLIBS_LOG_DEBUG("fontsizeScale:"<<fontsizeScale);
  for (size_t j = 0; j < annoEl.size(); j++) {
    if (!horizontal && j != 0)
      y -= annoEl[j].height;

    if (getColourMode())
      glColor4ubv(currentColour.RGBA());
    else
      glIndexi(currentColour.Index());

    if (annoEl[j].polystyle == poly_fill || annoEl[j].polystyle == poly_both) {

      Colour fc = poptions.fillcolour;
      if (fc.A() < Colour::maxv && getColourMode()) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      }
      if (getColourMode())
        glColor4ubv(fc.RGBA());
      else
        glIndexi(fc.Index());
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glBegin(GL_POLYGON);
      glVertex2f(x - border, y - border);
      glVertex2f(x + border + annoEl[j].width, y - border);
      glVertex2f(x + border + annoEl[j].width, y + border + annoEl[j].height);
      glVertex2f(x - border, y + border + annoEl[j].height);
      glEnd();
      glDisable(GL_BLEND);
    }
    getStaticPlot()->UpdateOutput();

    // get coordinates of border, draw later
    if (annoEl[j].polystyle == poly_border || annoEl[j].polystyle == poly_both) {
      Border bl;
      bl.x1 = x - border;
      bl.y1 = y - border;
      bl.x2 = x + border + annoEl[j].width;
      bl.y2 = y + border + annoEl[j].height;
      borderline.push_back(bl);
    }

    if (not annoEl[j].textcolour.empty()) {
      Colour c(annoEl[j].textcolour);
      glColor4ubv(c.RGBA());
    }

    if (annoEl[j].eType == box) {
      wid = hei = 0.0;
      //keeping coor. of the bottom left corner
      float tmpx = x, tmpy = y;
      if (horizontal && !annoEl[j].horizontal)
        y += annoEl[j].height - annoEl[j].subelement[0].height;
      plotElements(annoEl[j].subelement, x, y, annoEl[j].height,
          annoEl[j].horizontal);
      if (horizontal)
        y = tmpy;
      else
        x = tmpx;
      wid = annoEl[j].width;
      hei = annoEl[j].height;
    } else if (annoEl[j].eType == text || annoEl[j].eType == input) {
      //change text colour if editable
      if (annoEl[j].eType == input && isMarked) {
        if (annoEl[j].isInside) {
          //red - ctrl-e works
          glColor4f(1, 0, 0, 1.0);
        } else if (isMarked) {
          //show text to be edited in blue
          glColor4f(0, 0, 1, 1.0);
        }
      }
      getStaticPlot()->getFontPack()->set(poptions.fontname, annoEl[j].eFace, annoEl[j].eSize
          * fontsizeToPlot);
      std::string astring = annoEl[j].eText;
      astring += " ";
      getStaticPlot()->getFontPack()->getStringSize(astring.c_str(), wid, hei);
      wid *= fontsizeScale;
      hei *= fontsizeScale;
      if (annoEl[j].eHalign == align_right && j + 1 == annoEl.size())
        x = bbox.x2 - wid - border;
      getStaticPlot()->getFontPack()->drawStr(astring.c_str(), x, y, 0.0);
      //remember corners of box around text for marking and editing
      annoEl[j].x1 = x;
      annoEl[j].y1 = y;
      annoEl[j].x2 = x + wid;
      annoEl[j].y2 = y + hei;
      if (annoEl[j].inEdit) {
        //Hvis* editeringstext,tegn strek for mark�r og for
        //markert tekst
        float w, h;
        std::string substring = astring.substr(0, annoEl[j].itsCursor);
        getStaticPlot()->getFontPack()->getStringSize(substring.c_str(), w, h);
        glColor4f(0, 0, 0, 1.0);
        glBegin(GL_LINE_STRIP);
        glVertex2f(annoEl[j].x1 + w, annoEl[j].y1);
        glVertex2f(annoEl[j].x1 + w, annoEl[j].y2);
        glEnd();
      }
    } else if (annoEl[j].eType == symbol) {
      getStaticPlot()->getFontPack()->set(annoEl[j].eFont, annoEl[j].eFace, annoEl[j].eSize
          * fontsizeToPlot);
      float tmpwid;
      getStaticPlot()->getFontPack()->getCharSize(annoEl[j].eCharacter, tmpwid, hei);
      getStaticPlot()->getFontPack()->drawChar(annoEl[j].eCharacter, x, y, 0.0);
      //set back to normal font and draw one blank
      getStaticPlot()->getFontPack()->set(poptions.fontname, poptions.fontface, annoEl[j].eSize
          * fontsizeToPlot);
      std::string astring = " ";
      getStaticPlot()->getFontPack()->drawStr(astring.c_str(), x, y, 0.0);
      getStaticPlot()->getFontPack()->getStringSize(astring.c_str(), wid, hei);
      wid *= fontsizeScale;
      wid += tmpwid;
      hei *= fontsizeScale;
    } else if (annoEl[j].eType == image) {
      ImageGallery ig;
      float scale = annoEl[j].eSize * scaleFactor;
      hei = ig.height_(annoEl[j].eImage) * getStaticPlot()->getPhysToMapScaleY() * scale;
      wid = ig.width_(annoEl[j].eImage) * getStaticPlot()->getPhysToMapScaleX() * scale;
      ig.plotImage(getStaticPlot(), annoEl[j].eImage, x + wid / 2, y + hei / 2, true, scale,
          annoEl[j].eAlpha);
    } else if (annoEl[j].eType == table) {
      hei = annoEl[j].classplot->height();
      if (poptions.v_align == align_top)
        annoEl[j].classplot->plotLegend(x, y + annoHeight);
      else if (poptions.v_align == align_center)
        annoEl[j].classplot->plotLegend(x, y + (annoHeight + hei) / 2.);
      else
        annoEl[j].classplot->plotLegend(x, y + hei);
      wid = annoEl[j].classplot->width();
    } else if (annoEl[j].eType == arrow) {
      wid = plotArrow(x, y, annoEl[j].arrowLength, annoEl[j].arrowFeather);
    }
    if (horizontal)
      x += wid;
  }
  return true;
}

float AnnotationPlot::plotArrow(float x, float y, float l, bool feather)
{
  if (feather) {
    x += 0.1 * l;
  }

  float dx = 0.333333 * l;
  float dy = dx * 0.5;

  //  glLineWidth(2.0);
  glBegin(GL_LINES);
  glVertex2f(x, y + dy);
  glVertex2f(x + l, y + dy);
  glVertex2f(x + l, y + dy);
  glVertex2f(x + l - dx, y + dy * 2);
  glVertex2f(x + l, y + dy);
  glVertex2f(x + l - dx, y);
  glEnd();

  if (feather) {
    float x1 = x;
    float y1 = y + dy;
    float x2 = x1 - 0.13 * l;
    float y2 = y1 + 0.28 * l;

    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    x1 += 0.13 * l;
    x2 += 0.13 * l;
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    x1 += 0.13 * l;
    x2 += 0.13 * l;
    x2 = (x1 + x2) / 2;
    y2 = (y1 + y2) / 2;
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
  }

  glLineWidth(1.0);

  return l;
}

void AnnotationPlot::plotBorders()
{
  if (getColourMode())
    glColor4ubv(poptions.bordercolour.RGBA());
  else
    glIndexi(poptions.bordercolour.Index());

  glLineWidth(clinewidth);

  int n = borderline.size();
  for (int i = 0; i < n; i++) {
    glBegin(GL_LINE_LOOP);
    glVertex2f(borderline[i].x1, borderline[i].y1);
    glVertex2f(borderline[i].x1, borderline[i].y2);
    glVertex2f(borderline[i].x2, borderline[i].y2);
    glVertex2f(borderline[i].x2, borderline[i].y1);
    glEnd();
  }
}

void AnnotationPlot::getAnnoSize(vector<element> &annoEl, float& wid,
    float& hei, bool horizontal)
{
  METLIBS_LOG_SCOPE(LOGVAL(annoEl.size()));

  float fontsizeScale;

  float width = 0, height = 0;
  for (size_t j = 0; j < annoEl.size(); j++) {
    float w = 0.0, h = 0.0;
    if (annoEl[j].eType == text || annoEl[j].eType == input) {
      getStaticPlot()->getFontPack()->set(poptions.fontname, annoEl[j].eFace, annoEl[j].eSize
          * fontsizeToPlot);
      std::string astring = annoEl[j].eText;
      astring += " ";
      getStaticPlot()->getFontPack()->getStringSize(astring.c_str(), w, h);
      if (getStaticPlot()->hardcopy)
        fontsizeScale = getStaticPlot()->getFontPack()->getSizeDiv();
      else
        fontsizeScale = 1.0;
      w *= fontsizeScale;
      h *= fontsizeScale;
    } else if (annoEl[j].eType == image) {
      std::string aimage = annoEl[j].eImage;
      ImageGallery ig;
      float scale = annoEl[j].eSize * scaleFactor;
      h = ig.height_(aimage) * getStaticPlot()->getPhysToMapScaleY() * scale;
      w = ig.width_(aimage) * getStaticPlot()->getPhysToMapScaleX() * scale;
    } else if (annoEl[j].eType == table) {
      h = annoEl[j].classplot->height();
      w = annoEl[j].classplot->width();
    } else if (annoEl[j].eType == arrow) {
      h = annoEl[j].arrowLength / 2.;
      w = annoEl[j].arrowLength;
    } else if (annoEl[j].eType == symbol) {
      getStaticPlot()->getFontPack()->set(annoEl[j].eFont, poptions.fontface, annoEl[j].eSize
          * fontsizeToPlot);
      getStaticPlot()->getFontPack()->getCharSize(annoEl[j].eCharacter, w, h);
      getStaticPlot()->getFontPack()->set(poptions.fontname, poptions.fontface, annoEl[j].eSize
          * fontsizeToPlot);
      if (getStaticPlot()->hardcopy)
        fontsizeScale = getStaticPlot()->getFontPack()->getSizeDiv();
      else
        fontsizeScale = 1.0;
      std::string astring = " ";
      getStaticPlot()->getFontPack()->getStringSize(astring.c_str(), w, h);
      w *= fontsizeScale;
      w += w;
      h *= fontsizeScale;
    } else if (annoEl[j].eType == box) {
      getAnnoSize(annoEl[j].subelement, w, h, annoEl[j].horizontal);
    }

    if (horizontal) {
      width += w;
      if (h > height)
        height = h;
    } else {
      height += h;
      if (w > width)
        width = w;
    }
    annoEl[j].width = w;
    annoEl[j].height = h;

  }
  wid = width;
  hei = height;
}

/**
 * Gets an appropriate bounding box for the annotations, setting the bbox
 * member variable as a result. When this method is called, the bbox
 * corresponds to the plot window.
 */

void AnnotationPlot::getXYBox()
{
  //  METLIBS_LOG_DEBUG("getXYBox()");

  float xoffset = 0, yoffset = 0;
  xoffset = cxoffset * bbox.width();
  yoffset = cyoffset * bbox.height();

  float maxhei = 0;
  float wid, hei;
  float totalhei = 0;
  maxwid = 0;
  spacing = 0;

  // Find width and height of each annotation, and total width and height.
  int n = annotations.size();

  for (int i = 0; i < n; i++) {
    getAnnoSize(annotations[i].annoElements, wid, hei);

    annotations[i].wid = wid;
    annotations[i].hei = hei;

    if (wid > maxwid)
      maxwid = wid;
    if (hei > maxhei)
      maxhei = hei;
    totalhei += hei;
    if (annotations[i].spaceLine)
      totalhei -= (hei + spacing) * 0.5;
  }
  spacing = cspacing * maxhei;

  float width = bbox.width();
  float height = bbox.height();

  if (scaleAnno) {
    width *= cxratio;
    height *= cyratio;

    // The scale factor is the ratio of the width of the annotation (minus borders)
    // to the maximum width of items in the annotation. This gives us a factor we
    // can use to fit the items into the annotation.
    scaleFactor = min((width - 2 * border) / maxwid, float(1.0));
    fontsizeToPlot = fontsizeToPlot * scaleFactor;

    float scaledHeight = totalhei * scaleFactor;
    float scaledSpacing = (height - scaledHeight - (2 * border))/(n - 1);
    spacing = min(scaledSpacing, spacing);

    // Scale the width and height of each annotation.
    for (int i = 0; i < n; i++) {
      annotations[i].wid *= scaleFactor;
      annotations[i].hei *= scaleFactor;
    }

    // Shrink the annotation vertically if there is free space (if the
    // spacing used was less than that available).
    if (spacing < scaledSpacing)
        height -= (n - 1)*(scaledSpacing - spacing);
  } else {
    width = maxwid + 2 * border;
    height = totalhei + (n - 1) * spacing + 2 * border;
  }

  // Set the x coordinates for the annotation's bounding box.
  if (poptions.h_align == align_left) {
    bbox.x1 += xoffset;
  } else if (poptions.h_align == align_right) {
    bbox.x1 = bbox.x2 - width - xoffset;
  } else if (poptions.h_align == align_center) {
    bbox.x1 = (bbox.x1 + bbox.x2) / 2.0 - width / 2.0;
  }
  bbox.x2 = bbox.x1 + width;

  // Set the y coordinates for the annotation's bounding box.
  if (poptions.v_align == align_bottom) {
    bbox.y1 += yoffset;
  } else if (poptions.v_align == align_top) {
    bbox.y1 = bbox.y2 - height - yoffset;
  } else if (poptions.v_align == align_center) {
    bbox.y1 = (bbox.y1 + bbox.y2) / 2.0 - (height + yoffset) / 2.0;
  }
  bbox.y2 = bbox.y1 + height;

  // Arrange the annotations.
  float x1, x2, y1, y2;

  if (poptions.v_align == align_bottom)
    y2 = bbox.y1 + height - border;
  else if (poptions.v_align == align_top)
    y2 = bbox.y2 - border;
  else // if (poptions.v_align == align_center)
    y2 = (bbox.y1 + bbox.y2) / 2.0 + height / 2.0;

  for (int i = 0; i < n; i++) {
    if (annotations[i].hAlign == align_left) {
      x1 = bbox.x1 + border;
      x2 = x1 + annotations[i].wid + border;
    } else if (annotations[i].hAlign == align_right) {
      x2 = bbox.x2 - border;
      x1 = x2 - annotations[i].wid - border;
    } else { // if (annotations[i].hAlign == align_center)
      x1 = (bbox.x2 + bbox.x1) / 2 - annotations[i].wid / 2;
      x2 = (bbox.x2 + bbox.x1) / 2 + annotations[i].wid / 2;
    }
    y1 = y2 - annotations[i].hei;
    annotations[i].rect = Rectangle(x1, y1, x2, y2);
    y2 -= annotations[i].hei + spacing;
  }
}

bool AnnotationPlot::markAnnotationPlot(int x, int y)
{
  //if not editable annotationplot, do not mark
  if (!editable)
    return false;
  bool wasMarked = isMarked;
  isMarked = false;
  //convert x and y...
  const XY pos = getStaticPlot()->PhysToMap(XY(x, y));
  if (bbox.isinside(pos.x(), pos.y())) {
    isMarked = true;
    //loop over elements
    int n = annotations.size();
    for (int i = 0; i < n; i++) {
      Annotation & anno = annotations[i];
      int nel = anno.annoElements.size();
      for (int j = 0; j < nel; j++) {
        element & annoEl = anno.annoElements[j];
        annoEl.isInside = false;
        if (annoEl.eType == input) {
          if (pos.x() > annoEl.x1 && pos.x() < annoEl.x2
              && pos.y() > annoEl.y1 && pos.y() < annoEl.y2)
            annoEl.isInside = true;
        }
      }
    }
  }
  //check if something changed so replotting is necessary
  if (wasMarked == false && isMarked == false)
    return false;
  else
    return true;
}

std::string AnnotationPlot::getMarkedAnnotation()
{
  std::string text;
  if (isMarked) {
    int n = annotations.size();
    for (int i = 0; i < n; i++) {
      int nel = annotations[i].annoElements.size();
      for (int j = 0; j < nel; j++) {
        element annoEl = annotations[i].annoElements[j];
        if (annoEl.isInside == true) {
          text = annoEl.eText;
          //miutil::trim(text);
        }
      }
    }
  }
  return text;
}

void AnnotationPlot::changeMarkedAnnotation(std::string text, int cursor,
    int sel1, int sel2)
{
  if (isMarked) {
    int n = annotations.size();
    for (int i = 0; i < n; i++) {
      int nel = annotations[i].annoElements.size();
      for (int j = 0; j < nel; j++) {
        element & annoEl = annotations[i].annoElements[j];
        if (annoEl.isInside == true) {
          annoEl.eText = text;
          annoEl.itsCursor = cursor;
          annoEl.itsSel1 = sel1;
          annoEl.itsSel2 = sel2;
        }
      }
    }
  }
}

void AnnotationPlot::DeleteMarkedAnnotation()
{
  if (!editable)
    return;
  if (isMarked) {
    int n = annotations.size();
    for (int i = 0; i < n; i++) {
      vector<element>::iterator p = annotations[i].annoElements.begin();
      while (p != annotations[i].annoElements.end()) {
        if (p->isInside == true) {
          p = annotations[i].annoElements.erase(p);
        } else
          p++;
      }
    }
  }
}

void AnnotationPlot::startEditAnnotation()
{
  int n = annotations.size();
  for (int i = 0; i < n; i++) {
    int nel = annotations[i].annoElements.size();
    for (int j = 0; j < nel; j++) {
      element & annoEl = annotations[i].annoElements[j];
      if (annoEl.isInside == true) {
        annoEl.inEdit = true;
      }
    }
  }
}

void AnnotationPlot::stopEditAnnotation()
{
  int n = annotations.size();
  for (int i = 0; i < n; i++) {
    int nel = annotations[i].annoElements.size();
    for (int j = 0; j < nel; j++) {
      element & annoEl = annotations[i].annoElements[j];
      if (annoEl.isInside == true) {
        annoEl.inEdit = false;
      }
    }
  }
}

void AnnotationPlot::editNextAnnoElement()
{
  int n = annotations.size();
  bool next = false;
  //find next input element after the one that is edited and mark
  for (int i = 0; i < n; i++) {
    int nel = annotations[i].annoElements.size();
    for (int j = 0; j < nel; j++) {
      element & annoEl = annotations[i].annoElements[j];
      if (next && annoEl.eType == input) {
        annoEl.isInside = true;
        annoEl.inEdit = true;
        return;
      }
      if (annoEl.isInside == true && annoEl.inEdit == true) {
        annoEl.isInside = false;
        annoEl.inEdit = false;
        next = true;
      }
    }
  }
  //if we come to the end, find first input element and mark
  if (next) {
    for (int i = 0; i < n; i++) {
      int nel = annotations[i].annoElements.size();
      for (int j = 0; j < nel; j++) {
        element & annoEl = annotations[i].annoElements[j];
        if (annoEl.eType == input) {
          annoEl.isInside = true;
          annoEl.inEdit = true;
          return;
        }
      }
    }
  }
}

void AnnotationPlot::editLastAnnoElement()
{
  int n = annotations.size();
  bool last = false;
  //find next element after the one that is edited and mark
  for (int i = n - 1; i > -1; i--) {
    int nel = annotations[i].annoElements.size();
    for (int j = nel - 1; j > -1; j--) {
      element & annoEl = annotations[i].annoElements[j];
      if (last && annoEl.eType == input) {
        annoEl.isInside = true;
        annoEl.inEdit = true;
        return;
      }
      if (annoEl.isInside == true && annoEl.inEdit == true) {
        annoEl.isInside = false;
        annoEl.inEdit = false;
        last = true;
      }
    }
  }
  //if we come to the start, find last input element and mark
  if (last) {
    for (int i = n - 1; i > -1; i--) {
      int nel = annotations[i].annoElements.size();
      for (int j = nel - 1; j > -1; j--) {
        element & annoEl = annotations[i].annoElements[j];
        if (annoEl.eType == input) {
          annoEl.isInside = true;
          annoEl.inEdit = true;
          return;
        }
      }
    }
  }
}

vector<std::string> AnnotationPlot::split(const std::string eString, const char s1, const char s2)
{
  /*finds entries delimited by s1 and s2
   (f.ex. s1=<,s2=>) <"this is the entry">
   anything inside " " is ignored as delimiters
   f.ex. <"this is also > an entry">
   */
  int stop, start, stop2, start2, len;
  vector<std::string> vec;

  if (eString.empty())
    return vec;

  len = eString.length();
  start = eString.find_first_of(s1, 0) + 1;
  while (start >= 1 && start < len) {
    start2 = eString.find_first_of('"', start);
    stop = eString.find_first_of(s2, start);
    while (start2 > -1 && start2 < stop) {
      // search for second ", which should come before >
      stop2 = eString.find_first_of('"', start2 + 1);
      if (stop2 > -1) {
        start2 = eString.find_first_of('"', stop2 + 1);
        stop = eString.find_first_of(s2, stop2 + 1);
      } else
        start2 = -1;
    }

    //if maching s2 found, add entry
    if (stop > 0 && stop < len)
      vec.push_back(eString.substr(start, stop - start));

    //next s1
    if (stop < len)
      start = eString.find_first_of(s1, stop) + 1;
    else
      start = len;
  }
  return vec;
}

std::string AnnotationPlot::writeElement(element& annoEl)
{
  std::string str = "<";
  switch (annoEl.eType) {
  case (text):
    str += "text=\"" + annoEl.eText + "\"";
    if (!annoEl.eFont.empty())
      str += ",font=" + annoEl.eFont;
    break;
  case (symbol):
    str += "symbol=" + miutil::from_number(annoEl.eCharacter);
    if (!annoEl.eFont.empty())
      str += ",font=" + annoEl.eFont;
    break;
  case (image):
    str += "image=" + annoEl.eImage;
    break;
  case (input):
    str += "input=\"" + annoEl.eText + "\"";
    if (!annoEl.eFont.empty())
      str += ",font=" + annoEl.eFont;
    if (!annoEl.eName.empty())
      str += ",name=" + annoEl.eName;
    break;
  case (box):
    str += "box";
    if (annoEl.horizontal) {
      str += ",horizontal";
    } else {
      str += ",vertical";
    }
    if (annoEl.polystyle == poly_fill) {
      str += ",pstyle=fill";
    } else if (annoEl.polystyle == poly_border) {
      str += ",pstyle=border";
    } else if (annoEl.polystyle == poly_both) {
      str += ",pstyle=both";
    } else if (annoEl.polystyle == poly_none) {
      str += ",pstyle=none";
    }
    break;
  default:
    break;
  }
  if (annoEl.eSize != 1.0) {
    ostringstream ostr;
    ostr << ",size=" << annoEl.eSize;
    str += ostr.str();
  }
  if (annoEl.textcolour != "black") {
    str += ",tcolour=" + annoEl.textcolour;
  }
  if (!annoEl.eFace.empty() && annoEl.eFace != poptions.fontface)
    str += ",face=" + annoEl.eFace;
  if (annoEl.eHalign == align_right)
    str += ",hal=right";
  else if (annoEl.eHalign == align_center)
    str += ",hal=center";
  str += ">";

  if (annoEl.subelement.size()) {
    for (unsigned int i = 0; i < annoEl.subelement.size(); i++) {
      str += writeElement(annoEl.subelement[i]);
    }
    str += "<\\box>";
  }

  return str;
}

std::string AnnotationPlot::writeAnnotation(std::string prodname)
{
  std::string str;
  if (prodname != productname)
    return str;
  str = "LABEL";
  int n = annotations.size();
  for (int i = 0; i < n; i++) {
    str += " anno=";
    int nel = annotations[i].annoElements.size();
    for (int j = 0; j < nel; j++) {
      element annoEl = annotations[i].annoElements[j];
      //write annoelement here !
      str += writeElement(annoEl);
    }
    if (annotations[i].hAlign == align_right)
      str += "<horalign=right>";
    else if (annotations[i].hAlign == align_center)
      str += "<horalign=center>";
    //else if (annotations[i].hAlign==align_left)
    //  str+="<horalign=left>";
  }
  str += labelstrings;

  return str;
}

void AnnotationPlot::updateInputLabels(const AnnotationPlot * oldAnno,
    bool newProduct)
{
  int n = annotations.size();
  for (int i = 0; i < n; i++) {
    int nel = annotations[i].annoElements.size();
    for (int j = 0; j < nel; j++) {
      element & annoEl = annotations[i].annoElements[j];
      if (annoEl.eType == input && not annoEl.eName.empty()) {
        map<std::string, std::string> oldInputText = oldAnno->inputText;
        map<std::string, std::string>::iterator pf = oldInputText.find(annoEl.eName);
        if (pf != oldInputText.end()) {
          annoEl.eText = pf->second;
          if (annoEl.eName == "number" && newProduct)
            //increase the number by one if new product !
            annoEl.eText = miutil::from_number(miutil::to_int(annoEl.eText) + 1);
        }
      }
    }
  }
}

const vector<AnnotationPlot::Annotation>& AnnotationPlot::getAnnotations()
{
  return annotations;
}

vector<vector<std::string> > AnnotationPlot::getAnnotationStrings()
{
  METLIBS_LOG_SCOPE(LOGVAL(annotations.size()));
  bool orig = false;

  int n = annotations.size();
  for (int i = 0; i < n; i++) {
    int m = annotations[i].vstr.size();
    for (int j = 0; j < m; j++) {
      if (miutil::contains(annotations[i].vstr[j], "arrow")) {
        orig = true;
        break;
      }
    }
    if (orig)
      break;
  }

  vector<vector<std::string> > vvstr;
  if (orig) {
    int m = orig_annotations.size();
    for (int i = 0; i < m; i++)
      vvstr.push_back(orig_annotations[i].vstr);
  } else {
    int m = annotations.size();
    for (int i = 0; i < m; i++)
      vvstr.push_back(annotations[i].vstr);
  }
  return vvstr;
}

bool AnnotationPlot::setAnnotationStrings(vector<vector<std::string> >& vvstr)
{
  METLIBS_LOG_SCOPE(LOGVAL(vvstr.size()));
  if (vvstr.empty())
    return false;

  const size_t m = annotations.size();

  if (vvstr.size() != m)
    return false;

  bool keep = false;
  for (size_t i = 0; i < vvstr.size(); i++) {
    if (vvstr[i].size())
      keep = true;
    annotations[i].vstr = vvstr[i];
  }
  putElements();

  return keep;
}
