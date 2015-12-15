/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2014 met.no

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

#include <qmath.h>
#include "diDrawingManager.h"
#include "EditItems/drawingtext.h"
#include "EditItems/drawingstylemanager.h"
#include "diPlotModule.h"
#include "diGLPainter.h"

#define MILOGGER_CATEGORY "diana.DrawingItem.Text"
#include <miLogger/miLogging.h>

namespace DrawingItem_Text {

Text::Text(int id)
  : DrawingItemBase(id)
{
  propertiesRef().insert("style:margin", defaultMargin());
  propertiesRef().insert("style:spacing", defaultSpacing());
  propertiesRef().insert("style:fontname", QString::fromStdString(PlotOptions::defaultFontName()));
  propertiesRef().insert("style:fontface", QString::fromStdString(PlotOptions::defaultFontFace()));
  propertiesRef().insert("style:fontsize", PlotOptions::defaultFontSize());
  propertiesRef().insert("style:cornersegments", 0);
  propertiesRef().insert("style:cornerradius", defaultMargin());
}

Text::~Text()
{
}

void Text::draw(DiGLPainter* gl)
{
  if (points_.isEmpty() || text().isEmpty())
    return;

  DrawingStyleManager *styleManager = DrawingStyleManager::instance();

  QList<QPointF> points = styleManager->linesForBBox(this);

  // Use the fill colour defined in the style to fill the text area.
  styleManager->beginFill(gl, this);
  styleManager->fillLoop(gl, this, points);
  styleManager->endFill(gl, this);

  // Draw the outline using the border colour and line pattern defined in the style.
  styleManager->beginLine(gl, this);
  styleManager->drawLines(gl, this, points, 0, true);
  styleManager->endLine(gl, this);

  // Draw the text itself.
  styleManager->drawText(gl, this);
}

/**
 * Get the string bounding rectangle in viewport/screen units.
 */
QRectF Text::getStringRect(const QString &text, int index) const
{
  if (index == -1)
    index = text.size();

  DrawingStyleManager *styleManager = DrawingStyleManager::instance();
  styleManager->setFont(this);

  // Obtain the width and height of the text in plot coordinates.
  float x = 0, y = 0, width = 0, height = 0;
  if (DiCanvas* canvas = styleManager->canvas()) {
    canvas->getTextRect(text.left(index), x, y, width, height);
    if (height == 0)
      canvas->getTextRect("X", x, y, width, height);
  }

  const double scale = 1/PlotModule::instance()->getStaticPlot()->getPhysToMapScaleX();
  return QRectF(scale * x, scale * y, scale * width, scale * height);
}

/**
 * Get the string size in viewport/screen units.
 */
QSizeF Text::getStringSize(const QString &text, int index) const
{
  if (index == -1)
    index = text.size();

  DrawingStyleManager *styleManager = DrawingStyleManager::instance();
  styleManager->setFont(this);

  // Obtain the width and height of the text in plot coordinates.
  float width = 0, height = 0;
  if (DiCanvas* canvas = styleManager->canvas()) {
    canvas->getTextSize(text.left(index), width, height);
    if (height == 0)
      canvas->getTextSize("X", width, height);
  }

  const double scale = 1/PlotModule::instance()->getStaticPlot()->getPhysToMapScaleX();
  return QSizeF(scale * width, scale * height);
}

float Text::margin() const
{
  bool ok = false;
  const float val = properties().value("style:margin", defaultMargin()).toFloat(&ok);
  return ok ? val : defaultMargin();
}

float Text::spacing() const
{
  bool ok = false;
  const float val = properties().value("style:spacing", defaultSpacing()).toFloat(&ok);
  return ok ? val : defaultSpacing();
}

float Text::fontSize() const
{
  bool ok = false;
  const float val = properties().value("style:fontsize", PlotOptions::defaultFontSize()).toFloat(&ok);
  return ok ? val : PlotOptions::defaultFontSize();
}

DrawingItemBase::Category Text::category() const
{
  return DrawingItemBase::Text;
}

QStringList Text::text() const
{
  return ConstDrawing(this)->property("text").toStringList();
}

QRectF Text::drawingRect() const
{
  QRectF bbox = boundingRect();
  //  if (property("alignment", Qt::AlignCenter) == Qt::AlignCenter) ### This must also be considered in DrawingStyleManager::drawText()
  //    bbox.translate(-bbox.width()/2, bbox.height()/2);
  return bbox;
}

void Text::updateRect()
{
  float x = points_.at(0).x();
  float y = points_.at(0).y() - margin();
  qreal width = 0;
  qreal height = fontSize();

  QStringList lines_ = text();

  for (int i = 0; i < lines_.size(); ++i) {
    QString text = lines_.at(i);
    QSizeF size = getStringSize(text);
    width = qMax(width, size.width());
    height = fontSize();
    if (i == 0 || i == lines_.size() - 1)
      height = qMax(size.height(), height);

    y -= height;
    if (i < lines_.size() - 1)
      y -= height * spacing();
  }

  points_[1] = QPointF(x + width + (2 * margin()), y - margin());
}

DrawingItemBase::HitType Text::hit(const QPointF &pos, bool selected) const
{
  QRectF textbox = drawingRect();
  if (textbox.contains(pos))
    return Area;
  else
    return None;
}

DrawingItemBase::HitType Text::hit(const QRectF &bbox) const
{
  if (points_.size() < 2)
    return None;

  QRectF textbox = drawingRect();
  if (textbox.intersects(bbox))
    return Area;
  else
    return None;
}

void Text::setText(const QStringList &lines)
{
  setProperty("text", lines);
  if (!points_.isEmpty())
    updateRect();
}

QDomNode Text::toKML(const QHash<QString, QString> &extraExtData) const
{
  QHash<QString, QString> extra;
  QStringList lines = text();
  extra["text"] = lines.join("\n");
  return DrawingItemBase::toKML(extra.unite(extraExtData));
}

void Text::fromKML(const QHash<QString, QString> &extraExtData)
{
  // Remove any previously defined default properties.
  properties_.clear();
  setText(extraExtData.value("met:text", "").split("\n"));
}

} // namespace
