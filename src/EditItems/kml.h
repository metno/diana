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

#ifndef KML_H
#define KML_H

#include "config.h"

#include <QSet>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QMap>
#include <QDomElement>
#include <QXmlSchema>
#include <QPointF>

class DrawingItemBase;

// API for saving/loading to/from KML files.
namespace KML {

void saveToFile(const QString &fileName, const QSet<DrawingItemBase *> &items, const QSet<DrawingItemBase *> &selItems, QString *error);

int findGroupId(const QDomNode &, bool &, QString *);

QHash<QString, QString> getExtendedData(const QDomNode &);

QList<QPointF> getPoints(const QDomNode &, QString *);

void findAncestorElements(const QDomNode &, QMap<QString, QDomElement> *, QString *);

QString getName(const QDomElement &, QString *);

QPair<QString, QString> getTimeSpan(const QDomElement &, QString *);

bool loadSchema(QXmlSchema &, QString *);
QDomDocument createDomDocument(const QByteArray &, const QXmlSchema &, const QUrl &, QString *);
bool convertFromOldFormat(QByteArray &, QString *);

// Creates and returns a new PolyLine.
template<typename PolyLineType>
static inline PolyLineType *createPolyLine(const QList<QPointF> &points, const QDomNode &coordsNode)
{
  Q_UNUSED(coordsNode); // might be needed later
  PolyLineType *polyLine = new PolyLineType();
  polyLine->setLatLonPoints(points);
  return polyLine;
}

// Creates and returns a new Symbol.
template<typename SymbolType>
static inline SymbolType *createSymbol(const QPointF &point, const QDomNode &coordsNode)
{
  Q_UNUSED(coordsNode); // might be needed later
  SymbolType *symbol = new SymbolType();
  symbol->setLatLonPoints(QList<QPointF>() << point);
  return symbol;
}

// Returns a set of items extracted from DOM document \a doc.
// Upon success, the function returns a non-empty set of items and leaves \a error empty.
// Otherwise, the function returns an empty set of items and a failure reason in \a error.
template<typename BaseType, typename PolyLineType, typename SymbolType,
         typename TextType, typename CompositeType>
static inline QSet<BaseType *> createFromDomDocument(const QDomDocument &doc, QString *error)
{
  *error = QString();

  QSet<BaseType *> items;
  QMap<int, int> finalGroupId;

  // loop over <coordinates> elements
  QDomNodeList coordsNodes = doc.elementsByTagName("coordinates");
  for (int i = 0; i < coordsNodes.size(); ++i) {
    const QDomNode coordsNode = coordsNodes.item(i);
    bool found;
    const int groupId = findGroupId(coordsNode, found, error);
    if (!error->isEmpty())
      break;
    if (found) {
      // create item
      const QList<QPointF> points = getPoints(coordsNode, error);
      if (!error->isEmpty())
        break;
      BaseType *item = 0;
      // create either a PolyLine or a Symbol based on the number of points alone
      if (points.size() > 1) {
        item = createPolyLine<PolyLineType>(points, coordsNode);
      } else if (points.size() == 1) {
        item = createSymbol<SymbolType>(points.first(), coordsNode);
      } else {
        *error = QString("empty <coordinates> element found");
        break;
      }
      Q_ASSERT(item);
      items.insert(item);

      if (!error->isEmpty())
        break;

      // set general properties
      if (!finalGroupId.contains(groupId))
          finalGroupId.insert(groupId, Drawing(item)->id()); // NOTE: item is just created, and its ID is globally unique!
      Drawing(item)->setProperty("groupId", finalGroupId.value(groupId));

      QHash<QString, QString> extdata = getExtendedData(coordsNode);
      if (extdata.contains("met:style"))
        Drawing(item)->setProperty("style:type", extdata.value("met:style"));

      QMap<QString, QDomElement> ancElems;
      findAncestorElements(coordsNode, &ancElems, error);
      if (!error->isEmpty())
        break;

      if (ancElems.contains("Placemark")) {
          Drawing(item)->setProperty("Placemark:name", getName(ancElems.value("Placemark"), error));
          if (!error->isEmpty())
            break;
      } else {
        *error = "found <coordinates> element outside a <Placemark> element";
        break;
      }

      if (ancElems.contains("Folder")) {
          Drawing(item)->setProperty("Folder:name", getName(ancElems.value("Folder"), error));
          if (!error->isEmpty())
            break;

          QPair<QString, QString> timeSpan = getTimeSpan(ancElems.value("Folder"), error);
          if (!error->isEmpty())
            break;
          Drawing(item)->setProperty("TimeSpan:begin", timeSpan.first);
          Drawing(item)->setProperty("TimeSpan:end", timeSpan.second);
      }
    } else {
      *error = "found <coordinates> element not associated with a met:groupId";
      break;
    }
  }

  if (!error->isEmpty()) {
    foreach (BaseType *item, items)
      delete item;
    return QSet<BaseType *>();
  } else if (items.isEmpty()) {
    *error = "no items found";
  }

  return items;
}

// Returns a set of items extracted from \a fileName.
// Upon success, the function returns a non-empty set of items and leaves \a error empty.
// Otherwise, the function returns an empty set of items and a failure reason in \a error.
template<typename BaseType, typename PolyLineType, typename SymbolType,
         typename TextType, typename CompositeType>
static inline QSet<BaseType *> createFromFile(const QString &fileName, QString *error)
{
  *error = QString();

  // load schema
  QXmlSchema schema;
  if (!loadSchema(schema, error)) {
    *error = QString("failed to load KML schema: %1").arg(*error);
    return QSet<BaseType *>();
  }

  // load data
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    *error = QString("failed to open file %1 for reading").arg(fileName);
    return QSet<BaseType *>();
  }
  QByteArray data = file.readAll();

  // create document from data validated against schema
  const QUrl docUri(QUrl::fromLocalFile(fileName));
  QDomDocument doc = createDomDocument(data, schema, docUri, error);
  if (doc.isNull()) {
    // assume that the failure was caused by data being in old format
    QString old2newError;
    if (!convertFromOldFormat(data, &old2newError)) {
      *error = QString("failed to create DOM document:<br/>%1<br/><br/>also failed to convert from old format:<br/>%2").arg(*error).arg(old2newError);
      return QSet<BaseType *>();
    }

    doc = createDomDocument(data, schema, docUri, error);
    if (doc.isNull()) {
      *error = QString("failed to create DOM document after successfully converting from old format: %1").arg(*error);
      return QSet<BaseType *>();
    }
  }

  // at this point, a document is successfully created from either the new or the old format

  // parse document and create items
  return createFromDomDocument<BaseType, PolyLineType, SymbolType, TextType, CompositeType>(doc, error);
}

} // namespace

#endif // KML_H
