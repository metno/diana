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

#include <diDrawingManager.h>
#include <EditItems/drawingcomposite.h>
#include <EditItems/drawingpolyline.h>
#include <EditItems/drawingsymbol.h>
#include <EditItems/drawingtext.h>
#include <EditItems/itemgroup.h>
#include <EditItems/kml.h>
#include <EditItems/drawingstylemanager.h>
#include "diGLPainter.h"
#include <diPlotModule.h>
#include <diLocalSetupParser.h>

#include <puTools/miSetupParser.h>
#include "diCommonTypes.h"

#include <set>

#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QSvgRenderer>

#define PLOTM PlotModule::instance()

//#define DEBUGPRINT
#define MILOGGER_CATEGORY "diana.DrawingManager"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

DrawingManager *DrawingManager::self_ = 0;
Rectangle DrawingManager::editRect_;

DrawingManager::DrawingManager()
{
  setEditRect(PLOTM->getPlotSize());
  styleManager_ = DrawingStyleManager::instance();
}

DrawingManager::~DrawingManager()
{
}

DrawingManager *DrawingManager::instance()
{
  if (!DrawingManager::self_)
    DrawingManager::self_ = new DrawingManager();

  return DrawingManager::self_;
}

int DrawingManager::nextJoinId_ = 1;

int DrawingManager::nextJoinId(bool bump)
{
  // ### this function is not thread safe; use a mutex for that
  const int origVal = nextJoinId_;
  if (bump)
    nextJoinId_++;
  return origVal;
}

void DrawingManager::setNextJoinId(int val)
{
  // ### this function is not thread safe; use a mutex for that
  if (val <= nextJoinId_) // only allow a higher value
    return;
  nextJoinId_ = val;
}

// Adjusts any join IDs in \a items to avoid conflicts with any existing joins (assuming existing join IDs are all < nextJoinId_).
void DrawingManager::separateJoinIds(const QList<DrawingItemBase *> &items)
{
  int loAbsJoinId = 0;
  foreach (DrawingItemBase *item, items) {
    const int absJoinId = qAbs(item->joinId());
    if (absJoinId > 0)
      loAbsJoinId = (loAbsJoinId ? qMin(loAbsJoinId, absJoinId) : absJoinId);
  }

  const int add = (DrawingManager::instance()->nextJoinId(false) - loAbsJoinId);
  int hiAbsJoinId = 0;
  foreach (DrawingItemBase *item, items) {
    const int joinId = item->joinId();
    if (joinId) {
      const int newAbsJoinId = qAbs(joinId) + add;
      item->propertiesRef().insert("joinId", (joinId < 0) ? -newAbsJoinId : newAbsJoinId);
      hiAbsJoinId = qMax(hiAbsJoinId, newAbsJoinId);
    }
  }

  if (hiAbsJoinId)
    setNextJoinId(hiAbsJoinId + 1);
}

bool DrawingManager::parseSetup()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE("DrawingManager::parseSetup");
#endif

  // Store a list of file names in the internal drawing model for use by the dialog.
  vector<string> section;

  if (!SetupParser::getSection("DRAWING", section))
    METLIBS_LOG_WARN("No DRAWING section.");

  for (unsigned int i = 0; i < section.size(); ++i) {

    // Split the line into tokens.
    vector<string> tokens = miutil::split_protected(section[i], '\"', '\"', " ", true);
    QHash<QString, QString> items;

    for (unsigned int j = 0; j < tokens.size(); ++j) {
      string key, value;
      SetupParser::splitKeyValue(tokens[j], key, value);
      items[QString::fromStdString(key)] = QString::fromStdString(value);
    }

    // Check for different types of definition.
    if (items.contains("file")) {
      if (items.contains("symbol")) {

        // The default section is an empty string.
        QString section = items.value("section", "");

        // Symbol definitions
        QFile f(items["file"]);
        if (f.open(QFile::ReadOnly)) {

          // Store a combination of the section name and the symbol name in
          // the common symbol map.
          QString name = items["symbol"];

          if (!section.isEmpty())
            name = section + "|" + name;

          items["symbol"] = name;
          symbols_[name] = f.readAll();
          f.close();

          // Add the internal symbol name to the relevant section.
          symbolSections_[section].insert(name);
          styleManager_->addStyle(DrawingItemBase::Symbol, items);
        } else
          METLIBS_LOG_WARN("Failed to load drawing symbol file: " << items["file"].toStdString());

      } else {
        // Drawing definitions
        QString name;
        if (items.contains("name"))
          name = items["name"];
        else
          name = items["file"];

        drawings_[name] = items["file"];
      }
    } else if (items.contains("style")) {
      // Read-only style definitions
      styleManager_->addStyle(DrawingItemBase::PolyLine, items);
    } else if (items.contains("workdir")) {
      workDir_ = items["workdir"];
    } else if (items.contains("textstyle")) {
      styleManager_->addStyle(DrawingItemBase::Text, items);
    } else if (items.contains("composite")) {
      styleManager_->addStyle(DrawingItemBase::Composite, items);
    } else if (items.contains("complextext")) {
      QStringList strings = items["complextext"].split(",");
      styleManager_->setComplexTextList(strings);
    } else if (items.contains("tseries")) {
      const QString timeSeries = items.value("tseries");
      const QString filePattern = items.value("tsfiles");
      drawings_[timeSeries] = filePattern;
    }
  }

  if (workDir_.isNull()) {
    std::string workdir = LocalSetupParser::basicValue("workdir");
    if (workdir.empty())
      workdir = LocalSetupParser::basicValue("homedir");
    workDir_ = QString::fromStdString(workdir);
  }

  return true;
}

/**
 * Processes the plot commands passed as a vector of strings, creating items
 * as required.
 */
bool DrawingManager::processInput(const std::vector<std::string>& inp)
{
  loaded_.clear();

  // Compile a list of files to load.
  QStringList toLoad;

  vector<string>::const_iterator it;
  for (it = inp.begin(); it != inp.end(); ++it) {
    // Split each input line into a collection of "words".
    vector<string> pieces = miutil::split_protected(*it, '"', '"');
    // Skip the first piece ("DRAWING").
    pieces.erase(pieces.begin());

    QVariantMap properties;
    QVariantList points;
    vector<string>::const_iterator it;

    for (it = pieces.begin(); it != pieces.end(); ++it) {

      QString key, value;
      if (!parseKeyValue(*it, key, value))
        continue;

      // Read the specified file, skipping to the next line if successful,
      // but returning false to indicate an error if unsuccessful.
      if (key == "file" || key == "name")
        toLoad.append(value);
    }
  }

  QMap<QString, EditItems::ItemGroup *> loaded;
  QMap<QString, EditItems::ItemGroup *>::const_iterator itl;

  foreach (const QString &name, toLoad) {
    // If the name corresponds to a key in the list of drawings then look up
    // associated file name.
    QString fileName;
    if (drawings_.contains(name))
      fileName = drawings_[name];
    else
      fileName = name;

    // Is the file already loaded?
    bool isLoaded = false;
    EditItems::ItemGroup *group;

    for (itl = itemGroups_.begin(); itl != itemGroups_.end(); ++itl) {
      group = itl.value();
      if (group->fileName() == fileName) {
        isLoaded = true;
        break;
      }
    }

    // If not, try to load it.
    if (!isLoaded) {
      if (!loadDrawing(name, fileName).isEmpty())
        return false;

      // Obtain the group created by the loadDrawing() call.
      group = itemGroups_.value(name);
    }

    // Record the layer group in the collection of replacement drawings.
    loaded[name] = group;
    loaded_[name] = fileName;
  }

  // Delete layer groups that are no longer loaded and replace the list with
  // the new list of loaded groups.
  for (itl = itemGroups_.begin(); itl != itemGroups_.end(); ++itl) {
    QString name = itl.key();
    EditItems::ItemGroup *group = itl.value();
    if (!loaded.contains(name))
      delete group;
  }
  itemGroups_ = loaded;

  // Only enable this manager if there are specific files loaded, not just
  // if there are items in the layer manager.
  setEnabled(!loaded_.isEmpty());

  // Emit a signal to inform other components that the list of layer groups
  // has been updated.
  emit updated();

  return true;
}

std::vector<std::string> DrawingManager::getAnnotations() const
{
  vector<string> output;
  foreach (QString drawing, loaded_.keys())
    output.push_back(drawing.toStdString());
  return output;
}

DrawingItemBase *DrawingManager::createItem(const QString &type)
{
  DrawingItemBase *item = 0;
  if (type == "PolyLine") {
    item = new DrawingItem_PolyLine::PolyLine();
  } else if (type == "Symbol") {
    item = new DrawingItem_Symbol::Symbol();
  } else if (type == "Text") {
    item = new DrawingItem_Text::Text();
  } else if (type == "Composite") {
    item = new DrawingItem_Composite::Composite();
  }
  return item;
}

DrawingItemBase *DrawingManager::createItemFromVarMap(const QVariantMap &vmap, QString &error)
{
  if (vmap.empty() || !vmap.contains("type") || !vmap.value("type").canConvert(QVariant::String))
    return 0;

  QString type = vmap.value("type").toString().split("::").last();
  DrawingItemBase *item = createItem(type);

  if (item) {
    item->setProperties(vmap);
    setFromLatLonPoints(item, item->getLatLonPoints());

    DrawingItem_Composite::Composite *c = dynamic_cast<DrawingItem_Composite::Composite *>(item);
    if (c)
      c->createElements();
  }

  return item;
}

void DrawingManager::addItem_(DrawingItemBase *item, EditItems::ItemGroup *group)
{
  group->addItem(item);
}

QString DrawingManager::loadDrawing(const QString &name, const QString &fileName)
{
  QString error;

  QList<DrawingItemBase *> items = KML::createFromFile(name, fileName, error);
  if (!error.isEmpty()) {
    METLIBS_LOG_SCOPE("Failed to open file: " << fileName.toStdString());
    return error;
  }

  // Create a layer group for the file that is not editable but is active.
  EditItems::ItemGroup *itemGroup = new EditItems::ItemGroup(name, false, true);
  itemGroup->setFileName(fileName);
  itemGroup->setItems(items);
  itemGroups_[name] = itemGroup;

  // Record the file name.
  drawings_[name] = fileName;
  emit drawingLoaded(name);

  return error;
}

void DrawingManager::removeItem_(DrawingItemBase *item, EditItems::ItemGroup *group)
{
  group->removeItem(item);
}

QList<QPointF> DrawingManager::getLatLonPoints(const DrawingItemBase *item) const
{
  const QList<QPointF> points = item->getPoints();
  return PhysToGeo(points);
}

inline XY fromQ(const QPointF& p) { return XY(p.x(), p.y()); }

// Returns geographic coordinates converted from screen coordinates.
QList<QPointF> DrawingManager::PhysToGeo(const QList<QPointF> &points) const
{
  const StaticPlot* sp = PLOTM->getStaticPlot();
  const XY dxy = sp->MapToPhys(XY(editRect_.x1, editRect_.y1));

  int n = points.size();

  QList<QPointF> latLonPoints;
  latLonPoints.reserve(n);
  for (int i = 0; i < n; ++i) {
    const XY lonlat = sp->PhysToGeo(fromQ(points.at(i)) + dxy);
    latLonPoints.append(QPointF(lonlat.y(), lonlat.x()));
  }

  return latLonPoints;
}

void DrawingManager::setFromLatLonPoints(DrawingItemBase *item, const QList<QPointF> &latLonPoints) const
{
  if (!latLonPoints.isEmpty()) {
    QList<QPointF> points = GeoToPhys(latLonPoints);
    item->setPoints(points);
  }
}

// Returns screen coordinates converted from geographic coordinates.
QList<QPointF> DrawingManager::GeoToPhys(const QList<QPointF> &latLonPoints) const
{
  QList<QPointF> points;
  int n = latLonPoints.size();

  for (int i = 0; i < n; ++i) {
    float x, y;
    PLOTM->GeoToPhys(latLonPoints.at(i).x(),
                     latLonPoints.at(i).y(),
                     x, y);
    points.append(QPointF(x, y));
  }

  return points;
}

/**
 * Returns a vector containing the times for which the manager has data.
*/
std::vector<miutil::miTime> DrawingManager::getTimes() const
{
  std::vector<miutil::miTime> output;
  std::set<miutil::miTime> times;

  // Query the layer groups to find the available times.
  QMap<QString, EditItems::ItemGroup *>::const_iterator it;
  for (it = itemGroups_.begin(); it != itemGroups_.end(); ++it) {
    QSet<QString> groupTimes = it.value()->getTimes();
    foreach (const QString &time, groupTimes)
      times.insert(miutil::miTime(time.toStdString()));
  }

  output.assign(times.begin(), times.end());

  // Sort the times.
  std::sort(output.begin(), output.end());

  return output;
}

/**
 * Prepares the manager for display of, and interaction with, items that
 * correspond to the given \a time.
*/
bool DrawingManager::prepare(const miutil::miTime &time)
{
  bool found = false;

  // Check the requested time against the available times.
  std::vector<miutil::miTime>::const_iterator it;
  std::vector<miutil::miTime> times = getTimes();

  for (it = times.begin(); it != times.end(); ++it) {
    if (*it == time) {
      found = true;
      break;
    }
  }

  // Update layer groups to change the visibility of items.
  QString timeStr = QString::fromStdString(time.isoTime());
  QDateTime dateTime = QDateTime::fromString(timeStr, Qt::ISODate);

  QMap<QString, EditItems::ItemGroup *>::iterator itl;
  for (itl = itemGroups_.begin(); itl != itemGroups_.end(); ++itl) {

    EditItems::ItemGroup *itemGroup = itl.value();
    bool allVisible = true;

    if (itemGroup->isCollection()) {

      // For layer groups containing a collection of files, make the layers
      // visible only if the current file is appropriate for the new time.
      allVisible = (dateTime == itemGroup->time());

      if (!allVisible && itemGroup->hasTime(dateTime)) {

        // Another time was requested and is available. Replace the existing
        // items with those from the corresponding file.

        QString fileName = itemGroup->fileName(dateTime);

        QString error;
        QList<DrawingItemBase *> items = KML::createFromFile(itemGroup->name(), fileName, error);
        if (!error.isEmpty())
          METLIBS_LOG_WARN(QString("DrawingManager::prepare: failed to load layer group from %1: %2")
                           .arg(fileName).arg(error).toStdString());

        itemGroup->setItems(items);
        allVisible = true;
      }
    }

    itemGroup->setTime(dateTime, allVisible);
  }

  return found;
}

bool DrawingManager::changeProjection(const Area& newArea)
{
  // Record the new plot rectangle and area.
  // Update the edit rectangle so that objects are positioned consistently.
  const Rectangle& r = PLOTM->getPlotSize();
  setEditRect(r);
  return true;
}

void DrawingManager::setCanvas(DiCanvas* canvas)
{
  Manager::setCanvas(canvas);
  styleManager_->setCanvas(canvas);
  imageCache_.clear(); // GL format might change
}

void DrawingManager::plot(DiGLPainter* gl, bool under, bool over)
{
  if (!under)
    return;

  // Apply a transformation so that the items can be plotted with screen coordinates
  // while everything else is plotted in map coordinates.
  gl->PushMatrix();
  gl->Translatef(editRect_.x1, editRect_.y1, 0.0);
  gl->Scalef(PLOTM->getStaticPlot()->getPhysToMapScaleX(),
      PLOTM->getStaticPlot()->getPhysToMapScaleY(), 1.0);

  foreach (DrawingItemBase *item, allItems()) {

    if (isItemVisible(item)) {
      applyPlotOptions(gl, item);
      setFromLatLonPoints(item, item->getLatLonPoints());
      item->draw(gl);
    }
  }
  gl->PopMatrix();
}

QMap<QString, QString> &DrawingManager::getDrawings()
{
  return drawings_;
}

QMap<QString, QString> &DrawingManager::getLoaded()
{
  return loaded_;
}

QString DrawingManager::getWorkDir() const
{
  return workDir_;
}

void DrawingManager::setWorkDir(const QString &dir)
{
  workDir_ = dir;
}

QStringList DrawingManager::symbolNames(const QString &section) const
{
  if (section.isNull())
    return symbols_.keys();

  QStringList names = symbolSections_.value(section).toList();
  return names;
}

QStringList DrawingManager::symbolSectionNames() const
{
  return symbolSections_.keys();
}

QImage DrawingManager::getCachedImage(const QString &name, int width, int height) const
{
  const QString key = QString("%1 %2x%3").arg(name).arg(width).arg(height);
  if (!imageCache_.contains(key)) {
    if (DiGLCanvas* c = dynamic_cast<DiGLCanvas*>(canvas()))
      imageCache_.insert(key, c->convertToGLFormat(getSymbolImage(name, width, height)));
  }
  return imageCache_.value(key);
}

QImage DrawingManager::getSymbolImage(const QString &name, int width, int height) const
{
  QImage image(width, height, QImage::Format_ARGB32);
  if (width == 0 || height == 0)
    return image;

  QSvgRenderer renderer(symbols_.value(name));
  image.fill(QColor(0, 0, 0, 0).rgba());
  QPainter painter;
  painter.begin(&image);
  renderer.render(&painter);
  painter.end();

  return image;
}

QSize DrawingManager::getSymbolSize(const QString &name) const
{
  QSvgRenderer renderer(symbols_.value(name));
  return renderer.defaultSize();
}

void DrawingManager::applyPlotOptions(DiGLPainter *gl, const DrawingItemBase *item) const
{
  const bool antialiasing = item->property("antialiasing", true).toBool();
  if (antialiasing)
    gl->Enable(DiGLPainter::gl_MULTISAMPLE);
  else
    gl->Disable(DiGLPainter::gl_MULTISAMPLE);
}

void DrawingManager::setEditRect(Rectangle r)
{
  DrawingManager::editRect_ = Rectangle(r.x1, r.y1, r.x2, r.y2);
}

std::vector<PlotElement> DrawingManager::getPlotElements()
{
  std::vector<PlotElement> pel;
  plotElements_.clear();

  int i = 0;
  foreach (EditItems::ItemGroup *group, itemGroups_) {
    if (group->isActive()) {
      QString str = QString("%1").arg(i);
      pel.push_back(PlotElement(plotElementTag().toStdString(), str.toStdString(),
                                plotElementTag().toStdString(), group->isActive()));
      plotElements_[str] = group;
      i += 1;
    }
  }

  return pel;
}

QString DrawingManager::plotElementTag() const
{
  return "DRAWING";
}

void DrawingManager::enablePlotElement(const PlotElement &pe)
{
  QString str = QString::fromStdString(pe.str);
  if (!plotElements_.contains(str))
    return;

  EditItems::ItemGroup *group = plotElements_.value(str);
  group->setActive(pe.enabled);
}

/**
 * Handles mouse events when used as part of Diana instead of bdiana.
 */
void DrawingManager::sendMouseEvent(QMouseEvent* event, EventResult& res)
{
  if (event->type() == QEvent::MouseMove && event->buttons() == Qt::NoButton) {
    // Find a list of items at the point passed in the event.
    QList<DrawingItemBase *> missed;
    QList<DrawingItemBase *> hit = findHitItems(event->pos(), missed);
    if (hit.size() > 0)
      emit itemsHovered(hit);
  }
}

QList<DrawingItemBase *> DrawingManager::findHitItems(
    const QPointF &pos, QList<DrawingItemBase *> &missedItems) const
{
  QList<DrawingItemBase *> hitItems;

  QMap<QString, EditItems::ItemGroup *>::const_iterator it;
  for (it = itemGroups_.begin(); it != itemGroups_.end(); ++it) {
    foreach (DrawingItemBase *item, it.value()->items()) {
      if (item->hit(pos, false))
        hitItems.append(item);
      else
        missedItems.append(item);
    }
  }

  return hitItems;
}

/**
 * Returns a vector containing triples of IDs, labels and coordinates for
 * polylines in the KML file with the given \a fileName.
 */
vector<PolyLineInfo> DrawingManager::loadCoordsFromKML(const string &fileName)
{
  vector<PolyLineInfo> info;

  QString error;
  QString fname = QString::fromStdString(fileName);
  QList<DrawingItemBase *> items = KML::createFromFile(fname, fname, error);

  if (error.isEmpty()) {
    foreach (DrawingItemBase *item, items) {
      DrawingItem_PolyLine::PolyLine *polyLine = dynamic_cast<DrawingItem_PolyLine::PolyLine *>(item);
      if (polyLine) {
        vector<LonLat> coords;
        foreach (QPointF p, polyLine->getLatLonPoints())
          coords.push_back(LonLat::fromDegrees(p.y(), p.x()));

        info.push_back(PolyLineInfo(polyLine->id(), polyLine->property("Placemark:name").toString().toStdString(), coords));
      }
    }
  }

  return info;
}

QList<DrawingItemBase *> DrawingManager::allItems() const
{
  QList<DrawingItemBase *> items;
  QMap<QString, EditItems::ItemGroup *>::const_iterator it;
  for (it = itemGroups_.begin(); it != itemGroups_.end(); ++it) {
    if (it.value()->isActive())
      items += it.value()->items();
  }

  return items;
}

/**
 * Returns true if the item is intrinsically visible and not excluded by
 * the current filter.
 */
bool DrawingManager::isItemVisible(DrawingItemBase *item) const
{
  bool visible = item->isVisible();
  if (!visible) return false;

  return matchesFilter(item);
}

/**
 * Returns true if the item is included by the current filter.
 */
bool DrawingManager::matchesFilter(DrawingItemBase *item) const
{
  // Each item is visible if none of its properties match those in the
  // property list. Otherwise, items are invisible by default.
  bool visible = false;
  bool hasAtLeastOneProperty = false;

  foreach (const QString &property, filter_.first) {
    QVariant value = item->property(property);
    hasAtLeastOneProperty |= value.isValid();
    if (value.isValid() && filter_.second.contains(value.toString())) {
      visible = true;
      break;
    }
  }

  if (hasAtLeastOneProperty)
    return visible;
  else
    return true;
}

void DrawingManager::setFilter(const QPair<QStringList, QSet<QString> > &filter)
{
  filter_ = filter;
}

/**
 * Returns the layer group identified by the given name.
 */
EditItems::ItemGroup *DrawingManager::itemGroup(const QString &name)
{
  if (itemGroups_.contains(name))
    return itemGroups_.value(name);
  else
    return 0;
}

/**
 * Removes the layer group with the given name. The items held within are not deleted.
 */
void DrawingManager::removeItemGroup(const QString &name)
{
  itemGroups_.remove(name);
  emit updated();
}
