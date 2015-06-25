/*
  Diana - A Free Meteorological Visualisation Tool

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

// Needed for construction of version-specific paths:
#include "config.h"

#include "kml.h"
#include "drawingitembase.h"
#include "drawingpolyline.h"
#include "drawingsymbol.h"
#include "drawingcomposite.h"
#include <diPlotModule.h>
#include <cmath>
#include <QAbstractMessageHandler>
#include <QXmlSchemaValidator>
#include <QRegExp>
#include <QDateTime>

namespace KML {

// Finalizes KML document \a doc and returns a textual representation of it.
static QByteArray createKMLText(QDomDocument &doc, const QDomDocumentFragment &innerStruct)
{
  // add <kml> root element
  QDomElement kmlElem = doc.createElement("kml");
  kmlElem.setAttribute("xmlns", "http://www.opengis.net/kml/2.2"); // required to match schema
  doc.appendChild(kmlElem);

  // add <Document> element
  QDomElement docElem = doc.createElement("Document");
  kmlElem.appendChild(docElem);

  // compress innerStruct if necessary (so represent identical <Folder> elements as one <Folder> element for items etc.) ... TBD

  // add inner structure
  docElem.appendChild(innerStruct);

  QByteArray kml;
  kml.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n"); // XML declaration
  kml.append(doc.toByteArray(2)); // DOM structure

  return kml;
}

// Saves a list of \a items to \a fileName.
// The function leaves \a error empty iff it succeeds.
QString saveItemsToFile(const QList<DrawingItemBase *> &items, const QString &fileName)
{
  QDomDocument doc;

  QDomDocumentFragment innerStruct = doc.createDocumentFragment();

  // insert items
  foreach (const DrawingItemBase *item, items) {
    QHash<QString, QString> extraExtData;
    innerStruct.appendChild(item->toKML(extraExtData));
  }

  const QByteArray kmlText = createKMLText(doc, innerStruct);

  // save KML text to file
  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      return QString("failed to open %1 for writing").arg(fileName);
  }
  if (file.write(kmlText) == -1) {
    return QString("failed to write to %1: %2").arg(fileName).arg(file.errorString());
  }
  file.close();

  return QString();
}

QDomElement createExtDataDataElement(QDomDocument &doc, const QString &name, const QString &value)
{
  QDomElement valueElem = doc.createElement("value");
  valueElem.appendChild(doc.createTextNode(value));
  QDomElement dataElem = doc.createElement("Data");
  dataElem.setAttribute("name", name);
  dataElem.appendChild(valueElem);
  return dataElem;
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
QList<QPointF> getPoints(const QDomNode &coordsNode, QString &error)
{
  const QString coords = coordsNode.firstChild().nodeValue();
  QList<QPointF> points;

  foreach (QString coord, coords.split(QRegExp("\\s+"), QString::SkipEmptyParts)) {
    const QStringList coordComps = coord.split(",", QString::SkipEmptyParts);
    if (coordComps.size() < 2) {
      error = QString("expected at least two components (i.e. lat, lon) in coordinate, found %1: %2")
          .arg(coordComps.size()).arg(coord);
      return QList<QPointF>();
    }
    bool ok;
    const double lon = coordComps.at(0).toDouble(&ok);
    if (!ok) {
      error = QString("failed to convert longitude string to double value: %1").arg(coordComps.at(0));
      return QList<QPointF>();
    }
    const double lat = coordComps.at(1).toDouble(&ok);
    if (!ok) {
      error = QString("failed to convert latitude string to double value: %1").arg(coordComps.at(1));
      return QList<QPointF>();
    }
    points.append(QPointF(lat, lon)); // note lat,lon order
  }

  return points;
}

// Fills \a ancElems in with all ancestor elements of \a node. Assumes (and checks for) uniqueness
// of ancestor tag names. Leaves \a error empty iff no error occurs.
void findAncestorElements(const QDomNode &node, QMap<QString, QDomElement> *ancElems, QString &error)
{
  ancElems->clear();
  error = QString();
  if (node.isNull())
    return;

  for (QDomNode n = node.parentNode(); !n.isNull(); n = n.parentNode()) {
    const QDomElement elem = n.toElement();
    if (!elem.isNull())
      ancElems->insert(elem.tagName(), elem);
  }
}

// Returns the string contained in the \a child of \a elem.
// Leaves \a error empty iff no error occurs (such as when \a elem doesn't have a <name> child!).
QString getChildText(const QDomElement &elem, const QString &child, QString &error)
{
  Q_ASSERT(!elem.isNull());
  error = QString();
  const QDomElement childElem = elem.firstChildElement(child);
  if (childElem.isNull()) {
    error = QString("element <%1> contains no <name> child").arg(elem.tagName());
    return "";
  }
  return childElem.firstChild().nodeValue();
}

// Returns the pair of strings that represents begin- and end time contained in the <TimeSpan> child of \a elem.
// Leaves \a error empty iff no error occurs (such as when \a elem doesn't have a <TimeSpan> child!).
QPair<QString, QString> getTimeSpan(const QDomElement &elem, QString &error)
{
  Q_ASSERT(!elem.isNull());
  error = QString();

  const QDomElement timeSpanElem = elem.firstChildElement("TimeSpan");
  if (timeSpanElem.isNull()) {
    error = QString("element <%1> contains no <TimeSpan> child").arg(elem.tagName());
    return QPair<QString, QString>();
  }

  const QDomElement beginTimeElem = timeSpanElem.firstChildElement("begin");
  if (beginTimeElem.isNull()) {
    error = "time span contains no begin time";
    return QPair<QString, QString>();
  }
  //
  const QDomElement endTimeElem = timeSpanElem.firstChildElement("end");
  if (endTimeElem.isNull()) {
    error = "time span contains no end time";
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
static bool loadSchemaFromFile(QXmlSchema &schema, const QString &fileName, QString &error)
{
  error = QString();

  QUrl schemaUrl(QUrl(QString("file://%1").arg(fileName)));
  if (!schemaUrl.isValid()) {
    error = QString("invalid schema: %1, reason: %2").arg(schemaUrl.path()).arg(schemaUrl.errorString());
    return false;
  }

  MessageHandler msgHandler;
  schema.setMessageHandler(&msgHandler);
  msgHandler.reset();
  if (!schema.load(schemaUrl)) {
    Q_ASSERT(!schema.isValid());
    error = QString("failed to load schema: %1, reason: %2").arg(schemaUrl.path()).arg(msgHandler.lastMessage());
    return false;
  }

  Q_ASSERT(schema.isValid());
  return true;
}

// Loads the KML schema from pre-defined candidate files in prioritized order.
// Upon success, the function returns true and a valid schema in \a schema.
// Otherwise, the function returns false and the failure reason for each candidate file in \a error.
bool loadSchema(QXmlSchema &schema, QString &error)
{
  error = QString();

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
    if (loadSchemaFromFile(schema, schemaFileName, schemaLoadError)) {
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
    error = QString(candSchemaErrors.join(", "));
    return false;
  }

  return true;
}

// Creates a DOM document from the KML structure in \a data, validating against \a schema.
// Returns a non-null document upon success, or a null document and a failure reason in \a error upon failure.
QDomDocument createDomDocument(const QByteArray &data, const QXmlSchema &schema, const QUrl &docUri, QString &error)
{
  Q_ASSERT(schema.isValid());
  error = QString();
  MessageHandler msgHandler;

  // validate against schema
  QXmlSchemaValidator validator(schema);
  validator.setMessageHandler(&msgHandler);
  msgHandler.reset();
  if (!validator.validate(data, docUri)) {
    error = QString("failed to validate against schema: %1, reason: %2").arg(schema.documentUri().path()).arg(msgHandler.lastMessage());
    return QDomDocument();
  }

  // create DOM document
  int line;
  int col;
  QString err;
  QDomDocument doc;
  if (doc.setContent(data, &err, &line, &col) == false) {
    error = QString("parse error at line %1, column %2: %3").arg(line).arg(col).arg(err);
    return QDomDocument();
  }

  Q_ASSERT(!doc.isNull());
  return doc;
}

// Returns a property map from \a s assumed to be in old format.
// Sets \a error to a non-empty reason iff a failure occurs.
static QMap<QString, QString> extractOldProperties(const QString &s, QString &error)
{
  error = QString();

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
static QList<QPointF> getOldLatLonPoints(const QString &lonLatPoints, QString &error)
{
  error = QString();
  QList<QPointF> points;

  QStringList plist = lonLatPoints.split(QRegExp("[,\\s]+"), QString::SkipEmptyParts);
  if (plist.isEmpty()) {
    error = QString("no points found: %1").arg(lonLatPoints);
    return QList<QPointF>();
  }
  if (plist.size() % 2) {
    error = QString("found %1 points, which is not an even number: %2").arg(plist.size()).arg(lonLatPoints);
    return QList<QPointF>();
  }

  bool ok;
  for (int i = 0; i < plist.size() / 2; ++i) {
    const qreal lon = plist.at(2 * i).toDouble(&ok);
    if (!ok) {
      error = QString("failed to extract longitude component of point %1: %2").arg(i).arg(plist.at(2 * i));
      return QList<QPointF>();
    }
    const qreal lat = plist.at(2 * i + 1).toDouble(&ok);
    if (!ok) {
      error = QString("failed to extract latitude component of point %1: %2").arg(i).arg(plist.at(2 * i + 1));
      return QList<QPointF>();
    }

    points.append(QPointF(lat, lon)); // note order
  }

  return points;
}

// Creates a DrawingItemBase item of the right type from \a props.
// Upon success, the function leaves \a error empty and returns a pointer to the new item.
// Upon failure, the function puts a non-empty failure reason in \a error and returns 0.
DrawingItemBase *createItemFromOldProperties(QMap<QString, QString> props, QString &error)
{
  error = QString();

  // ensure that mandatory properties exist:
  if (!props.contains("Object")) {
    error = "no 'Object' property found";
    return 0;
  }
  if (!props.contains("LongitudeLatitude")) {
    error = "no 'LongitudeLatitude' property found";
    return 0;
  }

  // extract geographic point(s):
  const QList<QPointF> points = getOldLatLonPoints(props.value("LongitudeLatitude"), error);
  if (points.isEmpty()) {
    error = QString("failed to extract point(s): %1").arg(error);
    return 0;
  }
  if ((props.value("Object") == "Symbol") && (points.size() != 1)) {
    error = QString("invalid number of points for symbol: %1 (expected 1)").arg(points.size());
    return 0;
  } else if ((props.value("Object") != "Symbol") && (points.size() < 2)) {
    error = QString("invalid number of points for non-symbol: %1 (expected at least 2)").arg(points.size());
    return 0;
  }

  DrawingItemBase *item = 0;

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
    styleProps.insert("type", "Default"); // ### for now
  if (props.contains("LineWidth"))
    styleProps.insert("linewidth", props.value("LineWidth"));
  if (props.contains("RGBA"))
    styleProps.insert("linecolour", QString(props.value("RGBA")).replace(',', ':'));
  DrawingStyleManager::instance()->setStyle(Drawing(item), styleProps);

  return item;
}

// Converts \a data from old format to new KML format. Upon success, the function replaces \a data with the converted data and returns true.
// Otherwise, the function leaves \a data unchanged, passes a failure reason in \a error, and returns false.
bool convertFromOldFormat(QByteArray &data, QString &error)
{
  error = QString();
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
    error = "failed to extract date";
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
      if (!error.isEmpty()) {
        error = QString("failed to extract object properties in old format: %1").arg(error);
        return false;
      }

      const QSharedPointer<DrawingItemBase> item(createItemFromOldProperties(props, error));
      if (!error.isEmpty()) {
        error = QString("failed to create tmp item from properties in old format: %1").arg(error);
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

/**
 * Returns a list of items extracted from the given DOM document \a doc
 * originally loaded as a product with the given \a name from source file
 * \a srcFileName.
 * Upon success, the function returns a non-empty list of items and leaves
 * \a error empty. Upon failure, the function returns an empty list of items
 * and a failure reason in \a error.
 */
QList<DrawingItemBase *> createFromDomDocument(const QDomDocument &doc, const QString &name,
                                               const QString &srcFileName, QString &error)
{
  QList<DrawingItemBase *> items;
  error = QString();

  // *** PHASE 1: extract items

  // loop over <coordinates> elements
  QDomNodeList coordsNodes = doc.elementsByTagName("coordinates");
  for (int i = 0; i < coordsNodes.size(); ++i) {
    const QDomNode coordsNode = coordsNodes.item(i);
    // create item
    const QList<QPointF> points = getPoints(coordsNode, error);
    if (!error.isEmpty())
      return QList<DrawingItemBase *>();

    // Find the extended data associated with the coordinates.

    const QHash<QString, QString> pmExtData = getExtendedData(coordsNode, "Placemark");
    if (pmExtData.isEmpty()) {
      error = "<Placemark> element without <ExtendedData> element found";
      return QList<DrawingItemBase *>();
    }

    // Create a suitable item for this KML structure.
    QString objectType = pmExtData.value("met:objectType");

    // Try to create an editable item using the Edit Item Manager. If that
    // fails, fall back on the Drawing Manager to create display items.
    DrawingItemBase *itemObj;
    DrawingManager *manager = dynamic_cast<DrawingManager *>(PlotModule::instance()->getManager("EDITDRAWING"));
    if (manager)
      itemObj = manager->createItem(objectType);
    else
      itemObj = DrawingManager::instance()->createItem(objectType);

    if (!itemObj) {
      error = QString("unknown element found");
      return QList<DrawingItemBase *>();
    }

    if (objectType == "Composite") {
      Drawing(itemObj)->setProperty("style:type", pmExtData.value("met:style:type"));
      static_cast<DrawingItem_Composite::Composite *>(itemObj)->createElements();
    }

    // Initialise the geographic position and read the extended data for the item.
    itemObj->setLatLonPoints(points);
    itemObj->fromKML(pmExtData);

    items.append(itemObj);
    DrawingItemBase *ditem = Drawing(itemObj);

    DrawingStyleManager::instance()->setStyle(ditem, pmExtData, "met:style:");

    ditem->setProperty("product", name);
    ditem->setProperty("srcFile", srcFileName);

    // Keep all the met: properties, treating the joinId property specially.
    QHashIterator<QString, QString> it(pmExtData);
    while (it.hasNext()) {

      it.next();

      if (it.key() == "met:joinId") {
        bool ok;
        const int joinId = it.value().toInt(&ok);
        if (!ok) {
          error = QString("failed to parse met:joinId as integer: %1").arg(it.value());
          return QList<DrawingItemBase *>();
        }
        ditem->setProperty("joinId", joinId);

      } else if (it.key() == "met:text") {
        ;

      } else if (it.key().startsWith("met:"))
        ditem->setProperty(it.key(), it.value());
    }

    QMap<QString, QDomElement> ancElems;
    findAncestorElements(coordsNode, &ancElems, error);
    if (!error.isEmpty())
      return QList<DrawingItemBase *>();

    if (ancElems.contains("Placemark")) {
      ditem->setProperty("Placemark:name", getChildText(ancElems.value("Placemark"), "name", error));
      if (!error.isEmpty())
        return QList<DrawingItemBase *>();

      QString warning;

      // Optionally obtain and use TimeSpan elements.
      ditem->setProperty("Placemark:description", getChildText(ancElems.value("Placemark"), "description", warning));

      // Optionally obtain and use TimeSpan elements.
      QPair<QString, QString> timeSpan = getTimeSpan(ancElems.value("Placemark"), warning);
      if (warning.isEmpty()) {
        ditem->setProperty("TimeSpan:begin", timeSpan.first);
        ditem->setProperty("TimeSpan:end", timeSpan.second);
      }

    } else {
      error = "found <coordinates> element outside a <Placemark> element";
      return QList<DrawingItemBase *>();
    }

    if (ancElems.contains("Folder")) {
      ditem->setProperty("Folder:name", getChildText(ancElems.value("Folder"), "name", error));
      if (!error.isEmpty())
        return QList<DrawingItemBase *>();

      QPair<QString, QString> timeSpan = getTimeSpan(ancElems.value("Folder"), error);
      if (!error.isEmpty())
        return QList<DrawingItemBase *>();
      ditem->setProperty("TimeSpan:begin", timeSpan.first);
      ditem->setProperty("TimeSpan:end", timeSpan.second);
    }
  }

  return items;
}

/**
 * Returns a list of items extracted from \a fileName.
 * Upon success, the function returns a non-empty list of item layers and leaves \a error empty.
 * Otherwise, the function returns an empty list of item layers and a failure reason in \a error.
 */
QList<DrawingItemBase *> createFromFile(const QString &name, const QString &fileName, QString &error)
{
  error = QString();
  QList<DrawingItemBase *> items;

  // load schema
  QXmlSchema schema;
  if (!loadSchema(schema, error)) {
    error = QString("failed to load KML schema: %1").arg(error);
    return QList<DrawingItemBase *>();
  }

  // load data
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    error = QString("failed to open file %1 for reading").arg(fileName);
    return QList<DrawingItemBase *>();
  }
  QByteArray data = file.readAll();

  // create document from data validated against schema
  const QUrl docUri(QUrl::fromLocalFile(fileName));
  QDomDocument doc = createDomDocument(data, schema, docUri, error);
  if (doc.isNull()) {
    // assume that the failure was caused by data being in old format
    QString old2newError;
    if (!convertFromOldFormat(data, old2newError)) {
      error = QString("failed to create DOM document:<br/>%1<br/><br/>also failed to convert from old format:<br/>%2").arg(error).arg(old2newError);
      return QList<DrawingItemBase *>();
    }

    doc = createDomDocument(data, schema, docUri, error);
    if (doc.isNull()) {
      error = QString("failed to create DOM document after successfully converting from old format: %1").arg(error);
      return QList<DrawingItemBase *>();
    }
  }

  // at this point, a document is successfully created from either the new or the old format

  // parse document and create items
  items = createFromDomDocument(doc, name, fileName, error);

  return items;
}

} // namespace
