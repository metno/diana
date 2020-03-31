/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2018 met.no

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

#include "diPainter.h"

#include "diField/diRectangle.h"
#include "diLocalSetupParser.h"
#include "miSetupParser.h"
#include "util/string_util.h"

#include <mi_fieldcalc/math_util.h>

#include <puTools/miStringFunctions.h>

#include <QImage>
#include <QPointF>
#include <QPolygonF>

#include <cmath>

#define MILOGGER_CATEGORY "diana.DiPainter"
#include <miLogger/miLogging.h>

struct DiCanvasPrivate
{
  typedef std::map<std::string, std::string, diutil::op_iless> aliases_t;
  aliases_t aliases;
  bool printing;
};

DiCanvas::DiCanvas()
    : mP(new DiCanvasPrivate)
{
  mP->printing = false;
}

DiCanvas::~DiCanvas()
{
}

void DiCanvas::parseFontSetup()
{
  const std::string sf_name = "FONTS";
  std::vector<std::string> sect_fonts;
  if (miutil::SetupParser::getSection(sf_name, sect_fonts))
    parseFontSetup(sect_fonts);
  setFont(diutil::BITMAPFONT, diutil::F_NORMAL, 10);
}

void DiCanvas::parseFontSetup(const std::vector<std::string>& sect_fonts)
{
  METLIBS_LOG_SCOPE();
  const std::string key_font = "font";
  const std::string key_fonttype = "type";
  const std::string key_fontface = "face";
  const std::string key_fontname = "name";
  const std::string key_postscript = "postscript";
  const std::string key_psxscale = "ps-scale-x";
  const std::string key_psyscale = "ps-scale-y";
  const std::string key_fontpath = "fontpath";

  const std::string key_bitmap = "bitmap";
  const std::string key_scaleable = "scaleable";
  const std::string key_ttbitmap = "tt_bitmap"; // use bitmap FTGL font
  const std::string key_ttpixmap = "tt_pixmap";
  const std::string key_tttexture = "tt_texture";
  const std::string key_texture = "texture";

  mP->aliases.clear();

  std::string fontpath = LocalSetupParser::basicValue("fontpath");
  if (fontpath.empty())
    fontpath = "fonts/";

  for (std::vector<std::string>::const_iterator it = sect_fonts.begin(); it != sect_fonts.end(); ++it) {
    std::string fontfam;
    std::string fontname;
    std::string fonttype;
    std::string fontface = "NORMAL";

    std::vector<std::string> stokens = miutil::split(*it, " ");
    for (unsigned int j = 0; j < stokens.size(); j++) {
      std::string key;
      std::string val;
      miutil::SetupParser::splitKeyValue(stokens[j], key, val);

      if (key == key_font)
        fontfam = val;
      else if (key == key_fonttype)
        fonttype = val;
      else if (key == key_fontface)
        fontface = val;
      else if (key == key_fontname)
        fontname = val;
      else if (key == key_postscript || key == key_psxscale || key == key_psyscale)
        ; // ignore these options
      else if (key == key_fontpath)
        fontpath = val;
      else
        mP->aliases[key] = val;
    }

    if (fonttype.empty() || fontfam.empty() || fontname.empty())
      continue;

    const std::string fonttype_lc = miutil::to_lower(fonttype);
    std::string fontfilename = fontpath + "/" + fontname;
    const bool use_bitmap = (fonttype_lc == key_ttbitmap);

    if (fonttype_lc == key_bitmap) {
      // nothing
    } else if (fonttype_lc == key_scaleable || use_bitmap || fonttype_lc == key_ttpixmap
        || fonttype_lc == key_tttexture || fonttype_lc == key_texture)
    {
      defineFont(fontfam, fontfilename, diutil::fontFaceFromString(fontface), use_bitmap);
    }
  }

  const std::string dianafontpath = DATAROOTDIR "/diana/" PVERSION "/fonts";
  const std::string& symbolfont = lookupFontAlias(diutil::METSYMBOLFONT);
  if (!hasFont(symbolfont)) {
    const std::string file = dianafontpath + "/metsymbols.ttf";
    METLIBS_LOG_WARN("adding default symbol font '" << symbolfont << "' from '" << file << "'");
    defineFont(symbolfont, file, diutil::F_NORMAL, false);
  }

  const std::string& scalefont = lookupFontAlias(diutil::SCALEFONT); // also fallback for BITMAPFONT
  if (!hasFont(scalefont)) {
    const std::string file = dianafontpath + "/Vera.ttf";
    METLIBS_LOG_WARN("adding default scale font '" << scalefont << "' from '" << file << "'");
    defineFont(scalefont, file, diutil::F_NORMAL, false);
    defineFont(scalefont, dianafontpath + "/VeraBd.ttf", diutil::F_BOLD, false);
    defineFont(scalefont, dianafontpath + "/VeraIt.ttf", diutil::F_ITALIC, false);
    defineFont(scalefont, dianafontpath + "/VeraBI.ttf", diutil::F_BOLD_ITALIC, false);
  }
}

const std::string& DiCanvas::lookupFontAlias(const std::string& family)
{
  DiCanvasPrivate::aliases_t::const_iterator it = mP->aliases.find(family);
  const std::string* f;
  if (it != mP->aliases.end())
    f = &it->second;
  else
    f = &family;
  if (!hasFont(*f)) {
    // try to fallback to some font alias that is likely defined
    if (!diutil::iequals(*f, diutil::METSYMBOLFONT) && !diutil::iequals(*f, diutil::SCALEFONT)) {
      static const std::string METSYMBOL = "metsymbol";
      const bool sym = diutil::icontains(*f, METSYMBOL);
      const std::string& fallback = sym ? diutil::METSYMBOLFONT : diutil::SCALEFONT;
      it = mP->aliases.find(fallback);
      if (it != mP->aliases.end())
        f = &it->second;
    }
  }
  return *f;
}

bool DiCanvas::setFont(const std::string& family)
{
  return selectFont(lookupFontAlias(family));
}

bool DiCanvas::setFont(const std::string& family, diutil::FontFace face, float size)
{
  return selectFont(lookupFontAlias(family), face, size);
}

bool DiCanvas::setFont(const std::string& family, const std::string& face, float size)
{
  return setFont(family, diutil::fontFaceFromString(face), size);
}

bool DiCanvas::getTextSize(const std::string& text, float& w, float& h)
{
  return getTextSize(QString::fromStdString(text), w, h);
}

bool DiCanvas::getTextSize(const char* text, float& w, float& h)
{
  return getTextSize(QString::fromLatin1(text), w, h);
}

bool DiCanvas::getTextRect(const std::string& text, float& x, float& y, float& w, float& h)
{
  return getTextRect(QString::fromStdString(text), x, y, w, h);
}

bool DiCanvas::getTextRect(const char* text, float& x, float& y, float& w, float& h)
{
  return getTextRect(QString::fromLatin1(text), x, y, w, h);
}

bool DiCanvas::getTextSize(const QString& str, float& w, float& h)
{
  float dummy_x, dummy_y;
  return getTextRect(str, dummy_x, dummy_y, w, h);
}

bool DiCanvas::isPrinting() const
{
  return mP->printing;
}

void DiCanvas::setPrinting(bool printing)
{
  mP->printing = printing;
}

bool DiCanvas::getCharSize(int c, float& w, float& h)
{
  return getTextSize(QString(QChar(c)), w, h);
}

// ========================================================================

DiPainter::DiPainter(DiCanvas* canvas)
  : mCanvas(canvas)
{
}

void DiPainter::setVpGlSize(int vpw, int vph, float glw, float glh)
{
  if (canvas())
    canvas()->setVpGlSize(vpw, vph, glw, glh);
}

bool DiPainter::setFont(const std::string& font)
{
  if (!canvas())
    return false;
  return canvas()->setFont(font);
}

bool DiPainter::setFont(const std::string& font, float size, diutil::FontFace face)
{
  if (!canvas())
    return false;
  return canvas()->setFont(font, face, size);
}

bool DiPainter::setFont(const std::string& font, const std::string& face, float size)
{
  if (!canvas())
    return false;
  return canvas()->setFont(font, face, size);
}

bool DiPainter::setFontSize(float size)
{
  if (!canvas())
    return false;
  return canvas()->setFontSize(size);
}

bool DiPainter::getCharSize(int ch, float& w, float& h)
{
  if (!canvas())
    return false;
  return canvas()->getCharSize(ch, w, h);
}

bool DiPainter::getTextSize(const char* text, float& w, float& h)
{
  if (!canvas())
    return false;
  return canvas()->getTextSize(text, w, h);
}

bool DiPainter::getTextSize(const std::string& text, float& w, float& h)
{
  if (!canvas())
    return false;
  return canvas()->getTextSize(text, w, h);
}

bool DiPainter::getTextSize(const QString& text, float& w, float& h)
{
  if (!canvas())
    return false;
  return canvas()->getTextSize(text, w, h);
}

bool DiPainter::drawText(const QString& text, float x, float y, float angle)
{
  return drawText(text, QPointF(x, y), angle);
}

bool DiPainter::drawText(const char* text, float x, float y, float angle)
{
  return drawText(QString(text), QPointF(x, y), angle);
}

bool DiPainter::drawText(const std::string& text, float x, float y, float angle)
{
  return drawText(QString::fromStdString(text), QPointF(x, y), angle);
}

bool DiPainter::drawChar(int c, float x, float y, float angle)
{
  return drawText(QString(QChar(c)), QPointF(x, y), angle);
}

void DiPainter::drawRect(bool fill, const Rectangle& r)
{
  drawRect(fill, r.x1, r.y1, r.x2, r.y2);
}

void DiPainter::drawCross(float x, float y, float dxy, bool diagonal)
{
  if (diagonal) {
    drawLine(x - dxy, y - dxy, x + dxy, y + dxy);
    drawLine(x - dxy, y + dxy, x + dxy, y - dxy);
  } else {
    drawLine(x-dxy, y, x+dxy, y);
    drawLine(x, y-dxy, x, y+dxy);
  }
}

void DiPainter::drawArrow(float x1, float y1, float x2, float y2, float headsize)
{
  // direction
  drawLine(x1, y1, x2, y2);
  drawArrowHead(x1, y1, x2, y2, headsize);
}

void DiPainter::drawArrowHead(float x1, float y1, float x2, float y2, float headsize)
{
  // arrow (drawn as two lines)
  float dx = x2 - x1, dy = y2 - y1;
  if (dx == 0 && dy == 0)
    return;

  if (headsize != 0) {
    const float scale = headsize / miutil::absval(dx, dy);
    dx *= scale;
    dy *= scale;
  }
  const float a = -1/3., s = a / 2;
  QPolygonF points;
  points << QPointF(x2 + a*dx + s*dy, y2 + a*dy - s*dx);
  points << QPointF(x2, y2);
  points << QPointF(x2 + a*dx - s*dy, y2 + a*dy + s*dx);
  drawPolyline(points);
}
