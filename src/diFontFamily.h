/*
 based on libglText - OpenGL Text Rendering Library

 Copyright (C) 2006-2015 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 email: diana@met.no

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef DIFONTFAMILY_H
#define DIFONTFAMILY_H 1

#include <string>

class FTFont; // either from miFTGL or standard FTGL

class FontFamily {
public:
  enum {
    MAXFONTFACES = 4, MAXFONTSIZES = 40
  };
  enum FontFace {
    F_NORMAL = 0, F_BOLD = 1, F_ITALIC = 2, F_BOLD_ITALIC = 3
  };

public:
  FontFamily(bool bitmap);

  FontFace getFontFace() const
    { return Face; }
  float getFontSize() const
    { return numSizes ?  Sizes[SizeIndex] : 0; }

  // requested-size/actual-size
  float getSizeDiv();

  // define new font (family,name,face,size)
  bool defineFont(const std::string& fname, FontFace face, int size);

  // choose font, size and face
  bool set(FontFace face, float size);
  bool setFontFace(FontFace);
  bool setFontSize(float size);

  // drawing commands
  bool drawStr(const std::string& s,  float x, float y, float a = 0);
  bool drawStr(const std::wstring& s, float x, float y, float a = 0);

  // Metric commands
  void setGlSize(float, float);
  void setVpSize(int, int);
  bool getStringRect(const std::string& s,  float& x, float&y, float& w, float& h);
  bool getStringRect(const std::wstring& s, float& x, float&y, float& w, float& h);

private:
  struct ttfont {
    std::string fontname; //! the font filename
    bool created; //! true if there was an attempt to create this font
    FTFont *pfont; //! the FTGL font instance
    ttfont() : created(false), pfont(0) { }
    ~ttfont() { destroy(); }
    void destroy();
  };

private:
  bool _addSize(const int, int&);
  bool _findSize(const int, int&, const bool = false);
  bool _checkFont();
  bool _calcScaling();
  void prepareDraw(float x, float y, float angle);
  ttfont* getFont();

  ttfont* prepareStringRect(size_t length,  float& x, float&y, float& w, float& h);
  void scaleStringRect(float& x, float&y, float& r_w, float& u_h);

private:
  bool mUseBitmap;

  FontFace Face; // current font face

  int Sizes[MAXFONTSIZES];
  int numSizes; // number of defined fontsizes
  int SizeIndex; // current fontsize index

  float reqSize; // last requested font size
  int vpWidth, vpHeight; // viewport size in pixels
  float glWidth, glHeight; // viewport size in gl-coord.
  float pixWidth, pixHeight; // size of pixel in gl-coord.
  float scalex, scaley; // font scaling
  float xscale; // finetuning

  ttfont fonts[MAXFONTFACES][MAXFONTSIZES]; // tt fonts
};

#endif // DIFONTFAMILY_H
