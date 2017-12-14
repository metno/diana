/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017 met.no

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

#include "diLineMerger.h"

#include "string_util.h"

namespace {
const std::string CONTINUATION = "\\";
} // namespace

namespace diutil {

LineMerger::LineMerger()
    : complete_(true)
    , lineno_(0)
    , linecount_(0)
{
}

bool LineMerger::push(const std::string& line)
{
  linecount_ += 1;
  if (complete_) {
    mergedline_.clear();
    lineno_ = linecount_;
  }

  if (line.empty() || !diutil::endswith(line, CONTINUATION)) {
    mergedline_ += line;
    complete_ = true;
  } else {
    mergedline_ += line.substr(0, line.size() - CONTINUATION.size());
    complete_ = false;
  }
  return complete_;
}

} // namespace diutil
