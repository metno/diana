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
#include "drawingpolyline.h"
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

  QRectF bbox(points_.at(0), points_.at(1));

  QList<QPointF> points;
  points << bbox.bottomLeft() << bbox.bottomRight() << bbox.topRight() << bbox.topLeft();

  // Use the fill colour defined in the style to fill the text area.
  styleManager->beginFill(this);
  styleManager->fillLoop(this, points);
  styleManager->endFill(this);

  foreach (DrawingItemBase *element, elements_)
    element->draw();

  // Draw the outline using the border colour and line pattern defined in
  // the style.
  styleManager->beginLine(this);
  styleManager->drawLines(this, points);
  styleManager->endLine(this);
}

QRectF Composite::boundingRect() const
{
  QRectF thisRect;

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
  QHash<QString, QString> extra;
  extra["children"] = toKMLExtraData();
  return DrawingItemBase::toKML(extra.unite(extraExtData));
}

void Composite::fromKML(const QHash<QString, QString> &extraExtData)
{
  fromKMLExtraData(extraExtData.value("met:children"));
  writeExtraProperties();
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

  if (points_.isEmpty()) {
    points_.append(QPointF(0, 0));
    points_.append(QPointF(0, 0));
  }

  QVariantMap style = DrawingStyleManager::instance()->getStyle(this);
  QStringList objects = style.value("objects").toStringList();
  QStringList values = style.value("values").toStringList();
  QStringList styles = style.value("styles").toStringList();
  QString layout = style.value("layout").toString();

  if (layout == "horizontal")
    layout_ = Horizontal;
  else if (layout == "vertical")
    layout_ = Vertical;
  else if (layout == "diagonal")
    layout_ = Diagonal;
  else {
    layout_ = Horizontal;
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
      element = newTextItem();
    else if (objects.at(i) == "symbol")
      element = newSymbolItem();
    else if (objects.at(i) == "composite")
      element = newCompositeItem();
    else if (objects.at(i) == "line")
      element = newPolylineItem();
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

  arrangeElements();

  // Write any properties for child elements already present in this item's
  // property map into the property maps of the children themselves.
  writeExtraProperties();

  // Read the child elements' properties back into the main children entry of
  // this item's property map.
  readExtraProperties();
}

void Composite::arrangeElements()
{
  QRectF previousRect = QRectF(points_[0], QSizeF(0, 0));

  for (int i = 0; i < elements_.size(); ++i) {

    DrawingItemBase *element = elements_.at(i);
    QSizeF size = element->getSize();
    QPointF point;

    QList<QPointF> points;

    if (dynamic_cast<DrawingItem_PolyLine::PolyLine *>(element)) {

      // Treat lines differently. Position all the points based on the previous
      // element, discarding the existing geometry of the line.
      if (i != 0) {
        switch (layout_) {
        case Horizontal:
          points << QPointF(previousRect.right(), previousRect.bottom());
          points << QPointF(previousRect.right(), previousRect.top());
          previousRect.translate(2, 0);
          break;
        case Vertical:
          points << QPointF(previousRect.left(), previousRect.top());
          points << QPointF(previousRect.right(), previousRect.top());
          previousRect.translate(0, -2);
          break;
        case Diagonal:
          points << QPointF(previousRect.left() + previousRect.width()/2,
                            previousRect.top() - previousRect.height()/2);
          points << QPointF(previousRect.right() + previousRect.width()/2,
                            previousRect.bottom() - previousRect.height()/2);
          previousRect.translate(2, -2);
          break;
        default:
          ;
        }
      }

      element->setPoints(points);

    } else {
      if (i == 0)
        point = QPointF(previousRect.left(), previousRect.top());
      else {
        switch (layout_) {
        case Horizontal:
          //point = QPointF(previousRect.right(), previousRect.bottom() - previousRect.height() + size.height());
          point = QPointF(previousRect.right(), previousRect.bottom() - (previousRect.height() - size.height())/2);
          break;
        case Vertical:
          point = QPointF(previousRect.left() + (previousRect.size().width() - size.width())/2, previousRect.top());
          break;
        case Diagonal:
          point = QPointF(previousRect.right(), previousRect.top());
          break;
        default:
          ;
        }
      }

      // Determine the change in position of the element.
      QPointF offset = point - element->boundingRect().bottomLeft();

      points = element->getPoints();
      for (int j = 0; j < points.size(); ++j)
        points[j] += offset;

      element->setPoints(points);
      previousRect = element->boundingRect();
    }
  }
}

DrawingItemBase *Composite::elementAt(int index) const
{
  return elements_.at(index);
}

void Composite::setPoints(const QList<QPointF> &points)
{
  QPointF offset;

  // Calculate the offset for each of the points using the first point passed.
  if (points_.isEmpty()) {
    DrawingItemBase::setPoints(points);
    offset = QPointF(0, 0);
  } else
    offset = points.at(0) - points_.at(0);

  // Adjust the child elements using the offset.
  foreach (DrawingItemBase *element, elements_) {

    QList<QPointF> newPoints;
    foreach (QPointF point, element->getPoints())
      newPoints.append(point + offset);

    element->setPoints(newPoints);
  }

  // Update the points to contain the child elements inside the object.
  updateRect();
}

/**
 * Read the properties of the child elements, storing them in a list in this
 * item's property map.
 */
void Composite::readExtraProperties()
{
  // Examine the child elements of this item and their children, incorporating their
  // properties into the properties of this item.
  QVariantList extraProperties;

  foreach (DrawingItemBase *element, elements_) {
    // For child elements that are composite items, ensure that they contain
    // up-to-date information about their children.
    Composite *c = dynamic_cast<Composite *>(element);
    if (c)
      c->readExtraProperties();

    extraProperties.append(element->properties());
  }

  properties_["children"] = extraProperties;
}

/**
 * Write the properties of the child elements, storing them in a list in this
 * item's property map.
 */
void Composite::writeExtraProperties()
{
  QVariantList childList = properties_["children"].toList();

  for (int i = 0; i < childList.size(); ++i) {
    DrawingItemBase *element = elements_.at(i);
    element->setProperties(childList.at(i).toMap());

    // Update any child elements that are composite items using their updated
    // properties.
    Composite *c = dynamic_cast<Composite *>(element);
    if (c)
      c->writeExtraProperties();
  }
}

DrawingItemBase *Composite::newCompositeItem() const
{
  return new DrawingItem_Composite::Composite();
}

DrawingItemBase *Composite::newPolylineItem() const
{
  return new DrawingItem_PolyLine::PolyLine();
}

DrawingItemBase *Composite::newSymbolItem() const
{
  return new DrawingItem_Symbol::Symbol();
}

DrawingItemBase *Composite::newTextItem() const
{
  return new DrawingItem_Text::Text();
}

QString Composite::toKMLExtraData() const
{
  QVariantList childList = properties_.value("children").toList();
  QByteArray data;
  QDataStream stream(&data, QIODevice::WriteOnly);
  stream << childList;
  return QString(data.toBase64());
}

void Composite::fromKMLExtraData(const QString &data)
{
  QVariantList childList;
  QByteArray bytes = QByteArray::fromBase64(data.toAscii());
  QDataStream stream(&bytes, QIODevice::ReadOnly);
  stream >> childList;
  properties_["children"] = childList;
}

} // namespace
