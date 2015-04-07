/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

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

#include "GL/gl.h"
#include "diDrawingManager.h"
#include "EditItems/drawingtext.h"
#include "EditItems/drawingstylemanager.h"
#include "diFontManager.h"
#include "diPlotModule.h"

namespace DrawingItem_Text {

Text::Text(int id)
  : DrawingItemBase(id)
{
  propertiesRef().insert("style:margin", defaultMargin());
  propertiesRef().insert("style:spacing", defaultSpacing());
  propertiesRef().insert("style:fontname", QString::fromStdString(PlotOptions::defaultFontName()));
  propertiesRef().insert("style:fontface", QString::fromStdString(PlotOptions::defaultFontFace()));
  propertiesRef().insert("style:fontsize", PlotOptions::defaultFontSize());
}

Text::~Text()
{
}

void Text::draw()
{
  if (points_.isEmpty() || text().isEmpty())
    return;

  DrawingStyleManager *styleManager = DrawingStyleManager::instance();

  QRectF bbox = drawingRect();
  QList<QPointF> points;
  points << bbox.bottomLeft() << bbox.bottomRight() << bbox.topRight() << bbox.topLeft();

  // Use the fill colour defined in the style to fill the text area.
  styleManager->beginFill(this);
  styleManager->fillLoop(this, points);
  styleManager->endFill(this);

  // Draw the outline using the border colour and line pattern defined in the style.
  styleManager->beginLine(this);
  styleManager->drawLines(this, points, 0, true);
  styleManager->endLine(this);

  // Draw the text itself.
  styleManager->drawText(this);
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
  float width, height;
  if (!PlotModule::instance()->getStaticPlot()->getFontPack()->getStringSize(text.left(index).toStdString().c_str(), width, height))
    width = height = 0;

  float scale = 1/PlotModule::instance()->getStaticPlot()->getPhysToMapScaleX();

  QSizeF size(scale * width, scale * height);

  if (height == 0) {
    PlotModule::instance()->getStaticPlot()->getFontPack()->getStringSize("X", width, height);
    size.setHeight(scale * height);
  }

  return size;
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
  float y = points_.at(0).y();
  qreal width = 0;
  QStringList lines_ = text();

  for (int i = 0; i < lines_.size(); ++i) {
    QString text = lines_.at(i);
    QSizeF size = getStringSize(text);
    width = qMax(width, size.width());
    size.setHeight(qMax(size.height(), qreal(fontSize())));
    y -= size.height();
    if (i < lines_.size() - 1)
      y -= size.height() * spacing();
  }

  points_[1] = QPointF(x + width + 2 * margin(), y - 2 * margin());
}

bool Text::hit(const QPointF &pos, bool selected) const
{
  QRectF textbox = drawingRect();
  return textbox.contains(pos);
}

bool Text::hit(const QRectF &bbox) const
{
  if (points_.size() < 2)
    return false;

  QRectF textbox = drawingRect();
  return textbox.intersects(bbox);
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
  DrawingManager::instance()->setFromLatLonPoints(*this, getLatLonPoints());
  setText(extraExtData.value("met:text", "").split("\n"));
}

} // namespace
