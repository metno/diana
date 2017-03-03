
#include "qstring_util.h"

#include <QCoreApplication>

#include <cmath>

namespace diutil {

QString longitudeWest()
{
  return QCoreApplication::translate("QStringUtil", "W");
}

QString latitudeNorth()
{
  return QCoreApplication::translate("QStringUtil", "N");
}

QString longitudeEastWest(float lon)
{
  if (lon >= 0)
    return QCoreApplication::translate("QStringUtil", "E");
  else
    return longitudeWest();
}

QString latitudeNorthSouth(float lat)
{
  if (lat >= 0)
    return latitudeNorth();
  else
    return QCoreApplication::translate("QStringUtil", "S");
}

QString formatLongitude(float lon, int precision, int width)
{
  return (QString::number(std::abs(lon), 'f', precision) + longitudeEastWest(lon))
      .rightJustified(width);
}

QString formatLatitude(float lat, int precision, int width)
{
  return (QString::number(std::abs(lat), 'f', precision) + latitudeNorthSouth(lat))
      .rightJustified(width);
}

QString formatDegMin(float value)
{
  int vmin = int(std::abs(value)*60.+0.5);
  int vdeg = vmin/60;
  vmin %= 60;
  return QString::number(vdeg) + "°" + QString::number(vmin).rightJustified(2, QLatin1Char('0')) + "'";
}

std::vector<std::string> toVector(const QStringList& sl)
{
  std::vector<std::string> v;
  v.reserve(sl.size());
  for (QStringList::const_iterator it = sl.begin(); it != sl.end(); ++it)
    v.push_back(it->toStdString());
  return v;
}

} // namespace diutil
