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

#include <QAbstractMessageHandler>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QMap>
#include <QDomElement>
#include <QXmlSchema>
#include <QXmlSchemaValidator>

class DrawingItemBase;

// API for saving/loading to/from KML files.
namespace KML {

class MessageHandler : public QAbstractMessageHandler
{
  virtual void handleMessage(QtMsgType, const QString &, const QUrl &, const QSourceLocation &);
  int n_;
  QtMsgType type_;
  QString descr_;
  QUrl id_;
  QSourceLocation srcLoc_;
public:
  MessageHandler();
  void reset();
  QString lastMessage() const;
};

void saveToFile(const QString &fileName, const QSet<DrawingItemBase *> &items, const QSet<DrawingItemBase *> &selItems, QString *error);

int findGroupId(const QDomNode &, bool &, QString *);

QList<QPointF> getPoints(const QDomNode &, QString *);

void findAncestorElements(const QDomNode &, QMap<QString, QDomElement> *, QString *);

QString getName(const QDomElement &, QString *);

QPair<QString, QString> getTimeSpan(const QDomElement &, QString *);

bool loadSchema(const QString &, QXmlSchema &, MessageHandler *, QString *);

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

// Returns a set of items extracted from \a fileName.
// The function leaves \a error empty iff it succeeds.
template<typename BaseType, typename PolyLineType, typename SymbolType>
static inline QSet<BaseType *> createFromFile(const QString &fileName, QString *error)
{
  *error = QString();
  QSet<BaseType *> items;
  QMap<int, int> finalGroupId;

  // open file
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    *error = QString("failed to open file %1 for reading").arg(fileName);
    return QSet<BaseType *>();
  }

  const QByteArray data = file.readAll();
  MessageHandler msgHandler;

  // open the schema file from a list of candidates in prioritized order
  const QString schemaBaseFileName("ogckml22.xsd");
  QStringList candSchemaFileNames;
  candSchemaFileNames.append(QString("/usr/share/diana/%1/%2").arg(PVERSION_MAJOR_DOT_MINOR).arg(schemaBaseFileName));
  candSchemaFileNames.append(QString("%1/%2").arg(qgetenv("KMLSCHEMADIR").constData()).arg(schemaBaseFileName));
  QXmlSchema schema;
  QStringList candSchemaErrors;
  foreach (const QString schemaFileName, candSchemaFileNames) {
    QString schemaLoadError;
    if (loadSchema(schemaFileName, schema, &msgHandler, &schemaLoadError)) {
      Q_ASSERT(schema.isValid());
      Q_ASSERT(schemaLoadError.isEmpty());
      break;
    }
    Q_ASSERT(!schema.isValid());
    Q_ASSERT(!schemaLoadError.isEmpty());
    candSchemaErrors.append(schemaLoadError);
  }
  if (!schema.isValid()) {
    *error = QString("failed to open a valid KML schema file among the candidates; reasons: %1").arg(candSchemaErrors.join(", "));
    return QSet<BaseType *>();
  }

  // validate against schema
  // Q_ASSERT(schema.isValid());
  QXmlSchemaValidator validator(schema);
  validator.setMessageHandler(&msgHandler);
  msgHandler.reset();
  if (!validator.validate(data, QUrl::fromLocalFile(fileName))) {
    *error = QString("failed to validate against schema: %1, reason: %2").arg(schema.documentUri().path()).arg(msgHandler.lastMessage());
    return QSet<BaseType *>();
  }

  // create DOM document
  int line;
  int col;
  QString err;
  QDomDocument doc;
  if (doc.setContent(data, &err, &line, &col) == false) {
    *error = QString("parse error at line %1, column %2: %3").arg(line).arg(col).arg(err);
    return QSet<BaseType *>();
  }

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
  }

  return items;
}

} // namespace

#endif // KML_H
