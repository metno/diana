
#include "diGLPainter.h"

#include "diColour.h"
#include "diLinetype.h"

#include <QPolygonF>

#include "vcross_v2/VcrossQtPaint.h"

#include <cmath>

#define MILOGGER_CATEGORY "diana.DiGLPainter"
#include <miLogger/miLogging.h>

DiGLCanvas::GLuint DiGLCanvas::GenLists(GLsizei range)
{
  Q_UNUSED(range);
  return 0;
}

DiGLCanvas::GLboolean DiGLCanvas::IsList(GLuint list)
{
  Q_UNUSED(list);
  return false;
}

void DiGLCanvas::DeleteLists(GLuint list, GLsizei range)
{
  Q_UNUSED(list);
  Q_UNUSED(list);
}

bool DiGLCanvas::supportsDrawLists() const
{
  return false;
}

// ========================================================================

void DiGLPainter::CallList(GLuint list)
{
  Q_UNUSED(list);
}

void DiGLPainter::EndList()
{
}

void DiGLPainter::NewList(GLuint list, GLenum mode)
{
  Q_UNUSED(list);
  Q_UNUSED(mode);
}

void DiGLPainter::drawCircle(float centerx, float centery, float radius)
{
  const int NPOINTS = 120;
  const float STEP = 2 * M_PI / NPOINTS;

  Begin(gl_LINE_LOOP);
  for (int i=0; i<NPOINTS; i++) {
    const float angle = i*STEP;
    Vertex2f(radius*std::cos(angle), radius*std::sin(angle));
  }
  End();
}

void DiGLPainter::fillCircle(float centerx, float centery, float radius)
{
  const int NPOINTS = 120;
  const float STEP = 2 * M_PI / NPOINTS;

  PolygonMode(gl_FRONT_AND_BACK, gl_FILL);
  Begin(gl_POLYGON);
  for (int i=0; i<NPOINTS; i++) {
    const float angle = i*STEP;
    Vertex2f(radius*std::cos(angle), radius*std::sin(angle));
  }
  End();
}

void DiGLPainter::drawLine(float x1, float y1, float x2, float y2)
{
  Begin(gl_LINES);
  Vertex2f(x1, y1);
  Vertex2f(x2, y2);
  End();
}

void DiGLPainter::drawRect(float x1, float y1, float x2, float y2)
{
  PolygonMode(gl_FRONT_AND_BACK, gl_LINE);
  Begin(gl_LINE_LOOP);
  Vertex2f(x1, y1);
  Vertex2f(x2, y1);
  Vertex2f(x2, y2);
  Vertex2f(x1, y2);
  End();
}

void DiGLPainter::fillRect(float x1, float y1, float x2, float y2)
{
  ShadeModel(gl_FLAT);
  PolygonMode(gl_FRONT_AND_BACK, gl_FILL);
  Begin(gl_POLYGON);
  Vertex2f(x1, y1);
  Vertex2f(x2, y1);
  Vertex2f(x2, y2);
  Vertex2f(x1, y2);
  End();
}

void DiGLPainter::drawWindArrow(float u, float v, float x, float y,
    float arrowSize, bool withArrowHead)
{
  QVector<QLineF> lines;
  std::vector<QPointF> trianglePoints;
  // beware of y direction!
  vcross::PaintWindArrow::makeArrowPrimitives(lines, trianglePoints,
      arrowSize, withArrowHead, -1, u, v, x, y);

  for (int i=0; i<lines.size(); ++i) {
    const QLineF& line = lines.at(i);
    drawLine(line.x1(), line.y1(), line.x2(), line.y2());
  }

  // draw triangles
  const int nTrianglePoints = trianglePoints.size();
  if (nTrianglePoints >= 3) {
    ShadeModel(gl_FLAT);
    PolygonMode(gl_FRONT_AND_BACK, gl_FILL);
    Begin(gl_TRIANGLES);
    for (int i = 0; i < nTrianglePoints; ++i) {
      const QPointF& p = trianglePoints[i];
      Vertex2f(p.x(), p.y());
    }
    End();
    PolygonMode(gl_FRONT_AND_BACK, gl_LINE);
  }
}

void DiGLPainter::clear(const Colour& colour)
{
  ClearColor(colour.fR(), colour.fG(), colour.fB(), 1.0);
  Clear(gl_COLOR_BUFFER_BIT);
}

void DiGLPainter::setColour(const Colour& c, bool alpha)
{
  if (alpha)
    Color4ubv(c.RGBA());
  else
    Color3ubv(c.RGB());
}

void DiGLPainter::setLineStyle(const Colour& c, float lw, bool alpha)
{
  setColour(c, alpha);
  LineWidth(lw);
  Disable(gl_LINE_STIPPLE);
}

void DiGLPainter::setLineStyle(const Colour& c, float lw, const Linetype& lt, bool alpha)
{
  setLineStyle(c, lw, alpha);
  if (lt.stipple) {
    Enable(gl_LINE_STIPPLE);
    LineStipple(lt.factor, lt.bmap);
  } else {
    Disable(gl_LINE_STIPPLE);
  }
}

void DiGLPainter::drawPolyline(const QPolygonF& points)
{
  if (points.size() < 2)
    return;

  Begin(gl_LINE_STRIP);
  for (int i=0; i<points.size(); i++) {
    const QPointF& p = points.at(i);
    Vertex2f(p.x(), p.y());
  }
  End();
}

void DiGLPainter::fillQuadStrip(const QPolygonF& points)
{
  if (points.size() < 2)
    return;

  ShadeModel(gl_FLAT);
  PolygonMode(gl_FRONT_AND_BACK, gl_FILL);
  Begin(gl_QUAD_STRIP);
  for (int i=0; i<points.size(); i++) {
    const QPointF& p = points.at(i);
    Vertex2f(p.x(), p.y());
  }
  End();
}
