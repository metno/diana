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

#include <diFontManager.h>
#ifdef USE_XLIB
#include <glText/glTextX.h>
#endif
#include <glText/glTextQtTexture.h>
#include <glText/glTextTT.h>
#include <glp/GLP.h>

using namespace::miutil;

miString FontManager::fontpath;
miString FontManager::display_name;
map<miString, miString> FontManager::defaults;

static const miString key_bitmap = "bitmap";
static const miString key_scaleable = "scaleable";
static const miString key_texture = "texture";

static const miString key_bitmapfont = "bitmapfont";
static const miString key_scalefont = "scalefont";
static const miString key_metsymbolfont = "metsymbolfont";

FontManager::FontManager() :
#ifdef USE_XLIB
  xfonts(0),
#endif
  ttfonts(0), texfonts(0), current_engine(0)
{
#ifdef USE_XLIB
  if (display_name.exists()) // do not use environment-var DISPLAY
    xfonts = new glTextX(display_name);
  else
    xfonts = new glTextX();
#endif
  ttfonts = new glTextTT();
  texfonts = new glTextQtTexture();
}

FontManager::~FontManager()
{
#ifdef USE_XLIB
  if (xfonts)
    delete xfonts;
#endif
  if (ttfonts)
    delete ttfonts;
  if (texfonts)
    delete texfonts;
}

void FontManager::startHardcopy(GLPcontext* gc)
{
#ifdef USE_XLIB
  if (xfonts)
    xfonts->startHardcopy(gc);
#endif
  if (ttfonts)
    ttfonts->startHardcopy(gc);
  if (texfonts)
    texfonts->startHardcopy(gc);
}

void FontManager::endHardcopy()
{
#ifdef USE_XLIB
  if (xfonts)
    xfonts->endHardcopy();
#endif
  if (ttfonts)
    ttfonts->endHardcopy();
  if (texfonts)
    texfonts->endHardcopy();
}

// fill fontpack for testing
bool FontManager::testDefineFonts(miString path)
{
  bool res = true;
  int n;
  xfam.clear();
  ttfam.clear();
#ifdef USE_XLIB
  res = xfonts->testDefineFonts(path);
  n = xfonts->getNumFonts();
  cerr << "-- TEST Defined X-fonts:" << endl;
  for (int i = 0; i < n; i++) {
    miString s = xfonts->getFontName(i);
    xfam.insert(s);
    cerr << i << " " << s << endl;
  }
#endif
  res = res && ttfonts->testDefineFonts(path);
  n = ttfonts->getNumFonts();
  cerr << "-- TEST Defined TT-fonts:" << endl;
  for (int i = 0; i < n; i++) {
    miString s = ttfonts->getFontName(i);
    ttfam.insert(s);
    cerr << i << " " << s << endl;
  }

  return res;
}

glText::FontFace FontManager::fontFace(const miString& s)
{
  miString suface = s.upcase();
  suface.trim();
  glText::FontFace face = glText::F_NORMAL;
  if (suface != "NORMAL") {
    if (suface == "ITALIC")
      face = glText::F_ITALIC;
    else if (suface == "BOLD")
      face = glText::F_BOLD;
    else if (suface == "BOLD_ITALIC")
      face = glText::F_BOLD_ITALIC;
  }

  return face;
}

bool FontManager::parseSetup(SetupParser& sp)
{
  const miString sf_name = "FONTS";
  vector<miString> sect_fonts;

  const miString key_font = "font";
  const miString key_fonttype = "type";
  const miString key_fontface = "face";
  const miString key_fontname = "name";
  const miString key_postscript = "postscript";
  const miString key_fontpath = "fontpath";

  defaults[key_bitmapfont] = "Helvetica";
  defaults[key_scalefont] = "Arial";
  defaults[key_metsymbolfont] = "Symbol";

  xfam.clear();
  ttfam.clear();
  texfam.clear();

  if (!fontpath.exists()) {
    fontpath = sp.basicValue("fontpath");
    if (!fontpath.exists())
      fontpath = "fonts/";
  }

  if (!sp.getSection(sf_name, sect_fonts)) {
    //cerr << "Missing section " << sf_name << " in setupfile." << endl;
    testDefineFonts(fontpath);
    return false;
  }

  int n = sect_fonts.size();
  for (int i = 0; i < n; i++) {
    miString fontfam = "";
    miString fontname = "";
    miString fonttype = "";
    miString fontface = "";
    miString postscript = "";

    vector<miString> stokens = sect_fonts[i].split(" ");
    for (unsigned int j = 0; j < stokens.size(); j++) {
      miString key;
      miString val;
      sp.splitKeyValue(stokens[j], key, val);
      //cerr << "Key:" << key << " Value:" << val << endl;

      if (key == key_font)
        fontfam = val;
      else if (key == key_fonttype)
        fonttype = val;
      else if (key == key_fontface)
        fontface = val;
      else if (key == key_fontname)
        fontname = val;
      else if (key == key_postscript)
        postscript = val;
      else if (key == key_fontpath)
        fontpath = val;
      else
        defaults[key] = val;
    }

    if (!fonttype.exists() || !fontfam.exists() || !fontname.exists())
      continue;

    if (fonttype.downcase() == key_bitmap) {
#ifdef USE_XLIB
      xfam.insert(fontfam);
      if (xfonts)
        xfonts->defineFonts(fontname, fontfam, postscript);
#else
      std::cerr << "X-FONTS not supported!" << std::endl;
#endif
    } else if (fonttype.downcase() == key_scaleable) {
      ttfam.insert(fontfam);
      if (ttfonts)
        ttfonts->defineFont(fontfam, fontpath + "/" + fontname, fontFace(
            fontface), 20);

    } else if (fonttype.downcase() == key_texture) {
      texfam.insert(fontfam);
      if (texfonts)
        texfonts->defineFont(fontfam, fontname, fontFace(fontface), 10,
            postscript);
    }
  }

  /*
   std::set<miString>::iterator fitr;
   int i;
   cerr << "-- Defined X-fonts:" << endl;
   for (i = 0, fitr = xfam.begin(); fitr != xfam.end(); fitr++, i++) {
   cerr << i << " " << *fitr << endl;
   }

   cerr << "-- Defined TT-fonts:" << endl;
   for (i = 0, fitr = ttfam.begin(); fitr != ttfam.end(); fitr++, i++) {
   cerr << i << " " << *fitr << endl;
   }

   cerr << "-- Defined TEX-fonts:" << endl;
   for (i = 0, fitr = texfam.begin(); fitr != texfam.end(); fitr++, i++) {
   cerr << i << " " << *fitr << endl;
   }
   */

  return true;
}

bool FontManager::check_family(const miString& fam, miString& family)
{
  if (defaults.count(fam.downcase()) > 0)
    family = defaults[fam.downcase()];
  else
    family = fam;

  if (find(xfam.begin(), xfam.end(), family) != xfam.end()) {
#ifdef USE_XLIB
    current_engine = xfonts;
#else
    std::cerr << "X-FONTS not supported!" << std::endl;
    return false;
#endif
  } else if (find(ttfam.begin(), ttfam.end(), family) != ttfam.end()) {
    current_engine = ttfonts;
  } else if (find(texfam.begin(), texfam.end(), family) != texfam.end()) {
    current_engine = texfonts;
  } else {
    cerr << "FontManager::check_family ERROR, unknown font family:" << family
        << endl;
    return false;
  }
  return true;
}

// choose font, size and face
bool FontManager::set(const miString fam, const glText::FontFace face,
    const float size)
{
  miString family;
  if (check_family(fam, family)) {
    return current_engine->set(family, face, size);
  }
  return false;
}

// choose font, size and face
bool FontManager::set(const miString fam, const miString sface,
    const float size)
{
  glText::FontFace face = fontFace(sface);
  return set(fam, face, size);
}

bool FontManager::setFont(const miString fam)
{
  miString family;
  if (check_family(fam, family)) {
    return current_engine->setFont(family);
  }
  return false;
}

bool FontManager::setFontFace(const glText::FontFace face)
{
  if (!current_engine)
    return false;
  return current_engine->setFontFace(face);
}

bool FontManager::setFontFace(const miString sface)
{
  glText::FontFace face = fontFace(sface);
  return setFontFace(face);
}

bool FontManager::setFontSize(const float size)
{
  if (!current_engine)
    return false;
  return current_engine->setFontSize(size);
}

// printing commands
bool FontManager::drawChar(const int c, const float x, const float y,
    const float a)
{
  if (!current_engine)
    return false;
  return current_engine->drawChar(c, x, y, a);
}

bool FontManager::drawStr(const char* s, const float x, const float y,
    const float a)
{
  if (!current_engine)
    return false;
  return current_engine->drawStr(s, x, y, a);
}

// Metric commands
void FontManager::adjustSize(const int sa)
{
#ifdef USE_XLIB
  xfonts->adjustSize(sa);
#endif
  ttfonts->adjustSize(sa);
  texfonts->adjustSize(sa);
}

void FontManager::setScalingType(const glText::FontScaling fs)
{
#ifdef USE_XLIB
  xfonts->setScalingType(fs);
#endif
  ttfonts->setScalingType(fs);
  texfonts->setScalingType(fs);
}

// set viewport size in GL coordinates
void FontManager::setGlSize(const float glx1, const float glx2,
    const float gly1, const float gly2)
{
  float glw = glx2 - glx1;
  float glh = gly2 - gly1;
#ifdef USE_XLIB
  xfonts->setGlSize(glw, glh);
#endif
  ttfonts->setGlSize(glw, glh);
  texfonts->setGlSize(glw, glh);
  //texfonts->setGlSize(glx1, glx2, gly1, gly2);
}

// set viewport size in GL coordinates
void FontManager::setGlSize(const float glw, const float glh)
{
#ifdef USE_XLIB
  xfonts->setGlSize(glw, glh);
#endif
  ttfonts->setGlSize(glw, glh);
  texfonts->setGlSize(glw, glh);
}

// set viewport size in physical coordinates (pixels)
void FontManager::setVpSize(const float vpw, const float vph)
{
#ifdef USE_XLIB
  xfonts->setVpSize(vpw, vph);
#endif
  ttfonts->setVpSize(vpw, vph);
  texfonts->setVpSize(vpw, vph);
}

void FontManager::setPixSize(const float pw, const float ph)
{
#ifdef USE_XLIB
  xfonts->setPixSize(pw, ph);
#endif
  ttfonts->setPixSize(pw, ph);
  texfonts->setPixSize(pw, ph);
}

bool FontManager::getCharSize(const int c, float& w, float& h)
{
  if (!current_engine)
    return false;
  return current_engine->getCharSize(c, w, h);
}

bool FontManager::getMaxCharSize(float& w, float& h)
{
  if (!current_engine)
    return false;
  return current_engine->getMaxCharSize(w, h);
}

bool FontManager::getStringSize(const char* s, float& w, float& h)
{
  if (!current_engine)
    return false;
  return current_engine->getStringSize(s, w, h);
}

// return info
glText::FontScaling FontManager::getFontScaletype()
{
  if (!current_engine)
    return glText::S_FIXEDSIZE;
  return current_engine->getFontScaletype();
}

int FontManager::getNumFonts()
{
  if (!current_engine)
    return 0;
  return current_engine->getNumFonts();
}

int FontManager::getNumSizes()
{
  if (!current_engine)
    return 0;
  return current_engine->getNumSizes();
}

glText::FontFace FontManager::getFontFace()
{
  if (!current_engine)
    return glText::F_NORMAL;
  return current_engine->getFontFace();
}

float FontManager::getFontSize()
{
  if (!current_engine)
    return 0;
  return current_engine->getFontSize();
}

int FontManager::getFontSizeIndex()
{
  if (!current_engine)
    return 0;
  return current_engine->getFontSizeIndex();
}

miString FontManager::getFontName(const int index)
{
  if (!current_engine)
    return "";
  return current_engine->getFontName(index);
}

float FontManager::getSizeDiv()
{
  if (!current_engine)
    return 1.0;
  return current_engine->getSizeDiv();
}

