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

#ifndef KML_H
#define KML_H

#include <QSet>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QMap>
#include <QDomElement>
#include <QXmlSchema>
#include <QPointF>
#include <diDrawingManager.h>
#include <EditItems/drawingstylemanager.h>
#include <EditItems/drawingitembase.h>

// API for saving/loading to/from KML files.
namespace KML
{
  QString saveItemsToFile(const QList<DrawingItemBase *> &items, const QString &fileName);

  QDomElement createExtDataDataElement(QDomDocument &, const QString &, const QString &);
  QHash<QString, QString> getExtendedData(const QDomNode &, const QString &);

  QList<QPointF> getPoints(const QDomNode &, QString &);
  void findAncestorElements(const QDomNode &, QMap<QString, QDomElement> *, QString &);

  QString getChildText(const QDomElement &, const QString &, QString &);
  QPair<QString, QString> getTimeSpan(const QDomElement &, QString &);

  bool loadSchema(QXmlSchema &, QString &);
  QDomDocument createDomDocument(const QByteArray &, const QXmlSchema &, const QUrl &, QString &);
  bool convertFromOldFormat(QByteArray &, QString &);

  QList<DrawingItemBase *> createFromDomDocument(const QDomDocument &doc, const QString &name, const QString &srcFileName, QString &error);
  QList<DrawingItemBase *> createFromFile(const QString &name, const QString &fileName, QString &error);
}

#endif // KML_H
