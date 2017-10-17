
#include "diPainter.h"

#include "diLocalSetupParser.h"
#include "miSetupParser.h"

#include "diField/diRectangle.h"
#include "util/math_util.h"

#include <puTools/miStringFunctions.h>

#include <QImage>
#include <QPointF>
#include <QPolygonF>

#include <cmath>

#define MILOGGER_CATEGORY "diana.DiPainter"
#include <miLogger/miLogging.h>

DiCanvas::DiCanvas()
  : mPrinting(false)
{
}

void DiCanvas::parseFontSetup()
{
  const std::string sf_name = "FONTS";
  std::vector<std::string> sect_fonts;
  if (miutil::SetupParser::getSection(sf_name, sect_fonts))
    parseFontSetup(sect_fonts);
  setFont("BITMAPFONT", 10, F_NORMAL);
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

  const std::string key_bitmapfont = "bitmapfont";
  const std::string key_scalefont = "scalefont";
  const std::string key_metsymbolfont = "metsymbolfont";

  fontFamilyAliases.clear();
  fontFamilyAliases[key_bitmapfont] = "Helvetica";
  fontFamilyAliases[key_scalefont] = "Arial";
  fontFamilyAliases[key_metsymbolfont] = "Symbol";

  std::string fontpath = LocalSetupParser::basicValue("fontpath");
  if (fontpath.empty())
    fontpath = "fonts/";

  for (std::vector<std::string>::const_iterator it = sect_fonts.begin(); it != sect_fonts.end(); ++it) {
    std::string fontfam = "";
    std::string fontname = "";
    std::string fonttype = "";
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
        fontFamilyAliases[key] = val;
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
      defineFont(fontfam, fontfilename, fontface, use_bitmap);
    }
  }
}

std::string DiCanvas::lookupFontAlias(const std::string& name)
{
  std::map<std::string, std::string>::const_iterator it
      = fontFamilyAliases.find(miutil::to_lower(name));
  if (it != fontFamilyAliases.end())
    return it->second;
  else
    return name;
}

bool DiCanvas::setFont(const std::string& font, const std::string& face, float size)
{
  FontFace f = F_NORMAL;
  if (face == "NORMAL") {
    // nothing
  } else if (face == "BOLD") {
    f = F_BOLD;
  } else if (face == "BOLD_ITALIC") {
    f = F_BOLD_ITALIC;
  } else if (face == "ITALIC") {
    f = F_ITALIC;
  } else {
    const std::string lface = miutil::to_lower(face);
    if (lface == "bold")
      f = F_BOLD;
    else if (lface == "bold_italic")
      f = F_BOLD_ITALIC;
    else if (lface == "italic")
      f = F_ITALIC;
  }
  return setFont(font, size, f);
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

bool DiPainter::setFont(const std::string& font, float size, DiCanvas::FontFace face)
{
  if (!canvas())
    return false;
  return canvas()->setFont(font, size, face);
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
    const float scale = headsize/diutil::absval(dx, dy);
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
