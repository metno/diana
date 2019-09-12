/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2019 met.no

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

#include "diFontManager.h"

#include "diFontFamily.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.FontManager"
#include <miLogger/miLogging.h>

FontManager::FontManager()
{
}

FontManager::~FontManager()
{
  clearFamilies();
}

void FontManager::clearFamilies()
{
  families.clear();
  currentFamily = 0;
}

void FontManager::defineFont(const std::string& fontfam, const std::string& fontfilename, diutil::FontFace fontface, bool use_bitmap)
{
  families_t::iterator it = families.find(fontfam);
  FontFamily_p f;
  if (it == families.end()) {
    f = std::make_shared<FontFamily>(use_bitmap);

    if (families.empty())
      currentFamily = f;
    families.insert(std::make_pair(fontfam, f));
  } else {
    f = it->second;
  }
  f->defineFont(fontfilename, fontface, 20);
}

void FontManager::findFamily(const std::string& family)
{
  const families_t::iterator itF = families.find(family);
  if (itF != families.end()) {
    currentFamily = itF->second;
  } else {
    METLIBS_LOG_ERROR("unknown font family: '" << family << "'");
    currentFamily = nullptr;
  }
}

bool FontManager::set(const std::string& fam, diutil::FontFace face, float size)
{
  findFamily(fam);
  if (!currentFamily)
    return false;
  return currentFamily->set(face, size);
}

bool FontManager::setFont(const std::string& fam)
{
  findFamily(fam);
  return currentFamily != 0;
}

bool FontManager::hasFont(const std::string& family)
{
  return (families.find(family) != families.end());
}

bool FontManager::setFontFace(diutil::FontFace face)
{
  if (!currentFamily)
    return false;
  return currentFamily->setFontFace(face);
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

// set viewport size in physical coordinates (pixels)
void FontManager::setVpGlSize(int vpw, int vph, float glw, float glh)
{
  for (const auto& it : families)
    it.second->setVpGlSize(vpw, vph, glw, glh);
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
