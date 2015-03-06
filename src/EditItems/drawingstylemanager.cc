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

#include "diPlot.h"
#include "diPlotModule.h"
#include "diDrawingManager.h"
#include "EditItems/drawingstylemanager.h"
#include "EditItems/drawingitembase.h"
#include "EditItems/drawingsymbol.h"
#include "EditItems/drawingtext.h"
#include <QApplication>
#include <QComboBox>
#include <QPainter>
#include <QVector2D>
#include <qmath.h>

// Use the predefined fill patterns already defined for the existing editing and objects modes.
#include "polyStipMasks.h"
#include <diTesselation.h>

static QColor parseColour(const QString &text)
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

static ushort parseLinePattern(const QString &s)
{
  bool ok = false;
  const ushort pattern = s.toUShort(&ok);
  if (ok)
    return pattern;
  if (s == "dashed")
    return 0x0f0f; // ### for now
  return 0xffff;
}

QString DSP_linecolour::name() { return "linecolour"; }
DrawingStyleManager::StyleCategory DSP_linecolour::styleCategory() const { return DrawingStyleManager::Line; }
DrawingStyleManager::LockCategory DSP_linecolour::lockCategory() const { return DrawingStyleManager::LockColour; }
QVariant DSP_linecolour::parse(const QHash<QString, QString> &def) const { return parseColour(lineColour(def)); }

QString DSP_linealpha::name() { return "linealpha"; }
DrawingStyleManager::StyleCategory DSP_linealpha::styleCategory() const { return DrawingStyleManager::Line; }
DrawingStyleManager::LockCategory DSP_linealpha::lockCategory() const { return DrawingStyleManager::LockAlpha; }
QVariant DSP_linealpha::parse(const QHash<QString, QString> &def) const { return def.value(name(), "255").toInt(); }

QString DSP_linewidth::name() { return "linewidth"; }
DrawingStyleManager::StyleCategory DSP_linewidth::styleCategory() const { return DrawingStyleManager::Line; }
QVariant DSP_linewidth::parse(const QHash<QString, QString> &def) const { return def.value(name(), "1.0").toFloat(); }

QString DSP_linepattern::name() { return "linepattern"; }
DrawingStyleManager::StyleCategory DSP_linepattern::styleCategory() const { return DrawingStyleManager::Line; }
QVariant DSP_linepattern::parse(const QHash<QString, QString> &def) const { return parseLinePattern(linePattern(def)); }

QString DSP_linesmooth::name() { return "linesmooth"; }
DrawingStyleManager::StyleCategory DSP_linesmooth::styleCategory() const { return DrawingStyleManager::General; }
QVariant DSP_linesmooth::parse(const QHash<QString, QString> &def) const { return def.value(name(), "false") == "true"; }

QString DSP_fillcolour::name() { return "fillcolour"; }
DrawingStyleManager::StyleCategory DSP_fillcolour::styleCategory() const { return DrawingStyleManager::Area; }
DrawingStyleManager::LockCategory DSP_fillcolour::lockCategory() const { return DrawingStyleManager::LockColour; }
QVariant DSP_fillcolour::parse(const QHash<QString, QString> &def) const { return parseColour(fillColour(def)); }

QString DSP_fillalpha::name() { return "fillalpha"; }
DrawingStyleManager::StyleCategory DSP_fillalpha::styleCategory() const { return DrawingStyleManager::Area; }
DrawingStyleManager::LockCategory DSP_fillalpha::lockCategory() const { return DrawingStyleManager::LockAlpha; }
QVariant DSP_fillalpha::parse(const QHash<QString, QString> &def) const { return def.value(name(), "50").toInt(); }

QString DSP_fillpattern::name() { return "fillpattern"; }
DrawingStyleManager::StyleCategory DSP_fillpattern::styleCategory() const { return DrawingStyleManager::Area; }
QVariant DSP_fillpattern::parse(const QHash<QString, QString> &def) const { return def.value(name()); }

QString DSP_closed::name() { return "closed"; }
DrawingStyleManager::StyleCategory DSP_closed::styleCategory() const { return DrawingStyleManager::General; }
QVariant DSP_closed::parse(const QHash<QString, QString> &def) const { return def.value(name(), "true") == "true"; }

QString DSP_reversed::name() { return "reversed"; }
DrawingStyleManager::StyleCategory DSP_reversed::styleCategory() const { return DrawingStyleManager::Decoration; }
QVariant DSP_reversed::parse(const QHash<QString, QString> &def) const { return def.value(name(), "false") == "true"; }

QString DSP_decoration1::name() { return "decoration1"; }
DrawingStyleManager::StyleCategory DSP_decoration1::styleCategory() const { return DrawingStyleManager::Decoration; }
QVariant DSP_decoration1::parse(const QHash<QString, QString> &def) const { return def.value(name()).split(","); }

QString DSP_decoration1_colour::name() { return "decoration1.colour"; }
DrawingStyleManager::StyleCategory DSP_decoration1_colour::styleCategory() const { return DrawingStyleManager::Decoration; }
DrawingStyleManager::LockCategory DSP_decoration1_colour::lockCategory() const { return DrawingStyleManager::LockColour; }
QVariant DSP_decoration1_colour::parse(const QHash<QString, QString> &def) const { return parseColour(def.value(name(), lineColour(def))); }

QString DSP_decoration1_alpha::name() { return "decoration1.alpha"; }
DrawingStyleManager::StyleCategory DSP_decoration1_alpha::styleCategory() const { return DrawingStyleManager::Decoration; }
DrawingStyleManager::LockCategory DSP_decoration1_alpha::lockCategory() const { return DrawingStyleManager::LockAlpha; }
QVariant DSP_decoration1_alpha::parse(const QHash<QString, QString> &def) const { return def.value(name(), "255").toInt(); }

QString DSP_decoration1_offset::name() { return "decoration1.offset"; }
DrawingStyleManager::StyleCategory DSP_decoration1_offset::styleCategory() const { return DrawingStyleManager::Decoration; }
QVariant DSP_decoration1_offset::parse(const QHash<QString, QString> &def) const { return def.value(name(), "0").toInt(); }

QString DSP_decoration2::name() { return "decoration2"; }
DrawingStyleManager::StyleCategory DSP_decoration2::styleCategory() const { return DrawingStyleManager::Decoration; }
QVariant DSP_decoration2::parse(const QHash<QString, QString> &def) const { return def.value(name()).split(","); }

QString DSP_decoration2_colour::name() { return "decoration2.colour"; }
DrawingStyleManager::StyleCategory DSP_decoration2_colour::styleCategory() const { return DrawingStyleManager::Decoration; }
DrawingStyleManager::LockCategory DSP_decoration2_colour::lockCategory() const { return DrawingStyleManager::LockColour; }
QVariant DSP_decoration2_colour::parse(const QHash<QString, QString> &def) const { return parseColour(def.value(name(), lineColour(def))); }

QString DSP_decoration2_alpha::name() { return "decoration2.alpha"; }
DrawingStyleManager::StyleCategory DSP_decoration2_alpha::styleCategory() const { return DrawingStyleManager::Decoration; }
DrawingStyleManager::LockCategory DSP_decoration2_alpha::lockCategory() const { return DrawingStyleManager::LockAlpha; }
QVariant DSP_decoration2_alpha::parse(const QHash<QString, QString> &def) const { return def.value(name(), "255").toInt(); }

QString DSP_decoration2_offset::name() { return "decoration2.offset"; }
DrawingStyleManager::StyleCategory DSP_decoration2_offset::styleCategory() const { return DrawingStyleManager::Decoration; }
QVariant DSP_decoration2_offset::parse(const QHash<QString, QString> &def) const { return def.value(name(), "0").toInt(); }

QString DSP_symbolcolour::name() { return "symbolcolour"; }
DrawingStyleManager::StyleCategory DSP_symbolcolour::styleCategory() const { return DrawingStyleManager::Symbol; }
QVariant DSP_symbolcolour::parse(const QHash<QString, QString> &def) const { return parseColour(symbolColour(def)); }

QString DSP_symbolalpha::name() { return "symbolalpha"; }
DrawingStyleManager::StyleCategory DSP_symbolalpha::styleCategory() const { return DrawingStyleManager::Symbol; }
QVariant DSP_symbolalpha::parse(const QHash<QString, QString> &def) const { return def.value(name(), "255").toInt(); }

QString DSP_textcolour::name() { return "textcolour"; }
DrawingStyleManager::StyleCategory DSP_textcolour::styleCategory() const { return DrawingStyleManager::General; }
DrawingStyleManager::LockCategory DSP_textcolour::lockCategory() const { return DrawingStyleManager::LockColour; }
QVariant DSP_textcolour::parse(const QHash<QString, QString> &def) const { return parseColour(textColour(def)); }

QString DSP_textalpha::name() { return "textalpha"; }
DrawingStyleManager::StyleCategory DSP_textalpha::styleCategory() const { return DrawingStyleManager::General; }
DrawingStyleManager::LockCategory DSP_textalpha::lockCategory() const { return DrawingStyleManager::LockAlpha; }
QVariant DSP_textalpha::parse(const QHash<QString, QString> &def) const { return def.value(name(), "255").toInt(); }

QString DSP_fontname::name() { return "fontname"; }
DrawingStyleManager::StyleCategory DSP_fontname::styleCategory() const { return DrawingStyleManager::General; }
QVariant DSP_fontname::parse(const QHash<QString, QString> &def) const { return def.value(name(), QString::fromStdString(PlotOptions::defaultFontName())); }

QString DSP_fontface::name() { return "fontface"; }
DrawingStyleManager::StyleCategory DSP_fontface::styleCategory() const { return DrawingStyleManager::General; }
QVariant DSP_fontface::parse(const QHash<QString, QString> &def) const { return def.value(name(), QString::fromStdString(PlotOptions::defaultFontFace())); }

QString DSP_fontsize::name() { return "fontsize"; }
DrawingStyleManager::StyleCategory DSP_fontsize::styleCategory() const { return DrawingStyleManager::General; }
QVariant DSP_fontsize::parse(const QHash<QString, QString> &def) const { return def.value(name(), QString::number(PlotOptions::defaultFontSize())); }

QString DrawingStyleProperty::lineColour(const QHash<QString, QString> &def) { return def.value(DSP_linecolour::name(), "black"); }
QString DrawingStyleProperty::linePattern(const QHash<QString, QString> &def) { return def.value(DSP_linepattern::name(), "solid"); }
QString DrawingStyleProperty::fillColour(const QHash<QString, QString> &def) { return def.value(DSP_fillcolour::name(), "128:128:128"); }
QString DrawingStyleProperty::symbolColour(const QHash<QString, QString> &def) { return def.value(DSP_symbolcolour::name(), "black"); }
QString DrawingStyleProperty::textColour(const QHash<QString, QString> &def) { return def.value(DSP_textcolour::name(), "black"); }

QString DSP_objects::name() { return "objects"; }
DrawingStyleManager::StyleCategory DSP_objects::styleCategory() const { return DrawingStyleManager::General; }
QVariant DSP_objects::parse(const QHash<QString, QString> &def) const { return def.value(name()).split(":"); }

QString DSP_values::name() { return "values"; }
DrawingStyleManager::StyleCategory DSP_values::styleCategory() const { return DrawingStyleManager::General; }
QVariant DSP_values::parse(const QHash<QString, QString> &def) const
{
  return def.value(name()).split(":");
}

QString DSP_styles::name() { return "styles"; }
DrawingStyleManager::StyleCategory DSP_styles::styleCategory() const { return DrawingStyleManager::General; }
QVariant DSP_styles::parse(const QHash<QString, QString> &def) const
{
  return def.value(name()).split(":");
}

QString DSP_layout::name() { return "layout"; }
DrawingStyleManager::StyleCategory DSP_layout::styleCategory() const { return DrawingStyleManager::General; }
QVariant DSP_layout::parse(const QHash<QString, QString> &def) const
{
  return def.value(name(), "horizontal");
}

QString DSP_hide::name() { return "hide"; }
DrawingStyleManager::StyleCategory DSP_hide::styleCategory() const { return DrawingStyleManager::General; }
QVariant DSP_hide::parse(const QHash<QString, QString> &def) const
{
  return def.value(name(), "false") == "true";
}

DrawingStyleManager *DrawingStyleManager::self_ = 0;

DrawingStyleManager::DrawingStyleManager()
{
  self_ = this;

  // Define the supported polyline style properties.
  properties_[DrawingItemBase::PolyLine].insert(DSP_linecolour::name(), new DSP_linecolour);
  properties_[DrawingItemBase::PolyLine].insert(DSP_linealpha::name(), new DSP_linealpha);
  properties_[DrawingItemBase::PolyLine].insert(DSP_linewidth::name(), new DSP_linewidth);
  properties_[DrawingItemBase::PolyLine].insert(DSP_linepattern::name(), new DSP_linepattern);
  properties_[DrawingItemBase::PolyLine].insert(DSP_linesmooth::name(), new DSP_linesmooth);
  properties_[DrawingItemBase::PolyLine].insert(DSP_fillcolour::name(), new DSP_fillcolour);
  properties_[DrawingItemBase::PolyLine].insert(DSP_fillalpha::name(), new DSP_fillalpha);
  properties_[DrawingItemBase::PolyLine].insert(DSP_fillpattern::name(), new DSP_fillpattern);
  properties_[DrawingItemBase::PolyLine].insert(DSP_closed::name(), new DSP_closed);
  properties_[DrawingItemBase::PolyLine].insert(DSP_reversed::name(), new DSP_reversed);
  properties_[DrawingItemBase::PolyLine].insert(DSP_decoration1::name(), new DSP_decoration1);
  properties_[DrawingItemBase::PolyLine].insert(DSP_decoration1_colour::name(), new DSP_decoration1_colour);
  properties_[DrawingItemBase::PolyLine].insert(DSP_decoration1_alpha::name(), new DSP_decoration1_alpha);
  properties_[DrawingItemBase::PolyLine].insert(DSP_decoration1_offset::name(), new DSP_decoration1_offset);
  properties_[DrawingItemBase::PolyLine].insert(DSP_decoration2::name(), new DSP_decoration2);
  properties_[DrawingItemBase::PolyLine].insert(DSP_decoration2_colour::name(), new DSP_decoration2_colour);
  properties_[DrawingItemBase::PolyLine].insert(DSP_decoration2_alpha::name(), new DSP_decoration2_alpha);
  properties_[DrawingItemBase::PolyLine].insert(DSP_decoration2_offset::name(), new DSP_decoration2_offset);

  // Define the supported symbol style properties.
  properties_[DrawingItemBase::Symbol].insert(DSP_symbolcolour::name(), new DSP_symbolcolour);
  properties_[DrawingItemBase::Symbol].insert(DSP_symbolalpha::name(), new DSP_symbolalpha);

  // Define the supported text style properties.
  properties_[DrawingItemBase::Text].insert(DSP_linecolour::name(), new DSP_linecolour);
  properties_[DrawingItemBase::Text].insert(DSP_linealpha::name(), new DSP_linealpha);
  properties_[DrawingItemBase::Text].insert(DSP_linewidth::name(), new DSP_linewidth);
  properties_[DrawingItemBase::Text].insert(DSP_linepattern::name(), new DSP_linepattern);
  properties_[DrawingItemBase::Text].insert(DSP_fillcolour::name(), new DSP_fillcolour);
  properties_[DrawingItemBase::Text].insert(DSP_fillalpha::name(), new DSP_fillalpha);
  properties_[DrawingItemBase::Text].insert(DSP_textcolour::name(), new DSP_textcolour);
  properties_[DrawingItemBase::Text].insert(DSP_textalpha::name(), new DSP_textalpha);
  properties_[DrawingItemBase::Text].insert(DSP_fontname::name(), new DSP_fontname);
  properties_[DrawingItemBase::Text].insert(DSP_fontface::name(), new DSP_fontface);
  properties_[DrawingItemBase::Text].insert(DSP_fontsize::name(), new DSP_fontsize);

  // Define the supported composite style properties.
  properties_[DrawingItemBase::Composite].insert(DSP_objects::name(), new DSP_objects);
  properties_[DrawingItemBase::Composite].insert(DSP_values::name(), new DSP_values);
  properties_[DrawingItemBase::Composite].insert(DSP_styles::name(), new DSP_styles);
  properties_[DrawingItemBase::Composite].insert(DSP_layout::name(), new DSP_layout);
  properties_[DrawingItemBase::Composite].insert(DSP_linecolour::name(), new DSP_linecolour);
  properties_[DrawingItemBase::Composite].insert(DSP_linealpha::name(), new DSP_linealpha);
  properties_[DrawingItemBase::Composite].insert(DSP_fillcolour::name(), new DSP_fillcolour);
  properties_[DrawingItemBase::Composite].insert(DSP_fillalpha::name(), new DSP_fillalpha);
  properties_[DrawingItemBase::Composite].insert(DSP_hide::name(), new DSP_hide);
  properties_[DrawingItemBase::Composite].insert(DSP_closed::name(), new DSP_closed);
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
  foreach (QString propName, properties_[category].keys())
    style[propName] = properties_[category].value(propName)->parse(definition);
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

void DrawingStyleManager::beginLine(DrawingItemBase *item)
{
  glPushAttrib(GL_LINE_BIT);

  const QVariantMap style = getStyle(item);

  const QString lpString = style.value(DSP_linepattern::name()).toString();
  bool ok = false;
  const ushort linePattern = lpString.toUShort(&ok);
  if (ok) {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(2, linePattern);
  }

  float lineWidth = style.value(DSP_linewidth::name()).toFloat();
  glLineWidth(lineWidth);

  QColor borderColour = style.value(DSP_linecolour::name()).value<QColor>();
  bool alphaOk;
  const int alpha = style.value(DSP_linealpha::name()).toInt(&alphaOk);
  if (borderColour.isValid())
    glColor4ub(borderColour.red(), borderColour.green(), borderColour.blue(), alphaOk ? alpha : 255);
}

void DrawingStyleManager::endLine(DrawingItemBase *item)
{
  Q_UNUSED(item)

  glPopAttrib();
}

void DrawingStyleManager::beginFill(DrawingItemBase *item)
{
  glPushAttrib(GL_POLYGON_BIT);

  QVariantMap style = getStyle(item);

  QColor fillColour = style.value(DSP_fillcolour::name()).value<QColor>();
  bool alphaOk;
  const int alpha = style.value(DSP_fillalpha::name()).toInt(&alphaOk);
  glColor4ub(fillColour.red(), fillColour.green(), fillColour.blue(), alphaOk ? alpha : 255);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  QString fillPattern = style.value(DSP_fillpattern::name()).toString();

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

  glPopAttrib();
}

DrawingStyleManager::StyleCategory DrawingStyleManager::styleCategory(const DrawingItemBase::Category &itemCategory, const QString &name) const
{
  if (properties_.contains(itemCategory) && properties_.value(itemCategory).contains(name))
    return properties_.value(itemCategory).value(name)->styleCategory();
  return Invalid;
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

DrawingStyleManager::LockCategory DrawingStyleManager::lockCategory(const DrawingItemBase::Category &itemCategory, const QString &name) const
{
  if (properties_.contains(itemCategory) && properties_.value(itemCategory).contains(name))
    return properties_.value(itemCategory).value(name)->lockCategory();
  return LockNone;
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

QVariantMap DrawingStyleManager::getStyle(const DrawingItemBase *item) const
{
  QHash<QString, QString> styleProperties;
  // Build a hash containing only the item properties that are style-related.
  foreach (QString key, item->propertiesRef().keys()) {
    if (key.startsWith("style:"))
      styleProperties[key.mid(6)] = item->propertiesRef().value(key).toString();
  }

  // If the style is customised then parse the contents of the hash; otherwise
  // just return the style that corresponds to the style name.
  if ((item->category() == DrawingItemBase::PolyLine) ||
      (item->category() == DrawingItemBase::Symbol) ||
      (item->category() == DrawingItemBase::Text))
    return parse(item->category(), styleProperties);
  else {
    const QString styleName = item->property("style:type").toString();
    return styles_[item->category()].value(styleName);
  }
}

QStringList DrawingStyleManager::getComplexTextList() const
{
  return complexTextList_;
}

void DrawingStyleManager::highlightPolyLine(const DrawingItemBase *item, const QList<QPointF> &points, int lineWidth, const QColor &col, bool forceClosed) const
{
  QVariantMap style = getStyle(item);
  const bool closed = forceClosed || style.value(DSP_closed::name()).toBool();

  const int z = 0;
  const qreal lw_2 = lineWidth / 2.0;

  QList<QPointF> points_;
  if (style.value(DSP_linesmooth::name()).toBool())
    points_ = interpolateToPoints(points, closed);
  else
    points_ = points;

  glColor4ub(col.red(), col.green(), col.blue(), col.alpha());

  glPushAttrib(GL_POLYGON_BIT);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  for (int i = 1; i < (points_.size() + (closed ? 1 : 0)); ++i) {
    const QPointF p1 = points_.at(i % points_.size());
    const QPointF p0 = points_.at(i - 1);
    const QVector2D v(p1.x() - p0.x(), p1.y() - p0.y());
    const QVector2D u = QVector2D(-v.y(), v.x()).normalized();

    glBegin(GL_POLYGON);
    glVertex3f(p0.x() + lw_2 * u.x(), p0.y() + lw_2 * u.y(), z);
    glVertex3f(p1.x() + lw_2 * u.x(), p1.y() + lw_2 * u.y(), z);
    glVertex3f(p1.x() - lw_2 * u.x(), p1.y() - lw_2 * u.y(), z);
    glVertex3f(p0.x() - lw_2 * u.x(), p0.y() - lw_2 * u.y(), z);
    glEnd(); // GL_POLYGON
  }

  glPopAttrib();
}

void DrawingStyleManager::drawLines(const DrawingItemBase *item, const QList<QPointF> &points, int z, bool forceClosed) const
{
  QVariantMap style = getStyle(item);
  bool closed = forceClosed || style.value(DSP_closed::name()).toBool();

  if (closed)
    glBegin(GL_LINE_LOOP);
  else
    glBegin(GL_LINE_STRIP);

  QList<QPointF> points_;

  // For smooth lines, we try to fit a smooth curve through the points defined
  // for the existing lines. The list of points obtained contains enough
  // points to show a smooth line.

  if (style.value(DSP_linesmooth::name()).toBool())
    points_ = interpolateToPoints(points, closed);
  else
    points_ = points;

  {
    bool alphaOk;
    const int alpha = style.value(DSP_linealpha::name()).toInt(&alphaOk);
    if ((!alphaOk) || (alpha >= 0)) {
      foreach (QPointF p, points_)
        glVertex3i(p.x(), p.y(), z);
    }
  }

  glEnd(); // GL_LINE_LOOP or GL_LINE_STRIP

  const bool reversed = !style.value(DSP_reversed::name()).toBool();

  if (style.value(DSP_decoration1::name()).isValid()) {
    QColor colour = style.value(DSP_decoration1_colour::name()).value<QColor>();
    bool alphaOk;
    const int alpha = style.value(DSP_decoration1_alpha::name()).toInt(&alphaOk);
    glColor4ub(colour.red(), colour.green(), colour.blue(), alphaOk ? alpha : colour.alpha());

    unsigned int offset = style.value(DSP_decoration1_offset::name()).toInt();
    foreach (QVariant v, style.value(DSP_decoration1::name()).toList()) {
      QString decor = v.toString();
      drawDecoration(style, decor, closed, reversed ? Outside : Inside, points_, z, offset);
      offset += 1;
    }
  }

  if (style.value(DSP_decoration2::name()).isValid()) {
    QColor colour = style.value(DSP_decoration2_colour::name()).value<QColor>();
    bool alphaOk;
    const int alpha = style.value(DSP_decoration2_alpha::name()).toInt(&alphaOk);
    glColor4ub(colour.red(), colour.green(), colour.blue(), alphaOk ? alpha : colour.alpha());

    unsigned int offset = style.value(DSP_decoration2_offset::name()).toInt();
    foreach (QVariant v, style.value(DSP_decoration2::name()).toList()) {
      QString decor = v.toString();
      drawDecoration(style, decor, closed, reversed ? Inside : Outside, points_, z, offset);
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
void DrawingStyleManager::drawDecoration(const QVariantMap &style, const QString &decoration, bool closed,
                                         const Side &side, const QList<QPointF> &points, int z,
                                         unsigned int offset) const
{
  int di = closed ? 0 : -1;
  int sidef = (side == Inside) ? 1 : -1;

  if (decoration == "triangles") {

    int lineWidth = style.value(DSP_linewidth::name()).toInt();
    int lineLength = lineWidth * 9;
    QList<QPointF> points_ = getDecorationLines(points, lineLength);
    qreal size = lineWidth * 5;

    glPushAttrib(GL_POLYGON_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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

    glPopAttrib();

  } else if (decoration == "arches") {

    int lineWidth = style.value(DSP_linewidth::name()).toInt();
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

      glPushAttrib(GL_POLYGON_BIT);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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

      glPopAttrib();
    }

  } else if (decoration == "crosses") {

    int lineWidth = style.value(DSP_linewidth::name()).toInt();
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
    int lineWidth = style.value(DSP_linewidth::name()).toInt();
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

    glPushAttrib(GL_POLYGON_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBegin(GL_TRIANGLES);
    glVertex3f(points_.last().x(), points_.last().y(), z);
    glVertex3f(p1.x(), p1.y(), z);
    glVertex3f(p2.x(), p2.y(), z);
    glEnd(); // GL_TRIANGLES

    glPopAttrib();

  } else if (decoration == "SIGWX") {

    int lineWidth = style.value(DSP_linewidth::name()).toInt();
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
  bool closed = style.value(DSP_closed::name()).toBool();

  QList<QPointF> points_;
  if (style.value(DSP_linesmooth::name()).toBool())
    points_ = interpolateToPoints(points, closed);
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

void DrawingStyleManager::drawText(const DrawingItemBase *item_) const
{
  const DrawingItem_Text::Text *item = dynamic_cast<const DrawingItem_Text::Text *>(item_);
  if (!item)
    qFatal("DrawingStyleManager::drawText(): not a text item");

  QVariantMap style = getStyle(item);

  const QColor textColour = style.value(DSP_textcolour::name()).value<QColor>();
  bool alphaOk;
  const int alpha = style.value(DSP_textalpha::name()).toInt(&alphaOk);
  if (textColour.isValid())
    glColor4ub(textColour.red(), textColour.green(), textColour.blue(), alphaOk ? alpha : 255);
  else
    glColor4ub(0, 0, 0, 255);

  setFont(item);
  const float scale = 1/PlotModule::instance()->getStaticPlot()->getPhysToMapScaleX();

  const float x = item->getPoints().at(0).x() + item->margin();
  float y = item->getPoints().at(0).y() - item->margin();

  foreach (QString text, item->text()) {
    const QSizeF size = item->getStringSize(text);
    glPushMatrix();
    glTranslatef(x, y - size.height(), 0);
    glScalef(scale, scale, 1.0);
    PlotModule::instance()->getStaticPlot()->getFontPack()->drawStr(text.toStdString().c_str(), 0, 0, 0);
    glPopMatrix();
    y -= size.height() * (1.0 + item->spacing());
  }
}

void DrawingStyleManager::setFont(const DrawingItemBase *item) const
{
  QVariantMap style = getStyle(item);

  // Fill in the default font settings from the plot options object. These
  // will be overridden if equivalent properties are found.
  const QString fontName = style.value(DSP_fontname::name(), QString::fromStdString(PlotOptions::defaultFontName())).toString();
  const QString fontFace = style.value(DSP_fontface::name(), QString::fromStdString(PlotOptions::defaultFontFace())).toString();
  const float fontSize = style.value(DSP_fontsize::name(), PlotOptions::defaultFontSize()).toFloat();

  PlotModule::instance()->getStaticPlot()->getFontPack()->set(fontName.toStdString(), fontFace.toStdString(), fontSize);
}

void DrawingStyleManager::drawSymbol(const DrawingItemBase *item) const
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

  glPushAttrib(GL_PIXEL_MODE_BIT);

  const QColor colour = style.value(DSP_symbolcolour::name()).value<QColor>();
  if (colour.isValid()) {
    glPixelTransferf(GL_RED_BIAS, colour.red() / 255.0);
    glPixelTransferf(GL_GREEN_BIAS, colour.green() / 255.0);
    glPixelTransferf(GL_BLUE_BIAS, colour.blue() / 255.0);
  }

  bool alphaOk;
  const int alpha = style.value(DSP_symbolalpha::name()).toInt(&alphaOk);
  if (alphaOk)
    glPixelTransferf(GL_ALPHA_SCALE, alpha / 255.0);

  glEnable(GL_BLEND);
  glRasterPos2f(
        item->getPoints().at(0).x() - size / 2,
        item->getPoints().at(0).y() - aspect * size / 2);
  glDrawPixels(image.width(), image.height(), GL_RGBA, GL_UNSIGNED_BYTE, image.bits());

  glPopAttrib();
}

QImage DrawingStyleManager::toImage(const DrawingItemBase::Category &category, const QString &name, const QString &value) const
{
  DrawingManager *dm = DrawingManager::instance();

  switch (category) {
  case DrawingItemBase::Text:
  {
    QFontMetrics fm(QApplication::font());
    QString v = value;
    if (v.isEmpty()) v = "Text";
    QImage image(fm.size(Qt::TextSingleLine, v), QImage::Format_ARGB32);
    image.fill(qRgba(255,255,255,255));
    QPainter painter(&image);
    painter.drawText(QRect(QPoint(0, 0), image.size()), Qt::AlignCenter, "Text");
    painter.end();
    return image;
  }
  case DrawingItemBase::Symbol:
    return dm->getSymbolImage(name, DEFAULT_SYMBOL_SIZE, DEFAULT_SYMBOL_SIZE);;
  case DrawingItemBase::PolyLine:
    return QImage();
  case DrawingItemBase::Composite:
  default:
    ;
  }

  QVariantMap thisStyle = getStyle(category, name);
  QStringList objects = thisStyle.value(DSP_objects::name()).toStringList();
  QStringList values = thisStyle.value(DSP_values::name()).toStringList();
  QStringList styles = thisStyle.value(DSP_styles::name()).toStringList();
  QString layout = thisStyle.value(DSP_layout::name()).toString();

  QList<QImage> images;
  QList<float> positions;
  float pos = 0.0;
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

    positions.append(pos);

    // Record the positions of elements and keep track of the maximum size.
    // A null image is a placeholder for a line.
    if (image.isNull())
      pos += 2;
    else if (layout == "horizontal") {
      pos += image.width();
      maxSize = maxSize.expandedTo(QSize(pos, image.height()));
    } else {
      pos += image.height();
      maxSize = maxSize.expandedTo(QSize(image.width(), pos));
    }
  }

  // If the image has no size then return a null image. This usually means
  // that a composite item is incorrectly defined.
  if (maxSize.isEmpty())
    return QImage();

  QImage thisImage(maxSize.toSize(), QImage::Format_ARGB32);
  thisImage.fill(qRgba(255,255,255,255));
  QPainter painter;
  painter.begin(&thisImage);

  for (int i = 0; i < images.size(); ++i) {
    QImage image = images.at(i);
    if (layout == "horizontal") {
      if (image.isNull()) {
        image = QImage(1, maxSize.toSize().height(), QImage::Format_ARGB32);
        image.fill(qRgba(0,0,0,255));
      }
      painter.drawImage(positions.at(i), (maxSize.height() - image.height())/2, image);
    } else {
      if (image.isNull()) {
        image = QImage(maxSize.toSize().width(), 1, QImage::Format_ARGB32);
        image.fill(qRgba(0,0,0,255));
      }
      painter.drawImage((maxSize.width() - image.width())/2, positions.at(i), image);
    }
  }

  painter.end();

  return thisImage;
}
