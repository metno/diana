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

#include <diCommonTypes.h>
#include <diAnnotationPlot.h>
#include <diLegendPlot.h>
#include <diFontManager.h>
#include <diImageGallery.h>
#include <iostream>
#include <GL/gl.h>

#include <math.h>

using namespace std; using namespace miutil;

// Default constructor
AnnotationPlot::AnnotationPlot() :
  Plot()
{
#ifdef DEBUGPRINT
  DEBUG_ << "++ AnnotationPlot::Default Constructor";
#endif
  init();
}

// Constructor
AnnotationPlot::AnnotationPlot(const miString& po) :
  Plot()
{
#ifdef DEBUGPRINT
  DEBUG_ << "++ AnnotationPlot::Constructor: " << po;
#endif
  init();
  prepare(po);
}

// Destructor
AnnotationPlot::~AnnotationPlot()
{
  size_t n = annotations.size();
  for ( size_t i = 0; i < n; ++i ) {
    size_t m = annotations[i].annoElements.size();
    for ( size_t j = 0; j < m; ++j ) {
      delete annotations[i].annoElements[j].classplot;
      annotations[i].annoElements[j].classplot = NULL;
    }
  }

}

void AnnotationPlot::init()
{
  plotRequested = false;
  isMarked = false;
  labelstrings = miString();
  productname = miString();
  editable = false;
  useAnaTime = false;
}

const miString AnnotationPlot::insertTime(const miString& s, const miTime& time)
{

  bool norwegian = true;
  miString es = s;
  if (es.contains("$")) {
    if (es.contains("$dayeng")) {
      es.replace("$dayeng", "%A");
      norwegian = false;
    }
    if (es.contains("$daynor")) {
      es.replace("$daynor", "%A");
      norwegian = true;
    }
    if (es.contains("$day")) {
      es.replace("$day", "%A");
      norwegian = true;
    }
    es.replace("$hour", "%H");
    es.replace("$min", "%M");
    es.replace("$sec", "%S");
    es.replace("$auto", "$miniclock");
  }
  if (es.contains("%")) {
    if (es.contains("%anor")) {
      es.replace("%anor", "%a");
      norwegian = true;
    }
    if (es.contains("%Anor")) {
      es.replace("%Anor", "%A");
      norwegian = true;
    }
    if (es.contains("%bnor")) {
      es.replace("%bnor", "%b");
      norwegian = true;
    }
    if (es.contains("%Bnor")) {
      es.replace("%Bnor", "%B");
      norwegian = true;
    }
    if (es.contains("%aeng")) {
      es.replace("%aeng", "%a");
      norwegian = false;
    }
    if (es.contains("%Aeng")) {
      es.replace("%Aeng", "%A");
      norwegian = false;
    }
    if (es.contains("%beng")) {
      es.replace("%beng", "%b");
      norwegian = false;
    }
    if (es.contains("%Beng")) {
      es.replace("%Beng", "%B");
      norwegian = false;
    }
  }
  if ((es.contains("%") || es.contains("$")) && !time.undef()) {
    if (norwegian)
      es = time.format(es, miDate::Norwegian);
    else
      es = time.format(es, miDate::English);
  }
  return es;
}

const vector<miString> AnnotationPlot::expanded(const vector<miString>& vs)
{

  vector<miString> evs;
  int nvs = vs.size();
  for (int j = 0; j < nvs; j++) {
    miString es = insertTime(vs[j], ctime);

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
      es.replace("@", "$");
      es.replace("&", "%");
      es = insertTime(es, anaTime);
    }

    evs.push_back(es);
  }

  return evs;
}

void AnnotationPlot::setfillcolour(miString colname)
{
  if (atype == anno_data) {
    Colour c(colname);
    poptions.fillcolour = c;
  }
}

bool AnnotationPlot::prepare(const miString& pin)
{

  pinfo = pin;
  poptions.fontname = "BITMAPFONT"; //default
  poptions.textcolour = Colour("black");
  PlotOptions::parsePlotOption(pinfo, poptions);

  useAnaTime = miutil::contains(pinfo, "@") || miutil::contains(pinfo, "&");

  const vector<std::string> tokens = miutil::split_protected(pinfo, '"', '"');
  vector<miString> stokens;
  miString key, value;
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
    miString labeltype = tokens[i];
    stokens = labeltype.split('\"', '\"', "=", true);
    if (stokens.size() > 1) {
      key = stokens[0].downcase();
      value = stokens[1];
    }
    if (labeltype.upcase() == "DATA") {
      atype = anno_data;
    } else if (labeltype.upcase().contains("ANNO=")) {
      //complex annotation with <><>
      atype = anno_text;
      Annotation a;
      a.str = labeltype.substr(5, labeltype.length() - 5);
      if (a.str[0] == '"')
        a.str = a.str.substr(1, a.str.length() - 2);
      a.col = poptions.textcolour;
      a.spaceLine = false;
      annotations.push_back(a);
    } else if (labeltype.upcase().contains("TEXT=")) {
      atype = anno_text;
      if (stokens.size() > 1) {
        Annotation a;
        a.str = stokens[1];
        if (a.str[0] == '"')
          a.str = a.str.substr(1, a.str.length() - 2);
        a.col = poptions.textcolour;
        miString s = a.str;
        s.trim();
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

  int n = annotations.size();
  for (int i = 0; i < n; i++) {
    annotations[i].oldFormat = false;
    annotations[i].vstr = split(annotations[i].str, '<', '>');
    //use the whole string if no <> delimiters found - old text format
    if (!annotations[i].vstr.size()) {
      annotations[i].vstr.push_back(annotations[i].str);
      annotations[i].oldFormat = true;
    }
  }
  orig_annotations = annotations;
}

bool AnnotationPlot::putElements()
{
  //  DEBUG_ << "AnnotationPlot::putElements";
  //decode strings, put into elements...
  vector<miString> stokens, tokens, elementstrings;
  vector<Annotation> anew;
  nothingToDo = true;

  int n = annotations.size();
  for (int i = 0; i < n; i++) {
    int subel = 0;
    size_t m = annotations[i].annoElements.size();
    for ( size_t j = 0; j < m; ++j ) {
      delete annotations[i].annoElements[j].classplot;
      annotations[i].annoElements[j].classplot = NULL;
    }
    annotations[i].annoElements.clear();
    annotations[i].hAlign = align_left;//default
    annotations[i].polystyle = poly_none;//default
    annotations[i].spaceLine = false;
    annotations[i].wid = 0;
    annotations[i].hei = 0;
    if (annotations[i].str.empty())
      continue;
    elementstrings.clear();
    elementstrings = expanded(annotations[i].vstr);
    int nel = elementstrings.size();
    for (int l = 0; l < nel; l++) {
      //      DEBUG_ <<"  elementstrings[l]:"<<elementstrings[l];
      if (elementstrings[l].contains("horalign=")) {
        //sets alignment for the whole annotation !
        stokens = elementstrings[l].split('\"', '\"', ",", true);
        int mtokens = stokens.size();
        for (int k = 0; k < mtokens; k++) {
          vector<miString> subtokens = stokens[k].split('=');
          if (subtokens.size() != 2)
            continue;
          if (subtokens[0] == "horalign") {
            if (subtokens[1] == "center")
              annotations[i].hAlign = align_center;
            if (subtokens[1] == "right")
              annotations[i].hAlign = align_right;
            if (subtokens[1] == "left")
              annotations[i].hAlign = align_left;
          }
        }
      } else if (elementstrings[l].contains("bcolour=")) {
        vector<miString> stokens = elementstrings[l].split('=');
        if (stokens.size() != 2)
          continue;
        Colour c = Colour(stokens[1]);
        annotations[i].bordercolour = c;
      } else if (elementstrings[l].contains("polystyle=")) {
        vector<miString> stokens = elementstrings[l].split('=');
        if (stokens.size() != 2)
          continue;
        if (stokens[1] == "fill")
          annotations[i].polystyle = poly_fill;
        else if (stokens[1] == "border")
          annotations[i].polystyle = poly_border;
        else if (stokens[1] == "both")
          annotations[i].polystyle = poly_both;
        else if (stokens[1] == "none")
          annotations[i].polystyle = poly_none;

      } else if (elementstrings[l] == "\\box") {
        subel--;
      } else {
        element e;
        if (annotations[i].oldFormat) {
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
          annotations[i].annoElements.push_back(e);
        } else {
          if (decodeElement(elementstrings[l], e)) {
            nothingToDo = false;
            addElement2Vector(annotations[i].annoElements, e, subel);
          }
          if (elementstrings[l].compare(0, 3, "box") == 0)
            subel++;
        }
      }
    }
    anew.push_back(annotations[i]);
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

bool AnnotationPlot::decodeElement(miString elementstring, element& e)
{
  //    DEBUG_ <<"EL:"<<elementstring;
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
  if (elementstring.contains("symbol=")) {
    e.eType = symbol;
  } else if (elementstring.contains("arrow=")) {
    e.eType = arrow;
  } else if (elementstring.contains("image=")) {
    e.eType = image;
  } else if (elementstring.contains("text=")) {
    e.eType = text;
  } else if (elementstring.contains("input=")) {
    e.eType = input;
    editable = true;
  } else if (elementstring.contains("table=") && elementstring.contains(";")) {
    e.eType = table;
    vector<miString> stokens = elementstring.split('\"', '\"', ",", true);
    e.classplot = new LegendPlot(stokens[0]);
    e.classplot->setPlotOptions(poptions);
  } else if (elementstring.contains("box")) {
    e.eType = box;
  } else {
    return false;
  }

  //element options
  vector<miString> stokens = elementstring.split('\"', '\"', ",", true);
  int mtokens = stokens.size();
  for (int k = 0; k < mtokens; k++) {
    if (stokens[k] == "vertical") {
      e.horizontal = false;
    } else {
      vector<miString> subtokens = stokens[k].split('\"', '\"', "=", true);
      if (subtokens.size() != 2)
        continue;
      if (subtokens[0] == "text" || subtokens[0] == "input") {
        e.eText = subtokens[1];
        editable = true;
        if (e.eText[0] == '"')
          e.eText = e.eText.substr(1, e.eText.length() - 2);
      }
      if (subtokens[0] == "title" && e.eType == table) {
        miString title = subtokens[1];
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
        subtokens[1].remove('"');
        e.classplot->setSuffix(subtokens[1]);
      }
    }
  }
  return true;
}

bool AnnotationPlot::plot()
{
#ifdef DEBUGPRINT
  DEBUG_ << "++ AnnotationPlot::plot() ++";
#endif
  if (!enabled || !annotations.size() || nothingToDo)
    return false;

  borderline.clear();
  scaleAnno = false;
  plotAnno = true;

  Rectangle window;
  if (plotRequested && requestedarea.P() == area.P()) {
    window = requestedarea.R();
    if (window.x1 == window.x2 || window.y1 == window.y2)
      window = fullrect;
    if (cxratio > 0 && cyratio > 0)
      scaleAnno = true;
  } else
    window = fullrect;

  fontsizeToPlot = int(poptions.fontsize);

  scaleFactor = 1.0;

  //border and offset of annotations
  border = cmargin * window.width();
  //get dimensions of annotations and of box around annotations(xbox,ybox)
  bbox = window;
  getXYBox();
  //check if annotations should be scaled, get new dimensions
  if (scaleAnno)
    getXYBoxScaled(window);

  int n = annotations.size();

  // draw filled area
  Colour fc = poptions.fillcolour;
  if (fc.A() < Colour::maxv && rgbmode) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  if (poptions.polystyle != poly_border && poptions.polystyle != poly_none) {
    if (rgbmode)
      glColor4ubv(fc.RGBA());
    else
      glIndexi(fc.Index());
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBegin(GL_POLYGON);
    glVertex2f(bbox.x1, bbox.y1);
    glVertex2f(bbox.x1, bbox.y2);
    glVertex2f(bbox.x2, bbox.y2);
    glVertex2f(bbox.x2, bbox.y1);
    glEnd();
  }
  glDisable(GL_BLEND);

  // draw outline
  if (poptions.polystyle != poly_fill && poptions.polystyle != poly_none) {
    if (rgbmode)
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

  UpdateOutput();

  //plotAnno could be false if annotations too big for box
  // return here after plotted box only
  if (!plotAnno)
    return true;

  float x, y;

  // draw the annotations

  y = bbox.y2 - border;

  for (int i = 0; i < n; i++) {
    if (annotations[i].spaceLine) {
      y -= (annotations[i].hei + spacing) * 0.5;
    } else {
      x = bbox.x1 + border;
      Annotation & anno = annotations[i];
      y -= anno.hei;
      if (anno.hAlign == align_center)
        x += 0.5 * (maxwid - anno.wid);
      if (anno.hAlign == align_right)
        x += (maxwid - anno.wid);
      //draw one annotation - one line
      Colour c = anno.col;
      if (c == backgroundColour)
        c = backContrastColour;
      currentColour = c;
      if (rgbmode)
        glColor4ubv(c.RGBA());
      else
        glIndexi(c.Index());
      plotElements(annotations[i].annoElements, x, y, annotations[i].hei);
      y -= spacing;
    }
  }
  UpdateOutput();

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

  UpdateOutput();

  return true;
}

bool AnnotationPlot::plotElements(vector<element>& annoEl, float& x, float& y,
    float annoHeight, bool horizontal)
{
  //  DEBUG_ <<"plotElements:"<<annoEl.size();
  float fontsizeScale;
  float wid, hei;
  int nel = annoEl.size();
  if (hardcopy)
    fontsizeScale = fp->getSizeDiv();
  else
    fontsizeScale = 1.0;

  //    DEBUG_ <<"fontsizeScale:"<<fontsizeScale;
  for (int j = 0; j < nel; j++) {
    if (!horizontal && j != 0)
      y -= annoEl[j].height;

    if (rgbmode)
      glColor4ubv(currentColour.RGBA());
    else
      glIndexi(currentColour.Index());

    if (annoEl[j].polystyle == poly_fill || annoEl[j].polystyle == poly_both) {

      Colour fc = poptions.fillcolour;
      if (fc.A() < Colour::maxv && rgbmode) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      }
      if (rgbmode)
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
    UpdateOutput();

    // get coordinates of border, draw later
    if (annoEl[j].polystyle == poly_border || annoEl[j].polystyle == poly_both) {
      Border bl;
      bl.x1 = x - border;
      bl.y1 = y - border;
      bl.x2 = x + border + annoEl[j].width;
      bl.y2 = y + border + annoEl[j].height;
      borderline.push_back(bl);
    }

    if (annoEl[j].textcolour.exists()) {
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
      fp->set(poptions.fontname, annoEl[j].eFace, annoEl[j].eSize
          * fontsizeToPlot);
      miString astring = annoEl[j].eText;
      astring += " ";
      fp->getStringSize(astring.c_str(), wid, hei);
      wid *= fontsizeScale;
      hei *= fontsizeScale;
      if (annoEl[j].eHalign == align_right && j + 1 == nel)
        x = bbox.x2 - wid - border;
      fp->drawStr(astring.c_str(), x, y, 0.0);
      //remember corners of box around text for marking and editing
      annoEl[j].x1 = x;
      annoEl[j].y1 = y;
      annoEl[j].x2 = x + wid;
      annoEl[j].y2 = y + hei;
      if (annoEl[j].inEdit) {
        //Hvis* editeringstext,tegn strek for mark�r og for
        //markert tekst
        float w, h;
        miString substring = astring.substr(0, annoEl[j].itsCursor);
        fp->getStringSize(substring.c_str(), w, h);
        glColor4f(0, 0, 0, 1.0);
        glBegin(GL_LINE_STRIP);
        glVertex2f(annoEl[j].x1 + w, annoEl[j].y1);
        glVertex2f(annoEl[j].x1 + w, annoEl[j].y2);
        glEnd();
      }
    } else if (annoEl[j].eType == symbol) {
      fp->set(annoEl[j].eFont, annoEl[j].eFace, annoEl[j].eSize
          * fontsizeToPlot);
      float tmpwid;
      fp->getCharSize(annoEl[j].eCharacter, tmpwid, hei);
      fp->drawChar(annoEl[j].eCharacter, x, y, 0.0);
      //set back to normal font and draw one blank
      fp->set(poptions.fontname, poptions.fontface, annoEl[j].eSize
          * fontsizeToPlot);
      miString astring = " ";
      fp->drawStr(astring.c_str(), x, y, 0.0);
      fp->getStringSize(astring.c_str(), wid, hei);
      wid *= fontsizeScale;
      wid += tmpwid;
      hei *= fontsizeScale;
    } else if (annoEl[j].eType == image) {
      ImageGallery ig;
      float scale = annoEl[j].eSize * scaleFactor;
      hei = ig.height(annoEl[j].eImage) * scale;
      wid = ig.width(annoEl[j].eImage) * scale;
      ig.plotImage(annoEl[j].eImage, x + wid / 2, y + hei / 2, true, scale,
          annoEl[j].eAlpha);
    } else if (annoEl[j].eType == table) {
      hei = annoEl[j].classplot->height();
      if (poptions.v_align == align_top)
        annoEl[j].classplot->plot(x, y + annoHeight);
      else if (poptions.v_align == align_center)
        annoEl[j].classplot->plot(x, y + (annoHeight + hei) / 2.);
      else
        annoEl[j].classplot->plot(x, y + hei);
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

  if (rgbmode)
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
#ifdef DEBUGPRINT
  DEBUG_ << "++ AnnotationPlot::getAnnoSize:" <<annoEl.size();
#endif
  float fontsizeScale;

  float width = 0, height = 0;
  int nel = annoEl.size();
  for (int j = 0; j < nel; j++) {
    float w = 0.0, h = 0.0;
    if (annoEl[j].eType == text || annoEl[j].eType == input) {
      fp->set(poptions.fontname, annoEl[j].eFace, annoEl[j].eSize
          * fontsizeToPlot);
      miString astring = annoEl[j].eText;
      astring += " ";
      fp->getStringSize(astring.c_str(), w, h);
      if (hardcopy)
        fontsizeScale = fp->getSizeDiv();
      else
        fontsizeScale = 1.0;
      w *= fontsizeScale;
      h *= fontsizeScale;
    } else if (annoEl[j].eType == image) {
      miString aimage = annoEl[j].eImage;
      ImageGallery ig;
      float scale = annoEl[j].eSize * scaleFactor;
      h = ig.height(aimage) * scale;
      w = ig.width(aimage) * scale;
    } else if (annoEl[j].eType == table) {
      h = annoEl[j].classplot->height();
      w = annoEl[j].classplot->width();
    } else if (annoEl[j].eType == arrow) {
      h = annoEl[j].arrowLength / 2.;
      w = annoEl[j].arrowLength;
    } else if (annoEl[j].eType == symbol) {
      fp->set(annoEl[j].eFont, poptions.fontface, annoEl[j].eSize
          * fontsizeToPlot);
      fp->getCharSize(annoEl[j].eCharacter, w, h);
      fp->set(poptions.fontname, poptions.fontface, annoEl[j].eSize
          * fontsizeToPlot);
      if (hardcopy)
        fontsizeScale = fp->getSizeDiv();
      else
        fontsizeScale = 1.0;
      miString astring = " ";
      fp->getStringSize(astring.c_str(), w, h);
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

void AnnotationPlot::getXYBox()
{
  //  DEBUG_ <<"getXYBox()";

  float xoffset = 0, yoffset = 0;
  xoffset = cxoffset * bbox.width();
  yoffset = cyoffset * bbox.height();

  float maxhei = 0;
  float wid, hei;
  float totalhei = 0;
  maxwid = 0;
  spacing = 0;
  //find width and height of each annotation, and max width and height
  int n = annotations.size();
  for (int i = 0; i < n; i++) {
    getAnnoSize(annotations[i].annoElements, wid, hei);
    //needed???????????
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
  maxwid += 2 * border;
  spacing = cspacing * maxhei;
  totalhei += (n - 1) * spacing + 2 * border;

  //size of total annotation (bbox)
  // set x-coordinates
  if (poptions.h_align == align_left) {
    bbox.x1 += xoffset;
  } else if (poptions.h_align == align_right) {
    bbox.x1 = bbox.x2 - maxwid - xoffset;
  } else if (poptions.h_align == align_center) {
    bbox.x1 = (bbox.x1 + bbox.x2) / 2.0 - (maxwid + xoffset) / 2.0;
  }
  bbox.x2 = bbox.x1 + maxwid;

  // set y-coordinates
  if (poptions.v_align == align_bottom) {
    bbox.y1 += yoffset;
  } else if (poptions.v_align == align_top) {
    bbox.y1 = bbox.y2 - totalhei - yoffset;
  } else if (poptions.v_align == align_center) {
    bbox.y1 = (bbox.y1 + bbox.y2) / 2.0 - (totalhei + yoffset) / 2.0;
  }
  bbox.y2 = bbox.y1 + totalhei;

  //rectangle for each annotation
  float x1=0, x2=0, y1=0, y2=0;
  if (poptions.v_align == align_bottom) {
    y1 = bbox.y1 + totalhei;
  } else if (poptions.v_align == align_top) {
    y1 = bbox.y2;
  } else if (poptions.v_align == align_center) {
    y1 = (bbox.y1 + bbox.y2) / 2.0 + totalhei / 2.0;
  }
  y2 = y1 - annotations[0].hei - border;

  for (int i = 0; i < n; i++) {
    if (poptions.h_align == align_left) {
      x1 = bbox.x1;
      x2 = x1 + annotations[i].wid + 2 * border;
    } else if (poptions.h_align == align_right) {
      x2 = bbox.x2;
      x1 = x2 - annotations[i].wid - 2 * border;
    } else if (poptions.h_align == align_center) {
      x1 = bbox.x1 + (bbox.x2 - bbox.x1) / 2 - annotations[i].wid / 2 - border;
      x2 = bbox.x1 + (bbox.x2 - bbox.x1) / 2 + annotations[i].wid / 2 + border;
    }
    if (i == n - 1)
      y2 -= border;
    annotations[i].rect = Rectangle(x1, y1, x2, y2);
    if (i == n - 1)
      break;
    y1 = y2;
    y2 -= annotations[i + 1].hei;
  }

}

void AnnotationPlot::getXYBoxScaled(Rectangle& window)
{
  float dxFrame = 0, dyFrame = 0;
  float xboxScale, yboxScale;
  //annotations should be scaled
  //xratio is width of annotation divided by width of frame
  dxFrame = window.width();
  dyFrame = window.height();
  xboxScale = cxratio * dxFrame;
  yboxScale = cyratio * dyFrame;
  scaleFactor = (xboxScale - 2 * border) / maxwid;
  fontsizeToPlot = fontsizeToPlot * scaleFactor;
  //get estimated width with new fontsizetoplot
  getXYBox();

  //Keep old bbox in order to calc scale
  Rectangle bboxOld = bbox;

  //check if annotation is too big for the box, this could happen if
  // not small enough fonts available (plot empty box)
  if (bbox.width() * 0.5 > xboxScale || bbox.height() * 0.5 > yboxScale)
    plotAnno = false;

  //change bbox to correct size and position
  if (poptions.h_align == align_right)
    bbox.x1 = bbox.x2 - xboxScale;
  else if (poptions.h_align == align_left)
    bbox.x2 = bbox.x1 + xboxScale;
  else {
    float w = bbox.width();
    bbox.x1 = bbox.x1 + 0.5 * (w - xboxScale);
    bbox.x2 = bbox.x2 - 0.5 * (w - xboxScale);
  }
  if (poptions.v_align == align_top)
    bbox.y1 = bbox.y2 - yboxScale;
  else if (poptions.h_align == align_bottom)
    bbox.y2 = bbox.y1 + yboxScale;
  else {
    float h = bbox.height();
    bbox.y1 = bbox.y1 + 0.5 * (h - yboxScale);
    bbox.y2 = bbox.y2 - 0.5 * (h - yboxScale);
  }

  //scale width and height of each annotation
  int n = annotations.size();
  for (int i = 0; i < n; i++) {
    annotations[i].wid *= bbox.width() / bboxOld.width();
    annotations[i].hei *= bbox.height() / bboxOld.height();
  }
  spacing *= bbox.height() / bboxOld.height();

}

bool AnnotationPlot::markAnnotationPlot(int x, int y)
{
  //if not editable annotationplot, do not mark
  if (!editable)
    return false;
  bool wasMarked = isMarked;
  isMarked = false;
  //convert x and y...
  float xpos = x * fullrect.width() / pwidth + fullrect.x1;
  float ypos = y * fullrect.height() / pheight + fullrect.y1;
  if (xpos > bbox.x1 && xpos < bbox.x2 && ypos > bbox.y1 && ypos < bbox.y2) {
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
          if (xpos > annoEl.x1 && xpos < annoEl.x2 && ypos > annoEl.y1 && ypos
              < annoEl.y2)
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

miString AnnotationPlot::getMarkedAnnotation()
{
  miString text;
  if (isMarked) {
    int n = annotations.size();
    for (int i = 0; i < n; i++) {
      int nel = annotations[i].annoElements.size();
      for (int j = 0; j < nel; j++) {
        element annoEl = annotations[i].annoElements[j];
        if (annoEl.isInside == true) {
          text = annoEl.eText;
          //text.trim();
        }
      }
    }
  }
  return text;
}

void AnnotationPlot::changeMarkedAnnotation(miString text, int cursor,
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

vector<miString> AnnotationPlot::split(const miString eString, const char s1,
    const char s2)
{
  /*finds entries delimited by s1 and s2
   (f.ex. s1=<,s2=>) <"this is the entry">
   anything inside " " is ignored as delimiters
   f.ex. <"this is also > an entry">
   */
  int stop, start, stop2, start2, len;
  vector<miString> vec;

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

miString AnnotationPlot::writeElement(element& annoEl)
{

  miString str = "<";
  switch (annoEl.eType) {
  case (text):
    str += "text=\"" + annoEl.eText + "\"";
    if (!annoEl.eFont.empty())
      str += ",font=" + annoEl.eFont;
    break;
  case (symbol):
    str += "symbol=" + miString(annoEl.eCharacter);
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

miString AnnotationPlot::writeAnnotation(miString prodname)
{

  miString str;
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
      if (annoEl.eType == input && annoEl.eName.exists()) {
        map<miString, miString> oldInputText = oldAnno->inputText;
        map<miString, miString>::iterator pf = oldInputText.find(annoEl.eName);
        if (pf != oldInputText.end()) {
          annoEl.eText = pf->second;
          if (annoEl.eName == "number" && newProduct)
            //increase the number by one if new product !
            annoEl.eText = miString(atoi(annoEl.eText.c_str()) + 1);
        }
      }
    }
  }
}

const vector<AnnotationPlot::Annotation>& AnnotationPlot::getAnnotations()
{
  return annotations;
}

vector<vector<miString> > AnnotationPlot::getAnnotationStrings()
{
  //  DEBUG_ <<"AnnotationPlot::getAnnotationStrings():"<<annotations.size();
  vector<vector<miString> > vvstr;
  bool orig = false;

  int n = annotations.size();
  for (int i = 0; i < n; i++) {
    int m = annotations[i].vstr.size();
    for (int j = 0; j < m; j++) {
      if (annotations[i].vstr[j].contains("arrow")) {
        orig = true;
        break;
      }
    }
    if (orig)
      break;
  }

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

bool AnnotationPlot::setAnnotationStrings(vector<vector<miString> >& vvstr)
{
  int n = vvstr.size();
  //  DEBUG_ <<"setAnnotationStrings:"<<n;
  if (n == 0)
    return false;

  int m = annotations.size();

  if (n != m)
    return false;

  bool keep = false;
  for (int i = 0; i < n; i++) {
    if (vvstr[i].size())
      keep = true;
    annotations[i].vstr = vvstr[i];
  }
  putElements();

  return keep;
}
