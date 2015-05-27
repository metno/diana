/*
 Diana - A Free Meteorological Visualisation Tool

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

#include "diFontManager.h"

#ifdef USE_XLIB
#include <glText/glTextX.h>
#endif
#include <glText/glTextTT.h>
#include <glText/glTextTTBitmap.h>
#include <glText/glTextTTPixmap.h>
#include <glText/glTextTTTexture.h>
#include <glText/glTextQtTexture.h>

#include "diLocalSetupParser.h"
#include <puTools/miSetupParser.h>

#define MILOGGER_CATEGORY "diana.FontManager"
#include <miLogger/miLogging.h>

using namespace miutil;
using namespace std;

std::string FontManager::fontpath;
std::string FontManager::display_name;
map<std::string, std::string> FontManager::defaults;

static const std::string key_bitmap = "bitmap";
static const std::string key_scaleable = "scaleable";
static const std::string key_ttbitmap = "tt_bitmap";
static const std::string key_ttpixmap = "tt_pixmap";
static const std::string key_tttexture = "tt_texture";
static const std::string key_texture = "texture";

static const std::string key_bitmapfont = "bitmapfont";
static const std::string key_scalefont = "scalefont";
static const std::string key_metsymbolfont = "metsymbolfont";

FontManager::FontManager() :
  current_engine(0)
{
#ifdef USE_XLIB
  glTextX * xfonts;
  if (!display_name.empty()) // do not use environment-var DISPLAY
  xfonts = new glTextX(display_name);
  else
  xfonts = new glTextX();
  fontengines[key_bitmap] = xfonts;
#endif

  glTextTT * ttfonts = new glTextTT();
  fontengines[key_scaleable] = ttfonts;

  glTextTTBitmap * ttbfonts = new glTextTTBitmap();
  fontengines[key_ttbitmap] = ttbfonts;

  glTextTTPixmap * ttpixfonts = new glTextTTPixmap();
  fontengines[key_ttpixmap] = ttpixfonts;

  glTextTTTexture * tttexfonts = new glTextTTTexture();
  fontengines[key_tttexture] = tttexfonts;

  glTextQtTexture * texfonts = new glTextQtTexture();
  fontengines[key_texture] = texfonts;
}

FontManager::~FontManager()
{
  std::map<std::string, glText*>::iterator itr = fontengines.begin();
  for (; itr != fontengines.end(); ++itr) {
    if (itr->second) {
      delete itr->second;
    }
  }
}

// fill fontpack for testing
bool FontManager::testDefineFonts(std::string path)
{
  bool res = true;

  std::map<std::string, glText*>::iterator itr = fontengines.begin();
  for (; itr != fontengines.end(); ++itr) {
    if (itr->second) {
      itr->second->testDefineFonts(path);
    }
  }

  return res;
}

glText::FontFace FontManager::fontFace(const std::string& s)
{
  std::string suface = miutil::to_upper(s);
  miutil::trim(suface);
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

bool FontManager::parseSetup()
{
  const std::string sf_name = "FONTS";
  vector<std::string> sect_fonts;

  const std::string key_font = "font";
  const std::string key_fonttype = "type";
  const std::string key_fontface = "face";
  const std::string key_fontname = "name";
  const std::string key_postscript = "postscript";
  const std::string key_psxscale = "ps-scale-x";
  const std::string key_psyscale = "ps-scale-y";
  const std::string key_fontpath = "fontpath";

  defaults[key_bitmapfont] = "Helvetica";
  defaults[key_scalefont] = "Arial";
  defaults[key_metsymbolfont] = "Symbol";

  enginefamilies.clear();

  if (fontpath.empty()) {
    fontpath = LocalSetupParser::basicValue("fontpath");
    if (fontpath.empty())
      fontpath = "fonts/";
  }

  if (!SetupParser::getSection(sf_name, sect_fonts)) {
    //METLIBS_LOG_WARN("Missing section " << sf_name << " in setupfile.");
    testDefineFonts(fontpath);
    return false;
  }

  int n = sect_fonts.size();
  for (int i = 0; i < n; i++) {
    std::string fontfam = "";
    std::string fontname = "";
    std::string fonttype = "";
    std::string fontface = "";
    std::string postscript = "";
    float psxscale = 1.0;
    float psyscale = 1.0;

    vector<std::string> stokens = miutil::split(sect_fonts[i], " ");
    for (unsigned int j = 0; j < stokens.size(); j++) {
      std::string key;
      std::string val;
      SetupParser::splitKeyValue(stokens[j], key, val);

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
      else if (key == key_psxscale)
        psxscale = atof(val.c_str());
      else if (key == key_psyscale)
        psyscale = atof(val.c_str());
      else if (key == key_fontpath)
        fontpath = val;
      else
        defaults[key] = val;
    }

    if (fonttype.empty() || fontfam.empty() || fontname.empty())
      continue;

    const std::string fonttype_lc = miutil::to_lower(fonttype);
    std::string fontfilename = fontpath + "/" + fontname;

    if (fonttype_lc == key_bitmap) {
#if defined(USE_XLIB)
      enginefamilies[key_bitmap].insert(fontfam);
      if (fontengines[key_bitmap])
        fontengines[key_bitmap]->defineFonts(fontname, fontfam, postscript);
#else
      METLIBS_LOG_WARN("X-FONTS not supported!");
#endif
    } else if (fonttype_lc == key_scaleable) {
      enginefamilies[key_scaleable].insert(fontfam);
      if (fontengines[key_scaleable]) {
        fontengines[key_scaleable]
            ->defineFont(fontfam, fontfilename, fontFace(fontface), 20);
      }

    } else if (fonttype_lc == key_ttbitmap) {
      enginefamilies[key_ttbitmap].insert(fontfam);
      if (fontengines[key_ttbitmap]) {
        fontengines[key_ttbitmap]
            ->defineFont(fontfam, fontfilename, fontFace(fontface), 20,
                postscript, psxscale, psyscale);
      }

    } else if (fonttype_lc == key_ttpixmap) {
      enginefamilies[key_ttpixmap].insert(fontfam);
      if (fontengines[key_ttpixmap]) {
        fontengines[key_ttpixmap]->defineFont(
          fontfam, fontfilename, fontFace(fontface), 20, postscript,
          psxscale, psyscale);
      }

    } else if (fonttype_lc == key_tttexture) {
      enginefamilies[key_tttexture].insert(fontfam);
      if (fontengines[key_tttexture]) {
        fontengines[key_tttexture]->defineFont(
          fontfam, fontfilename, fontFace(fontface), 20, postscript,
          psxscale, psyscale);
      }

    } else if (fonttype_lc == key_texture) {
      enginefamilies[key_texture].insert(fontfam);
      if (fontengines[key_texture]) {
        fontengines[key_texture]->defineFont(
          fontfam, fontname, fontFace(fontface), 20, postscript, psxscale,
          psyscale);
      }
    }
  }

  return true;
}

bool FontManager::check_family(const std::string& fam, std::string& family)
{
  const std::string fam_lower = miutil::to_lower(fam);
  if (defaults.count(fam_lower) > 0)
    family = defaults[fam_lower];
  else
    family = fam;

  std::map<std::string, std::set<std::string> >::iterator itr =
      enginefamilies.begin();
  for (; itr != enginefamilies.end(); ++itr) {
    if (itr->second.find(family) != itr->second.end()) {
      if (fontengines[itr->first]) {
        current_engine = fontengines[itr->first];
        break;
      }
    }
  }

  if (itr == enginefamilies.end()) {
    METLIBS_LOG_ERROR("FontManager::check_family ERROR, unknown font family:" << family);
    return false;
  }

  return true;
}

// choose font, size and face
bool FontManager::set(const std::string fam, const glText::FontFace face,
    const float size)
{
  std::string family;
  if (check_family(fam, family)) {
    return current_engine->set(family, face, size);
  }
  return false;
}

// choose font, size and face
bool FontManager::set(const std::string fam, const std::string sface,
    const float size)
{
  glText::FontFace face = fontFace(sface);
  return set(fam, face, size);
}

bool FontManager::setFont(const std::string fam)
{
  std::string family;
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

bool FontManager::setFontFace(const std::string sface)
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
  std::map<std::string, glText*>::iterator itr = fontengines.begin();
  for (; itr != fontengines.end(); ++itr) {
    if (itr->second) {
      itr->second->adjustSize(sa);
    }
  }
}

void FontManager::setScalingType(const glText::FontScaling fs)
{
  std::map<std::string, glText*>::iterator itr = fontengines.begin();
  for (; itr != fontengines.end(); ++itr) {
    if (itr->second) {
      itr->second->setScalingType(fs);
    }
  }
}

// set viewport size in GL coordinates
void FontManager::setGlSize(const float glx1, const float glx2,
    const float gly1, const float gly2)
{
  setGlSize(glx2 - glx1, gly2 - gly1);
}

// set viewport size in GL coordinates
void FontManager::setGlSize(const float glw, const float glh)
{
  std::map<std::string, glText*>::iterator itr = fontengines.begin();
  for (; itr != fontengines.end(); ++itr) {
    if (itr->second) {
      itr->second->setGlSize(glw, glh);
    }
  }
}

// set viewport size in physical coordinates (pixels)
void FontManager::setVpSize(const float vpw, const float vph)
{
  std::map<std::string, glText*>::iterator itr = fontengines.begin();
  for (; itr != fontengines.end(); ++itr) {
    if (itr->second) {
      itr->second->setVpSize(vpw, vph);
    }
  }
}

void FontManager::setPixSize(const float pw, const float ph)
{
  std::map<std::string, glText*>::iterator itr = fontengines.begin();
  for (; itr != fontengines.end(); ++itr) {
    if (itr->second) {
      itr->second->setPixSize(pw, ph);
    }
  }
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

std::string FontManager::getFontName(const int index)
{
  if (!current_engine)
    return "";
  return current_engine->getFontName(index);
}
