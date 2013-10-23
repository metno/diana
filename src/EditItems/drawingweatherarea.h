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

#ifndef DRAWINGITEM_WEATHERAREA_H
#define DRAWINGITEM_WEATHERAREA_H

#include <QColor>
#include <QDomDocument>
#include "drawingitembase.h"

namespace DrawingItem_WeatherArea {

class WeatherArea : public DrawingItemBase
{
public:
  WeatherArea();
  virtual ~WeatherArea();

  virtual void draw();

protected:
  QColor color_;
};

// Attempts to create new WeatherArea objects from the KML structure in \a data (originally read from \a origFileName).
// If successful, the function returns a list of pointers to the new objects.
// Otherwise, the function returns an empty list and passes an explanation in \a error.
template<typename ItemType>
static inline QList<ItemType *> createFromKML(const QByteArray &data, const QString &origFileName, QString *error)
{
  *error = QString();

  int line;
  int col;
  QString err;
  QDomDocument doc;
  if (doc.setContent(data, &err, &line, &col) == false) {
    *error = QString("parse error at line=%1, column=%2: %3").arg(line).arg(col).arg(err);
    return QList<ItemType *>();
  }

  const QDomNodeList folderNodes = doc.elementsByTagName("Folder");
  if (folderNodes.size() == 0) {
    *error = QString("no Folder elements found");
    return QList<ItemType *>();
  }

  QList<ItemType *> areas;
  int groupId = -1;

  // loop over folders
  for (int i = 0; i < folderNodes.size(); ++i) {
    const QDomNode folderNode = folderNodes.item(i);

    // folder name
    const QDomElement folderNameElement = folderNode.firstChildElement("name");
    if (folderNameElement.isNull()) {
      *error = QString("no folder name element found");
      break;
    }
    const QString folderName = folderNameElement.firstChild().nodeValue();

    // placemarks
    const QDomNodeList placemarkNodes = folderNode.toElement().elementsByTagName("Placemark");
    if (placemarkNodes.size() == 0) {
      *error = QString("no Placemark elements found in Folder element %1").arg(folderName);
      break;
    }

    // loop over placemarks
    for (int j = 0; j < placemarkNodes.size(); ++j) {
      const QDomNode placemarkNode = placemarkNodes.item(j);

      // placemark name
      const QDomElement placemarkNameElement = placemarkNode.firstChildElement("name");
      if (placemarkNameElement.isNull()) {
        *error = QString("no name element found in Placemark element %1 in Folder element %2")
            .arg(j).arg(folderName);
        break;
      }
      const QString placemarkName = placemarkNameElement.firstChild().nodeValue();

      // placemark coordinates
      const QDomNodeList coordinatesNodes = placemarkNode.toElement().elementsByTagName("coordinates");
      if (coordinatesNodes.size() != 1) {
        *error = QString("exactly one coordinates element expected in Placemark element %1 in Folder element %2, found %3")
            .arg(placemarkName).arg(folderName).arg(coordinatesNodes.size());
        break;
      }
      if (coordinatesNodes.item(0).childNodes().size() != 1) {
        *error = QString("one child expected in coordinates element in Placemark element %1 in Folder element %2, found %3")
            .arg(placemarkName).arg(folderName).arg(coordinatesNodes.item(0).childNodes().size());
        break;
      }
      const QString coords = coordinatesNodes.item(0).firstChild().nodeValue();
      QList<QPointF> points;
      foreach (QString coord, coords.split(QRegExp("\\s+"), QString::SkipEmptyParts)) {
        const QStringList coordComps = coord.split(",", QString::SkipEmptyParts);
        if (coordComps.size() < 2) {
          *error = QString("expected at least two components (i.e. lat, lon) in coordinate in Placemark element %1 in Folder element %2, found %3: %4")
              .arg(placemarkName).arg(folderName).arg(coordComps.size()).arg(coord);
          break;
        }
        bool ok;
        const double lon = coordComps.at(0).toDouble(&ok);
        if (!ok) {
          *error = QString("failed to convert longitude string to double value in Placemark element %1 in Folder element %2: %3")
              .arg(placemarkName).arg(folderName).arg(coordComps.at(0));
          break;
        }
        const double lat = coordComps.at(1).toDouble(&ok);
        if (!ok) {
          *error = QString("failed to convert latitude string to double value in Placemark element %1 in Folder element %2: %3")
              .arg(placemarkName).arg(folderName).arg(coordComps.at(1));
          break;
        }
        points.append(QPointF(lat, lon)); // note lat,lon order
      }

      if (!error->isEmpty())
        break;

      ItemType *area = new ItemType();
      area->setLatLonPoints(points);
      if (groupId == -1)
          groupId = area->id(); // use the first area's ID for group ID
      area->propertiesRef().insert("groupId", groupId);
      area->propertiesRef().insert("origFileName", origFileName);
      area->propertiesRef().insert("origKML", data);
      area->propertiesRef().insert("folderName", folderName);
      area->propertiesRef().insert("placemarkName", placemarkName);
      areas.append(area);

    } // placemarks

    if (!error->isEmpty())
      break;

  } // folders

  if (!error->isEmpty()) {
    foreach (ItemType *area, areas)
      delete area;
    areas.clear();
  }

  return areas;
}

} // namespace

#endif
