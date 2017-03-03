/*
  Diana - A Free Meteorological Visualisation Tool

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

#include "diPlot.h"
#include "diPlotModule.h"
#include "diDrawingManager.h"
#include "EditItems/drawingstylemanager.h"
#include "EditItems/drawingitembase.h"
#include "EditItems/drawingsymbol.h"
#include "EditItems/drawingtext.h"
#include "diGLPainter.h"
#include "diGlUtilities.h"
#include <QApplication>
#include <QComboBox>
#include <QPainter>
#include <QVector2D>
#include <qmath.h>

// Use the predefined fill patterns already defined for the existing editing and objects modes.
#include "polyStipMasks.h"

DrawingStyleProperty::DrawingStyleProperty()
{
}

QVariant DrawingStyleProperty::parse(const QString &text) const
{
  return text;
}

DSP_Colour::DSP_Colour(const QString &text)
{
  defaultColour = text;
}

QVariant DSP_Colour::parse(const QString &text) const
{
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

  QColor colour(text);

  if (colour.isValid())
    return colour;
  else
    return QColor(defaultColour);
}

QVariant DSP_Int::parse(const QString &text) const
{
  return qMax(0, text.toInt());
}

DSP_Alpha::DSP_Alpha(int defaultValue)
{
  defaultAlpha = defaultValue;
}

QVariant DSP_Alpha::parse(const QString &text) const
{
  bool ok = true;
  int alpha = text.toInt(&ok);
  if (!ok)
    alpha = defaultAlpha;
  return qBound(0, alpha, 255);
}

QVariant DSP_Float::parse(const QString &text) const
{
  return text.toFloat();
}

QVariant DSP_Width::parse(const QString &text) const
{
  bool ok = true;
  int width = text.toFloat(&ok);
  if (!ok)
    width = 1.0;
  return width;
}

QVariant DSP_LinePattern::parse(const QString &text) const
{
  bool ok = false;
  const ushort pattern = text.toUShort(&ok);
  if (ok)
    return pattern;
  if (text == "dashed")
    return 0x0f0f; // ### for now
  return 0xffff;
}

QVariant DSP_Boolean::parse(const QString &text) const
{
  return text == "true";
}

DSP_StringList::DSP_StringList(const QString &text)
{
  separator = text;
}

QVariant DSP_StringList::parse(const QString &text) const
{
  return text.split(separator);
}


DrawingStyleManager *DrawingStyleManager::self_ = 0;

DrawingStyleManager::DrawingStyleManager()
{
  self_ = this;

  DrawingStyleProperty *stringProp = new DrawingStyleProperty();
  DrawingStyleProperty *lineColourProp = new DSP_Colour("black");
  DrawingStyleProperty *fillColourProp = new DSP_Colour("#808080");
  DrawingStyleProperty *lineAlphaProp = new DSP_Alpha(255);
  DrawingStyleProperty *fillAlphaProp = new DSP_Alpha(50);
  DrawingStyleProperty *widthProp = new DSP_Width();
  DrawingStyleProperty *linePatternProp = new DSP_LinePattern();
  DrawingStyleProperty *booleanProp = new DSP_Boolean();
  DrawingStyleProperty *stringListProp = new DSP_StringList(",");
  DrawingStyleProperty *objListProp = new DSP_StringList(":");
  DrawingStyleProperty *intProp = new DSP_Int();
  DrawingStyleProperty *floatProp = new DSP_Float();

  // Define the supported polyline style properties.
  QHash<QString, DrawingStyleProperty *> polyProps;
  polyProps.insert("linecolour", lineColourProp);
  polyProps.insert("linealpha", lineAlphaProp);
  polyProps.insert("linewidth", widthProp);
  polyProps.insert("linepattern", linePatternProp);
  polyProps.insert("linesmooth", booleanProp);
  polyProps.insert("fillcolour", fillColourProp);
  polyProps.insert("fillalpha", fillAlphaProp);
  polyProps.insert("fillpattern", stringProp);
  polyProps.insert("closed", booleanProp);
  polyProps.insert("reversed", booleanProp);
  polyProps.insert("decoration1", stringListProp);
  polyProps.insert("decoration1.colour", lineColourProp);
  polyProps.insert("decoration1.alpha", lineAlphaProp);
  polyProps.insert("decoration1.offset", intProp);
  polyProps.insert("decoration2", stringListProp);
  polyProps.insert("decoration2.colour", lineColourProp);
  polyProps.insert("decoration2.alpha", lineAlphaProp);
  polyProps.insert("decoration2.offset", intProp);
  properties_[DrawingItemBase::PolyLine] = polyProps;

  // Define the supported symbol style properties.
  QHash<QString, DrawingStyleProperty *> symbolProps;
  symbolProps.insert("symbolcolour", lineColourProp);
  symbolProps.insert("symbolalpha", lineAlphaProp);
  properties_[DrawingItemBase::Symbol] = symbolProps;

  // Define the supported text style properties.
  QHash<QString, DrawingStyleProperty *> textProps;
  textProps.insert("linecolour", lineColourProp);
  textProps.insert("linealpha", lineAlphaProp);
  textProps.insert("linewidth", widthProp);
  textProps.insert("linepattern", linePatternProp);
  textProps.insert("fillcolour", fillColourProp);
  textProps.insert("fillalpha", fillAlphaProp);
  textProps.insert("textcolour", lineColourProp);
  textProps.insert("textalpha", lineAlphaProp);
  textProps.insert("fontsize", widthProp);
  textProps.insert("cornersegments", intProp);
  textProps.insert("cornerradius", floatProp);
  textProps.insert("hide", booleanProp);
  properties_[DrawingItemBase::Text] = textProps;

  // Define the supported composite style properties.
  QHash<QString, DrawingStyleProperty *> compProps;
  compProps.insert("objects", objListProp);
  compProps.insert("values", objListProp);
  compProps.insert("styles", objListProp);
  compProps.insert("layout", stringProp);
  compProps.insert("linecolour", lineColourProp);
  compProps.insert("linewidth", widthProp);
  compProps.insert("linealpha", lineAlphaProp);
  compProps.insert("fillcolour", fillColourProp);
  compProps.insert("fillalpha", fillAlphaProp);
  compProps.insert("cornersegments", intProp);
  compProps.insert("cornerradius", floatProp);
  compProps.insert("hide", booleanProp);
  compProps.insert("closed", booleanProp);
  compProps.insert("border", stringProp);
  properties_[DrawingItemBase::Composite] = compProps;
}

DrawingStyleManager::~DrawingStyleManager()
{
}

DrawingStyleManager *DrawingStyleManager::instance()
{
  if (!DrawingStyleManager::self_)
    DrawingStyleManager::self_ = new DrawingStyleManager();

  return DrawingStyleManager::self_;
}

QVariantMap DrawingStyleManager::parse(const DrawingItemBase::Category &category,
                                       const QHash<QString, QString> &definition) const
{
  QVariantMap style;

  // Use the instances of the predefined property classes for each object type
  // to parse the definitions from the setup file.
  foreach (QString propName, properties_[category].keys())
    style[propName] = properties_[category].value(propName)->parse(definition.value(propName));

  // All definitions can declare general editable properties that objects of
  // that type can have.
  style["properties"] = DSP_StringList(",").parse(definition.value("properties"));
  return style;
}

void DrawingStyleManager::addStyle(const DrawingItemBase::Category &category, const QHash<QString, QString> &definition)
{
  // Parse the definition and set the private members.
  QString styleType;
  if (definition.contains("textstyle"))
    styleType = definition.value("textstyle");
  else if (definition.contains("symbol"))
    styleType = definition.value("symbol");
  else if (definition.contains("composite"))
    styleType = definition.value("composite");
  else
    styleType = definition.value("style");

  styles_[category][styleType] = parse(category, definition);
}

void DrawingStyleManager::setStyle(DrawingItemBase *item, const QHash<QString, QString> &style, const QString &prefix) const
{
  foreach (QString key, style.keys()) {
    if (key.startsWith(prefix)) {
      const QString name = key.mid(prefix.size());
      item->setProperty(QString("style:%1").arg(name), style.value(key));
    }
  }

  // if the style type is defined, set missing style properties to their default values
  const QStringList origItemProps = item->propertiesRef().keys();
  if (origItemProps.contains("style:type")) {
    const QVariantMap typeStyle = getStyle(item->category(), item->propertiesRef().value("style:type").toString());
    foreach (const QString &prop, typeStyle.keys()) {
      const QString prefixedProp = QString("style:%1").arg(prop);
      if (!origItemProps.contains(prefixedProp))
        item->propertiesRef().insert(prefixedProp, typeStyle.value(prop));
    }
  }
}

void DrawingStyleManager::setStyle(DrawingItemBase *item, const QVariantMap &vstyle, const QString &prefix) const
{
  QHash<QString, QString> style;
  foreach (QString key, vstyle.keys()) {
    const QVariant v = vstyle.value(key);
    style.insert(key, (v.type() == QVariant::StringList) ? v.toStringList().join(",") : v.toString());
  }
  setStyle(item, style, prefix);
}

void DrawingStyleManager::setComplexTextList(const QStringList &strings)
{
  complexTextList_ = strings;
}

void DrawingStyleManager::beginLine(DiGLPainter* gl, DrawingItemBase *item)
{
  gl->PushAttrib(DiGLPainter::gl_LINE_BIT);

  const QVariantMap style = getStyle(item);
  const QString style_linepattern = style.value("linepattern").toString();
  DrawingStyleProperty* propedit_linepattern = properties_[DrawingItemBase::PolyLine].value("linepattern");

  const ushort linePattern = propedit_linepattern->parse(style_linepattern).toUInt();
  if (linePattern != 0xFFFF) {
    gl->Enable(DiGLPainter::gl_LINE_STIPPLE);
    gl->LineStipple(2, linePattern);
  }

  float lineWidth = style.value("linewidth").toFloat();
  gl->LineWidth(lineWidth);

  QColor borderColour = style.value("linecolour").value<QColor>();
  if (!borderColour.isValid())
    borderColour = QColor(Qt::black);

  bool alphaOk;
  const int alpha = style.value("linealpha").toInt(&alphaOk);
  if (borderColour.isValid())
    gl->Color4ub(borderColour.red(), borderColour.green(), borderColour.blue(), alphaOk ? alpha : 255);
}

void DrawingStyleManager::endLine(DiGLPainter* gl, DrawingItemBase *item)
{
  Q_UNUSED(item)

  gl->PopAttrib();
}

void DrawingStyleManager::beginFill(DiGLPainter* gl, DrawingItemBase *item)
{
  gl->PushAttrib(DiGLPainter::gl_POLYGON_BIT);

  QVariantMap style = getStyle(item);

  QColor fillColour = style.value("fillcolour").value<QColor>();

  bool alphaOk;
  int alpha = style.value("fillalpha").toInt(&alphaOk);

  if (!fillColour.isValid()) {
    fillColour = QColor(Qt::gray);
    alpha = 50;
    alphaOk = true;
  }

  gl->Color4ub(fillColour.red(), fillColour.green(), fillColour.blue(), alphaOk ? alpha : 255);

  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);

  QString fillPattern = style.value("fillpattern").toString();

  if (!fillPattern.isEmpty()) {
    const DiGLPainter::GLubyte *fillPatternData = 0;

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
    else if (fillPattern == "snow")
      fillPatternData = snow;
    else if (fillPattern == "rain")
      fillPatternData = rain;

    if (fillPatternData) {
      gl->Enable(DiGLPainter::gl_POLYGON_STIPPLE);
      gl->PolygonStipple(fillPatternData);
    }
  }
}

void DrawingStyleManager::endFill(DiGLPainter* gl, DrawingItemBase *item)
{
  Q_UNUSED(item)

  gl->PopAttrib();
}

QString DrawingStyleManager::styleCategoryName(const StyleCategory sc)
{
  if (sc == Invalid)
    return "Invalid";
  else if (sc == General)
    return "General";
  else if (sc == Line)
    return "Line";
  else if (sc == Area)
    return "Area";
  else if (sc == Decoration)
    return "Decoration";
  else if (sc == Symbol)
    return "Symbol";
  return "Unknown";
}

bool DrawingStyleManager::containsStyle(const DrawingItemBase::Category &category, const QString &name) const
{
  return styles_.contains(category) && styles_.value(category).contains(name);
}

QStringList DrawingStyleManager::styles(const DrawingItemBase::Category &category) const
{
  return styles_[category].keys();
}

QStringList DrawingStyleManager::properties(const DrawingItemBase::Category &category) const
{
  return properties_[category].keys();
}

QVariantMap DrawingStyleManager::getStyle(const DrawingItemBase::Category &category, const QString &name) const
{
  return styles_[category].value(name);
}

QVariantMap DrawingStyleManager::getStyle(DrawingItemBase *item) const
{
  return getStyle(const_cast<const DrawingItemBase *>(item));
}

/**
 * Get the style for the given item, merging the base style with any style
 * properties defined in the item itself.
 */
QVariantMap DrawingStyleManager::getStyle(const DrawingItemBase *item) const
{
  // Obtain the base style.
  const QString styleName = item->property("style:type").toString();
  QVariantMap style = styles_[item->category()].value(styleName);

  // Collect any style properties in the item itself.
  const QVariantMap& props = item->propertiesRef();
  QHash<QString, QString> styleProperties;

  QHash<QString, DrawingStyleProperty *> cProps = properties_[item->category()];

  foreach (const QString& key, props.keys()) {
    // If the property is a style property with a corresponding default
    // then let it override the default value.
    if (!key.startsWith("style:"))
      continue;
    const QString skey = key.mid(6);
    if (!cProps.contains(skey))
      continue;

    const QVariant& value = props.value(key);
    if (value.type() != QVariant::StringList) {
      QString s = value.toString();
      if (s != style.value(skey).toString())
        styleProperties[skey] = s;
    }
  }

  // Override any properties with custom properties.
  for (QHash<QString, DrawingStyleProperty *>::const_iterator it = cProps.begin(); it != cProps.end(); ++it) {
    QString propName = it.key();
    if (styleProperties.contains(propName))
      style[propName] = it.value()->parse(styleProperties.value(propName));
  }

  return style;
}

QStringList DrawingStyleManager::getComplexTextList() const
{
  return complexTextList_;
}

void DrawingStyleManager::highlightPolyLine(DiGLPainter* gl, const DrawingItemBase *item,
    const QList<QPointF> &points, int lineWidth, const QColor &col, bool forceClosed) const
{
  QVariantMap style = getStyle(item);
  const bool closed = forceClosed || style.value("closed").toBool();

  const int z = 0;
  const qreal lw_2 = lineWidth / 2.0;

  QList<QPointF> points_;
  if (style.value("linesmooth").toBool())
    points_ = interpolateToPoints(points, closed);
  else
    points_ = points;

  gl->Color4ub(col.red(), col.green(), col.blue(), col.alpha());

  gl->PushAttrib(DiGLPainter::gl_POLYGON_BIT);
  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);

  for (int i = 1; i < (points_.size() + (closed ? 1 : 0)); ++i) {
    const QPointF p1 = points_.at(i % points_.size());
    const QPointF p0 = points_.at(i - 1);
    const QVector2D v(p1.x() - p0.x(), p1.y() - p0.y());
    const QVector2D u = QVector2D(-v.y(), v.x()).normalized();

    gl->Begin(DiGLPainter::gl_POLYGON);
    gl->Vertex3f(p0.x() + lw_2 * u.x(), p0.y() + lw_2 * u.y(), z);
    gl->Vertex3f(p1.x() + lw_2 * u.x(), p1.y() + lw_2 * u.y(), z);
    gl->Vertex3f(p1.x() - lw_2 * u.x(), p1.y() - lw_2 * u.y(), z);
    gl->Vertex3f(p0.x() - lw_2 * u.x(), p0.y() - lw_2 * u.y(), z);
    gl->End(); // DiGLPainter::gl_POLYGON
  }

  gl->PopAttrib();
}

QList<QPointF> DrawingStyleManager::linesForBBox(DrawingItemBase *item) const
{
  return linesForBBox(item->boundingRect(), item->property("style:cornersegments", 0).toInt(),
                                            item->property("style:cornerradius", 0.0).toFloat(),
                                            item->property("style:border").toString());
}

QList<QPointF> DrawingStyleManager::linesForBBox(const QRectF &bbox, int cornerSegments,
                                                 float cornerRadius, const QString &border) const
{
  QList<QPointF> points;

  if (border == "circle") {
    for (int i = 0; i < 32; ++i) {
      float angle = i*(2*M_PI)/32;
      points << bbox.center() + 0.5 * qMax(bbox.width(), bbox.height()) * QPointF(qCos(angle), qSin(angle));
    }
  } else if (border == "diamond") {
    QPointF c = bbox.center();
    points << QPointF(c.x(), bbox.top()) << QPointF(bbox.right(), c.y()) << QPointF(c.x(), bbox.bottom()) << QPointF(bbox.left(), c.y());
  } else if (border == "scroll") {
    for (int i = 0; i < 32; ++i) {
      float angle = i*(2*M_PI)/32;
      points << QPointF(bbox.left() + i*bbox.width()/32.0, bbox.top() - bbox.height()*qSin(angle)/8);
    }
    points << bbox.topRight();
    for (int i = 0; i < 32; ++i) {
      float angle = i*(2*M_PI)/32;
      points << QPointF(bbox.right() - i*bbox.width()/32.0, bbox.bottom() + bbox.height()*qSin(angle)/8);
    }
    points << bbox.bottomLeft();
  } else if (cornerSegments != 0 && cornerRadius != 0.0) {
    for (int i = 0; i < cornerSegments; ++i) {
      float angle = (i*M_PI/2)/cornerSegments;
      points << bbox.bottomLeft() + cornerRadius*QPointF(1.0 - qCos(angle), -1.0 + qSin(angle));
    }
    for (int i = 0; i < cornerSegments; ++i) {
      float angle = (i*M_PI/2)/cornerSegments;
      points << bbox.bottomRight() + cornerRadius*QPointF(-1.0 + qSin(angle), -1.0 + qCos(angle));
    }
    for (int i = 0; i < cornerSegments; ++i) {
      float angle = (i*M_PI/2)/cornerSegments;
      points << bbox.topRight() + cornerRadius*QPointF(-1.0 + qCos(angle), 1.0 - qSin(angle));
    }
    for (int i = 0; i < cornerSegments; ++i) {
      float angle = (i*M_PI/2)/cornerSegments;
      points << bbox.topLeft() + cornerRadius*QPointF(1.0 - qSin(angle), 1.0 - qCos(angle));
    }
  } else
    points << bbox.bottomLeft() << bbox.bottomRight() << bbox.topRight() << bbox.topLeft();

  return points;
}

void DrawingStyleManager::drawLines(DiGLPainter* gl, const DrawingItemBase *item,
    const QList<QPointF> &points, int z, bool forceClosed) const
{
  QVariantMap style = getStyle(item);
  bool closed = forceClosed || style.value("closed").toBool();

  if (closed)
    gl->Begin(DiGLPainter::gl_LINE_LOOP);
  else
    gl->Begin(DiGLPainter::gl_LINE_STRIP);

  QList<QPointF> points_;

  // For smooth lines, we try to fit a smooth curve through the points defined
  // for the existing lines. The list of points obtained contains enough
  // points to show a smooth line.

  if (style.value("linesmooth").toBool())
    points_ = interpolateToPoints(points, closed);
  else
    points_ = points;

  {
    bool alphaOk;
    const int alpha = style.value("linealpha").toInt(&alphaOk);
    if ((!alphaOk) || (alpha >= 0)) {
      foreach (QPointF p, points_)
        gl->Vertex3i(p.x(), p.y(), z);
    }
  }

  gl->End(); // DiGLPainter::gl_LINE_LOOP or DiGLPainter::gl_LINE_STRIP

  const bool reversed = !style.value("reversed").toBool();

  if (style.value("decoration1").isValid()) {

    QColor colour = style.value("decoration1.colour").value<QColor>();
    bool alphaOk;
    const int alpha = style.value("decoration1.alpha").toInt(&alphaOk);
    gl->Color4ub(colour.red(), colour.green(), colour.blue(), alphaOk ? alpha : colour.alpha());

    unsigned int offset = style.value("decoration1.offset").toInt();
    foreach (QVariant v, style.value("decoration1").toList()) {
      QString decor = v.toString();
      drawDecoration(gl, item, style, decor, closed, reversed ? Outside : Inside, points_, z, offset);
      offset += 1;
    }
  }

  if (style.value("decoration2").isValid()) {
    QColor colour = style.value("decoration2.colour").value<QColor>();
    bool alphaOk;
    const int alpha = style.value("decoration2.alpha").toInt(&alphaOk);
    gl->Color4ub(colour.red(), colour.green(), colour.blue(), alphaOk ? alpha : colour.alpha());

    unsigned int offset = style.value("decoration2.offset").toInt();
    foreach (QVariant v, style.value("decoration2").toList()) {
      QString decor = v.toString();
      drawDecoration(gl, item, style, decor, closed, reversed ? Inside : Outside, points_, z, offset);
      offset += 1;
    }
  }
}

/**
 * Draws the given \a decoration using the \a style specified on the polyline
 * described by the list of \a points.
 *
 * The \a closed argument describes whether the decoration is being applied
 * to a closed polyline, \a side determines which side of the polyline the
 * decoration will appear, and \a offset describes the spacing from the start
 * of the polyline to the first decoration.
 *
 * The \a z argument specifies the z coordinate of the decorations.
 */
void DrawingStyleManager::drawDecoration(DiGLPainter* gl, const DrawingItemBase *item,
     const QVariantMap &style, const QString &decoration, bool closed,
     const Side &side, const QList<QPointF> &points, int z, unsigned int offset) const
{
  int di = closed ? 0 : -1;
  int sidef = (side == Inside) ? 1 : -1;

  if (decoration == "triangles") {

    int lineWidth = style.value("linewidth").toInt();
    int lineLength = lineWidth * 9;
    QList<QPointF> points_ = getDecorationLines(points, lineLength);
    qreal size = lineWidth * 5;

    gl->PushAttrib(DiGLPainter::gl_POLYGON_BIT);
    gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);

    gl->Begin(DiGLPainter::gl_TRIANGLES);

    for (int i = offset; i < points_.size() + di; i += 4) {

      QLineF line(points_.at(i), points_.at((i + 1) % points_.size()));
      if (line.length() < lineLength*0.75)
        continue;

      QPointF midpoint = (line.p1() + line.p2())/2;
      QPointF normal = QPointF(line.normalVector().unitVector().dx(),
                               line.normalVector().unitVector().dy());

      QPointF p = midpoint + (sidef * size * normal);
      gl->Vertex3f(p.x(), p.y(), z);
      gl->Vertex3f(line.p1().x(), line.p1().y(), z);
      gl->Vertex3f(line.p2().x(), line.p2().y(), z);
    }

    gl->End(); // DiGLPainter::gl_TRIANGLES

    gl->PopAttrib();

  } else if (decoration == "arches") {

    int lineWidth = style.value("linewidth").toInt();
    int lineLength = lineWidth * 9;
    QList<QPointF> points_ = getDecorationLines(points, lineLength);
    qreal radius = lineWidth * 5;
    int npoints = lineWidth * 20;

    for (int i = offset; i < points_.size() + di; i += 4) {

      QLineF line(points_.at(i), points_.at((i + 1) % points_.size()));
      if (line.length() < lineLength*0.75)
        continue; // We cannot use this line segment. Try the next one.

      qreal start_angle = qAtan2(-line.dy(), -line.dx());
      qreal finish_angle = qAtan2(line.dy(), line.dx());

      QPointF midpoint = (line.p1() + line.p2())/2;
      qreal astep = qAbs(finish_angle - start_angle)/npoints;

      gl->PushAttrib(DiGLPainter::gl_POLYGON_BIT);
      gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);

      // Create an arc using points on the circle with the predefined radius.
      // The direction we go around the circle is chosen to be consistent with
      // previous behaviour.
      gl->Begin(DiGLPainter::gl_POLYGON);

      for (int j = 0; j < npoints; ++j) {
        QPointF p = midpoint + QPointF(radius * qCos(start_angle + sidef * j*astep),
                                       radius * qSin(start_angle + sidef * j*astep));
        gl->Vertex3f(p.x(), p.y(), z);
      }

      gl->End(); // DiGLPainter::gl_POLYGON

      gl->PopAttrib();
    }

  } else if (decoration == "crosses") {

    int lineWidth = style.value("linewidth").toInt();
    int lineLength = lineWidth * 9;
    QList<QPointF> points_ = getDecorationLines(points, lineLength);
    qreal size = lineWidth * 3;

    gl->Begin(DiGLPainter::gl_LINES);

    for (int i = offset; i < points_.size() + di; i += 2) {

      QLineF line(points_.at(i), points_.at((i + 1) % points_.size()));
      QPointF midpoint = (line.p1() + line.p2())/2;
      QPointF tangent = QPointF(line.unitVector().dx(), line.unitVector().dy());
      QPointF normal = QPointF(line.normalVector().unitVector().dx(),
                               line.normalVector().unitVector().dy());

      QPointF p = midpoint + (size * normal) + (size * tangent);
      gl->Vertex3f(p.x(), p.y(), z);
      p = midpoint - (size * normal) - (size * tangent);
      gl->Vertex3f(p.x(), p.y(), z);
      p = midpoint - (size * normal) + (size * tangent);
      gl->Vertex3f(p.x(), p.y(), z);
      p = midpoint + (size * normal) - (size * tangent);
      gl->Vertex3f(p.x(), p.y(), z);
    }

    gl->End(); // DiGLPainter::gl_LINES

  } else if (decoration == "fishbone") {

    int lineWidth = style.value("linewidth").toInt();
    int lineLength = lineWidth * 9;
    QList<QPointF> points_ = getDecorationLines(points, lineLength);
    qreal size = lineWidth * 3;

    gl->Begin(DiGLPainter::gl_LINES);
    int j = 0;
    for (int i = offset; i < points_.size() + di; i += 2) {

      QLineF line(points_.at(i), points_.at((i + 1) % points_.size()));
      QPointF midpoint = (line.p1() + line.p2())/2;
      QPointF tangent = QPointF(line.unitVector().dx(), line.unitVector().dy());
      QPointF normal = QPointF(line.normalVector().unitVector().dx(),
                               line.normalVector().unitVector().dy());
      if (j%2 == 0) {
        QPointF p = midpoint;
        gl->Vertex3f(p.x(), p.y(), z);
        p = midpoint - (size * normal) - (size * tangent);
        gl->Vertex3f(p.x(), p.y(), z);
      } else {
        QPointF p = midpoint;
        gl->Vertex3f(p.x(), p.y(), z);
        p = midpoint + (size * normal) - (size * tangent);
        gl->Vertex3f(p.x(), p.y(), z);
      }
      j++;
    }

    gl->End(); // DiGLPainter::gl_LINES

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

    gl->PushAttrib(DiGLPainter::gl_POLYGON_BIT);
    gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);

    gl->Begin(DiGLPainter::gl_TRIANGLES);
    gl->Vertex3f(points_.last().x(), points_.last().y(), z);
    gl->Vertex3f(p1.x(), p1.y(), z);
    gl->Vertex3f(p2.x(), p2.y(), z);
    gl->End(); // DiGLPainter::gl_TRIANGLES

    gl->PopAttrib();

  } else if (decoration == "SIGWX") {

    int lineWidth = style.value("linewidth").toInt();
    int lineLength = lineWidth * 8;
    QList<QPointF> points_ = getDecorationLines(points, lineLength);
    int npoints = lineWidth * 12;

    gl->Begin(DiGLPainter::gl_LINE_STRIP);

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
        gl->Vertex3f(p.x(), p.y(), z);
      }
    }
    gl->End(); // DiGLPainter::gl_LINE_STRIP

  } else if (decoration == "jetstream") {

    // Draw triangles and "feathers" on the line to represent wind speed:
    // triangle=50 knots, long feather=10 knots, short feather=5 knots.
    int lineWidth = style.value("linewidth").toInt();
    int lineLength = lineWidth * 3;
    QList<QPointF> points_ = getDecorationLines(points, lineLength);
    qreal size = lineWidth * 6;

    int speed = item->property("met:info:speed", 0).toInt();

    // Determine how many points on the line we need.
    int n = (speed / 50)*2 + ((speed % 50) / 10) + ((speed % 10) / 5);

    // Place the decoration closer to the start of the line that the end.
    int i = qMax(0, (points_.size() - n)/4);

    gl->PushAttrib(DiGLPainter::gl_POLYGON_BIT);
    gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
    gl->Begin(DiGLPainter::gl_TRIANGLES);

    while (i < points_.size() && speed >= 50) {

      QLineF line(points_.at(i), points_.at((i + 2) % points_.size()));
        if (line.length() < lineLength*0.75) {
          // We cannot use this line segment. Try the next one.
          i++;
          continue;
        }

      QPointF midpoint = (line.p1() + line.p2())/2;
      QPointF normal = QPointF(line.normalVector().unitVector().dx(),
                               line.normalVector().unitVector().dy());
      QPointF p = midpoint + (sidef * size * normal);

      gl->Vertex3f(p.x(), p.y(), z);
      gl->Vertex3f(line.p1().x(), line.p1().y(), z);
      gl->Vertex3f(line.p2().x(), line.p2().y(), z);
      speed -= 50;
      i += 2;
    }

    gl->End(); // DiGLPainter::gl_TRIANGLES

    gl->Begin(DiGLPainter::gl_LINES);

    while (i < points_.size() && speed >= 5) {

      QLineF line(points_.at(i), points_.at((i + 1) % points_.size()));
      QPointF normal = QPointF(line.normalVector().unitVector().dx(),
                               line.normalVector().unitVector().dy());
      QPointF p = (sidef * size * normal);
      QPointF beginp = line.p2();
      QPointF endp = line.p1() + p;

      if (speed >= 10) {
        gl->Vertex3f(beginp.x(), beginp.y(), z);
        gl->Vertex3f(endp.x(), endp.y(), z);
        speed -= 10;
        i += 1;

      } else if (speed >= 5) {
        gl->Vertex3f(beginp.x(), beginp.y(), z);
        gl->Vertex3f(0.5*(beginp.x() + endp.x()), 0.5*(beginp.y() + endp.y()), z);
        speed -= 5;
        i += 1;
      }
    }

    gl->End(); // DiGLPainter::gl_TRIANGLES
    gl->PopAttrib();
  }
}

void DrawingStyleManager::fillLoop(DiGLPainter* gl, const DrawingItemBase *item, const QList<QPointF> &points) const
{
  QVariantMap style = getStyle(item);
  QVariant value = style.value("closed");
  bool closed = value.isValid() ? value.toBool() : true;

  QList<QPointF> points_;
  if (style.value("linesmooth").toBool())
    points_ = interpolateToPoints(points, closed);
  else
    points_ = points;

  // draw the interior
  gl->BlendFunc( DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA );
  gl->Enable( DiGLPainter::gl_BLEND );
  if (points_.size() >= 3)
    gl->drawPolygon(QPolygonF::fromList(points_));
  else
    gl->drawPolyline(QPolygonF::fromList(points_));
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
    float prev_l = previous_v.length();
    float next_l = next_v.length();
    float l = qMin(prev_l, next_l);

    // Adjust the previous and next points to lie the same distance away from
    // the current point.
    QVector2D new_previous = p + previous_v.normalized() * l;
    QVector2D new_next = p + next_v.normalized() * l;

    // The line between the adjusted previous and next points is used as a
    // tangent or gradient that we extend from the current point to position
    // control points.
    QVector2D gradient = (new_next - new_previous).normalized();
    prev_l = qMin((float)QVector2D::dotProduct(p - previous, gradient), l)/3.0;
    next_l = qMin((float)QVector2D::dotProduct(next - p, gradient), l)/3.0;

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

void DrawingStyleManager::drawText(DiGLPainter* gl, const DrawingItemBase *item_) const
{
  const DrawingItem_Text::Text *item = dynamic_cast<const DrawingItem_Text::Text *>(item_);
  if (!item) return;

  QVariantMap style = getStyle(item);

  const QColor textColour = style.value("textcolour").value<QColor>();
  bool alphaOk;
  const int alpha = style.value("textalpha").toInt(&alphaOk);
  if (textColour.isValid())
    gl->Color4ub(textColour.red(), textColour.green(), textColour.blue(), alphaOk ? alpha : 255);
  else
    gl->Color4ub(0, 0, 0, 255);

  setFont(item);
  const float scale = 1/PlotModule::instance()->getStaticPlot()->getPhysToMapScaleX();

  const float x0 = item->getPoints().at(0).x();

  float y = item->getPoints().at(0).y() - item->margin();
  QStringList lines = item->text();

  for (int i = 0; i < lines.size(); ++i) {
    QString text = lines.at(i);
    const QRectF rect = item->getStringRect(text);
    qreal height = item->fontSize();
    if (i == 0)
      height = qMax(rect.height(), height);

    diutil::GlMatrixPushPop pushpop(gl);
    gl->Translatef(x0 + item->margin() - rect.x(), y - height, 0);
    gl->Scalef(scale, scale, 1.0);
    gl->drawText(text.toStdString(), 0, 0, 0);
    y -= height * (1.0 + item->spacing());
  }
}

void DrawingStyleManager::setFont(const DrawingItemBase *item) const
{
  QVariantMap style = getStyle(item);

  // Fill in the default font settings from the plot options object. These
  // will be overridden if equivalent properties are found.
  const QString fontName = style.value("fontname", QString::fromStdString(PlotOptions::defaultFontName())).toString();
  const QString fontFace = style.value("fontface", QString::fromStdString(PlotOptions::defaultFontFace())).toString();
  const float fontSize = style.value("fontsize", PlotOptions::defaultFontSize()).toFloat();

  if (mCanvas)
    mCanvas->setFont(fontName.toStdString(), fontFace.toStdString(), fontSize);
}

void DrawingStyleManager::drawSymbol(DiGLPainter* gl, const DrawingItemBase *item) const
{
  DrawingManager *dm = DrawingManager::instance();

  const QString name = item->properties().value("style:type", "Default").toString();

  if (!dm->symbolNames().contains(name))
    return;

  const QSize defaultSize = dm->getSymbolSize(name);
  const float aspect = defaultSize.height() / float(defaultSize.width());
  const int size = item->properties().value("size", DEFAULT_SYMBOL_SIZE).toInt();
  const float width = size;
  const float height = aspect * size;
  const QImage image = dm->getCachedImage(name, width, height);

  const QVariantMap style = getStyle(item);

  gl->PushAttrib(DiGLPainter::gl_PIXEL_MODE_BIT);

  const QColor colour = style.value("symbolcolour").value<QColor>();
  if (colour.isValid()) {
    gl->PixelTransferf(DiGLPainter::gl_RED_BIAS, colour.redF());
    gl->PixelTransferf(DiGLPainter::gl_GREEN_BIAS, colour.greenF());
    gl->PixelTransferf(DiGLPainter::gl_BLUE_BIAS, colour.blueF());
  }

  bool alphaOk;
  const int alpha = style.value("symbolalpha").toInt(&alphaOk);
  if (alphaOk)
    gl->PixelTransferf(DiGLPainter::gl_ALPHA_SCALE, alpha / 255.0f);

  gl->Enable(DiGLPainter::gl_BLEND);
  gl->RasterPos2f(
        item->getPoints().at(0).x() - size / 2,
        item->getPoints().at(0).y() - aspect * size / 2);
  gl->DrawPixels(image.width(), image.height(), DiGLPainter::gl_RGBA, DiGLPainter::gl_UNSIGNED_BYTE, image.bits());

  gl->PopAttrib();
}

/**
 * Returns an image containing a preview of the item with the given \a category, \a name and \a value.
 */
QImage DrawingStyleManager::toImage(const DrawingItemBase::Category &category, const QString &name, const QString &value) const
{
  // Ideally, this would get the items to draw themselves.
  DrawingManager *dm = DrawingManager::instance();

  switch (category) {
  case DrawingItemBase::Text:
  {
    QFontMetrics fm(QApplication::font());
    QString v = value;
    if (v.isEmpty()) v = "Text";
    QImage image(fm.width(v) + (2 * DrawingItem_Text::Text::defaultMargin()),
                 fm.height() + (2 * DrawingItem_Text::Text::defaultMargin()), QImage::Format_ARGB32);
    image.fill(qRgba(0, 0, 0, 0));
    QPainter painter(&image);
    painter.setFont(QApplication::font());
    painter.drawText(QRect(QPoint(0, 0), image.size()), Qt::AlignCenter, v);
    painter.end();
    return image;
  }
  case DrawingItemBase::Symbol:
  {
    QSize size = dm->getSymbolSize(name);
    return dm->getSymbolImage(name, size.width(), size.height());
  }
  case DrawingItemBase::PolyLine:
    return QImage();
  case DrawingItemBase::Composite:
  default:
    ;
  }

  QVariantMap thisStyle = getStyle(category, name);
  QStringList objects = thisStyle.value("objects").toStringList();
  QStringList values = thisStyle.value("values").toStringList();
  QStringList styles = thisStyle.value("styles").toStringList();
  QString layout = thisStyle.value("layout").toString();

  QList<QImage> images;
  QList<QPointF> positions;
  QPointF pos(0, 0);
  QSizeF maxSize;

  for (int i = 0; i < objects.size(); ++i) {

    QString style = styles.at(i);
    QImage image;
    if (objects.at(i) == "text") {
      image = toImage(DrawingItemBase::Text, style, values.at(i));
      images.append(image);
    } else if (objects.at(i) == "symbol") {
      image = toImage(DrawingItemBase::Symbol, style);
      images.append(image);
    } else if (objects.at(i) == "line") {
      image = toImage(DrawingItemBase::PolyLine, style);
      images.append(image);
    } else if (objects.at(i) == "composite") {
      image = toImage(DrawingItemBase::Composite, style);
      images.append(image);
    } else
      continue;

    positions.append(QPointF(pos));

    // Record the positions of elements and keep track of the maximum size.
    // A null image is a placeholder for a line.
    if (layout == "diagonal") {
      if (image.isNull()) {
        pos += QPointF(2, 2);
        maxSize += QSizeF(2, 2);
      } else {
        pos += QPointF(image.width(), image.height());
        maxSize += image.size();
      }
    } else if (layout == "vertical") {
      if (image.isNull()) {
        pos += QPointF(0, 2);
        maxSize += QSizeF(0, 2);
      } else {
        pos += QPointF(0, image.height());
        maxSize = maxSize.expandedTo(QSizeF(image.width(), pos.y()));
      }
    } else {
      // Horizontal by default
      if (image.isNull()) {
        pos += QPointF(2, 0);
        maxSize += QSizeF(2, 0);
      } else {
        pos += QPointF(image.width(), 0);
        maxSize = maxSize.expandedTo(QSizeF(pos.x(), image.height()));
      }
    }
  }

  // If the image has no size then return a null image. This usually means
  // that a composite item is incorrectly defined.
  if (maxSize.isEmpty())
    return QImage();

  // Obtain a polygon for the border decoration and use its bounding rectangle
  // to determine the size of the image.
  QRectF rect = QRectF(QPointF(0, 0), maxSize);
  QPolygonF poly = QPolygonF::fromList(linesForBBox(rect, thisStyle.value("cornersegments").toInt(),
                                                          thisStyle.value("cornerradius").toFloat(),
                                                          thisStyle.value("border").toString()));

  QImage thisImage(poly.boundingRect().size().toSize(), QImage::Format_ARGB32);
  thisImage.fill(qRgba(0, 0, 0, 0));
  QPainter painter;
  painter.begin(&thisImage);
  painter.setRenderHint(QPainter::Antialiasing);
  // Displace the contents to ensure that everything lies within the image.
  painter.translate(-poly.boundingRect().topLeft());

  for (int i = 0; i < images.size(); ++i) {
    QImage image = images.at(i);
    if (layout == "vertical") {
      if (image.isNull()) {
        pos = positions.at(i);
        painter.drawLine(pos, pos + QPointF(maxSize.toSize().width(), 0));
      } else
        painter.drawImage(positions.at(i) + QPointF((maxSize.width() - image.width())/2, 0), image);

    } else if (layout == "diagonal") {
      if (image.isNull()) {
        QImage previous;
        if (i > 0)
          previous = images.at(i - 1);

        pos = positions.at(i);
        painter.drawLine(pos + QPointF(-previous.width()/2 - 1, previous.height()/2 - 1),
                         pos + QPointF(previous.width()/2 - 1, -previous.height()/2 - 1));
      } else
        painter.drawImage(positions.at(i), image);

    } else {
      if (image.isNull()) {
        pos = positions.at(i);
        painter.drawLine(pos, pos + QPointF(0, maxSize.toSize().height()));
      } else
        painter.drawImage(positions.at(i) + QPointF(0, (maxSize.height() - image.height())/2), image);
    }
  }

  QColor lineColour = thisStyle.value("linecolour").value<QColor>();
  lineColour.setAlpha(thisStyle.value("linealpha").toInt());
  painter.setPen(lineColour);

  // The polygon itself is defined in a vertically inverted coordinate system.
  painter.translate(0, poly.boundingRect().top());
  painter.scale(1, -1);
  painter.translate(0, -poly.boundingRect().top() - thisImage.height());
  painter.drawPolygon(poly);

  painter.end();

  return thisImage;
}
