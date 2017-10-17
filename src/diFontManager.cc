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
#include "diFontFamily.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.FontManager"
#include <miLogger/miLogging.h>

FontManager::FontManager()
  : currentFamily(0)
{
}

FontManager::~FontManager()
{
  clearFamilies();
}

void FontManager::clearFamilies()
{
  for (families_t::iterator it = families.begin(); it != families.end(); ++it)
    delete it->second;
  families.clear();
  currentFamily = 0;
}

// static
FontFamily::FontFace FontManager::fontFace(const std::string& s)
{
  std::string suface = miutil::to_upper(s);
  miutil::trim(suface);
  FontFamily::FontFace face = FontFamily::F_NORMAL;
  if (suface != "NORMAL") {
    if (suface == "ITALIC")
      face = FontFamily::F_ITALIC;
    else if (suface == "BOLD")
      face = FontFamily::F_BOLD;
    else if (suface == "BOLD_ITALIC")
      face = FontFamily::F_BOLD_ITALIC;
  }

  return face;
}

void FontManager::defineFont(const std::string& fontfam, const std::string& fontfilename,
    const std::string& fontface, bool use_bitmap)
{
  families_t::iterator it = families.find(fontfam);
  FontFamily* f;
  if (it == families.end()) {
    f = new FontFamily(use_bitmap);

    if (families.empty())
      currentFamily = f;
    families.insert(std::make_pair(fontfam, f));
  } else {
    f = it->second;
  }
  f->defineFont(fontfilename, fontFace(fontface), 20);
}

FontFamily* FontManager::findFamily(const std::string& family)
{
  const families_t::iterator itF = families.find(family);
  if (itF != families.end()) {
    currentFamily = itF->second;
    return currentFamily;
  }

  METLIBS_LOG_ERROR("unknown font family: '" << family << "'");
  return 0;
}

// choose font, size and face
bool FontManager::set(const std::string& fam, FontFamily::FontFace face, float size)
{
  FontFamily* family = findFamily(fam);
  if (!family)
    return false;
  return family->set(face, size);
}

// choose font, size and face
bool FontManager::set(const std::string& fam, const std::string& face, float size)
{
  return set(fam, fontFace(face), size);
}

bool FontManager::setFont(const std::string& fam)
{
  FontFamily* family = findFamily(fam);
  return family != 0;
}

bool FontManager::setFontFace(FontFamily::FontFace face)
{
  if (!currentFamily)
    return false;
  return currentFamily->setFontFace(face);
}

bool FontManager::setFontFace(const std::string& face)
{
  return setFontFace(fontFace(face));
}

bool FontManager::setFontSize(const float size)
{
  if (!currentFamily)
    return false;
  return currentFamily->setFontSize(size);
}

bool FontManager::drawStr(const std::string& s, float x, float y, float a)
{
  if (!currentFamily)
    return false;
  return currentFamily->drawStr(s, x, y, a);
}

bool FontManager::drawStr(const std::wstring& s, float x, float y, float a)
{
  if (!currentFamily)
    return false;
  return currentFamily->drawStr(s, x, y, a);
}

// set viewport size in GL coordinates
void FontManager::setGlSize(float glx1, float glx2, float gly1, float gly2)
{
  setGlSize(glx2 - glx1, gly2 - gly1);
}

// set viewport size in GL coordinates
void FontManager::setGlSize(float glw, float glh)
{
  for (families_t::iterator it = families.begin(); it != families.end(); ++it)
    it->second->setGlSize(glw, glh);
}

// set viewport size in physical coordinates (pixels)
void FontManager::setVpSize(int vpw, int vph)
{
  for (families_t::iterator it = families.begin(); it != families.end(); ++it)
    it->second->setVpSize(vpw, vph);
}

bool FontManager::getStringRect(const std::string& s, float& x, float& y, float& w, float& h)
{
  if (!currentFamily)
    return false;
  return currentFamily->getStringRect(s, x, y, w, h);
}

bool FontManager::getStringRect(const std::wstring& s, float& x, float& y, float& w, float& h)
{
  if (!currentFamily)
    return false;
  return currentFamily->getStringRect(s, x, y, w, h);
}

FontFamily::FontFace FontManager::getFontFace()
{
  if (!currentFamily)
    return FontFamily::F_NORMAL;
  return currentFamily->getFontFace();
}

float FontManager::getFontSize()
{
  if (!currentFamily)
    return 0;
  return currentFamily->getFontSize();
}
