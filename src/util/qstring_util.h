
#ifndef DIANA_UTIL_QSTRING_UTIL_H
#define DIANA_UTIL_QSTRING_UTIL_H 1

#include <QString>
#include <QStringList>

#include <vector>

namespace diutil {

QString formatLongitude(float lon, int precision, int width=0);
QString formatLatitude(float lat, int precision, int width=0);

std::vector<std::string> toVector(const QStringList& sl);

} // namespace diutil

#endif // DIANA_UTIL_QSTRING_UTIL_H
