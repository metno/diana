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

#include <EditItems/drawingitembase.h>
#include <diDrawingManager.h>
#include <EditItems/drawingstylemanager.h>
#include <EditItems/kml.h>

DrawingItemBase::DrawingItemBase()
    : id_(nextId())
{
}

DrawingItemBase::~DrawingItemBase() {}

int DrawingItemBase::nextId_ = 0;

int DrawingItemBase::id() const { return id_; }

int DrawingItemBase::nextId()
{
  return nextId_++; // ### not thread safe; use a mutex for that
}

DrawingItemBase *DrawingItemBase::clone(const DrawingManager *dm) const
{
  DrawingItemBase *item = cloneSpecial();
  item->setLatLonPoints(dm->getLatLonPoints(*item));
  item->setProperties(properties());
  return item;
}

int DrawingItemBase::groupId() const
{
  if (!properties_.contains("groupId"))
      properties_.insert("groupId", id()); // note that properties_ is declared as mutable
  const QVariant vgid = properties_.value("groupId");
  bool ok;
  const int gid = vgid.toInt(&ok);
  Q_ASSERT(ok);
  return gid;
}

QVariant DrawingItemBase::property(const QString &name, const QVariant &default_) const
{
  if (properties_.contains(name))
    return properties_.value(name);
  else
    return default_;
}

void DrawingItemBase::setProperty(const QString &name, const QVariant &value)
{
  properties_[name] = value;
}

QVariantMap DrawingItemBase::properties() const
{
  return properties_;
}

QVariantMap &DrawingItemBase::propertiesRef()
{
  return properties_;
}

const QVariantMap &DrawingItemBase::propertiesRef() const
{
  return properties_;
}

void DrawingItemBase::setProperties(const QVariantMap &properties)
{
  properties_ = properties;
  if (properties.contains("points")) {
    QVariantList points = properties.value("points").toList();
    points_.clear();
    latLonPoints_.clear();
    foreach (QVariant v, points)
      latLonPoints_.append(v.toPointF());
  }

  QString styleType = properties_.value("style:type").toString();

  // Update the type name in the item's properties.
  properties_["style:type"] = styleType;

  // If the style is a custom style then update the properties using the
  // Default style as a template.
  if (styleType == "Custom") {
    QVariantMap style = DrawingStyleManager::instance()->getStyle(category(), "Default");
    foreach (QString key, style.keys()) {
      if (!properties_.contains("style:" + key)) {
        properties_["style:" + key] = style.value(key);
      }
    }
  }
}

QList<QPointF> DrawingItemBase::getPoints() const
{
    return points_;
}

void DrawingItemBase::setPoints(const QList<QPointF> &points)
{
    points_ = points;
}

QList<QPointF> DrawingItemBase::getLatLonPoints() const
{
    return latLonPoints_;
}

void DrawingItemBase::setLatLonPoints(const QList<QPointF> &points)
{
    latLonPoints_ = points;
}

// Returns a new <ExtendedData> element.
QDomElement DrawingItemBase::createExtDataElement(QDomDocument &doc, const QHash<QString, QString> &extra) const
{
  QDomElement extDataElem = doc.createElement("ExtendedData");

  // Explictly encode the object type in the extended data.
  QString objectType;
  switch (category()) {
  case PolyLine:
    objectType = "PolyLine";
    break;
  case Symbol:
    objectType = "Symbol";
    break;
  case Text:
    objectType = "Text";
    break;
  case Composite:
    objectType = "Composite";
    break;
  default:
    objectType = "Undefined";
    break;
  }
  extDataElem.appendChild(KML::createExtDataDataElement(doc, "met:objectType", objectType));

  // group ID
  extDataElem.appendChild(KML::createExtDataDataElement(doc, "met:groupId", QString::number(groupId())));

  // style properties
  foreach (const QString key, properties_.keys()) {
    if (key.startsWith("style:"))
      extDataElem.appendChild(KML::createExtDataDataElement(doc, QString("met:%1").arg(key), properties_.value(key).toString()));
  }

  // extra data
  foreach (const QString key, extra.keys()) {
    extDataElem.appendChild(KML::createExtDataDataElement(doc, QString("met:%1").arg(key), extra.value(key)));
  }

  return extDataElem;
}

// Returns a new <Point> or <Polygon> element (depending on whether points_.size() == 1 or > 1 respectively).
QDomElement DrawingItemBase::createPointOrPolygonElement(QDomDocument &doc) const
{
  Q_ASSERT(points_.size() > 0);

  // create the <coordinates> element
  QString coords;
  foreach (QPointF point, getLatLonPoints())
    coords.append(QString("%1,%2,0\n").arg(point.y()).arg(point.x())); // note lon,lat order
  QDomElement coordsElem = doc.createElement("coordinates");
  coordsElem.appendChild(doc.createTextNode(coords));

  // create a <Point> or <Polygon> element
  QDomElement popElem;
  if (points_.size() == 1) {
    popElem = doc.createElement("Point");
    popElem.appendChild(coordsElem);
  } else {
    QDomElement linearRingElem = doc.createElement("LinearRing");
    linearRingElem.appendChild(coordsElem);
    QDomElement outerBoundaryIsElem = doc.createElement("outerBoundaryIs");
    outerBoundaryIsElem.appendChild(linearRingElem);
    QDomElement tessellateElem = doc.createElement("tessellate");
    tessellateElem.appendChild(doc.createTextNode("1"));
    popElem = doc.createElement("Polygon");
    popElem.appendChild(tessellateElem);
    popElem.appendChild(outerBoundaryIsElem);
  }

  return popElem;
}

// Returns a new <TimeSpan> element.
QDomElement DrawingItemBase::createTimeSpanElement(QDomDocument &doc) const
{
  Q_ASSERT(propertiesRef().contains("TimeSpan:begin"));
  Q_ASSERT(propertiesRef().contains("TimeSpan:end"));
  QDomElement beginTimeElem = doc.createElement("begin");
  beginTimeElem.appendChild(doc.createTextNode(propertiesRef().value("TimeSpan:begin").toString()));
  QDomElement endTimeElem = doc.createElement("end");
  endTimeElem.appendChild(doc.createTextNode(propertiesRef().value("TimeSpan:end").toString()));
  QDomElement timeSpanElem = doc.createElement("TimeSpan");
  timeSpanElem.appendChild(beginTimeElem);
  timeSpanElem.appendChild(endTimeElem);
  return timeSpanElem;
}

// Returns a new <Placemark> element.
QDomElement DrawingItemBase::createPlacemarkElement(QDomDocument &doc) const
{
  QDomElement nameElem = doc.createElement("name");
  const QString name = propertiesRef().contains("Placemark:name")
      ? propertiesRef().value("Placemark:name").toString()
      : QString("anonymous placemark %1").arg(id());
  nameElem.appendChild(doc.createTextNode(name));
  QDomElement placemarkElem = doc.createElement("Placemark");
  placemarkElem.appendChild(nameElem);
  return placemarkElem;
}

QDomNode DrawingItemBase::toKML(const QHash<QString, QString> &extraExtData) const
{
  QDomDocument doc;
  QDomElement extDataElem = createExtDataElement(doc, extraExtData);
  QDomElement popElem = createPointOrPolygonElement(doc);
  QDomElement finalElem;

  if (propertiesRef().contains("Folder:name")) {
    QDomElement nameElem = doc.createElement("name");
    nameElem.appendChild(doc.createTextNode(propertiesRef().value("Folder:name").toString()));
    QDomElement timeSpanElem = createTimeSpanElement(doc);
    QDomElement placemarkElem = createPlacemarkElement(doc);
    placemarkElem.appendChild(extDataElem);
    placemarkElem.appendChild(popElem);
    QDomElement folderElem = doc.createElement("Folder");
    folderElem.appendChild(nameElem);
    folderElem.appendChild(timeSpanElem);
    folderElem.appendChild(placemarkElem);
    finalElem = folderElem;
  } else {
    QDomElement placemarkElem = createPlacemarkElement(doc);
    placemarkElem.appendChild(extDataElem);
    placemarkElem.appendChild(popElem);
    finalElem = placemarkElem;
  }

  return finalElem;
}

void DrawingItemBase::fromKML(const QHash<QString, QString> &extraExtData)
{
}
