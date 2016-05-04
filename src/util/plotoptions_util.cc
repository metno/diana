
#include "plotoptions_util.h"

#include "diPlotOptions.h"

#include <puTools/miStringFunctions.h>

namespace diutil {

void parseClasses(const PlotOptions& poptions, std::vector<int>& classValues,
    std::vector<std::string>& classNames, unsigned int& maxlen)
{
  maxlen = 0;

  if (poptions.discontinuous == 1 && !poptions.classSpecifications.empty()) {
    // discontinuous (classes)
    const std::vector<std::string> classSpec = miutil::split(poptions.classSpecifications, ",");
    const int nc = classSpec.size();
    for (int i = 0; i < nc; i++) {
      const std::vector<std::string> vstr = miutil::split(classSpec[i], ":");
      if (vstr.size() > 1) {
        classValues.push_back(miutil::to_int(vstr[0]));
        classNames.push_back(vstr[1]);
        if (maxlen < vstr[1].length())
          maxlen = vstr[1].length();
      }
    }
  }
}

std::vector<int> parseClassValues(const PlotOptions& poptions)
{
  std::vector<int> classValues;
  std::vector<std::string> classNames;
  unsigned int maxlen;
  parseClasses(poptions, classValues, classNames, maxlen);
  return classValues;
}

} // namespace diutil
