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

#include <qglobal.h>

#include <glText/glText.h>

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
  glText * current_engine;

  std::map<std::string, glText*> fontengines;
  std::map<std::string, std::set<std::string> > enginefamilies;

  static std::string fontpath;
  static std::string display_name;
  static std::map<std::string, std::string> defaults;

  glText::FontFace fontFace(const std::string&);
  bool check_family(const std::string& fam, std::string& family);

public:
  FontManager();
  ~FontManager();

  /// for use in batch - force different display
  static void set_display_name(const std::string name)
  {
    display_name = name;
  }

  /// for test purposes, sets up a standard set of fonts
  bool testDefineFonts(std::string path = "fonts");
  /// parse fontsection in setup file
  bool parseSetup();

  /// choose fonttype, face and size
  bool set(const std::string, const glText::FontFace, const float);
  /// choose fonttype, face and size
  bool set(const std::string, const std::string, const float);
  /// choose fonttype
  bool setFont(const std::string);
  /// choose fontface from type
  bool setFontFace(const glText::FontFace);
  /// choose fontface from name
  bool setFontFace(const std::string);
  /// choose fontsize
  bool setFontSize(const float);

  // printing commands
  /// draw one character
  bool drawChar(const int c, const float x, const float y, const float a = 0);
  /// draw a string
  bool drawStr(const char* s, const float x, const float y, const float a = 0);
  // Metric commands
  /// always add this to fontsize
  void adjustSize(const int sa);
  /// choose S_FIXEDSIZE or S_VIEWPORTSCALED
  void setScalingType(const glText::FontScaling fs);
  /// set viewport size in GL coordinates
  void setGlSize(const float glw, const float glh);
  void setGlSize(const float glx1, const float glx2, const float gly1,
      const float gly2);
  /// set viewport size in physical coordinates (pixels)
  void setVpSize(const float vpw, const float vph);
  /// set size of one pixel in GL coordinates
  void setPixSize(const float pw, const float ph);
  /// get plotting size of one character
  bool getCharSize(const int c, float& w, float& h);
  /// get maximum plotting size of a character with current settings
  bool getMaxCharSize(float& w, float& h);
  /// get plotting size of a string
  bool getStringSize(const char* s, float& w, float& h);

  // return info
  /// return selected font scaling
  glText::FontScaling getFontScaletype();
  /// return number of defined fonts
  int getNumFonts();
  /// return number of defined sizes
  int getNumSizes();
  /// return current font face
  glText::FontFace getFontFace();
  /// return current font size
  float getFontSize();
  /// return index of current font size
  int getFontSizeIndex();
  /// return name of font from index
  std::string getFontName(const int index);
};

#endif
