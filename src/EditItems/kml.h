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
#include <EditItems/drawingstylemanager.h>
#include <EditItems/drawingitembase.h>
#include <EditItems/layers.h>

// API for saving/loading to/from KML files.
namespace KML {

void saveToFile(const QString &fileName, const QList<QSharedPointer<EditItems::Layer> > &layers, QString *error);

QDomElement createExtDataDataElement(QDomDocument &, const QString &, const QString &);

int findGroupId(const QDomNode &, bool &, QString *);

QHash<QString, QString> getExtendedData(const QDomNode &, const QString &);

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

// Returns a list of item layers extracted from DOM document \a doc.
// Upon success, the function returns a non-empty list of item layers and leaves \a error empty.
// Otherwise, the function returns an empty list of item layers and a failure reason in \a error.
// If the document contains no layer information, the items are returned in a single layer with default properties.
template<typename BaseType, typename PolyLineType, typename SymbolType,
         typename TextType, typename CompositeType>
static inline QList<QSharedPointer<EditItems::Layer> > createFromDomDocument(const QDomDocument &doc, QString *error)
{
  *error = QString();

  // *** PHASE 1: extract items

  QList<QSharedPointer<DrawingItemBase> > items;
  QMap<int, int> finalGroupId;

  // loop over <coordinates> elements
  QDomNodeList coordsNodes = doc.elementsByTagName("coordinates");
  for (int i = 0; i < coordsNodes.size(); ++i) {
    const QDomNode coordsNode = coordsNodes.item(i);
    bool found;
    const int groupId = findGroupId(coordsNode, found, error);
    if (!error->isEmpty())
      return QList<QSharedPointer<EditItems::Layer> >();
    if (found) {
      // create item
      const QList<QPointF> points = getPoints(coordsNode, error);
      if (!error->isEmpty())
        return QList<QSharedPointer<EditItems::Layer> >();
      BaseType *itemObj = 0;
      // create either a PolyLine or a Symbol based on the number of points alone
      if (points.size() > 1) {
        itemObj = createPolyLine<PolyLineType>(points, coordsNode);
      } else if (points.size() == 1) {
        itemObj = createSymbol<SymbolType>(points.first(), coordsNode);
      } else {
        *error = QString("empty <coordinates> element found");
        return QList<QSharedPointer<EditItems::Layer> >();
      }
      Q_ASSERT(itemObj);
      QSharedPointer<DrawingItemBase> item(Drawing(itemObj));
      items.append(item);
      DrawingItemBase *ditem = item.data();

      // set general properties
      if (!finalGroupId.contains(groupId))
        finalGroupId.insert(groupId, ditem->id()); // NOTE: item->data() is just created, and its ID is globally unique!
      ditem->setProperty("groupId", finalGroupId.value(groupId));

      const QHash<QString, QString> pmExtData = getExtendedData(coordsNode, "Placemark");
      if (pmExtData.isEmpty()) {
        *error = "<Placemark> element without <ExtendedData> element found";
        return QList<QSharedPointer<EditItems::Layer> >();
      }
      DrawingStyleManager::instance()->setStyle(ditem, pmExtData, "met:style:");

      if (pmExtData.contains("met:layerId"))  {
        bool ok;
        const int layerId = pmExtData.value("met:layerId").toInt(&ok);
        if (!ok) {
          *error = QString("failed to parse met:layerId as integer: %1").arg(pmExtData.value("met:layerId"));
          return QList<QSharedPointer<EditItems::Layer> >();
        }
        ditem->setProperty("layerId", layerId);
      }

      QMap<QString, QDomElement> ancElems;
      findAncestorElements(coordsNode, &ancElems, error);
      if (!error->isEmpty())
        return QList<QSharedPointer<EditItems::Layer> >();

      if (ancElems.contains("Placemark")) {
        ditem->setProperty("Placemark:name", getName(ancElems.value("Placemark"), error));
        if (!error->isEmpty())
          return QList<QSharedPointer<EditItems::Layer> >();
      } else {
        *error = "found <coordinates> element outside a <Placemark> element";
        return QList<QSharedPointer<EditItems::Layer> >();
      }

      if (ancElems.contains("Folder")) {
        ditem->setProperty("Folder:name", getName(ancElems.value("Folder"), error));
        if (!error->isEmpty())
          return QList<QSharedPointer<EditItems::Layer> >();

          QPair<QString, QString> timeSpan = getTimeSpan(ancElems.value("Folder"), error);
          if (!error->isEmpty())
            return QList<QSharedPointer<EditItems::Layer> >();
          ditem->setProperty("TimeSpan:begin", timeSpan.first);
          ditem->setProperty("TimeSpan:end", timeSpan.second);
      }

    } else {
      *error = "found <coordinates> element not associated with a met:groupId";
      return QList<QSharedPointer<EditItems::Layer> >();
    }
  }


  // *** PHASE 2: extract layer structure (if any)

  QMap<int, QSharedPointer<EditItems::Layer> > idToLayer;
  QDomNodeList docNodes = doc.elementsByTagName("Document");
  if (docNodes.size() != 1) {
    *error = QString("number of <Document> elements != 1: %1").arg(docNodes.size());
    return QList<QSharedPointer<EditItems::Layer> >();
  }

  // expect to find layer information in the <ExtendedData> element directly under the <Document> element
  const QHash<QString, QString> docExtData = getExtendedData(docNodes.item(0), "Document");

  // read layer-specific key-value pairs in any order
  QRegExp rx("^met:layer:(\\d+):(name|visible)$");
  QHash<QString, int> nameToId; // to ensure unique layer names
  foreach (QString key, docExtData.keys()) {
    if (rx.indexIn(key) >= 0) {
      bool ok;
      const int id = rx.cap(1).toInt(&ok);
      if (!ok) {
        *error = QString("failed to extract layer ID as int: %1").arg(rx.cap(1));
        return QList<QSharedPointer<EditItems::Layer> >();
      }
      if (!idToLayer.contains(id))
        idToLayer.insert(id, QSharedPointer<EditItems::Layer>(new EditItems::Layer));
      QSharedPointer<EditItems::Layer> layer = idToLayer.value(id);
      if (rx.cap(2) == "name") {
        // register the layer name
        const QString name = docExtData.value(key).trimmed();
        if (name.isEmpty()) {
          *error = QString("empty layer name found for key %1").arg(key);
          return QList<QSharedPointer<EditItems::Layer> >();
        }
        if (nameToId.contains(name)) {
          *error = QString("same name for layers %1 and %2: %3").arg(nameToId.value(name)).arg(id).arg(name);
          return QList<QSharedPointer<EditItems::Layer> >();
        }
        nameToId.insert(name, id);
        layer->setName(name);
      } else if (rx.cap(2) == "visible") {
        // register the layer visibility
        const bool visible = ((QStringList() << "" << "0" << "false" << "off" << "no").indexOf(docExtData.value(key).trimmed().toLower()) == -1);
        layer->setVisible(visible);
      }
    }
  }

  // ... validate the ID-sequence
  for (int i = 0; i < idToLayer.size(); ++i) {
    if (!idToLayer.contains(i)) {
      *error = QString("missing layer with ID %1").arg(i);
      return QList<QSharedPointer<EditItems::Layer> >();
    }
  }

  // ... copy layers to list
  QList<QSharedPointer<EditItems::Layer> > layers;
  const int nlayers = idToLayer.size();
  for (int i = 0; i < nlayers; ++i)
    layers.append(idToLayer.value(i));


  // *** PHASE 3: insert items into layers

  QSharedPointer<EditItems::Layer> defaultLayer;
  foreach (QSharedPointer<DrawingItemBase> item, items) {
    bool ok;
    const int layerId = item->propertiesRef().value("layerId").toInt(&ok);
    if (ok) {
      // item has layer ID, so insert in one of the layers
      if ((layerId < 0) || (layerId >= layers.size())) {
        *error = QString("item with layer ID outside valid range ([0, %1]): %2").arg(layers.size() - 1).arg(layerId);
        return QList<QSharedPointer<EditItems::Layer> >();
      }
      layers.at(layerId)->itemsRef().insert(item);
    } else {
      // item does not have a layer ID, so insert in default layer
      if (defaultLayer.isNull())
        // create default layer with empty name (note: none of the other layers may have an empty name)
        defaultLayer = QSharedPointer<EditItems::Layer>(new EditItems::Layer);
      defaultLayer->itemsRef().insert(item);
    }
  }

  if (!defaultLayer.isNull())
    layers.prepend(defaultLayer);

  return layers;
}

// Returns a list of item layers extracted from \a fileName.
// Upon success, the function returns a non-empty list of item layers and leaves \a error empty.
// Otherwise, the function returns an empty list of item layers and a failure reason in \a error.
// If the file contains no layer information, the items are returned in a single layer with default properties.
template<typename BaseType, typename PolyLineType, typename SymbolType,
         typename TextType, typename CompositeType>
static inline QList<QSharedPointer<EditItems::Layer> > createFromFile(const QString &fileName, QString *error)
{
  *error = QString();

  // load schema
  QXmlSchema schema;
  if (!loadSchema(schema, error)) {
    *error = QString("failed to load KML schema: %1").arg(*error);
    return QList<QSharedPointer<EditItems::Layer> >();
  }

  // load data
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    *error = QString("failed to open file %1 for reading").arg(fileName);
    return QList<QSharedPointer<EditItems::Layer> >();
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
      return QList<QSharedPointer<EditItems::Layer> >();
    }

    doc = createDomDocument(data, schema, docUri, error);
    if (doc.isNull()) {
      *error = QString("failed to create DOM document after successfully converting from old format: %1").arg(*error);
      return QList<QSharedPointer<EditItems::Layer> >();
    }
  }

  // at this point, a document is successfully created from either the new or the old format

  // parse document and create items
  return createFromDomDocument<BaseType, PolyLineType, SymbolType, TextType, CompositeType>(doc, error);
}

} // namespace

#endif // KML_H
