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

#include "diFont.h"

#include <boost/algorithm/string/predicate.hpp>

namespace diutil {

const std::string BITMAPFONT = "BITMAPFONT";
const std::string SCALEFONT = "SCALEFONT";
const std::string METSYMBOLFONT = "METSYMBOLFONT";

static const std::string faces[4] = {"NORMAL", "BOLD", "ITALIC", "BOLD_ITALIC"};

const std::string& fontFaceToString(FontFace face)
{
  return faces[face];
}

FontFace fontFaceFromString(const std::string& face)
{
  if (boost::iequals(face, faces[F_BOLD]))
    return F_BOLD;
  else if (boost::iequals(face, faces[F_BOLD_ITALIC]))
    return F_BOLD_ITALIC;
  else if (boost::iequals(face, faces[F_ITALIC]))
    return F_ITALIC;
  else
    return F_NORMAL;
}

bool isBoldFont(FontFace face)
{
  return (face & 1) != 0;
}

bool isItalicFont(FontFace face)
{
  return (face & 2) != 0;
}

} // namespace diutil
