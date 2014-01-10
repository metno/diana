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

#include "drawingitembase.h"
#include "polyStipMasks.h"

PolygonStyle::PolygonStyle()
{
  borderColour = Qt::black;
  borderWidth = 1.0;
  dashed = false;
  fillColour = Qt::transparent;
  smooth = false;
  shaped = false;
  fillPattern = 0;
}

void PolygonStyle::parse(const QHash<QString, QString> &definition)
{
  // Parse the definition and set the private members.
  if (definition.contains("border")) {

    foreach (QString piece, definition["border"].split(",")) {
      if (piece == "thick")
        borderWidth = 2;
      else if (piece == "thin")
        borderWidth = 1;
      else if (piece == "dashed")
        dashed = true;
      else if (piece == "solid")
        dashed = false;
      else if (piece == "smooth")
        smooth = true;
      else if (piece == "shaped")
        shaped = true;
      else {
        // Treat the string as a colour name or RGBA value.
        if (piece.contains("@"))
          piece.replace("@", "#");
        borderColour = QColor(piece);
      }
    }
  }

  if (definition.contains("fill")) {

    foreach (QString piece, definition["fill"].split(",")) {
      // Treat the string as a colour name or RGBA value.
      if (piece.contains("@"))
        piece.replace("@", "#");
      fillColour = QColor(piece);
    }
  }

  if (definition.contains("fillpattern")) {

    QString filltype = definition["fillpattern"];
    if (filltype == "diagleft")
      fillPattern = diagleft;
    else if (filltype == "zigzag")
      fillPattern = zigzag;
    else if (filltype == "paralyse")
      fillPattern = paralyse;
    else if (filltype == "ldiagleft2")
      fillPattern = ldiagleft2;
    else if (filltype == "vdiagleft")
      fillPattern = vdiagleft;
    else if (filltype == "vldiagcross_little")
      fillPattern = vldiagcross_little;
  }
}

PolygonStyle::~PolygonStyle()
{
}

void PolygonStyle::beginLine()
{
  if (dashed) {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(2, 0xf0f0);
  }

  glColor3ub(borderColour.red(), borderColour.green(), borderColour.blue());
}

void PolygonStyle::endLine()
{
  if (dashed)
    glDisable(GL_LINE_STIPPLE);
}

void PolygonStyle::beginFill()
{
  glColor4ub(fillColour.red(), fillColour.green(), fillColour.blue(),
             fillColour.alpha());

  if (fillPattern) {
    glEnable(GL_POLYGON_STIPPLE);
    glPolygonStipple(fillPattern);
  }
}

void PolygonStyle::endFill()
{
  if (fillPattern)
    glDisable(GL_POLYGON_STIPPLE);
}


DrawingItemBase::DrawingItemBase()
    : id_(nextId())
{
}

DrawingItemBase::~DrawingItemBase()
{
}

int DrawingItemBase::nextId_ = 0;

int DrawingItemBase::id() const { return id_; }

int DrawingItemBase::nextId()
{
  return nextId_++; // ### not thread safe; use a mutex for that
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
    foreach (QVariant v, points) {
      latLonPoints_.append(v.toPointF());
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
QDomElement DrawingItemBase::createExtDataElement(QDomDocument &doc) const
{
  QDomElement valueElem = doc.createElement("value");
  valueElem.appendChild(doc.createTextNode(QString::number(groupId())));
  QDomElement dataElem = doc.createElement("Data");
  dataElem.setAttribute("name", "met:groupId");
  dataElem.appendChild(valueElem);
  QDomElement extDataElem = doc.createElement("ExtendedData");
  extDataElem.appendChild(dataElem);
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

QDomNode DrawingItemBase::toKML() const
{
  QDomDocument doc;
  QDomElement extDataElem = createExtDataElement(doc);
  QDomElement popElem = createPointOrPolygonElement(doc);
  QDomElement finalElem;

  if (propertiesRef().contains("Folder:name")) {
    QDomElement nameElem = doc.createElement("name");
    nameElem.appendChild(doc.createTextNode(propertiesRef().value("Folder:name").toString()));
    QDomElement timeSpanElem = createTimeSpanElement(doc);
    QDomElement placemarkElem = createPlacemarkElement(doc);
    placemarkElem.appendChild(popElem);
    QDomElement folderElem = doc.createElement("Folder");
    folderElem.appendChild(nameElem);
    folderElem.appendChild(timeSpanElem);
    folderElem.appendChild(extDataElem);
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
