
#include "qstring_util.h"

#include <cmath>

namespace diutil {

QString formatLongitude(float lon, int precision, int width)
{
  return (QString::number(std::abs(lon), 'f', precision) + QLatin1Char(lon >= 0 ? 'E' : 'W'))
      .rightJustified(width);
}

QString formatLatitude(float lat, int precision, int width)
{
  return (QString::number(std::abs(lat), 'f', 1) + QLatin1Char(lat >= 0 ? 'N' : 'S'))
      .rightJustified(width);
}

} // namespace diutil
