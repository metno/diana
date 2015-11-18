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

// glText.cc : Class definition for abstract GL-rendered fonts

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diFontFamily.h"

#if 0
#include <miFTGL/FTFont.h>
#include <miFTGL/FTGLPolygonFont.h>
#include <miFTGL/FTGLBitmapFont.h>
#else
#include <FTGL/ftgl.h>
#include <FTGL/FTFont.h>
#include <FTGL/FTGLPolygonFont.h>
#include <FTGL/FTGLBitmapFont.h>
#endif

#include <GL/gl.h>

#include <cmath>
#include <fstream>

#define MILOGGER_CATEGORY "diana.FontFamily"
#include <miLogger/miLogging.h>

void FontFamily::ttfont::destroy()
{
  delete pfont;
  pfont = 0;
}

FontFamily::FontFamily(bool bitmap)
  : mUseBitmap(bitmap)
  , Face(F_NORMAL)
  , numSizes(0)
  , SizeIndex(0)
  , reqSize(1.0)
  , vpWidth(1.0), vpHeight(1.0)
  , glWidth(1.0), glHeight(1.0)
  , pixWidth(1.0), pixHeight(1.0)
  , scalex(1.0), scaley(1.0), xscale(1.0)
{
}

bool FontFamily::_findSize(const int size, int& index, bool exact)
{
  if (!numSizes)
    return false;
  int diff = 100000, cdiff;
  for (int i = 0; i < numSizes; i++) {
    cdiff = abs(size - Sizes[i]);
    if (cdiff < diff) {
      diff = cdiff;
      index = i;
    }
  }
  return (exact ? diff == 0 : (diff < 1000));
}

bool FontFamily::_addSize(const int size, int &index)
{
  if (numSizes == MAXFONTSIZES)
    return false;
  index = numSizes;
  Sizes[index] = size;
  numSizes++;
  return true;
}

float FontFamily::getSizeDiv()
{
  if (SizeIndex >= 0 && SizeIndex < numSizes) {
    int s = Sizes[SizeIndex];
    if (s > 0) {
      return reqSize / float(s);
    }
  }
  return 1.0;
}

bool FontFamily::defineFont(const std::string& filename, FontFace face, int size)
{
  METLIBS_LOG_SCOPE(LOGVAL(filename) << LOGVAL(face) << LOGVAL(size));
  fonts[face][0].fontname = filename;
  return true;
}

// choose font, size and face
bool FontFamily::set(FontFace face, float size)
{
  Face = face;
  reqSize = size;
  if (mUseBitmap)
    reqSize *= 1.3;
  return _calcScaling();
}

bool FontFamily::setFontFace(FontFace face)
{
  Face = face;
  return _calcScaling();
}

bool FontFamily::setFontSize(float size)
{
  reqSize = size;
  if (mUseBitmap)
    reqSize *= 1.3;
  return _calcScaling();
}

bool FontFamily::_calcScaling()
{
  pixWidth = glWidth / vpWidth;
  pixHeight = glHeight / vpHeight;

  float xy;
  if (mUseBitmap) {
    xy = scalex = scaley = 1.0;
  } else {
    scalex = pixWidth;
    scaley = pixHeight;
    xy = pixHeight / pixWidth;
    scalex *= 1.30;
    scaley *= 1.30;
  }

  float truesize = reqSize * scalex;
  int size = static_cast<int> (roundf(reqSize));
  if (size < 1)
    size = 1;

  float qx = truesize / (1.0 * size);

  scalex = qx;
  scaley = qx * xy;

  // find correct sizeindex
  if (!_findSize(size, SizeIndex, true)) {
    if (!_addSize(size, SizeIndex)) {
      if (!_findSize(size, SizeIndex, false)) {
        SizeIndex = 0;
      }
    }
  }

  // create font if necessary
  return _checkFont();
}

void FontFamily::setGlSize(const float glw, const float glh)
{
  glWidth = glw;
  glHeight = glh;

  _calcScaling();
}

void FontFamily::setVpSize(const float vpw, const float vph)
{
  vpWidth = vpw;
  vpHeight = vph;

  _calcScaling();
}

FontFamily::ttfont* FontFamily::getFont()
{
  if (SizeIndex >= 0 && SizeIndex < numSizes)
    return &(fonts[Face][SizeIndex]);
  else
    return 0;
}

bool FontFamily::_checkFont()
{
  const char* facenames[MAXFONTFACES] = { "Normal", "Bold", "Italic", "Bold_Italic" };

  const std::string& fontname = fonts[Face][0].fontname;
  if (!fontname.size()) {
    METLIBS_LOG_ERROR("empty fontname");
    return false;
  }
  ttfont *tf = getFont();
  if (!tf) {
    METLIBS_LOG_ERROR("font not defined Face:" << facenames[Face]
        << " SizeIndex=" << SizeIndex << "/" << numSizes);
    return false;
  }
  if (!(tf->created)) {
    METLIBS_LOG_INFO("creating font from file '" << fontname << "'"
        << " Face:" << facenames[Face] << " Size:" << Sizes[SizeIndex]);
    tf->created = true;
    if (mUseBitmap)
      tf->pfont = new FTGLBitmapFont(fontname.c_str());
    else
      tf->pfont = new FTGLPolygonFont(fontname.c_str());
    FT_Error error = tf->pfont->Error();
    if (error != 0) {
      METLIBS_LOG_ERROR("could not create font, error=" << error);
      tf->destroy();
      return false;
    }

    // create the actual glyph
    tf->pfont->FaceSize(Sizes[SizeIndex], 85);
    error = tf->pfont->Error();
    if (error != 0) {
      METLIBS_LOG_ERROR("could not set size " << Sizes[SizeIndex] << ", error=" << error);
      tf->destroy();
      return false;
    }
  }

  return (tf->pfont != 0);
}

void FontFamily::prepareDraw(float x, float y, float angle)
{
  if (mUseBitmap) {
    glRasterPos2f(x, y);
  } else {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glTranslatef(x, y, 0);
    glRotatef(angle, 0, 0, 1);
    glScalef(scalex * xscale, scaley * xscale, 1.0);
  }
}

bool FontFamily::drawStr(const std::string& s, float x, float y, float angle)
{
  if (s.length() == 0)
    return false;

  glPushMatrix();
  prepareDraw(x, y, angle);

  ttfont *tf = getFont();
  if (tf && tf->pfont)
    tf->pfont->Render(s.c_str());

  glPopMatrix();

  return true;
}

bool FontFamily::drawStr(const std::wstring& s, float x, float y, float angle)
{
  if (s.length() == 0)
    return false;

  glPushMatrix();
  prepareDraw(x, y, angle);

  ttfont *tf = getFont();
  if (tf && tf->pfont)
    tf->pfont->Render(s.c_str());

  glPopMatrix();

  return true;
}

bool FontFamily::getStringSize(const std::string& s, float& w, float& h)
{
  w = h = 0;
  if (s.length() == 0)
    return false;

  ttfont *tf = getFont();
  if (!tf || !tf->pfont)
    return false;

  float llx, lly, llz, urx, ury, urz;
  tf->pfont->BBox(s.c_str(), llx, lly, llz, urx, ury, urz);
  w = (urx - llx) * scalex * xscale;
  h = (ury - lly) * scaley * xscale;
  if (mUseBitmap) {
    w *= pixWidth;
    h *= pixHeight;
  }
  return true;
}

bool FontFamily::getStringSize(const std::wstring& s, float& w, float& h)
{
  w = h = 0;
  if (s.length() == 0)
    return false;

  ttfont *tf = getFont();
  if (!(tf && tf->pfont))
    return false;

  float llx, lly, llz, urx, ury, urz;
  tf->pfont->BBox(s.c_str(), llx, lly, llz, urx, ury, urz);
  w = (urx - llx) * scalex * xscale;
  h = (ury - lly) * scaley * xscale;
  if (mUseBitmap) {
    w *= pixWidth;
    h *= pixHeight;
  }
  return true;
}

bool FontFamily::getStringRect(const std::string& s, float& x, float& y, float& w, float& h)
{
  x = y = w = h = 0;
  if (s.length() == 0)
    return false;

  ttfont *tf = getFont();
  if (!tf || !tf->pfont)
    return false;

  float llx, lly, llz, urx, ury, urz;
  tf->pfont->BBox(s.c_str(), llx, lly, llz, urx, ury, urz);
  x = llx * scalex * xscale;
  y = lly * scaley * xscale;
  w = (urx - llx) * scalex * xscale;
  h = (ury - lly) * scaley * xscale;
  if (mUseBitmap) {
    w *= pixWidth;
    h *= pixHeight;
  }
  return true;
}

bool FontFamily::getStringRect(const std::wstring& s, float& x, float& y, float& w, float& h)
{
  x = y = w = h = 0;
  if (s.length() == 0)
    return false;

  ttfont *tf = getFont();
  if (!(tf && tf->pfont))
    return false;

  float llx, lly, llz, urx, ury, urz;
  tf->pfont->BBox(s.c_str(), llx, lly, llz, urx, ury, urz);
  x = llx * scalex * xscale;
  y = lly * scaley * xscale;
  w = (urx - llx) * scalex * xscale;
  h = (ury - lly) * scaley * xscale;
  if (mUseBitmap) {
    w *= pixWidth;
    h *= pixHeight;
  }
  return true;
}
