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

#include "kml.h"
#include "drawingitembase.h"
#include <cmath>

namespace KML {

void MessageHandler::handleMessage(QtMsgType type, const QString &descr, const QUrl &id, const QSourceLocation &srcLoc)
{
  n_++;
  type_ = type;
  descr_ = descr;
  id_ = id;
  srcLoc_ = srcLoc;
}

MessageHandler::MessageHandler() : n_(0) {}

void MessageHandler::reset() { n_ = 0; }

QString MessageHandler::lastMessage() const
{
  if (n_ == 0)
    return "<no message>";
  return QString("%1 (line %2, column %3)").arg(descr_).arg(srcLoc_.line()).arg(srcLoc_.column());
}

// Saves selected items \a selItems to \a fileName.
//
// NOTE: All members of the selected groups (i.e. groups represented by at least one selected item) will be saved.
// (A group is a set of items with a common value for the "groupId" property.)
//
// The function leaves \a error empty iff it succeeds.
void saveToFile(const QString &fileName, const QSet<DrawingItemBase *> &items, const QSet<DrawingItemBase *> &selItems, QString *error)
{
  Q_ASSERT(false);

  *error = QString(); // ### use error in this function instead of all the Q_ASSERTs!
  QDomDocument doc;

  // find selected groups (groups for which at least one member is in \a selItems)
  QSet<int> selGroups;
  foreach (const DrawingItemBase *selItem, selItems)
    selGroups.insert(selItem->groupId());

  // document fragment to keep structures of individual items:
  QDomDocumentFragment itemsFrag = doc.createDocumentFragment();

  // loop over \a items and insert the KML for each item that is a member of a selected group
  foreach (const DrawingItemBase *item, items) {
    if (!selGroups.contains(item->groupId()))
      continue; // skip this item since its group is not represented among the selected items

    // append structure of this item:
    itemsFrag.appendChild(item->toKML());
  }

  // add <kml> root element
  QDomElement kmlElem = doc.createElement("kml");
  kmlElem.setAttribute("xmlns", "http://www.opengis.net/kml/2.2"); // required to match schema
  doc.appendChild(kmlElem);

  // add <Document> element
  QDomElement docElem = doc.createElement("Document");
  kmlElem.appendChild(docElem);

  // NOTE: We don't support styling for now, so styling elements will not be written to the file!

  // compress itemsFrag if necessary (so represent identical <Folder> elements as one <Folder> element etc.) ... TBD

  // add structures of individual items
  docElem.appendChild(itemsFrag);

  // save DOM document to file
  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      *error = QString("failed to open %1 for writing").arg(fileName);
      return;
  }
  QByteArray final;
  final.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n"); // XML declaration
  final.append(doc.toByteArray(2)); // DOM structure
  if (file.write(final) == -1) {
    *error = QString("failed to write to %1: %2").arg(fileName).arg(file.errorString());
    return;
  }
  file.close();
}


// Returns the met:groupId value associated with \a node. Sets \a found to true iff a value is found.
// Leaves \a error empty iff no errors occurs.
int findGroupId(const QDomNode &node, bool &found, QString *error)
{
  QHash<QString, QString> extdata = getExtendedData(node);
  if (extdata.isEmpty() || !error->isEmpty()) {
    *error = QString("No extended data available");
    found = false;
    return -1;
  }

  bool ok;
  int groupId = extdata.value("met:groupId").toInt(&ok);
  if (!ok) {
    *error = QString("failed to extract met:groupId as integer: %1").arg(groupId);
    found = false;
    return -1;
  } else {
    found = true;
    return groupId;
  }
}

QHash<QString, QString> getExtendedData(const QDomNode &node)
{
  QHash<QString, QString> extdata;

  // find the first ancestor (including \a node itself) with met:groupId in an <ExtendedData> child:

  for (QDomNode n = node; !n.isNull(); n = n.parentNode()) {
    const QDomElement extDataElem = n.firstChildElement("ExtendedData");
    if (!extDataElem.isNull()) {
      const QDomNodeList dataNodes = extDataElem.elementsByTagName("Data");
      for (int i = 0; i < dataNodes.size(); ++i) {
        const QDomElement dataElem = dataNodes.item(i).toElement();
        const QDomNodeList valueNodes = dataElem.elementsByTagName("value");
        for (int j = 0; j < valueNodes.size(); ++j) {
          const QDomElement valueElem = valueNodes.item(j).toElement();
          const QString value = valueElem.firstChild().nodeValue();
          extdata[dataElem.attribute("name")] = value;
        }
      }
    }
  }

  return extdata;
}

// Returns the sequence of (lat, lon) points of a <cooordinates> element.
// Leaves \a error empty iff no errors occurs.
QList<QPointF> getPoints(const QDomNode &coordsNode, QString *error)
{
  const QString coords = coordsNode.firstChild().nodeValue();
  QList<QPointF> points;

  foreach (QString coord, coords.split(QRegExp("\\s+"), QString::SkipEmptyParts)) {
    const QStringList coordComps = coord.split(",", QString::SkipEmptyParts);
    if (coordComps.size() < 2) {
      *error = QString("expected at least two components (i.e. lat, lon) in coordinate, found %1: %2")
          .arg(coordComps.size()).arg(coord);
      return QList<QPointF>();
    }
    bool ok;
    const double lon = coordComps.at(0).toDouble(&ok);
    if (!ok) {
      *error = QString("failed to convert longitude string to double value: %1").arg(coordComps.at(0));
      return QList<QPointF>();
    }
    const double lat = coordComps.at(1).toDouble(&ok);
    if (!ok) {
      *error = QString("failed to convert latitude string to double value: %1").arg(coordComps.at(1));
      return QList<QPointF>();
    }
    points.append(QPointF(lat, lon)); // note lat,lon order
  }

  return points;
}

// Fills \a ancElems in with all ancestor elements of \a node. Assumes (and checks for) uniqueness
// of ancestor tag names. Leaves \a error empty iff no error occurs.
void findAncestorElements(const QDomNode &node, QMap<QString, QDomElement> *ancElems, QString *error)
{
  ancElems->clear();
  *error = QString();
  if (node.isNull())
    return;

  for (QDomNode n = node.parentNode(); !n.isNull(); n = n.parentNode()) {
    const QDomElement elem = n.toElement();
    if (!elem.isNull())
      ancElems->insert(elem.tagName(), elem);
  }
}

// Returns the string contained in the <name> child of \a elem.
// Leaves \a error empty iff no error occurs (such as when \a elem doesn't have a <name> child!).
QString getName(const QDomElement &elem, QString *error)
{
  Q_ASSERT(!elem.isNull());
  *error = QString();
  const QDomElement nameElem = elem.firstChildElement("name");
  if (nameElem.isNull()) {
    *error = QString("element <%1> contains no <name> child").arg(elem.tagName());
    return "";
  }
  return nameElem.firstChild().nodeValue();
}

// Returns the pair of strings that represents begin- and end time contained in the <TimeSpan> child of \a elem.
// Leaves \a error empty iff no error occurs (such as when \a elem doesn't have a <TimeSpan> child!).
QPair<QString, QString> getTimeSpan(const QDomElement &elem, QString *error)
{
  Q_ASSERT(!elem.isNull());
  *error = QString();

  const QDomElement timeSpanElem = elem.firstChildElement("TimeSpan");
  if (timeSpanElem.isNull()) {
    *error = QString("element <%1> contains no <TimeSpan> child").arg(elem.tagName());
    return QPair<QString, QString>();
  }

  const QDomElement beginTimeElem = timeSpanElem.firstChildElement("begin");
  if (beginTimeElem.isNull()) {
    *error = "time span contains no begin time";
    return QPair<QString, QString>();
  }
  //
  const QDomElement endTimeElem = timeSpanElem.firstChildElement("end");
  if (endTimeElem.isNull()) {
    *error = "time span contains no end time";
    return QPair<QString, QString>();
  }

  return qMakePair(beginTimeElem.firstChild().nodeValue(), endTimeElem.firstChild().nodeValue());
}

// Attempts to load \a fileName as a valid XML schema into \a schema.
// Returns true and leaves \a error empty upon success. Otherwise returns false and puts the reason in \a error.
bool loadSchema(const QString &fileName, QXmlSchema &schema, MessageHandler *msgHandler, QString *error)
{
  *error = QString();
  QUrl schemaUrl(QUrl(QString("file://%1").arg(fileName)));
  if (!schemaUrl.isValid()) {
    *error = QString("invalid schema: %1, reason: %2").arg(schemaUrl.path()).arg(schemaUrl.errorString());
    return false;
  }
  schema.setMessageHandler(msgHandler);
  msgHandler->reset();
  if (!schema.load(schemaUrl)) {
    *error = QString("failed to load schema: %1, reason: %2").arg(schemaUrl.path()).arg(msgHandler->lastMessage());
    return false;
  }

  return true;
}

} // namespace
