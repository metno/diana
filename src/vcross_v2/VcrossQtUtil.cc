
#include "VcrossQtUtil.h"

#include <diField/VcrossUtil.h>
#include <QtGui/QPainter>

namespace vcross {
namespace util {

void updateMaxStringWidth(QPainter& painter, float& w, const std::string& txt)
{
  const float txt_w = painter.fontMetrics().width(QString::fromStdString(txt));
  maximize(w, txt_w);
}

void setDash(QPen& pen, bool stipple, int factor, unsigned short pattern)
{
  if (factor <= 0 or pattern == 0xFFFF or pattern == 0)
    stipple = false;

  if (not stipple) {
    pen.setStyle(Qt::SolidLine);
    return;
  }

  // adapted from glLineStipple in PaintGL/paintgl.cc 

  QVector<qreal> dashes;
  unsigned short state = pattern & 1;
  bool gapStart = (pattern & 1) == 0;
  int number = 0;
  int total = 0;
  
  for (int i = 0; i < 16; ++i) {
    unsigned short dash = pattern & 1;
    if (dash == state)
      number++;
    else {
      dashes << number * factor;
      total += number * factor;
      state = dash;
      number = 1;
    }
    pattern = pattern >> 1;
  }
  if (number > 0)
    dashes << number * factor;
  
  // Ensure that the pattern has an even number of elements by inserting
  // a zero-size element if necessary.
  if (dashes.size() % 2 == 1)
    dashes << 0;

  /* If the pattern starts with a gap then move it to the end of the
     vector and adjust the starting offset to its location to ensure
     that it appears at the start of the pattern. (This is because
     QPainter's dash pattern rendering assumes that the first element
     is a line. */
  int dashOffset = 0;
  if (gapStart) {
    dashes << dashes.first();
    dashes.pop_front();
    dashOffset = total - dashes.last();
  }
  
  pen.setDashPattern(dashes);
  pen.setDashOffset(dashOffset);
}

void setDash(QPen& pen, const Linetype& linetype)
{
  vcross::util::setDash(pen, linetype.stipple, linetype.factor, linetype.bmap);
}

} // namespace util
} // namespace vcross
