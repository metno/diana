
#ifndef DIANA_UTIL_QSTRING_UTIL_H
#define DIANA_UTIL_QSTRING_UTIL_H 1

#include <QString>
#include <QStringList>

#include <vector>

namespace diutil {

QString longitudeWest();
QString latitudeNorth();
QString longitudeEastWest(float lon);
QString latitudeNorthSouth(float lat);

QString formatLongitude(float lon, int precision, int width=0);
QString formatLatitude(float lat, int precision, int width=0);

QString formatDegMin(float value);

inline QString formatLongitudeDegMin(float lon)
  { return formatDegMin(lon) + longitudeEastWest(lon); }

inline QString formatLatitudeDegMin(float lat)
  { return formatDegMin(lat) + latitudeNorthSouth(lat); }

std::vector<std::string> toVector(const QStringList& sl);

} // namespace diutil

#endif // DIANA_UTIL_QSTRING_UTIL_H
