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

#ifndef EDITITEMSLAYERGROUP_H
#define EDITITEMSLAYERGROUP_H

#include <QDateTime>
#include <QFileInfo>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QSet>
#include <QString>
//#define QT_SHAREDPOINTER_TRACK_POINTERS
#include <QSharedPointer>
#include <QVariantMap>

namespace EditItems {

class Layer;
class LayerManager;

class LayerGroup : public QObject
{
  Q_OBJECT
  friend class LayerManager;
public:
  LayerGroup(const QString &, bool = true, bool = false);
  LayerGroup(const LayerGroup &);
  ~LayerGroup();
  int id() const;
  QString name() const;
  void setName(const QString &);
  QString fileName(const QDateTime &dateTime = QDateTime()) const;
  void setFileName(const QString &);
  bool isEditable() const;
  bool isActive() const;
  void setActive(bool);
  const QList<QSharedPointer<Layer> > &layersRef() const;
  QList<QSharedPointer<Layer> > &layersRef();
  QSet<QString> getTimes() const;
  QDateTime time() const;
  bool hasTime(const QDateTime &dateTime) const;
  void setTime(const QDateTime &dateTime, bool allVisible = true);
  QSet<QString> files() const;
  void setFiles(const QList<QPair<QFileInfo, QDateTime> > &tfiles);
  bool isCollection() const;

private:
  QString timeProperty(const QVariantMap &properties, QString &time_str);

  int id_;
  static int nextId_;
  static int nextId();
  QString name_;
  QString fileName_;
  bool editable_;
  bool active_;
  QList<QSharedPointer<Layer> > layers_;
  QMap<QDateTime, QFileInfo> tfiles_;
  QDateTime currentTime_;
};

} // namespace

#endif // EDITITEMSLAYERGROUP_H
