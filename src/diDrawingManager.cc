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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diDrawingManager.h>
#include <EditItems/drawingpolyline.h>
#include <EditItems/drawingsymbol.h>
#include <EditItems/kml.h>
#include <diPlotModule.h>
#include <diObjectManager.h>
#include <diAnnotationPlot.h>

#include <puCtools/puCglob.h>
#include <puCtools/glob_cache.h>
#include <puTools/miDirtools.h>
#include <puTools/miSetupParser.h>

#include <cmath>
#include <iomanip>
#include <fstream>
#include <set>

#include <QDateTime>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QSvgRenderer>

#if defined(USE_PAINTGL)
#include "PaintGL/paintgl.h"
#else
#include <QGLContext>
#endif

#define PLOTM PlotModule::instance()

//#define DEBUGPRINT
#define MILOGGER_CATEGORY "diana.DrawingManager"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

DrawingManager *DrawingManager::self = 0;

DrawingManager::DrawingManager()
{
  self = this;
  editRect = PLOTM->getPlotSize();
  currentArea = PLOTM->getCurrentArea();
}

DrawingManager::~DrawingManager()
{
}

DrawingManager *DrawingManager::instance()
{
  if (!DrawingManager::self)
    DrawingManager::self = new DrawingManager();

  return DrawingManager::self;
}

bool DrawingManager::parseSetup()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE("DrawingManager::parseSetup");
#endif

  // Store a list of file names in the internal drawing model for use by the dialog.
  vector<string> section;

  // Return true if there is no DRAWING section.
  if (!SetupParser::getSection("DRAWING", section)) {
    METLIBS_LOG_WARN("No DRAWING section.");
    return true;
  }

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

        // Symbol definitions
        QFile f(items["file"]);
        if (f.open(QFile::ReadOnly)) {
          symbols[items["symbol"]] = f.readAll();
          f.close();
        } else
          METLIBS_LOG_WARN("Failed to load symbol file: " << items["file"].toStdString());
      } else {

        // Drawing definitions
        drawings_.insert(items["file"]);
      }
    } else if (items.contains("type")) {

      // Read-only style definitions
      PolygonStyle style(true);
      style.parse(items);
      polygonStyles[items["type"]] = style;
    }
  }

  if (!polygonStyles.contains("Default")) {
    PolygonStyle style(true);
    QHash<QString, QString> items;
    style.parse(items);
    polygonStyles["Default"] = style;
  }
  return true;
}

/**
 * Processes the plot commands passed as a vector of strings, creating items
 * as required.
 */
bool DrawingManager::processInput(const std::vector<std::string>& inp)
{
  // New input has been submitted, so remove the items from the set.
  // This will automatically cause items to be removed from the editing
  // dialog in interactive mode.
  foreach (DrawingItemBase *item, items_.values())
    removeItem_(item);

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

      // Split each word into a key=value pair.
      vector<string> wordPieces = miutil::split_protected(*it, '"', '"', "=");
      if (wordPieces.size() == 0 || wordPieces.size() > 2)
        continue;
      else if (wordPieces.size() == 1)
        wordPieces.push_back("");

      QString key = QString::fromStdString(wordPieces[0]);
      QString value = QString::fromStdString(wordPieces[1]);
      if (value.startsWith('"') && value.endsWith('"') && value.size() >= 2)
        value = value.mid(1, value.size() - 2);

      if (key == "file") {
        // Read the specified file, skipping to the next line if successful,
        // but returning false to indicate an error if unsuccessful.
        if (loadItems(value))
          break;
        else
          return false;
      } else if (key == "type") {
        properties["type"] = value;
      } else if (key == "time") {
        properties["time"] = QDateTime::fromString(value, "yyyy-MM-ddThh:mm:ss");
      } else if (key == "group") {
        properties["groupId"] = value.toInt();
      } else if (key == "points") {
        QStringList pieces = value.split(":");
        foreach (QString piece, pieces) {
          QStringList coordinates = piece.split(",");
          if (coordinates.size() != 2)
            METLIBS_LOG_WARN("Invalid point in coordinate list for object: " << piece.toStdString());
          else
            points.append(QPointF(coordinates[0].toDouble(), coordinates[1].toDouble()));
        }
        properties["points"] = points;
      } else if (key.startsWith("Style:")) {
        properties[key] = value;
      }
    }

    if (!points.isEmpty()) {
      QString error;
      DrawingItemBase *item = createItemFromVarMap(properties, &error);
      if (item) {
        addItem_(item);
      } else
        METLIBS_LOG_WARN(error.toStdString());
    }
  }

  setEnabled(!items_.empty());
  return true;
}

DrawingItemBase *DrawingManager::createItemFromVarMap(const QVariantMap &properties, QString *error)
{
  return createItemFromVarMap_<DrawingItemBase, DrawingItem_PolyLine::PolyLine, DrawingItem_Symbol::Symbol>(properties, error);
}

void DrawingManager::addItem_(DrawingItemBase *item)
{
  items_.insert(item);
}

bool DrawingManager::loadItems(const QString &fileName)
{
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    METLIBS_LOG_WARN("Failed to open file " << fileName.toStdString() << " for reading.");
    return false;
  }

  QByteArray data = file.readAll();
  file.close();

  QString error;
  QSet<DrawingItemBase *> items = KML::createFromFile<DrawingItemBase, DrawingItem_PolyLine::PolyLine, DrawingItem_Symbol::Symbol>(fileName, &error);
  if (!error.isEmpty()) {
    METLIBS_LOG_WARN("Failed to create items from file " << fileName.toStdString() << ": " << error.toStdString());
    return false;
  } else if (!items.isEmpty()) {
    foreach (DrawingItemBase *item, items) {
      // Set the screen coordinates from the latitude and longitude values.
      setFromLatLonPoints(item, item->getLatLonPoints());
      items_.insert(item);
    }
  } else {
    METLIBS_LOG_WARN("File " << fileName.toStdString() << " contained no items");
    return false;
  }

  return true;
}

void DrawingManager::removeItem_(DrawingItemBase *item)
{
  items_.remove(item);
}

QList<QPointF> DrawingManager::getLatLonPoints(DrawingItemBase* item) const
{
  QList<QPointF> points = item->getPoints();
  return PhysToGeo(points);
}

QList<QPointF> DrawingManager::PhysToGeo(const QList<QPointF> &points) const
{
  int w, h;
  PLOTM->getPlotWindow(w, h);
  float dx = (plotRect.x1 - editRect.x1) * float(w)/plotRect.width();
  float dy = (plotRect.y1 - editRect.y1) * float(h)/plotRect.height();

  int n = points.size();

  QList<QPointF> latLonPoints;

  // Convert screen coordinates to geographic coordinates.
  for (int i = 0; i < n; ++i) {
    float x, y;
    PLOTM->PhysToGeo(points.at(i).x() - dx,
                     points.at(i).y() - dy,
                     x, y, currentArea, plotRect);
    latLonPoints.append(QPointF(x, y));
  }
  return latLonPoints;
}

void DrawingManager::setFromLatLonPoints(DrawingItemBase* item, const QList<QPointF> &latLonPoints)
{
  QList<QPointF> points = GeoToPhys(latLonPoints);
  item->setPoints(points);
}

QList<QPointF> DrawingManager::GeoToPhys(const QList<QPointF> &latLonPoints)
{
  int w, h;
  PLOTM->getPlotWindow(w, h);
  float dx = (plotRect.x1 - editRect.x1) * float(w)/plotRect.width();
  float dy = (plotRect.y1 - editRect.y1) * float(h)/plotRect.height();

  QList<QPointF> points;
  int n = latLonPoints.size();

  // Convert geographic coordinates to screen coordinates.
  for (int i = 0; i < n; ++i) {
    float x, y;
    PLOTM->GeoToPhys(latLonPoints.at(i).x(),
                     latLonPoints.at(i).y(),
                     x, y, currentArea, plotRect);
    points.append(QPointF(x + dx, y + dy));
  }

  return points;
}

/**
 * Returns a vector containing the times for which the manager has data.
*/
std::vector<miutil::miTime> DrawingManager::getTimes() const
{
  std::set<miutil::miTime> times;

  foreach (DrawingItemBase *item, items_) {

    std::string time_str;
    std::string prop_str = timeProperty(item->propertiesRef(), time_str);
    if (!time_str.empty())
      times.insert(miutil::miTime(time_str));
  }

  std::vector<miutil::miTime> output;
  output.assign(times.begin(), times.end());

  // Sort the times.
  std::sort(output.begin(), output.end());

  return output;
}

/**
 * Returns the property name used to find the time for an item, or an empty
 * string if no suitable property name was found. The associated time string
 * is also updated with the time obtained from the property, if found.
*/

std::string DrawingManager::timeProperty(const QVariantMap &properties, std::string &time_str) const
{
  static const char* timeProps[2] = {"time", "TimeSpan:begin"};

  for (unsigned int i = 0; i < 2; ++i) {
    time_str = properties.value(timeProps[i]).toString().toStdString();
    if (!time_str.empty())
      return timeProps[i];
  }

  return std::string();
}

/**
 * Prepares the manager for display of, and interaction with, items that
 * correspond to the given \a time.
*/
bool DrawingManager::prepare(const miutil::miTime &time)
{
  // Check the requested time against the available times.
  bool found = false;
  std::vector<miutil::miTime>::const_iterator it;
  std::vector<miutil::miTime> times = getTimes();

  for (it = times.begin(); it != times.end(); ++it) {
    if (*it == time) {
      found = true;
      break;
    }
  }

  // Change the visibility of items in the editor.

  foreach (DrawingItemBase *item, items_) {
    std::string time_str;
    std::string time_prop = timeProperty(item->propertiesRef(), time_str);
    if (time_prop.empty() || isEditing())
      item->setProperty("visible", true);
    else {
      bool visible = (time_str.empty() | ((time.isoTime("T") + "Z") == time_str));
      item->setProperty("visible", visible);
    }
  }

  return found;
}

bool DrawingManager::changeProjection(const Area& newArea)
{
  // Record the new plot rectangle and area.
  // Update the edit rectangle so that objects are positioned consistently.
  plotRect = editRect = PLOTM->getPlotSize();
  currentArea = newArea;
  return true;
}

void DrawingManager::plot(bool under, bool over)
{
  if (!over)
    return;

  // Apply a transformation so that the items can be plotted with screen coordinates
  // while everything else is plotted in map coordinates.
  glPushMatrix();
  plotRect = PLOTM->getPlotSize();
  int w, h;
  PLOTM->getPlotWindow(w, h);
  glTranslatef(editRect.x1, editRect.y1, 0.0);
  glScalef(plotRect.width()/w, plotRect.height()/h, 1.0);

  QList<DrawingItemBase *> items = items_.values();
  qStableSort(items.begin(), items.end(), DrawingManager::itemCompare());

  foreach (DrawingItemBase *item, items) {
    if (item->property("visible", true).toBool()) {
      setFromLatLonPoints(item, item->getLatLonPoints());
      item->draw();
    }
  }

  glPopMatrix();
}

QSet<DrawingItemBase *> DrawingManager::getItems() const
{
  return items_;
}

QSet<QString> &DrawingManager::drawings()
{
  return drawings_;
}

void DrawingManager::drawSymbol(const QString &name, float x, float y, int width, int height)
{
  if (!symbols.contains(name))
    return;

  GLuint texture = 0;
  QGLContext *glctx = const_cast<QGLContext *>(QGLContext::currentContext());
  QImage image(width, height, QImage::Format_ARGB32);

  // If an existing image is cached then delete the texture and prepare to
  // create another.
  bool found = false;

  if (imageCache.contains(name)) {
    image = imageCache[name];
    if (image.width() == width || image.height() == height) {
      texture = symbolTextures[name];
      found = true;
    }
  }

  if (!found) {
    QSvgRenderer renderer(symbols.value(name));
    image = QImage(width, height, QImage::Format_ARGB32);
    image.fill(QColor(0, 0, 0, 0));
    QPainter painter;
    painter.begin(&image);
    renderer.render(&painter);
    painter.end();

    texture = glctx->bindTexture(image.mirrored());
    symbolTextures[name] = texture;
    imageCache[name] = image;
  }

  glPushAttrib(GL_COLOR_BUFFER_BIT);
  glEnable(GL_BLEND);
  glctx->drawTexture(QPointF(x, y), texture);
  glPopAttrib();
}

PolygonStyle *DrawingManager::getPolygonStyle(const QString &name)
{
  if (name.isEmpty())
    return &polygonStyles["Default"];
  else
    return &polygonStyles[name];
}
