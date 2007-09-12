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
#ifndef _diFontManager_h
#define _diFontManager_h

#include <miString.h>
#include <glText.h>
#include <diSetupParser.h>
#include <set>

using namespace std; 

class glTextX;
class glTextTT;
class GLPcontext;

/**
   \brief Font manager for text plotting

   The font manager keeps multiple sets of font engines (X, truetype).
   - text managing commands are translated to the different font engines
   - supports plotting and querying on geometry

*/

class FontManager {
private:
  glTextX*  xfonts;
  glTextTT* ttfonts;
  bool usexf;
  std::set<miString> xfam;
  std::set<miString> ttfam;

  static miString fontpath;
  static miString display_name;
  static map<miString,miString> defaults;

  glText::FontFace fontFace( const miString& );
  bool check_family( const miString& fam, miString& family );

public:
  FontManager();
  ~FontManager();

  /// for use in batch - force different display
  static void set_display_name(const miString name)
  {display_name= name;}

  /// start PS output
  void startHardcopy(GLPcontext*);
  /// end PS output
  void endHardcopy();
  
  /// for test purposes, sets up a standard set of fonts
  bool testDefineFonts(miString path = "fonts");
  /// parse fontsection in setup file
  bool parseSetup(SetupParser& sp);

  /// choose fonttype, face and size
  bool set(const miString, const glText::FontFace, const float);
  /// choose fonttype, face and size
  bool set(const miString, const miString, const float);
  /// choose fonttype
  bool setFont(const miString);
  /// choose fontface from type
  bool setFontFace(const glText::FontFace);
  /// choose fontface from name
  bool setFontFace(const miString);
  /// choose fontsize
  bool setFontSize(const float);

  // printing commands
  /// draw one character
  bool drawChar(const int c, const float x,
		const float y, const float a= 0);
  /// draw a string
  bool drawStr(const char* s, const float x,
	       const float y, const float a= 0);
  // Metric commands
  /// always add this to fontsize
  void adjustSize(const int sa);
  /// choose S_FIXEDSIZE or S_VIEWPORTSCALED
  void setScalingType(const glText::FontScaling fs);
  /// set viewport size in GL coordinates
  void setGlSize(const float glw, const float glh);
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
  miString getFontName(const int index);
  /// requested-size divided by actual-size
  float getSizeDiv();
};

#endif
