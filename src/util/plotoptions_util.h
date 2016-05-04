
#ifndef DIANA_UTIL_PLOTOPTION_UTIL_H
#define DIANA_UTIL_PLOTOPTION_UTIL_H 1

#include <string>
#include <vector>

class PlotOptions;

namespace diutil {

void parseClasses(const PlotOptions& poptions, std::vector<int>& classValues,
    std::vector<std::string>& classNames, unsigned int& maxlen);

std::vector<int> parseClassValues(const PlotOptions& poptions);

} // namespace diutil

#endif // DIANA_UTIL_PLOTOPTION_UTIL_H
