/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2021 met.no

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

#include "diana_config.h"

#include "diAnnotationPlot.h"
#include "diGLPainter.h"
#include "diImageGallery.h"
#include "diLabelPlotCommand.h"
#include "diLegendPlot.h"
#include "diStaticPlot.h"
#include "util/charsets.h"
#include "util/string_util.h"

#include <mi_fieldcalc/math_util.h>

#include <puTools/miStringFunctions.h>

#include <QString>
#include <boost/range/adaptor/reversed.hpp>
#include <cmath>
#include <sstream>

#define MILOGGER_CATEGORY "diana.AnnotationPlot"
#include <miLogger/miLogging.h>

using namespace miutil;

namespace {
const char QUOTE = '"';

std::string unquote(const std::string& value)
{
  if (value.size() >= 2 && value.front() == QUOTE && value.back() == QUOTE)
    return std::string(++value.begin(), --value.end());
  else
    return value;
}

std::vector<std::string> anno_split(const std::string& text, const char* separators)
{
  return miutil::split_protected(text, QUOTE, QUOTE, separators, true);
}
} // namespace

AnnotationPlot::AnnotationPlot()
    : plotRequested(false)
    , editable(false)
    , isMarked(false)
    , useAnaTime(false)
{
}

AnnotationPlot::AnnotationPlot(const PlotCommand_cp& po)
    : AnnotationPlot()
{
  prepare(po);
}

AnnotationPlot::~AnnotationPlot()
{
  for (Annotation& a : annotations) {
    for (element& e : a.annoElements)
      delete e.classplot;
  }
}

const std::string AnnotationPlot::NORWEGIAN = "no";
const std::string AnnotationPlot::ENGLISH = "en";

const std::string AnnotationPlot::insertTime(const std::string& s, const miTime& time)
{
  std::string lang;
  return insertTime(s, time, lang);
}

const std::string AnnotationPlot::insertTime(const std::string& s, const miTime& time, std::string& lang)
{
  std::string es = s;
  if (miutil::contains(es, "$")) {
    if (miutil::contains(es, "$dayeng")) { miutil::replace(es, "$dayeng", "%A"); lang = ENGLISH; }
    if (miutil::contains(es, "$daynor")) { miutil::replace(es, "$daynor", "%A"); lang = NORWEGIAN; }
    miutil::replace(es, "$day", "%A");
    miutil::replace(es, "$hour", "%H");
    miutil::replace(es, "$min", "%M");
    miutil::replace(es, "$sec", "%S");
    miutil::replace(es, "$auto", "$miniclock");
  }
  if (miutil::contains(es, "%")) {
    if (miutil::contains(es, "%anor")) { miutil::replace(es, "%anor", "%a"); lang = NORWEGIAN; }
    if (miutil::contains(es, "%Anor")) { miutil::replace(es, "%Anor", "%A"); lang = NORWEGIAN; }
    if (miutil::contains(es, "%bnor")) { miutil::replace(es, "%bnor", "%b"); lang = NORWEGIAN; }
    if (miutil::contains(es, "%Bnor")) { miutil::replace(es, "%Bnor", "%B"); lang = NORWEGIAN; }
    if (miutil::contains(es, "%aeng")) { miutil::replace(es, "%aeng", "%a"); lang = ENGLISH; }
    if (miutil::contains(es, "%Aeng")) { miutil::replace(es, "%Aeng", "%A"); lang = ENGLISH; }
    if (miutil::contains(es, "%beng")) { miutil::replace(es, "%beng", "%b"); lang = ENGLISH; }
    if (miutil::contains(es, "%Beng")) { miutil::replace(es, "%Beng", "%B"); lang = ENGLISH; }
  }
  if ((miutil::contains(es, "%") || miutil::contains(es, "$")) && !time.undef())
    es = time.format(es, lang, true);
  return es;
}

const std::vector<std::string> AnnotationPlot::expanded(const std::vector<std::string>& vs)
{
  std::vector<std::string> evs;
  for (const std::string& s : vs) {
    std::string es = insertTime(s, getStaticPlot()->getTime());
    if (!fieldAnaTime.undef()) {
      miutil::replace(es, "@", "$");
      miutil::replace(es, "&", "%");
      es = insertTime(es, fieldAnaTime);
    }
    evs.push_back(es);
  }

  return evs;
}

void AnnotationPlot::setfillcolour(const Colour& c)
{
  if (atype == anno_data)
    poptions.fillcolour = c;
}

bool AnnotationPlot::prepare(const PlotCommand_cp& pc)
{
  METLIBS_LOG_SCOPE();
  LabelPlotCommand_cp cmd = std::dynamic_pointer_cast<const LabelPlotCommand>(pc);
  if (!cmd)
    return false;

  poptions.fontname = diutil::BITMAPFONT;
  poptions.textcolour = Colour::BLACK;
  setPlotInfo(cmd->all());
  if (cmd->size() < 2)
    return false;

  cmargin = 0.007f;
  cxoffset = 0.01f;
  cyoffset = 0.01f;
  cspacing = 0.2f;
  clinewidth = 1;
  cxratio = 0;
  cyratio = 0;
  useAnaTime = false;

  for (const miutil::KeyValue& kv : ooptions) {
    const std::string& key = kv.key();
    const std::string& value = kv.value();
    if (value.find_first_of("@&") != std::string::npos)
      useAnaTime = true;
    if (!kv.hasValue() && key == "data") {
      atype = anno_data;
    } else if (key == "anno" && kv.hasValue()) {
      //complex annotation with <><>
      atype = anno_text;
      Annotation a;
      a.str = unquote(value);
      a.col = poptions.textcolour;
      a.spaceLine = false;
      annotations.push_back(a);
    } else if (key == "text" && kv.hasValue()) {
      atype = anno_text;
      Annotation a;
      a.str = unquote(value);
      a.col = poptions.textcolour;
      a.spaceLine = miutil::trimmed(a.str).empty();
      annotations.push_back(a);
    } else if (key == "productname") {
      productname = value;
    } else {
      labelstrings.push_back(kv);
      if (key == "margin") {
        cmargin = kv.toFloat();
      } else if (key == "xoffset") {
        cxoffset = kv.toFloat();
      } else if (key == "yoffset") {
        cyoffset = kv.toFloat();
      } else if (key == "xratio") {
        cxratio = kv.toFloat();
      } else if (key == "yratio") {
        cyratio = kv.toFloat();
      } else if (key == "clinewidth") {
        clinewidth = kv.toInt();
      } else if (key == "plotrequested") {
        plotRequested = kv.toBool();
      }
    }
  }
  splitAnnotations();
  putElements();

  return true;
}

void AnnotationPlot::setData(const std::vector<Annotation>& a, const plottimes_t& fieldAnalysisTime)
{
  if (atype != anno_text)
    annotations = a;

  fieldAnaTime = miTime();
  if (useAnaTime) {
    // only using the latest analysis time (yet...)
    for (const miTime& fat : fieldAnalysisTime) {
      if (!fat.undef() && (fieldAnaTime.undef() || fieldAnaTime < fat))
        fieldAnaTime = fat;
    }
  }

  splitAnnotations();
  putElements();
}

void AnnotationPlot::splitAnnotations()
{
  for (Annotation& a : annotations) {
    a.oldFormat = false;
    a.vstr = split(a.str, '<', '>');
    //use the whole string if no <> delimiters found - old text format
    if (a.vstr.empty()) {
      a.vstr.push_back(a.str);
      a.oldFormat = true;
    }
  }
  orig_annotations = annotations;
}

bool AnnotationPlot::putElements()
{
  METLIBS_LOG_SCOPE();
  //decode strings, put into elements...
  std::vector<Annotation> anew;
  nothingToDo = true;

  for (Annotation& a : annotations) {
    for (element& e : a.annoElements) {
      delete e.classplot;
      e.classplot = 0;
    }
    a.annoElements.clear();

    a.hAlign = align_left;//default
    a.polystyle = poly_none;//default
    a.spaceLine = false;
    a.wid = 0;
    a.hei = 0;
    if (a.str.empty())
      continue;
    int subel = 0;
    for (const std::string& es : expanded(a.vstr)) {
      //      METLIBS_LOG_DEBUG("  es:"<<es);
      if (miutil::contains(es, "horalign=")) {
        //sets alignment for the whole annotation !
        for (const std::string& tok : anno_split(es, ",")) {
          const std::vector<std::string> subtokens = miutil::split(tok, 0, "=");
          if (subtokens.size() != 2)
            continue;
          if (subtokens[0] == "horalign") {
            if (subtokens[1] == "center")
              a.hAlign = align_center;
            if (subtokens[1] == "right")
              a.hAlign = align_right;
            if (subtokens[1] == "left")
              a.hAlign = align_left;
          }
        }
      } else if (miutil::contains(es, "bcolour=")) {
        std::vector<std::string> stokens = miutil::split(es, 0, "=");
        if (stokens.size() != 2)
          continue;
        a.bordercolour = Colour(stokens[1]);
      } else if (miutil::contains(es, "polystyle=")) {
        std::vector<std::string> stokens = miutil::split(es, 0, "=");
        if (stokens.size() != 2)
          continue;
        if (stokens[1] == "fill")
          a.polystyle = poly_fill;
        else if (stokens[1] == "border")
          a.polystyle = poly_border;
        else if (stokens[1] == "both")
          a.polystyle = poly_both;
        else if (stokens[1] == "none")
          a.polystyle = poly_none;

      } else if (es == "\\box") {
        subel--;
      } else {
        element e;
        if (a.oldFormat) {
          e.eSize = 1.0;
          e.eFace = poptions.fontface;
          e.eHalign = align_left;
          e.isInside = false;
          e.inEdit = false;
          e.x1 = e.x2 = e.y1 = e.y2 = 0;
          e.itsCursor = 0;
          e.eAlpha = 255;
          e.eType = text;
          e.eText = es;
          e.eFace = poptions.fontface;
          e.horizontal = true;
          e.polystyle = poly_none;
          e.arrowFeather = false;
          //	  e.textcolour="black";
          nothingToDo = false;
          a.annoElements.push_back(e);
        } else {
          if (decodeElement(es, e)) {
            nothingToDo = false;
            addElement2Vector(a.annoElements, e, subel);
          }
          if (es.compare(0, 3, "box") == 0)
            subel++;
        }
      }
    }
    anew.push_back(a);
  }
  annotations = anew;

  return true;
}

void AnnotationPlot::addElement2Vector(std::vector<element>& v_e, const element& e, int index)
{
  if (index > 0 && !v_e.empty())
    addElement2Vector(v_e.back().subelement, e, index-1);
  else
    v_e.push_back(e);
}

bool AnnotationPlot::decodeElement(const std::string& elementstring, element& e)
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
    std::vector<std::string> stokens = anno_split(elementstring, ",");
    e.classplot = new LegendPlot(stokens[0]);
    e.classplot->setPlotOptions(poptions);
  } else if (miutil::contains(elementstring, "box")) {
    e.eType = box;
  } else {
    return false;
  }

  //element options
  for (const std::string& tok : anno_split(elementstring, ",")) {
    if (tok == "vertical") {
      e.horizontal = false;
    } else {
      std::vector<std::string> subtokens = anno_split(tok, "=");
      if (subtokens.size() != 2)
        continue;
      if (subtokens[0] == "text" || subtokens[0] == "input") {
        e.eText = diutil::quote_removed(subtokens[1]);
        editable = true;
      }
      if (subtokens[0] == "title" && e.eType == table) {
        e.classplot->setTitle(diutil::quote_removed(subtokens[1]));
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
        e.eFace = diutil::fontFaceFromString(subtokens[1]);
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

void AnnotationPlot::plot(DiGLPainter* gl, PlotOrder zorder)
{
  METLIBS_LOG_SCOPE();
  if (zorder != PO_LINES && zorder != PO_OVERLAY)
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
  getXYBox(gl);

  // draw filled area
  Colour fc = poptions.fillcolour;
  if (fc.A() < Colour::maxv) {
    gl->Enable(DiGLPainter::gl_BLEND);
    gl->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);
  }
  if (poptions.polystyle != poly_border && poptions.polystyle != poly_none) {
    gl->setColour(fc);
    gl->drawRect(true, bbox);
  }
  gl->Disable(DiGLPainter::gl_BLEND);

  //plotAnno could be false if annotations too big for box
  // return here after plotted box only
  if (!plotAnno)
    return;

  // draw the annotations
  for (Annotation & anno : annotations) {
    //draw one annotation - one line
    const Colour& c = getStaticPlot()->notBackgroundColour(anno.col);
    currentColour = c;
    gl->setColour(c);
    plotElements(gl, anno.annoElements, anno.rect.x1, anno.rect.y1, anno.hei);
  }

  // draw outline
  if (poptions.polystyle != poly_fill && poptions.polystyle != poly_none) {
    gl->setLineStyle(Colour(poptions.bordercolour), clinewidth);
    gl->drawRect(false, bbox);
  }

  //draw borders
  for (Annotation & anno : annotations) {
    if (anno.polystyle == poly_border || anno.polystyle == poly_both) {
      gl->setColour(anno.bordercolour);
      gl->drawRect(true, anno.rect);
    }
    plotBorders(gl);
  }
}

bool AnnotationPlot::plotElements(DiGLPainter* gl,
    std::vector<element>& annoEl, float& x, float& y,
    float annoHeight, bool horizontal)
{
  METLIBS_LOG_SCOPE(LOGVAL(annoEl.size()));

  float wid, hei;
  for (size_t j=0; j<annoEl.size(); ++j) {
    element& e = annoEl[j];
    if (!horizontal && j != 0)
      y -= e.height;

    gl->setColour(currentColour);

    if (e.polystyle == poly_fill || e.polystyle == poly_both) {

      const Colour& fc = poptions.fillcolour;
      if (fc.A() < Colour::maxv ) {
        gl->Enable(DiGLPainter::gl_BLEND);
        gl->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);
      }
      gl->setColour(fc);
      gl->drawRect(true, x - border, y - border, x + border + e.width, y + border + e.height);
      gl->Disable(DiGLPainter::gl_BLEND);
    }

    // get coordinates of border, draw later
    if (e.polystyle == poly_border || e.polystyle == poly_both) {
      Border bl;
      bl.x1 = x - border;
      bl.y1 = y - border;
      bl.x2 = x + border + e.width;
      bl.y2 = y + border + e.height;
      borderline.push_back(bl);
    }

    if (not e.textcolour.empty())
      gl->setColour(Colour(e.textcolour));

    wid = hei = 0.0;
    if (e.eType == box) {
      //keeping coor. of the bottom left corner
      float tmpx = x, tmpy = y;
      if (horizontal && !e.horizontal)
        y += e.height - e.subelement[0].height;
      plotElements(gl, e.subelement, x, y, e.height, e.horizontal);
      if (horizontal)
        y = tmpy;
      else
        x = tmpx;
      wid = e.width;
      hei = e.height;
    } else if (e.eType == text || e.eType == input) {
      //change text colour if editable
      if (e.eType == input && isMarked) {
        if (e.isInside) {
          //red - ctrl-e works
          gl->setColour(Colour::RED);
        } else if (isMarked) {
          //show text to be edited in blue
          gl->setColour(Colour::BLUE);
        }
      }
      gl->setFont(poptions.fontname, e.eFace, e.eSize * fontsizeToPlot);
      const QString astring = QString::fromStdString(e.eText) + " ";
      gl->getTextSize(astring, wid, hei);
      if (e.eHalign == align_right && j + 1 == annoEl.size())
        x = bbox.x2 - wid - border;
      gl->drawText(astring, x, y, 0.0);
      //remember corners of box around text for marking and editing
      e.x1 = x;
      e.y1 = y;
      e.x2 = x + wid;
      e.y2 = y + hei;
      if (e.inEdit) {
        //Hvis* editeringstext,tegn strek for mark�r og for
        //markert tekst
        float w, h;
        QString substring = astring.left(e.itsCursor);
        gl->getTextSize(substring, w, h);
        gl->setColour(Colour::BLACK);
        gl->drawLine(e.x1 + w, e.y1, e.x1 + w, e.y2);
      }
    } else if (e.eType == symbol) {
      gl->setFont(e.eFont, e.eFace, e.eSize * fontsizeToPlot);
      float tmpwid;
      const QString echar(QLatin1Char(e.eCharacter));
      gl->getTextSize(echar, tmpwid, hei);
      gl->drawText(echar, x, y, 0.0);
      //set back to normal font and draw one blank
      gl->setFont(poptions.fontname, poptions.fontface, e.eSize * fontsizeToPlot);
      gl->drawText(" ", x, y, 0.0);
      gl->getTextSize(" ", wid, hei);
      wid += tmpwid;
    } else if (e.eType == image) {
      ImageGallery ig;
      float scale = e.eSize * scaleFactor;
      hei = ig.height_(e.eImage) * getStaticPlot()->getPhysToMapScaleY() * scale;
      wid = ig.width_(e.eImage) * getStaticPlot()->getPhysToMapScaleX() * scale;
      ig.plotImage(gl, getStaticPlot()->plotArea(), e.eImage, x + wid / 2, y + hei / 2, true, scale, e.eAlpha);
    } else if (e.eType == table) {
      hei = e.classplot->height(gl);
      wid = e.classplot->width(gl);
      float dy;
      if (poptions.v_align == align_top)
        dy = annoHeight;
      else if (poptions.v_align == align_center)
        dy = (annoHeight + hei) / 2.0;
      else
        dy = hei;
      e.classplot->plotLegend(gl, x, y + dy);
    } else if (e.eType == arrow) {
      wid = plotArrow(gl, x, y, e.arrowLength, e.arrowFeather);
    }
    if (horizontal)
      x += wid;
  }
  return true;
}

float AnnotationPlot::plotArrow(DiGLPainter* gl, float x, float y, float l, bool feather)
{
  if (feather) {
    x += 0.1f * l;
  }

  float dx = 0.333333f * l;
  float dy = dx * 0.5f;

  //  gl->LineWidth(2.0);
  gl->Begin(DiGLPainter::gl_LINES);
  gl->Vertex2f(x, y + dy);
  gl->Vertex2f(x + l, y + dy);
  gl->Vertex2f(x + l, y + dy);
  gl->Vertex2f(x + l - dx, y + dy * 2);
  gl->Vertex2f(x + l, y + dy);
  gl->Vertex2f(x + l - dx, y);
  gl->End();

  if (feather) {
    float x1 = x;
    float y1 = y + dy;
    float x2 = x1 - 0.13f * l;
    float y2 = y1 + 0.28f * l;

    gl->Begin(DiGLPainter::gl_LINES);
    gl->Vertex2f(x1, y1);
    gl->Vertex2f(x2, y2);
    x1 += 0.13f * l;
    x2 += 0.13f * l;
    gl->Vertex2f(x1, y1);
    gl->Vertex2f(x2, y2);
    x1 += 0.13f * l;
    x2 += 0.13f * l;
    x2 = (x1 + x2) / 2;
    y2 = (y1 + y2) / 2;
    gl->Vertex2f(x1, y1);
    gl->Vertex2f(x2, y2);
    gl->End();
  }

  gl->LineWidth(1.0);

  return l;
}

void AnnotationPlot::plotBorders(DiGLPainter* gl)
{
  gl->setLineStyle(poptions.bordercolour, clinewidth);

  for (const Border& b : borderline)
    gl->drawRect(false, b.x1, b.y1, b.x2, b.y2);
}

void AnnotationPlot::getAnnoSize(DiGLPainter* gl,
    std::vector<element> &annoEl, float& wid,
    float& hei, bool horizontal)
{
  METLIBS_LOG_SCOPE(LOGVAL(annoEl.size()));

  float width = 0, height = 0;
  for (size_t j=0; j<annoEl.size(); ++j) {
    element& e = annoEl[j];
    float w = 0.0, h = 0.0;
    if (e.eType == text || e.eType == input) {
      gl->setFont(poptions.fontname, e.eFace, e.eSize
          * fontsizeToPlot);
      const QString astring = QString::fromStdString(e.eText) + " ";
      gl->getTextSize(astring, w, h);
    } else if (e.eType == image) {
      const std::string& aimage = e.eImage;
      ImageGallery ig;
      float scale = e.eSize * scaleFactor;
      h = ig.height_(aimage) * getStaticPlot()->getPhysToMapScaleY() * scale;
      w = ig.width_(aimage) * getStaticPlot()->getPhysToMapScaleX() * scale;
    } else if (e.eType == table) {
      h = e.classplot->height(gl);
      w = e.classplot->width(gl);
    } else if (e.eType == arrow) {
      h = e.arrowLength / 2;
      w = e.arrowLength;
    } else if (e.eType == symbol) {
      gl->setFont(e.eFont, poptions.fontface, e.eSize
          * fontsizeToPlot);
      gl->getCharSize(e.eCharacter, w, h);
      gl->setFont(poptions.fontname, poptions.fontface, e.eSize
          * fontsizeToPlot);
      gl->getTextSize(" ", w, h);
      w *= 2;
    } else if (e.eType == box) {
      getAnnoSize(gl, e.subelement, w, h, e.horizontal);
    }

    if (horizontal) {
      width += w;
      miutil::maximize(height, h);
    } else {
      height += h;
      miutil::maximize(width, w);
    }
    e.width = w;
    e.height = h;

  }
  wid = width;
  hei = height;
}

/**
 * Gets an appropriate bounding box for the annotations, setting the bbox
 * member variable as a result. When this method is called, the bbox
 * corresponds to the plot window.
 */

void AnnotationPlot::getXYBox(DiGLPainter* gl)
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
  for (Annotation& a : annotations) {
    getAnnoSize(gl, a.annoElements, wid, hei);

    a.wid = wid;
    a.hei = hei;

    miutil::maximize(maxwid, wid);
    miutil::maximize(maxhei, hei);
    totalhei += hei;
    if (a.spaceLine)
      totalhei -= (hei + spacing) / 2;
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
    scaleFactor = std::min((width - 2 * border) / maxwid, float(1.0));
    fontsizeToPlot = fontsizeToPlot * scaleFactor;

    float scaledHeight = totalhei * scaleFactor;
    float scaledSpacing = (height - scaledHeight - (2 * border))/(annotations.size() - 1);
    spacing = std::min(scaledSpacing, spacing);

    // Scale the width and height of each annotation.
    for (Annotation& a : annotations) {
      a.wid *= scaleFactor;
      a.hei *= scaleFactor;
    }

    // Shrink the annotation vertically if there is free space (if the
    // spacing used was less than that available).
    if (spacing < scaledSpacing)
        height -= (annotations.size() - 1)*(scaledSpacing - spacing);
  } else {
    width = maxwid + 2 * border;
    height = totalhei + (annotations.size() - 1) * spacing + 2 * border;
  }

  // Set the x coordinates for the annotation's bounding box.
  if (poptions.h_align == align_left) {
    bbox.x1 += xoffset;
  } else if (poptions.h_align == align_right) {
    bbox.x1 = bbox.x2 - width - xoffset;
  } else if (poptions.h_align == align_center) {
    bbox.x1 = (bbox.x1 + bbox.x2 - width) / 2;
  }
  bbox.x2 = bbox.x1 + width;

  // Set the y coordinates for the annotation's bounding box.
  if (poptions.v_align == align_bottom) {
    bbox.y1 += yoffset;
  } else if (poptions.v_align == align_top) {
    bbox.y1 = bbox.y2 - height - yoffset;
  } else if (poptions.v_align == align_center) {
    bbox.y1 = ((bbox.y1 + bbox.y2) - (height + yoffset)) / 2;
  }
  bbox.y2 = bbox.y1 + height;

  // Arrange the annotations.
  float x1, x2, y1, y2;

  if (poptions.v_align == align_bottom)
    y2 = bbox.y1 + height - border;
  else if (poptions.v_align == align_top)
    y2 = bbox.y2 - border;
  else // if (poptions.v_align == align_center)
    y2 = (bbox.y1 + bbox.y2 + height) / 2;

  for (Annotation& a : annotations) {
    if (a.hAlign == align_left) {
      x1 = bbox.x1 + border;
      x2 = x1 + a.wid + border;
    } else if (a.hAlign == align_right) {
      x2 = bbox.x2 - border;
      x1 = x2 - a.wid - border;
    } else { // if (a.hAlign == align_center)
      x1 = (bbox.x2 + bbox.x1) / 2 - a.wid / 2;
      x2 = (bbox.x2 + bbox.x1) / 2 + a.wid / 2;
    }
    y1 = y2 - a.hei;
    a.rect = Rectangle(x1, y1, x2, y2);
    y2 -= a.hei + spacing;
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
    for (Annotation& anno : annotations) {
      for (element& annoEl : anno.annoElements) {
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
    for (Annotation& a : annotations) {
      for(element& annoEl : a.annoElements) {
        if (annoEl.isInside == true) {
          text = annoEl.eText;
        }
      }
    }
  }
  return text;
}

void AnnotationPlot::changeMarkedAnnotation(std::string text, int cursor, int sel1, int sel2)
{
  if (isMarked) {
    for (Annotation& a : annotations) {
      for(element& annoEl : a.annoElements) {
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
    for (Annotation& a : annotations) {
      std::vector<element>::iterator p = a.annoElements.begin();
      while (p != a.annoElements.end()) {
        if (p->isInside) {
          p = a.annoElements.erase(p);
        } else
          p++;
      }
    }
  }
}

void AnnotationPlot::startEditAnnotation()
{
  for (Annotation& a : annotations) {
    for(element& annoEl : a.annoElements) {
      if (annoEl.isInside)
        annoEl.inEdit = true;
    }
  }
}

void AnnotationPlot::stopEditAnnotation()
{
  for (Annotation& a : annotations) {
    for(element& annoEl : a.annoElements) {
      if (annoEl.isInside)
        annoEl.inEdit = false;
    }
  }
}

void AnnotationPlot::editNextAnnoElement()
{
  bool next = false;
  //find next input element after the one that is edited and mark
  for (Annotation& a : annotations) {
    for(element& annoEl : a.annoElements) {
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
    for (Annotation& a : annotations) {
      for(element& annoEl : a.annoElements) {
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
  bool last = false;
  //find next element after the one that is edited and mark
  for (Annotation& a : boost::adaptors::reverse(annotations)) {
    for(element& annoEl : boost::adaptors::reverse(a.annoElements)) {
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
    for (Annotation& a : boost::adaptors::reverse(annotations)) {
      for(element& annoEl : boost::adaptors::reverse(a.annoElements)) {
        if (annoEl.eType == input) {
          annoEl.isInside = true;
          annoEl.inEdit = true;
          return;
        }
      }
    }
  }
}

// static
std::vector<std::string> AnnotationPlot::split(const std::string eString, const char s1, const char s2)
{
  /*finds entries delimited by s1 and s2
   (f.ex. s1=<,s2=>) <"this is the entry">
   anything inside " " is ignored as delimiters
   f.ex. <"this is also > an entry">
   */
  std::vector<std::string> vec;

  if (eString.empty())
    return vec;

  const int len = eString.length();
  int start = eString.find_first_of(s1, 0) + 1;
  while (start >= 1 && start < len) {
    int start2 = eString.find_first_of('"', start);
    int stop = eString.find_first_of(s2, start);
    while (start2 > -1 && start2 < stop) {
      // search for second ", which should come before >
      const int stop2 = eString.find_first_of('"', start2 + 1);
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

std::string AnnotationPlot::writeElement(const element& annoEl)
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
  if (annoEl.eSize != 1) {
    std::ostringstream ostr;
    ostr << ",size=" << annoEl.eSize;
    str += ostr.str();
  }
  if (annoEl.textcolour != "black") {
    str += ",tcolour=" + annoEl.textcolour;
  }
  if (annoEl.eFace != diutil::F_NORMAL && annoEl.eFace != poptions.fontface)
    str += ",face=" + diutil::fontFaceToString(annoEl.eFace);
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

PlotCommand_cp AnnotationPlot::writeAnnotation(const std::string& prodname)
{
  if (prodname != productname)
    return PlotCommand_cp();

  LabelPlotCommand_p cmd = std::make_shared<LabelPlotCommand>();
  for (const Annotation& a: annotations) {
    std::string str;
    for (const element& annoEl : a.annoElements) {
      //write annoelement here !
      str += writeElement(annoEl);
    }
    if (a.hAlign == align_right)
      str += "<horalign=right>";
    else if (a.hAlign == align_center)
      str += "<horalign=center>";
    cmd->add("anno", str);
  }
  cmd->add(labelstrings);

  // we want only plotoptions actually used in AnnotationPlot
  cmd->add(poptions.toKeyValueListForAnnotation());

  return cmd;
}

void AnnotationPlot::updateInputLabels(const AnnotationPlot * oldAnno, bool newProduct)
{
  for (Annotation& a: annotations) {
    for (element& annoEl : a.annoElements) {
      if (annoEl.eType == input && not annoEl.eName.empty()) {
        const std::map<std::string, std::string>& oldInputText = oldAnno->inputText;
        const std::map<std::string, std::string>::const_iterator pf = oldInputText.find(annoEl.eName);
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

const std::vector<AnnotationPlot::Annotation>& AnnotationPlot::getAnnotations()
{
  return annotations;
}

std::vector<std::vector<std::string> > AnnotationPlot::getAnnotationStrings()
{
  METLIBS_LOG_SCOPE(LOGVAL(annotations.size()));

  bool orig = false;
  for (const Annotation& a: annotations) {
    for (const std::string& s : a.vstr) {
      if (miutil::contains(s, "arrow")) {
        orig = true;
        break;
      }
    }
    if (orig)
      break;
  }

  std::vector<std::vector<std::string> > vvstr;
  for (const Annotation& a : (orig ? orig_annotations : annotations))
    vvstr.push_back(a.vstr);
  return vvstr;
}

void AnnotationPlot::setAnnotationStrings(std::vector<std::vector<std::string> >& vvstr)
{
  METLIBS_LOG_SCOPE(LOGVAL(vvstr.size()));
  if (vvstr.empty())
    return;

  const size_t m = annotations.size();
  if (vvstr.size() != m)
    return;

  for (size_t i = 0; i < vvstr.size(); i++)
    annotations[i].vstr = vvstr[i];

  putElements();
}
