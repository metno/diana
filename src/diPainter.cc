
#include "diPainter.h"

#include <diField/diRectangle.h>
#include <puTools/miStringFunctions.h>

#include <QImage>
#include <QPointF>
#include <QPolygonF>

#include <cmath>

bool DiCanvas::setFont(const std::string& font, const std::string& face, float size)
{
  FontFace f = F_NORMAL;
  const std::string lface = miutil::to_lower(face);
  if (lface == "bold")
    f = F_BOLD;
  else if (lface == "bold_italic")
    f = F_BOLD_ITALIC;
  else if (lface == "italic")
    f = F_ITALIC;
  return setFont(font, size, f);
}

// ========================================================================

DiPainter::DiPainter(DiCanvas* canvas)
  : mCanvas(canvas)
{
}

void DiPainter::setVpGlSize(float vpw, float vph, float glw, float glh)
{
  if (canvas())
    canvas()->setVpGlSize(vpw, vph, glw, glh);
}

bool DiPainter::setFont(const std::string& font)
{
  if (!canvas())
    return false;
  return canvas()->setFont(font);
}

bool DiPainter::setFont(const std::string& font, float size, DiCanvas::FontFace face)
{
  if (!canvas())
    return false;
  return canvas()->setFont(font, size, face);
}

bool DiPainter::setFont(const std::string& font, const std::string& face, float size)
{
  if (!canvas())
    return false;
  return canvas()->setFont(font, face, size);
}

bool DiPainter::setFontSize(float size)
{
  if (!canvas())
    return false;
  return canvas()->setFontSize(size);
}

bool DiPainter::getCharSize(char ch, float& w, float& h)
{
  if (!canvas())
    return false;
  return canvas()->getCharSize(ch, w, h);
}

bool DiPainter::getTextSize(const std::string& text, float& w, float& h)
{
  if (!canvas())
    return false;
  return canvas()->getTextSize(text, w, h);
}

bool DiPainter::drawChar(char chr, float x, float y, float angle)
{
  const char chrs[2] = { chr, 0 };
  return drawText(chrs, x, y, angle);
}

void DiPainter::drawRect(const Rectangle& r)
{
  drawRect(r.x1, r.y1, r.x2, r.y2);
}

void DiPainter::fillRect(const Rectangle& r)
{
  fillRect(r.x1, r.y1, r.x2, r.y2);
}

void DiPainter::drawCross(float x, float y, float dxy, bool diagonal)
{
  if (diagonal) {
    drawLine(x - dxy, y - dxy, x + dxy, y + dxy);
    drawLine(x - dxy, y + dxy, x + dxy, y - dxy);
  } else {
    drawLine(x-dxy, y, x+dxy, y);
    drawLine(x, y-dxy, x, y+dxy);
  }
}

void DiPainter::drawArrow(float x1, float y1, float x2, float y2, float headsize)
{
  // direction
  drawLine(x1, y1, x2, y2);
  drawArrowHead(x1, y1, x2, y2, headsize);
}

void DiPainter::drawArrowHead(float x1, float y1, float x2, float y2, float headsize)
{
  // arrow (drawn as two lines)
  float dx = x2 - x1, dy = y2 - y1;
  if (dx == 0 && dy == 0)
    return;

  if (headsize != 0) {
    const float scale = headsize/std::sqrt(dx*dx + dy*dy);
    dx *= scale;
    dy *= scale;
  }
  const float a = -1/3., s = a / 2;
  QPolygonF points;
  points << QPointF(x2 + a*dx + s*dy, y2 + a*dy - s*dx);
  points << QPointF(x2, y2);
  points << QPointF(x2 + a*dx - s*dy, y2 + a*dy + s*dx);
  drawPolyline(points);
}

void DiPainter::drawReprojectedImage(const QImage& image, const float* mapPositionsXY, bool smooth)
{
  const diutil::Rect_v all(1, diutil::Rect(0, 0, image.width()-1, image.height()-1));
  drawReprojectedImage(image, mapPositionsXY, all, smooth);
}
