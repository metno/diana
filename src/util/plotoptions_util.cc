/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018-2022 met.no

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

#include "plotoptions_util.h"

#include "diPlotOptions.h"

#include <puTools/miStringFunctions.h>

namespace diutil {

void parseClasses(const PlotOptions& poptions, std::vector<float>& classValues, std::vector<std::string>& classNames, unsigned int& maxlen)
{
  std::map<float, std::string> classImages;
  parseClasses(poptions, classValues, classNames, classImages, maxlen);
}

void parseClasses(const PlotOptions& poptions, std::vector<float>& classValues, std::vector<std::string>& classNames, std::map<float, std::string>& classImages,
                  unsigned int& maxlen)
{
  maxlen = 0;

  if (poptions.discontinuous && !poptions.classSpecifications.empty()) {
    // discontinuous (classes)
    const std::vector<std::string> classSpec = miutil::split(poptions.classSpecifications, ",");
    const int nc = classSpec.size();
    for (int i = 0; i < nc; i++) {
      const std::vector<std::string> vstr = miutil::split(classSpec[i], ":");
      if (vstr.size() > 1) {
        const float value = miutil::to_float(vstr[0]);
        classValues.push_back(value);
        classNames.push_back(vstr[1]);
        if (maxlen < vstr[1].length())
          maxlen = vstr[1].length();
        if (vstr.size() > 2)
          classImages[value] = vstr[2];
      }
    }
  }
}

std::vector<float> parseClassValues(const PlotOptions& poptions)
{
  std::vector<float> classValues;
  std::vector<std::string> classNames;
  unsigned int maxlen;
  parseClasses(poptions, classValues, classNames, maxlen);
  return classValues;
}

void maybeSetDefaults(PlotOptions& po)
{
  if (!(po.use_lineinterval() || po.use_linevalues() || po.use_loglinevalues()))
    po.set_lineinterval(10);
}

} // namespace diutil
