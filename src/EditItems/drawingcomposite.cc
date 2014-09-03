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
#include "drawingcomposite.h"
#include "drawingsymbol.h"
#include "drawingtext.h"
#include "drawingstylemanager.h"
#include <diPlotModule.h>
#include <QDebug>

#define MILOGGER_CATEGORY "diana.Composite"
#include <miLogger/miLogging.h>

namespace DrawingItem_Composite {

Composite::Composite()
{
}

Composite::~Composite()
{
}

void Composite::draw()
{
  if (points_.isEmpty() || elements_.isEmpty())
    return;

  DrawingStyleManager *styleManager = DrawingStyleManager::instance();

  // Use the fill colour defined in the style to fill the text area.
  QRectF bbox(points_.at(0), points_.at(1));

  QList<QPointF> points;
  points << bbox.bottomLeft() << bbox.bottomRight() << bbox.topRight() << bbox.topLeft();

  styleManager->beginFill(this);
  styleManager->fillLoop(this, points);
  styleManager->endFill(this);

  foreach (DrawingItemBase *element, elements_) {
    element->draw();
  }
}

void Composite::setLatLonPoints(const QList<QPointF> &points)
{
  QPointF offset;

  if (latLonPoints_.size() > 0) {
    // Calculate the offset for each of the points using the first point passed.
    offset = points.at(0) - latLonPoints_.at(0);
  } else
    offset = QPointF(0, 0);

  // Adjust the child elements using the offset.
  foreach (DrawingItemBase *element, elements_) {

    QList<QPointF> newPoints;
    foreach (QPointF point, element->getLatLonPoints())
      newPoints.append(point + offset);

    element->setLatLonPoints(newPoints);
  }

  DrawingItemBase::setLatLonPoints(points);
}

QRectF Composite::boundingRect() const
{
  QRectF thisRect = DrawingItemBase::boundingRect();

  foreach (DrawingItemBase *element, elements_) {
    QRectF rect = element->boundingRect();
    thisRect = thisRect.united(rect);
  }

  return thisRect;
}

void Composite::updateRect()
{
  QRectF rect = boundingRect();
  points_[0] = rect.topLeft();
  points_[1] = rect.bottomRight();
}

QDomNode Composite::toKML(const QHash<QString, QString> &extraExtData) const
{
  return DrawingItemBase::toKML(); // call base implementation for now
}

DrawingItemBase::Category Composite::category() const
{
  return DrawingItemBase::Composite;
}

/**
 * Create the elements for the composite object based on the relevant style definition.
 */
void Composite::createElements()
{
  METLIBS_LOG_SCOPE();

  QVariantMap style = DrawingStyleManager::instance()->getStyle(this);
  QStringList objects = style.value("objects").toStringList();
  QStringList values = style.value("values").toStringList();
  QStringList styles = style.value("styles").toStringList();
  QString layout = style.value("layout").toString();

  if (layout == "horizontal")
    layout_ = Qt::Horizontal;
  else if (layout == "vertical")
    layout_ = Qt::Vertical;
  else {
    layout_ = Qt::Horizontal;
    if (objects.size() != 1)
      METLIBS_LOG_WARN("Invalid layout given in " << properties_.value("style:type").toString().toStdString());
  }

  // If the number of objects, styles and values do not match then warn and return.
  if ((objects.size() != values.size()) || (objects.size() != styles.size())) {
    METLIBS_LOG_WARN("Sizes of attributes do not match in " << properties_.value("style:type").toString().toStdString());
    return;
  }

  // Create the elements.

  for (int i = 0; i < objects.size(); ++i) {

    DrawingItemBase *element;

    if (objects.at(i) == "text")
      element = new DrawingItem_Text::Text();
    else if (objects.at(i) == "symbol")
      element = new DrawingItem_Symbol::Symbol();
    else if (objects.at(i) == "composite")
      element = new DrawingItem_Composite::Composite();
    else
      continue;

    element->setPoints(points_);

    // Copy the style property into the new element.
    element->setProperty("style:type", styles.at(i));

    if (objects.at(i) == "text") {
      // Copy the value into the element as text.
      QStringList lines;
      lines.append(values.at(i));
      static_cast<DrawingItem_Text::Text *>(element)->setText(lines);
    } else if (objects.at(i) == "composite") {
      DrawingItem_Composite::Composite *c = static_cast<DrawingItem_Composite::Composite *>(element);
      c->createElements();
    }

    elements_.append(element);
  }

  for (int i = 0; i < objects.size(); ++i)
    arrangeElement(i);
}

void Composite::arrangeElement(int i)
{
  QRectF previousRect;

  if (i < 1 || i >= elements_.size())
    previousRect = QRectF(boundingRect().topLeft(), QSizeF(0, 0));
  else
    previousRect = elements_.at(i - 1)->boundingRect();

  DrawingItemBase *element = elements_.at(i);
  QSizeF size = element->getSize();
  QPointF point;

  switch (layout_) {
  case Qt::Horizontal:
    //point = QPointF(previousRect.right(), previousRect.bottom() - previousRect.height() + size.height());
    point = QPointF(previousRect.right(), previousRect.bottom() - (previousRect.height() - size.height())/2);
    break;
  case Qt::Vertical:
    point = QPointF(previousRect.left() + (previousRect.size().width() - size.width())/2, previousRect.top());
    break;
  default:
    ;
  }

  // Determine the change in position of the element.
  QList<QPointF> points = element->getPoints();
  QPointF offset = point - points.at(0);

  for (int j = 0; j < points.size(); ++j)
    points[j] += offset;

  element->setPoints(points);
  qDebug() << element->boundingRect();

  // Obtain latitude and longitude coordinates for the element.
  setLatLonPoints(DrawingManager::instance()->getLatLonPoints(*element));
}

void Composite::setPoints(const QList<QPointF> &points)
{
  QPointF offset;

  if (points_.size() == 0) {
    // Ensure that the object itself has points.
    DrawingItemBase::setPoints(points);
    offset = QPointF(0, 0);
  }

  // Calculate the offset for each of the points using the first point passed.
  offset = points.at(0) - points_.at(0);

  // Adjust the child elements using the offset.
  foreach (DrawingItemBase *element, elements_) {

    QList<QPointF> newPoints;
    foreach (QPointF point, element->getPoints())
      newPoints.append(point + offset);

    element->setPoints(newPoints);
    element->setLatLonPoints(DrawingManager::instance()->getLatLonPoints(*element));
  }

  // Update the points to contain the child elements inside the object.
  //updateRect();
}

} // namespace
