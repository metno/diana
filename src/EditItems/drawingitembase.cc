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

DrawingItemBase::DrawingItemBase(int id__)
    : id_((id__ >= 0) ? id__ : nextId())
    , selected_(false)
    , joinCount_(0)
{
}

DrawingItemBase::~DrawingItemBase() {}

int DrawingItemBase::nextId_ = 0;

int DrawingItemBase::id() const { return id_; }

int DrawingItemBase::nextId()
{
  return nextId_++; // ### not thread safe; use a mutex for that
}

DrawingItemBase *DrawingItemBase::clone(const DrawingManager *dm, bool setUniqueId) const
{
  DrawingItemBase *item = cloneSpecial(setUniqueId);

  item->setLatLonPoints(dm->getLatLonPoints(*item));

  const_cast<QVariantMap &>(propertiesRef()).remove("points");
  item->setProperties(properties());

  item->selected_ = selected_;
  item->joinCount_ = joinCount_;

  return item;
}

void DrawingItemBase::setState(const DrawingItemBase *item)
{
  setPoints(item->getPoints());
  setLatLonPoints(item->getLatLonPoints());

  QVariantMap props(item->properties());
  props.remove("points");
  setProperties(props);

  selected_ = item->selected_;
  joinCount_ = item->joinCount_;
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

void DrawingItemBase::setProperties(const QVariantMap &properties, bool ignorePoints)
{
  properties_ = properties;
  if ((!ignorePoints) && properties.contains("points")) {
    QVariantList points = properties.value("points").toList();
    points_.clear();
    latLonPoints_.clear();
    foreach (QVariant v, points)
      latLonPoints_.append(v.toPointF());
  }
}

// Returns the current join ID of the item. Two or more items are considered joined if their "joinId" properties are non-zero
// and have the same absolute value. A negative and positive join ID indicates joining of the first and last end point respectively.
int DrawingItemBase::joinId() const
{
  bool ok;
  const int joinId = properties().value("joinId").toInt(&ok);
  return ok ? joinId : 0;
}

void DrawingItemBase::setJoinCount(int jc)
{
  joinCount_ = jc;
}

int DrawingItemBase::joinCount() const
{
  return joinCount_;
}

QList<QPointF> DrawingItemBase::getPoints() const
{
  return points_;
}

void DrawingItemBase::setPoints(const QList<QPointF> &points)
{
  points_ = points;
  updateRect();
}

QSizeF DrawingItemBase::getSize() const
{
  if (points_.size() < 2)
    return QSizeF(0, 0);

  return boundingRect().size();
}

QRectF DrawingItemBase::boundingRect() const
{
  if (points_.size() < 1)
    return QRectF();
  else if (points_.size() == 1)
    return QRectF(points_.at(0).x(), points_.at(0).y(), 0, 0);

  float xmin, xmax;
  xmin = xmax = points_.at(0).x();
  float ymin, ymax;
  ymin = ymax = points_.at(0).y();

  foreach (QPointF point, points_) {
    float x = point.x();
    float y = point.y();
    xmin = qMin(x, xmin);
    xmax = qMax(x, xmax);
    ymin = qMin(y, ymin);
    ymax = qMax(y, ymax);
  }

  return QRectF(xmin, ymin, xmax - xmin, ymax - ymin);
}

void DrawingItemBase::updateRect()
{
}

qreal DrawingItemBase::sqr(qreal x) { return x * x; }

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

  // style properties
  foreach (const QString key, properties_.keys()) {
    if (key.startsWith("style:"))
      extDataElem.appendChild(KML::createExtDataDataElement(doc, QString("met:%1").arg(key), properties_.value(key).toString()));
  }

  // join ID
  const int jid = joinId();
  if (jid)
    extDataElem.appendChild(KML::createExtDataDataElement(doc, QString("met:joinId"), QString::number(jid)));

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

void DrawingItemBase::setSelected(bool selected)
{
  selected_ = selected;
}

bool DrawingItemBase::selected() const
{
  return selected_;
}

QList<QVariantMap> DrawingItemBase::properties(const QList<QSharedPointer<DrawingItemBase> > &items)
{
  QList<QVariantMap> props;
  foreach(const QSharedPointer<DrawingItemBase> &item, items)
    props.append(item->properties());
  return props;
}
