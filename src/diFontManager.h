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
#ifndef _diFontManager_h
#define _diFontManager_h

#include "diFont.h"

#include <qglobal.h>

#include <map>
#include <memory>
#include <set>
#include <string>

class FontFamily;
typedef std::shared_ptr<FontFamily> FontFamily_p;

/**
 \brief Font manager for text plotting

 The font manager keeps multiple sets of font engines (X, truetype, texture).
 - text managing commands are translated to the different font engines
 - supports plotting and querying on geometry
 */
class FontManager {
private:
  typedef std::map<std::string, FontFamily_p> families_t;
  families_t families;
  FontFamily_p currentFamily;

  //! find font family and update currentFamily
  void findFamily(const std::string& family);

public:
  FontManager();
  ~FontManager();

  void defineFont(const std::string& fontfam, const std::string& fontfilename, diutil::FontFace face, bool use_bitmap);

  void clearFamilies();

  /// choose fonttype, face and size
  bool set(const std::string& family, diutil::FontFace, float);

  /// choose fonttype
  bool setFont(const std::string& family);

  /// check fonttype
  bool hasFont(const std::string& family);

  /// choose fontface from type
  bool setFontFace(diutil::FontFace);

  /// choose fontsize
  bool setFontSize(float);

  bool drawStr(const std::string& s,  const float x, const float y, const float a = 0);
  bool drawStr(const std::wstring& s, const float x, const float y, const float a = 0);

  /// get bounding rectangle of a string
  bool getStringRect(const std::string& s, float& x, float& y, float& w, float& h);
  bool getStringRect(const std::wstring& s, float& x, float& y, float& w, float& h);

  /// set viewport size in physical coordinates (pixels) and GL coordinates
  void setVpGlSize(int vpw, int vph, float glw, float glh);
};

#endif
