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
#ifndef _diFontManager_h
#define _diFontManager_h

#include "diFontFamily.h"

#include <qglobal.h>

#include <set>
#include <string>
#include <map>

/**
 \brief Font manager for text plotting

 The font manager keeps multiple sets of font engines (X, truetype, texture).
 - text managing commands are translated to the different font engines
 - supports plotting and querying on geometry
 */
class FontManager {
private:
  static std::string fontpath;

  typedef std::map<std::string, FontFamily*> families_t;
  families_t families;
  FontFamily *currentFamily;

  //! convert font face text to constant
  static FontFamily::FontFace fontFace(const std::string&);

  //! find font family, return 0 on error
  FontFamily* findFamily(const std::string& family);

public:
  FontManager();
  ~FontManager();

  void defineFont(const std::string& fontfam, const std::string& fontfilename,
      const std::string& face, bool use_bitmap);

  void clearFamilies();

  /// choose fonttype, face and size
  bool set(const std::string& family, FontFamily::FontFace, float);

  /// choose fonttype, face and size
  bool set(const std::string& family, const std::string&, float);

  /// choose fonttype
  bool setFont(const std::string& family);

  /// choose fontface from type
  bool setFontFace(FontFamily::FontFace);

  /// choose fontface from name
  bool setFontFace(const std::string&);

  /// choose fontsize
  bool setFontSize(float);

  bool drawStr(const std::string& s,  const float x, const float y, const float a = 0);
  bool drawStr(const std::wstring& s, const float x, const float y, const float a = 0);

  /// get bounding rectangle of a string
  bool getStringRect(const std::string& s, float& x, float& y, float& w, float& h);
  bool getStringRect(const std::wstring& s, float& x, float& y, float& w, float& h);

  /// set viewport size in GL coordinates
  void setGlSize(float glw, float glh);
  void setGlSize(float glx1, float glx2, float gly1, float gly2);

  /// set viewport size in physical coordinates (pixels)
  void setVpSize(int vpw, int vph);

  /// return current font face
  FontFamily::FontFace getFontFace();

  /// return current font size
  float getFontSize();
};

#endif
