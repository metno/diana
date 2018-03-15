/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018 met.no

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

#ifndef DIANA_DIFONT_H
#define DIANA_DIFONT_H

#include <string>

namespace diutil {

enum FontFace { F_NORMAL = 0, F_BOLD = 1, F_ITALIC = 2, F_BOLD_ITALIC = 3 };

const std::string& fontFaceToString(FontFace face);

FontFace fontFaceFromString(const std::string& face);

bool isBoldFont(FontFace face);
bool isItalicFont(FontFace face);

extern const std::string BITMAPFONT;
extern const std::string SCALEFONT;
extern const std::string METSYMBOLFONT;

} // namespace diutil

#endif // DIANA_DIFONT_H
