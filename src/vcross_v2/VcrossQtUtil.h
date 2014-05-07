
#ifndef VCROSSQTUTIL_HH
#define VCROSSQTUTIL_HH 1

#include "diColour.h"
#include "diLinetype.h"

#include <QtGui/QColor>
#include <string>

class QPainter;
class QPen;

namespace vcross {
namespace util {

void updateMaxStringWidth(QPainter& painter, float& w, const std::string& txt);

inline QColor QC(const Colour& c)
{ return QColor(c.R(), c.G(), c.B(), c.A()); }

void setDash(QPen& pen, bool stipple, int factor, unsigned short pattern);
void setDash(QPen& pen, const Linetype& linetype);

} // namespace util
} // namespace vcross

#endif // VCROSSQTUTIL_HH
