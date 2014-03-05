/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2013 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <EditItems/drawingstylemanager.h>
#include <EditItems/drawingitembase.h>
#include <QVector2D>
#include <qmath.h>

// Use the predefined fill patterns already defined for the existing editing and objects modes.
#include "polyStipMasks.h"
#include <diTesselation.h>

DrawingStyleManager *DrawingStyleManager::self = 0;

DrawingStyleManager::DrawingStyleManager()
{
  self = this;
}

DrawingStyleManager::~DrawingStyleManager()
{
}

DrawingStyleManager *DrawingStyleManager::instance()
{
  if (!DrawingStyleManager::self)
    DrawingStyleManager::self = new DrawingStyleManager();

  return DrawingStyleManager::self;
}

QVariantMap DrawingStyleManager::parse(const QHash<QString, QString> &definition) const
{
  QVariantMap style;

  QString lineColour = definition.value("linecolour", "black");
  style["linecolour"] = parseColour(lineColour);
  style["linewidth"] = definition.value("linewidth", "1.0").toFloat();
  style["linepattern"] = definition.value("linepattern", "solid");
  style["linesmooth"] = definition.value("linesmooth", "false") == "true";
  style["lineshape"] = definition.value("lineshape", "normal");
  style["fillcolour"] = parseColour(definition.value("fillcolour", "128:128:128:50"));
  style["fillpattern"] = definition.value("fillpattern");
  style["closed"] = definition.value("closed", "true") == "true";
  style["decoration1"] = definition.value("decoration1").split(",");
  style["decoration1.colour"] = parseColour(definition.value("decoration1.colour", lineColour));
  style["decoration1.offset"] = definition.value("decoration1.offset", "0").toInt();
  style["decoration2"] = definition.value("decoration2").split(",");
  style["decoration2.colour"] = parseColour(definition.value("decoration2.colour", lineColour));
  style["decoration2.offset"] = definition.value("decoration2.offset", "0").toInt();

  return style;
}

QColor DrawingStyleManager::parseColour(const QString &text) const
{
  const QColor defaultColour(text);

  if (text.startsWith('#')) {
    // assume format #RGBA with hexadecimal components in range [0, 255] (i.e. two-digit) and optional alpha
    bool ok;
    const int r = text.mid(1, 2).toInt(&ok, 16);
    if (!ok) return defaultColour;
    const int g = text.mid(3, 2).toInt(&ok, 16);
    if (!ok) return defaultColour;
    const int b = text.mid(5, 2).toInt(&ok, 16);
    if (!ok) return defaultColour;
    QColor colour(r, g, b);
    const int a = text.mid(7, 2).toInt(&ok, 16);
    if (ok)
      colour.setAlpha(a);
    return colour;
  } else if (text.contains(":")) {
    // assume format R:G:B:A with decimal components in range [0, 255] and optional alpha
    const QStringList pieces = text.split(":");
    bool ok;
    const int r = pieces.value(0).toInt(&ok);
    if (!ok) return defaultColour;
    const int g = pieces.value(1).toInt(&ok);
    if (!ok) return defaultColour;
    const int b = pieces.value(2).toInt(&ok);
    if (!ok) return defaultColour;
    QColor colour(r, g, b);
    const int a = pieces.value(3).toInt(&ok);
    if (ok)
      colour.setAlpha(a);
    return colour;
  }

  return defaultColour;
}

void DrawingStyleManager::addStyle(const QHash<QString, QString> &definition)
{
  // Parse the definition and set the private members.
  QString styleName = definition.value("style");

  if (styleName == "Custom") {
    // ### This doesn't make sense unless we want to have special default values for the custom style.
    // ### For now the custom style can just use the standard default values, so just skip (possibly give a warning?).
    return;
  }

  styles[styleName] = parse(definition);
}

void DrawingStyleManager::setStyle(DrawingItemBase *item, const QHash<QString, QString> &style, const QString &prefix) const
{
  foreach (QString key, style.keys()) {
    if (key.startsWith(prefix)) {
      const QString name = key.mid(prefix.size());
      item->setProperty(QString("style:%1").arg(name), style.value(key));
    }
  }
}

void DrawingStyleManager::setDefaultStyle(DrawingItemBase *item) const
{
  item->setProperty("style:type", "Custom");
  const QVariantMap vstyle = getStyle(item);
  QHash<QString, QString> style;
  foreach (QString key, vstyle.keys()) {
    const QVariant var = vstyle.value(key);
    if (var.type() == QVariant::Color) {
      // append the alpha component explicitly (var.toString() returns only "#rrggbb")
      const QColor col = var.value<QColor>();
      style.insert(key, QString("%1%2").arg(col.name()).arg(col.alpha(), 2, 16, QLatin1Char('0')));
    } else {
      style.insert(key, var.toString());
    }
  }
  setStyle(item, style);
}

void DrawingStyleManager::beginLine(DrawingItemBase *item)
{
  glPushAttrib(GL_LINE_BIT);

  const QVariantMap style = getStyle(item);

  QString linePattern = style.value("linepattern").toString();
  if (linePattern == "dashed") {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(2, 0xf0f0);
  }

  float lineWidth = style.value("linewidth").toFloat();
  glLineWidth(lineWidth);

  QColor borderColour = style.value("linecolour").value<QColor>();
  if (borderColour.isValid())
    glColor4ub(borderColour.red(), borderColour.green(), borderColour.blue(),
               borderColour.alpha());
}

void DrawingStyleManager::endLine(DrawingItemBase *item)
{
  Q_UNUSED(item)

  glPopAttrib(); // GL_LINE_BIT
}

void DrawingStyleManager::beginFill(DrawingItemBase *item)
{
  glPushAttrib(GL_POLYGON_BIT);

  QVariantMap style = getStyle(item);

  QColor fillColour = style.value("fillcolour").value<QColor>();
  glColor4ub(fillColour.red(), fillColour.green(), fillColour.blue(),
             fillColour.alpha());

  QString fillPattern = style.value("fillpattern").toString();

  if (!fillPattern.isEmpty()) {
    const GLubyte *fillPatternData = 0;

    if (fillPattern == "diagleft")
      fillPatternData = diagleft;
    else if (fillPattern == "zigzag")
      fillPatternData = zigzag;
    else if (fillPattern == "paralyse")
      fillPatternData = paralyse;
    else if (fillPattern == "ldiagleft2")
      fillPatternData = ldiagleft2;
    else if (fillPattern == "vdiagleft")
      fillPatternData = vdiagleft;
    else if (fillPattern == "vldiagcross_little")
      fillPatternData = vldiagcross_little;

    if (fillPatternData) {
      glEnable(GL_POLYGON_STIPPLE);
      glPolygonStipple(fillPatternData);
    }
  }
}

void DrawingStyleManager::endFill(DrawingItemBase *item)
{
  Q_UNUSED(item)

  glPopAttrib(); // GL_POLYGON_BIT
}

bool DrawingStyleManager::contains(const QString &name) const
{
  return styles.contains(name);
}

QVariantMap DrawingStyleManager::getStyle(const QString &name) const
{
  return styles.value(name);
}

QVariantMap DrawingStyleManager::getStyle(DrawingItemBase *item) const
{
  return getStyle(const_cast<const DrawingItemBase *>(item));
}

QVariantMap DrawingStyleManager::getStyle(const DrawingItemBase *item) const
{
  const QString styleName = item->property("style:type").toString();

  if (styleName == "Custom" || (!contains(styleName))) {
    // use default values, but override with values stored directly in the item
    QHash<QString, QString> styleProperties;
    foreach (QString key, item->propertiesRef().keys()) {
      if (key.startsWith("style:"))
        styleProperties[key.mid(6)] = item->propertiesRef().value(key).toString();
    }
    return parse(styleProperties);
  } else {
    // use fixed values for this style
    return styles.value(styleName);
  }
}

void DrawingStyleManager::drawLines(const DrawingItemBase *item, const QList<QPointF> &points, int z) const
{
  QVariantMap style = getStyle(item);
  bool closed = style.value("closed").toBool();

  if (closed)
    glBegin(GL_LINE_LOOP);
  else
    glBegin(GL_LINE_STRIP);

  QList<QPointF> points_;

  if (style.value("linesmooth").toBool())
    points_ = interpolateToPoints(points, closed);
  else
    points_ = points;

  if (style.value("linecolour").value<QColor>().alpha() != 0) {
    foreach (QPointF p, points_)
      glVertex3i(p.x(), p.y(), z);
  }

  glEnd(); // GL_LINE_LOOP or GL_LINE_STRIP

  if (style.value("decoration1").isValid()) {
    QColor colour = style.value("decoration1.colour").value<QColor>();
    glColor4ub(colour.red(), colour.green(), colour.blue(), colour.alpha());

    unsigned int offset = style.value("decoration1.offset").toInt();
    foreach (QVariant v, style.value("decoration1").toList()) {
      QString decor = v.toString();
      drawDecoration(style, decor, closed, Outside, points_, z, offset);
      offset += 1;
    }
  }

  if (style.value("decoration2").isValid()) {
    QColor colour = style.value("decoration2.colour").value<QColor>();
    glColor4ub(colour.red(), colour.green(), colour.blue(), colour.alpha());

    unsigned int offset = style.value("decoration2.offset").toInt();
    foreach (QVariant v, style.value("decoration2").toList()) {
      QString decor = v.toString();
      drawDecoration(style, decor, closed, Inside, points_, z, offset);
      offset += 1;
    }
  }
}

void DrawingStyleManager::drawDecoration(const QVariantMap &style, const QString &decoration, bool closed,
                                         const Side &side, const QList<QPointF> &points, int z,
                                         unsigned int offset) const
{
  int di = closed ? 0 : -1;
  int sidef = (side == Inside) ? 1 : -1;

  if (decoration == "triangles") {

    int lineWidth = style.value("linewidth").toInt();
    int lineLength = lineWidth * 9;
    QList<QPointF> points_ = getDecorationLines(points, lineLength);
    qreal size = lineWidth * 5;

    glBegin(GL_TRIANGLES);

    for (int i = offset; i < points_.size() + di; i += 4) {

      QLineF line(points_.at(i), points_.at((i + 1) % points_.size()));
      if (line.length() < lineLength*0.75)
        continue;

      QPointF midpoint = (line.p1() + line.p2())/2;
      QPointF normal = QPointF(line.normalVector().unitVector().dx(),
                               line.normalVector().unitVector().dy());

      QPointF p = midpoint + (sidef * size * normal);
      glVertex3f(p.x(), p.y(), z);
      glVertex3f(line.p1().x(), line.p1().y(), z);
      glVertex3f(line.p2().x(), line.p2().y(), z);
    }

    glEnd(); // GL_TRIANGLES

  } else if (decoration == "arches") {

    int lineWidth = style.value("linewidth").toInt();
    int lineLength = lineWidth * 9;
    QList<QPointF> points_ = getDecorationLines(points, lineLength);
    qreal radius = lineWidth * 5;
    int npoints = lineWidth * 20;

    for (int i = offset; i < points_.size() + di; i += 4) {

      QLineF line(points_.at(i), points_.at((i + 1) % points_.size()));
      if (line.length() < lineLength*0.75)
        continue;

      qreal start_angle = qAtan2(-line.dy(), -line.dx());
      qreal finish_angle = qAtan2(line.dy(), line.dx());

      QPointF midpoint = (line.p1() + line.p2())/2;
      qreal astep = qAbs(finish_angle - start_angle)/npoints;

      // Create an arc using points on the circle with the predefined radius.
      // The direction we go around the circle is chosen to be consistent with
      // previous behaviour.
      glBegin(GL_POLYGON);

      for (int j = 0; j < npoints; ++j) {
        QPointF p = midpoint + QPointF(radius * qCos(start_angle + sidef * j*astep),
                                       radius * qSin(start_angle + sidef * j*astep));
        glVertex3f(p.x(), p.y(), z);
      }

      glEnd(); // GL_POLYGON
    }

  } else if (decoration == "crosses") {

    int lineWidth = style.value("linewidth").toInt();
    int lineLength = lineWidth * 9;
    QList<QPointF> points_ = getDecorationLines(points, lineLength);
    qreal size = lineWidth * 3;

    glBegin(GL_LINES);

    for (int i = offset; i < points_.size() + di; i += 2) {

      QLineF line(points_.at(i), points_.at((i + 1) % points_.size()));
      QPointF midpoint = (line.p1() + line.p2())/2;
      QPointF tangent = QPointF(line.unitVector().dx(), line.unitVector().dy());
      QPointF normal = QPointF(line.normalVector().unitVector().dx(),
                               line.normalVector().unitVector().dy());

      QPointF p = midpoint + (size * normal) + (size * tangent);
      glVertex3f(p.x(), p.y(), z);
      p = midpoint - (size * normal) - (size * tangent);
      glVertex3f(p.x(), p.y(), z);
      p = midpoint - (size * normal) + (size * tangent);
      glVertex3f(p.x(), p.y(), z);
      p = midpoint + (size * normal) - (size * tangent);
      glVertex3f(p.x(), p.y(), z);
    }

    glEnd(); // GL_LINES

  } else if (decoration == "arrow") {

    if (points.size() < 2)
      return;

    // Draw a triangle with the forward point the at the end of the last
    // line segment.
    int lineWidth = style.value("linewidth").toInt();
    int lineLength = lineWidth * 8;
    QList<QPointF> points_ = getDecorationLines(points, lineLength);

    QPointF p = points_.at(points_.size() - 2);
    QLineF line(p, points_.last());
    QPointF tangent = QPointF(line.unitVector().dx(), line.unitVector().dy());
    QPointF normal = QPointF(line.normalVector().unitVector().dx(),
                             line.normalVector().unitVector().dy());
    QPointF dp = normal * lineWidth * 3;
    QPointF p1 = points_.last() - (lineLength * 2 * tangent) - dp;
    QPointF p2 = points_.last() - (lineLength * 2 * tangent) + dp;

    glBegin(GL_TRIANGLES);
    glVertex3f(points_.last().x(), points_.last().y(), z);
    glVertex3f(p1.x(), p1.y(), z);
    glVertex3f(p2.x(), p2.y(), z);
    glEnd(); // GL_TRIANGLES

  } else if (decoration == "SIGWX") {

    int lineWidth = style.value("linewidth").toInt();
    int lineLength = lineWidth * 8;
    QList<QPointF> points_ = getDecorationLines(points, lineLength);
    int npoints = lineWidth * 12;

    glBegin(GL_LINE_STRIP);

    for (int i = 0; i < points_.size() + di; ++i) {

      QLineF line(points_.at(i), points_.at((i + 1) % points_.size()));
      qreal start_angle = qAtan2(-line.dy(), -line.dx());
      qreal finish_angle = qAtan2(line.dy(), line.dx());

      QPointF midpoint = (line.p1() + line.p2())/2;
      qreal radius = line.length()/2;
      qreal astep = qAbs(finish_angle - start_angle)/npoints;

      // Create an arc using points on the circle with the predefined radius.
      // The direction we go around the circle is chosen to be consistent with
      // previous behaviour.
      for (int j = 0; j < npoints; ++j) {
        QPointF p = midpoint + QPointF(radius * qCos(start_angle + sidef * j*astep),
                                       radius * qSin(start_angle + sidef * j*astep));
        glVertex3f(p.x(), p.y(), z);
      }
    }
    glEnd(); // GL_LINE_STRIP

  }
}

void DrawingStyleManager::fillLoop(const DrawingItemBase *item, const QList<QPointF> &points) const
{
  QVariantMap style = getStyle(item);

  QList<QPointF> points_;
  if (style.value("linesmooth").toBool())
    points_ = interpolateToPoints(points, true);
  else
    points_ = points;

  // draw the interior
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glEnable( GL_BLEND );
  GLdouble *gldata = new GLdouble[points_.size() * 3];
  for (int i = 0; i < points_.size(); ++i) {
    const QPointF p = points_.at(i);
    gldata[3 * i] = p.x();
    gldata[3 * i + 1] = p.y();
    gldata[3 * i + 2] = 0.0;
  }

  beginTesselation();
  int npoints = points_.size();
  tesselation(gldata, 1, &npoints);
  endTesselation();
  delete[] gldata;
}

const QPainterPath DrawingStyleManager::interpolateToPath(const QList<QPointF> &points, bool closed)
{
  int size = points.size();
  if (size <= 2) {
    QPainterPath path;
    if (size == 0)
      return path;
    path.moveTo(points.at(0));
    if (size == 1)
      return path;
    path.lineTo(points.at(1));
    return path;
  }

  QList<QPointF> new_points;

  for (int i = 0; i < size; ++i) {

    int j = i - 1;    // Find the index of the previous point.
    if (j < 0) {
      if (closed)
        j += size;    // Closed polylines use the last point.
      else
        j = 0;        // Open polylines use the first point.
    }
    int k = i + 1;    // Find the index of the next point.
    if (k == size) {
      if (closed)
        k = 0;        // Closed polylines use the first point.
      else
        k = i;        // Open polylines use the last point.
    }

    // Take the previous, current and next points in the list.
    QVector2D previous(points.at(j));
    QVector2D p(points.at(i));
    QVector2D next(points.at(k));

    // Calculate the vectors from the current to the previous and next points.
    QVector2D previous_v = previous - p;
    QVector2D next_v = next - p;

    // Take the minimum length of the two vectors.
    qreal prev_l = previous_v.length();
    qreal next_l = next_v.length();
    qreal l = qMin(prev_l, next_l);

    // Adjust the previous and next points to lie the same distance away from
    // the current point.
    QVector2D new_previous = p + previous_v.normalized() * l;
    QVector2D new_next = p + next_v.normalized() * l;

    // The line between the adjusted previous and next points is used as a
    // tangent or gradient that we extend from the current point to position
    // control points.
    QVector2D gradient = (new_next - new_previous).normalized();
    prev_l = qMin(QVector2D::dotProduct(p - previous, gradient), l)/3.0;
    next_l = qMin(QVector2D::dotProduct(next - p, gradient), l)/3.0;

    // Construct two control points on either side of the current point.
    QVector2D p0 = p - prev_l * gradient;
    QVector2D p1 = p + next_l * gradient;

    new_points << p0.toPointF() << p1.toPointF();
  }

  // The first control point in the list belongs to the last point, so move
  // it to the end of the list.
  new_points.append(new_points.takeFirst());

  // For open paths, do not include the segment from the last point to the first.
  int end = (closed ? size : size - 1);

  QPainterPath path;
  path.moveTo(points.at(0));
  for (int i = 0; i < end; ++i)
    path.cubicTo(new_points.at(i*2), new_points.at((i*2)+1), points.at((i+1) % size));

  return path;
}

const QList<QPointF> DrawingStyleManager::interpolateToPoints(const QList<QPointF> &points, bool closed)
{
  QList<QPointF> new_points;
  QPainterPath path = interpolateToPath(points, closed);

  foreach (QPolygonF polygon, path.toSubpathPolygons())
    new_points << polygon.toList();

  return new_points;
}

const QList<QPointF> DrawingStyleManager::getDecorationLines(const QList<QPointF> &points, qreal lineLength)
{
  if (points.size() < 2)
    return points;

  QList<QPointF> new_points;
  QPointF last = points.at(0);
  qreal l = 0;
  qreal last_length = 0;

  new_points << last;

  for (int i = 1; i < points.size(); ++i) {

    QLineF this_line = QLineF(last, points.at(i));
    l += this_line.length();

    if (l >= lineLength) {

      // Find the point on the current line that corresponds to the required line length.
      while (l >= lineLength) {
        QLineF line = this_line;
        line.setLength(lineLength - last_length);
        new_points << line.p2();

        // Remove the part of the line already handled.
        this_line.setP1(line.p2());
        l -= lineLength;
        last_length = 0;
      }

      last_length = this_line.length();
    } else
      last_length = l;

    last = points.at(i);
  }
  if (new_points.last() != last)
    new_points << last;

  return new_points;
}

void DrawingStyleManager::beginText(DrawingItemBase *item)
{
  QVariantMap style = getStyle(item);
  QColor textColour = style.value("textcolour").value<QColor>();
  if (textColour.isValid())
    glColor4ub(textColour.red(), textColour.green(), textColour.blue(),
               textColour.alpha());
  else
    glColor3f(0.0, 0.0, 0.0);
}

void DrawingStyleManager::endText(DrawingItemBase *item)
{
  Q_UNUSED(item)
}
