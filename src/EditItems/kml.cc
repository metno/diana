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
#include "drawingpolyline.h"
#include "drawingsymbol.h"
#include <cmath>
#include <QAbstractMessageHandler>
#include <QXmlSchemaValidator>
#include <QRegExp>
#include <QDateTime>

namespace KML {

// Finalizes KML document \a doc and returns a textual representation of it.
static QByteArray createKMLText(QDomDocument &doc, const QDomDocumentFragment &itemsFrag)
{
  // add <kml> root element
  QDomElement kmlElem = doc.createElement("kml");
  kmlElem.setAttribute("xmlns", "http://www.opengis.net/kml/2.2"); // required to match schema
  doc.appendChild(kmlElem);

  // add <Document> element
  QDomElement docElem = doc.createElement("Document");
  kmlElem.appendChild(docElem);

  // NOTE: We don't support styling for now, so styling elements will not be written

  // compress itemsFrag if necessary (so represent identical <Folder> elements as one <Folder> element etc.) ... TBD

  // add structures of individual items
  docElem.appendChild(itemsFrag);

  QByteArray kml;
  kml.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n"); // XML declaration
  kml.append(doc.toByteArray(2)); // DOM structure

  return kml;
}


// Saves selected items \a selItems to \a fileName.
//
// NOTE: All members of the selected groups (i.e. groups represented by at least one selected item) will be saved.
// (A group is a set of items with a common value for the "groupId" property.)
//
// The function leaves \a error empty iff it succeeds.
void saveToFile(const QString &fileName, const QSet<QSharedPointer<DrawingItemBase> > &items, const QSet<QSharedPointer<DrawingItemBase> > &selItems, QString *error)
{
  Q_ASSERT(false);

  *error = QString(); // ### use error in this function instead of all the Q_ASSERTs!
  QDomDocument doc;

  // find selected groups (groups for which at least one member is in \a selItems)
  QSet<int> selGroups;
  foreach (const QSharedPointer<DrawingItemBase> selItem, selItems)
    selGroups.insert(selItem->groupId());

  // document fragment to keep structures of individual items:
  QDomDocumentFragment itemsFrag = doc.createDocumentFragment();

  // loop over \a items and insert the KML for each item that is a member of a selected group
  foreach (const QSharedPointer<DrawingItemBase> item, items) {
    if (!selGroups.contains(item->groupId()))
      continue; // skip this item since its group is not represented among the selected items

    // append structure of this item:
    itemsFrag.appendChild(item->toKML());
  }

  const QByteArray kmlText = createKMLText(doc, itemsFrag);

  // save KML text to file
  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      *error = QString("failed to open %1 for writing").arg(fileName);
      return;
  }
  if (file.write(kmlText) == -1) {
    *error = QString("failed to write to %1: %2").arg(fileName).arg(file.errorString());
    return;
  }
  file.close();
}


// Returns the met:groupId value associated with \a node. Sets \a found to true iff a value is found.
// Leaves \a error empty iff no errors occurs.
int findGroupId(const QDomNode &node, bool &found, QString *error)
{
  QHash<QString, QString> extdata = getExtendedData(node, "Placemark");
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

// Returns any extended data map located as a child of the nearest node
// with tag \a parentTag along the ancestor chain from (and including) \a node.
QHash<QString, QString> getExtendedData(const QDomNode &node, const QString &parentTag)
{
  QHash<QString, QString> extData;

  for (QDomNode n = node; !n.isNull(); n = n.parentNode()) {
    const QDomElement e = n.toElement();
    if ((!e.isNull()) && (e.tagName() == parentTag)) {
      const QDomElement extDataElem = n.firstChildElement("ExtendedData");
      if (!extDataElem.isNull()) {
        const QDomNodeList dataNodes = extDataElem.elementsByTagName("Data");
        for (int i = 0; i < dataNodes.size(); ++i) {
          const QDomElement dataElem = dataNodes.item(i).toElement();
          const QDomNodeList valueNodes = dataElem.elementsByTagName("value");
          for (int j = 0; j < valueNodes.size(); ++j) {
            const QDomElement valueElem = valueNodes.item(j).toElement();
            const QString value = valueElem.firstChild().nodeValue();
            extData[dataElem.attribute("name").trimmed()] = value;
          }
        }
      }
      break; // at nearest matching parent node, so stop searching
    }
  }

  return extData;
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

// Attempts to load \a fileName as a valid XML schema.
// Returns true upon success, or false and a reason in \a error upon failure.
static bool loadSchemaFromFile(QXmlSchema &schema, const QString &fileName, QString *error)
{
  *error = QString();

  QUrl schemaUrl(QUrl(QString("file://%1").arg(fileName)));
  if (!schemaUrl.isValid()) {
    *error = QString("invalid schema: %1, reason: %2").arg(schemaUrl.path()).arg(schemaUrl.errorString());
    return false;
  }

  MessageHandler msgHandler;
  schema.setMessageHandler(&msgHandler);
  msgHandler.reset();
  if (!schema.load(schemaUrl)) {
    Q_ASSERT(!schema.isValid());
    *error = QString("failed to load schema: %1, reason: %2").arg(schemaUrl.path()).arg(msgHandler.lastMessage());
    return false;
  }

  Q_ASSERT(schema.isValid());
  return true;
}

// Loads the KML schema from pre-defined candidate files in prioritized order.
// Upon success, the function returns true and a valid schema in \a schema.
// Otherwise, the function returns false and the failure reason for each candidate file in \a error.
bool loadSchema(QXmlSchema &schema, QString *error)
{
  *error = QString();

  // open the schema file from a list of candidates in prioritized order
  const QString schemaBaseFileName("ogckml22.xsd");
  QStringList candSchemaFileNames;
  candSchemaFileNames.append(QString("/usr/share/diana/%1/%2").arg(PVERSION_MAJOR_DOT_MINOR).arg(schemaBaseFileName));
  candSchemaFileNames.append(QString("%1/%2").arg(qgetenv("KMLSCHEMADIR").constData()).arg(schemaBaseFileName));
  QStringList candSchemaErrors;
  int i = 0;
  foreach (const QString schemaFileName, candSchemaFileNames) {
    i++;
    QString schemaLoadError;
    if (loadSchemaFromFile(schema, schemaFileName, &schemaLoadError)) {
      Q_ASSERT(schema.isValid());
      Q_ASSERT(schemaLoadError.isEmpty());
      break;
    }
    Q_ASSERT(!schema.isValid());
    Q_ASSERT(!schemaLoadError.isEmpty());
    candSchemaErrors.append(QString("error opening schema from candidate file %1:%2 (%3): %4")
                            .arg(i).arg(candSchemaFileNames.size()).arg(schemaFileName).arg(schemaLoadError));
  }

  if (!schema.isValid()) {
    *error = QString(candSchemaErrors.join(", "));
    return false;
  }

  return true;
}

// Creates a DOM document from the KML structure in \a data, validating against \a schema.
// Returns a non-null document upon success, or a null document and a failure reason in \a error upon failure.
QDomDocument createDomDocument(const QByteArray &data, const QXmlSchema &schema, const QUrl &docUri, QString *error)
{
  Q_ASSERT(schema.isValid());
  *error = QString();
  MessageHandler msgHandler;

  // validate against schema
  QXmlSchemaValidator validator(schema);
  validator.setMessageHandler(&msgHandler);
  msgHandler.reset();
  if (!validator.validate(data, docUri)) {
    *error = QString("failed to validate against schema: %1, reason: %2").arg(schema.documentUri().path()).arg(msgHandler.lastMessage());
    return QDomDocument();
  }

  // create DOM document
  int line;
  int col;
  QString err;
  QDomDocument doc;
  if (doc.setContent(data, &err, &line, &col) == false) {
    *error = QString("parse error at line %1, column %2: %3").arg(line).arg(col).arg(err);
    return QDomDocument();
  }

  Q_ASSERT(!doc.isNull());
  return doc;
}

// Returns a property map from \a s assumed to be in old format.
// Sets \a error to a non-empty reason iff a failure occurs.
static QMap<QString, QString> extractOldProperties(const QString &s, QString *error)
{
  *error = QString();

  QRegExp rx("^([^=;]+)=([^=;]*);");
  int pos = 0;
  QMap<QString, QString> props;

  while (true) {
    int pos2;
    if ((pos2 = rx.indexIn(s.mid(pos))) >= 0) {
      props.insert(rx.cap(1).trimmed(), rx.cap(2).trimmed());
      pos += (pos2 + rx.matchedLength());
    } else {
      break; // property list exhausted
    }
  }

  return props;
}

// Upon success, the function returns a non-empty list of (lat,lon) points from a text of (lon,lat) points (note the order!).
// Otherwise, the function puts a reason in \a error and returns an empty list.
static QList<QPointF> getOldLatLonPoints(const QString &lonLatPoints, QString *error)
{
  *error = QString();
  QList<QPointF> points;

  QStringList plist = lonLatPoints.split(QRegExp("[,\\s]+"), QString::SkipEmptyParts);
  if (plist.isEmpty()) {
    *error = QString("no points found: %1").arg(lonLatPoints);
    return QList<QPointF>();
  }
  if (plist.size() % 2) {
    *error = QString("found %1 points, which is not an even number: %2").arg(plist.size()).arg(lonLatPoints);
    return QList<QPointF>();
  }

  bool ok;
  for (int i = 0; i < plist.size() / 2; ++i) {
    const qreal lon = plist.at(2 * i).toDouble(&ok);
    if (!ok) {
      *error = QString("failed to extract longitude component of point %1: %2").arg(i).arg(plist.at(2 * i));
      return QList<QPointF>();
    }
    const qreal lat = plist.at(2 * i + 1).toDouble(&ok);
    if (!ok) {
      *error = QString("failed to extract latitude component of point %1: %2").arg(i).arg(plist.at(2 * i + 1));
      return QList<QPointF>();
    }

    points.append(QPointF(lat, lon)); // note order
  }

  return points;
}

// Creates a DrawingItemBase item of the right type from \a props.
// Upon success, the function leaves \a error empty and returns the new item wrapped in a shared pointer.
// Upon failure, the function puts a non-empty failure reason in \a error, but may still return an item (invalid in that case).
QSharedPointer<DrawingItemBase> createItemFromOldProperties(QMap<QString, QString> props, QString *error)
{
  *error = QString();
  DrawingItemBase *item = 0;

  // ensure that mandatory properties exist:
  if (!props.contains("Object")) {
    *error = "no 'Object' property found";
    return QSharedPointer<DrawingItemBase>();
  }
  if (!props.contains("LongitudeLatitude")) {
    *error = "no 'LongitudeLatitude' property found";
    return QSharedPointer<DrawingItemBase>();
  }

  // extract geographic point(s):
  const QList<QPointF> points = getOldLatLonPoints(props.value("LongitudeLatitude"), error);
  if (points.isEmpty()) {
    *error = QString("failed to extract point(s): %1").arg(*error);
    return QSharedPointer<DrawingItemBase>();
  }
  if ((props.value("Object") == "Symbol") && (points.size() != 1)) {
    *error = QString("invalid number of points for symbol: %1 (expected 1)").arg(points.size());
    return QSharedPointer<DrawingItemBase>();
  } else if ((props.value("Object") != "Symbol") && (points.size() < 2)) {
    *error = QString("invalid number of points for non-symbol: %1 (expected at least 2)").arg(points.size());
    return QSharedPointer<DrawingItemBase>();
  }

  // create item based on value of 'Object' property:
  if (props.value("Object") == "Symbol")
    item = new DrawingItem_Symbol::Symbol;
  else
    item = new DrawingItem_PolyLine::PolyLine;

  // set geographic point(s):
  item->setLatLonPoints(points);

  // set style properties ...
  QHash<QString, QString> styleProps;
  if (props.contains("Type"))
    styleProps.insert("type", "Custom"); // ### for now
  if (props.contains("LineWidth"))
    styleProps.insert("linewidth", props.value("LineWidth"));
  if (props.contains("RGBA"))
    styleProps.insert("linecolour", QString(props.value("RGBA")).replace(',', ':'));
  DrawingStyleManager::instance()->setStyle(Drawing(item), styleProps);

  return QSharedPointer<DrawingItemBase>(item);
}

// Converts \a data from old format to new KML format. Upon success, the function replaces \a data with the converted data and returns true.
// Otherwise, the function leaves \a data unchanged, passes a failure reason in \a error, and returns false.
bool convertFromOldFormat(QByteArray &data, QString *error)
{
  *error = QString();
  const QString in(data);
  QRegExp rx;
  int pos = 0;

  // read date
  rx = QRegExp("^\\s*Date\\s*=\\s*(\\d+);");
  QDateTime dt;
  if ((pos = rx.indexIn(in.mid(pos))) >= 0) {
    dt = QDateTime::fromString(rx.cap(1), "yyyyMMddhhmm");
    pos += rx.matchedLength();
  } else {
    *error = "failed to extract date";
    return false;
  }
  // ### dt unused for now

  QDomDocument doc;

  // document fragment to keep structures of individual items:
  QDomDocumentFragment itemsFrag = doc.createDocumentFragment();

  // read objects
  rx = QRegExp("^\\s*([^!]+)!");
  while (true) {
    int pos2;
    if ((pos2 = rx.indexIn(in.mid(pos))) >= 0) {
      QMap<QString, QString> props = extractOldProperties(rx.cap(1), error);
      if (!error->isEmpty()) {
        *error = QString("failed to extract object properties in old format: %1").arg(*error);
        return false;
      }

      const QSharedPointer<DrawingItemBase> item = createItemFromOldProperties(props, error);
      if (!error->isEmpty()) {
        *error = QString("failed to create tmp item from properties in old format: %1").arg(*error);
        return false;
      }

      // append structure of this item:
      itemsFrag.appendChild(item->toKML());

      pos += (pos2 + rx.matchedLength());
    } else {
      break; // object list exhausted
    }
  }

  data = createKMLText(doc, itemsFrag);

  return true;
}

} // namespace
