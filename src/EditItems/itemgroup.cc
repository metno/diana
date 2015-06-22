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

#include <EditItems/kml.h>
#include <EditItems/drawingitembase.h>
#include <EditItems/itemgroup.h>
#include <EditItems/timefilesextractor.h>

#define MILOGGER_CATEGORY "diana.ItemGroup"
#include <miLogger/miLogging.h>

namespace EditItems {

ItemGroup::ItemGroup(const QString &name, bool editable, bool active)
  : name_(name)
  , editable_(editable)
  , active_(active)
{
}

ItemGroup::ItemGroup(const ItemGroup &other)
  : name_(other.name_)
  , editable_(other.editable_)
  , active_(other.active_)
  , tfiles_(other.tfiles_)
{
  foreach (const DrawingItemBase *item, other.items_)
    items_.append(item->clone());
}

ItemGroup::~ItemGroup()
{
}

QString ItemGroup::name() const
{
  return name_;
}

void ItemGroup::setName(const QString &n)
{
  name_ = n;
}

QString ItemGroup::fileName(const QDateTime &dateTime) const
{
  if (dateTime.isNull())
    return fileName_;
  else
    return tfiles_.value(dateTime).filePath();
}

void ItemGroup::setFileName(const QString &filePathOrPattern)
{
  if (filePathOrPattern.contains("[")) {
    // For collections of files, find all the file names and store them internally.
    QList<QPair<QFileInfo, QDateTime> > tfiles = TimeFilesExtractor::getFiles(filePathOrPattern);
    setFiles(tfiles);

    if (!tfiles.isEmpty())
      fileName_ = tfiles.first().first.filePath();
  }
  else
    fileName_ = filePathOrPattern;
}

bool ItemGroup::isEditable() const
{
  return editable_;
}

bool ItemGroup::isActive() const
{
  return active_;
}

void ItemGroup::setActive(bool active)
{
  active_ = active;
}

QSet<QString> ItemGroup::getTimes() const
{
  QSet<QString> times;

  foreach (const QDateTime &dateTime, tfiles_.keys())
    times.insert(dateTime.toString(Qt::ISODate) + "Z");

  // The following strings could be made configurable:
  static const char* timeProps[2] = {"time", "TimeSpan:begin"};

  foreach (const DrawingItemBase *item, items_) {
    QString timeStr;
    for (unsigned int i = 0; i < 2; ++i) {
      timeStr = item->propertiesRef().value(timeProps[i]).toString();
      if (!timeStr.isEmpty()) {
        times.insert(timeStr);
        break;
      }
    }
  }

  return times;
}

QDateTime ItemGroup::time() const
{
  return currentTime_;
}

bool ItemGroup::hasTime(const QDateTime &dateTime) const
{
  return tfiles_.contains(dateTime);
}

void ItemGroup::setTime(const QDateTime &dateTime, bool allVisible)
{
  // Update the visibility of items held in the layers of this group.
  QString dateTimeStr = dateTime.toString(Qt::ISODate) + "Z";

  foreach (DrawingItemBase *item, items_) {
    QString time_str;
    QString time_prop = timeProperty(item->propertiesRef(), time_str);

    if (editable_)
      item->setVisible(true);

    else if (time_prop.isEmpty()) {
      if (isCollection()) {
        // For layer groups containing a collection of files, make the layers
        // visible only if the current file is appropriate for the new time.
        item->setVisible(allVisible);
      } else {
        // For layer groups containing a single file with its own times for
        // layers, if no time property was found, make the item visible.
        item->setVisible(true);
      }
    } else if (time_prop == "TimeSpan:begin") {
      // Make the item visible if the time is within the begin and end times
      // for the item.
      QDateTime beginTime = QDateTime::fromString(item->property("TimeSpan:begin").toString(), "yyyy-MM-ddThh:mm:ssZ");
      QDateTime endTime = QDateTime::fromString(item->property("TimeSpan:end").toString(), "yyyy-MM-ddThh:mm:ssZ");
      bool visible = (dateTime >= beginTime) && (dateTime < endTime);
      item->setVisible(visible);

    } else {
      // Make the item visible if the time is empty or equal to the current time.
      bool visible = (time_str.isEmpty() | (dateTimeStr == time_str));
      item->setVisible(visible);
    }
  }

  currentTime_ = dateTime;
}

/**
 * Returns the property name used to hold the time of an item in its properties map,
 * modifying the time_str string to return the time itself.
 */
QString ItemGroup::timeProperty(const QVariantMap &properties, QString &time_str)
{
  static const char* timeProps[2] = {"time", "TimeSpan:begin"};

  for (unsigned int i = 0; i < 2; ++i) {
    time_str = properties.value(timeProps[i]).toString();
    if (!time_str.isEmpty())
      return QString(timeProps[i]);
  }

  return QString();
}

QSet<QString> ItemGroup::files() const
{
  // If the layer does not contain a collection of files, return the single
  // file held. Otherwise, convert the collection to a set of file names.

  if (tfiles_.isEmpty())
    return QSet<QString>() << fileName();
  else {
    QList<QFileInfo> files = tfiles_.values();
    QSet<QString> f;
    foreach (const QFileInfo &fi, files)
      f.insert(fi.filePath());

    return f;
  }
}

void ItemGroup::setFiles(const QList<QPair<QFileInfo, QDateTime> > &tfiles)
{
  for (int i = 0; i < tfiles.size(); ++i)
    tfiles_[tfiles.at(i).second] = tfiles.at(i).first;
}

bool ItemGroup::isCollection() const
{
  return !tfiles_.isEmpty();
}

DrawingItemBase *ItemGroup::item(int id) const
{
  if (ids_.contains(id))
    return ids_.value(id);
  else
    return 0;
}

QList<DrawingItemBase *> ItemGroup::items() const
{
  return items_;
}

void ItemGroup::setItems(QList<DrawingItemBase *> items)
{
  // The layer owns the items within it, so we can delete the items as well
  // as clearing the list.
  qDeleteAll(items_);
  items_ = items;

  // Populate the identifier hash with the new items.
  ids_.clear();
  foreach (DrawingItemBase *item, items)
    ids_[item->id()] = item;
}

void ItemGroup::addItem(DrawingItemBase *item)
{
  // Since an item can be created and added to a layer group then re-added
  // immediately by the corresponding redo command, we need to ensure it is
  // not added twice.
  int id = item->id();
  if (ids_.contains(id)) return;

  items_.append(item);
  ids_[id] = item;
}

void ItemGroup::removeItem(DrawingItemBase *item)
{
  // Since an item can be removed from a layer group then re-removed
  // immediately by the corresponding redo command, we need to ensure it is
  // not removed twice.
  int id = item->id();
  if (!ids_.contains(id)) return;

  items_.removeOne(item);
  ids_.remove(id);
  return;
}

/**
 * Replace the states of items in this layer group whose identifiers match
 * those in the supplied hash.
 */
void ItemGroup::replaceStates(const QHash<int, QVariantMap> &states)
{
  foreach (int id, states.keys()) {
    if (ids_.contains(id)) {
      // Replace the item's state with the new one.
      DrawingItemBase *item = ids_.value(id);
      QVariantMap properties = states.value(id);
      QList<QVariant> llp = properties.value("latLonPoints").toList();
      properties.remove("latLonPoints");
      item->setProperties(properties, true);
      QList<QPointF> latLonPoints;
      foreach (QVariant v, llp)
        latLonPoints.append(v.toPointF());
      item->setLatLonPoints(latLonPoints);
    }
  }
}

} // namespace
