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

#include <diDrawingManager.h>
#include <EditItems/drawingcomposite.h>
#include <EditItems/drawingpolyline.h>
#include <EditItems/drawingsymbol.h>
#include <EditItems/drawingtext.h>
#include <EditItems/kml.h>
#include <EditItems/layermanager.h>
#include <EditItems/drawingstylemanager.h>
#include <diPlotModule.h>
#include <diLocalSetupParser.h>

#include <puTools/miSetupParser.h>

#include <set>

#include <QImage>
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
  styleManager = DrawingStyleManager::instance();
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
      styleManager->addStyle(DrawingItemBase::Symbol, items);
    } else if (items.contains("style")) {
      // Read-only style definitions
      styleManager->addStyle(DrawingItemBase::PolyLine, items);
    } else if (items.contains("workdir")) {
      workDir = items["workdir"];
    } else if (items.contains("textstyle")) {
      styleManager->addStyle(DrawingItemBase::Text, items);
    }
  }

  if (workDir.isNull()) {
    std::string workdir = LocalSetupParser::basicValue("workdir");
    if (workdir.empty())
      workdir = LocalSetupParser::basicValue("homedir");
    workDir = QString::fromStdString(workdir);
  }

  return true;
}

/**
 * Processes the plot commands passed as a vector of strings, creating items
 * as required.
 */
bool DrawingManager::processInput(const std::vector<std::string>& inp)
{
#if 1
  EditItems::LayerManager::instance()->resetFirstDefaultLayer();
#else
  const QSharedPointer<EditItems::Layer> layer = CurrentLayer;
  if (!layer.isNull()) {

    // New input has been submitted, so remove the items from the set.
    // This will automatically cause items to be removed from the editing
    // dialog in interactive mode.
    const QSet<QSharedPointer<DrawingItemBase> > items = layer->itemSet();
    foreach (QSharedPointer<DrawingItemBase> item, items.values())
      removeItem_(item);    
  } else {
    EditItems::Layers::instance()->addEmpty();
  }
#endif

  loaded_.clear();

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

      if (key == "file") {
        // Read the specified file, skipping to the next line if successful,
        // but returning false to indicate an error if unsuccessful.
        if (loadItems(value))
          break;
        else
          return false;
      }
    }
  }

  Q_ASSERT(CurrEditLayer);
  setEnabled(CurrEditLayer->itemCount() > 0);

  return true;
}

std::vector<std::string> DrawingManager::getAnnotations() const
{
  vector<string> output;
  foreach (QString drawing, loaded_)
    output.push_back(drawing.toStdString());
  return output;
}

QSharedPointer<DrawingItemBase> DrawingManager::createItemFromVarMap(const QVariantMap &properties, QString *error)
{
  return QSharedPointer<DrawingItemBase>(
        createItemFromVarMap_<DrawingItemBase, DrawingItem_PolyLine::PolyLine, DrawingItem_Symbol::Symbol,
        DrawingItem_Text::Text, DrawingItem_Composite::Composite>(properties, error));
}

void DrawingManager::addItem_(const QSharedPointer<DrawingItemBase> &item)
{
  Q_ASSERT(CurrEditLayer);
  CurrEditLayer->insertItem(item);
}

bool DrawingManager::loadItems(const QString &fileName)
{
  // parse file and create item layers
  QString error;
  QList<QSharedPointer<EditItems::Layer> > layers = \
      KML::createFromFile<DrawingItemBase, DrawingItem_PolyLine::PolyLine, DrawingItem_Symbol::Symbol,
      DrawingItem_Text::Text, DrawingItem_Composite::Composite>(fileName, &error);

  if (!error.isEmpty()) {
    METLIBS_LOG_WARN("Failed to create items from file " << fileName.toStdString() << ": " << error.toStdString());
    return false;
  }
  if (layers.isEmpty()) {
    METLIBS_LOG_WARN("File " << fileName.toStdString() << " contained no items");
    return false;
  }

  // initialize screen coordinates from lat/lon
  foreach (QSharedPointer<EditItems::Layer> layer, layers) {
    for (int i = 0; i < layer->itemCount(); ++i)
      setFromLatLonPoints(*(layer->itemRef(i)), layer->item(i)->getLatLonPoints());
  }

  EditItems::LayerManager::instance()->replaceInDefaultLayerGroup(layers);

  drawings_.insert(fileName);
  loaded_.insert(fileName);

  return true;
}

void DrawingManager::removeItem_(const QSharedPointer<DrawingItemBase> &item)
{
  Q_ASSERT(CurrEditLayer);
  CurrEditLayer->removeItem(item);
}

QList<QPointF> DrawingManager::getLatLonPoints(const DrawingItemBase &item) const
{
  const QList<QPointF> points = item.getPoints();
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

void DrawingManager::setFromLatLonPoints(DrawingItemBase &item, const QList<QPointF> &latLonPoints)
{
  QList<QPointF> points = GeoToPhys(latLonPoints);
  item.setPoints(points);
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
  std::vector<miutil::miTime> output;

  if (CurrLayer) {
    std::set<miutil::miTime> times;

    for (int i = 0; i < CurrLayer->itemCount(); ++i) {
      std::string time_str;
      std::string prop_str = timeProperty(CurrLayer->item(i)->propertiesRef(), time_str);
      if (!time_str.empty())
        times.insert(miutil::miTime(time_str));
    }

    output.assign(times.begin(), times.end());

    // Sort the times.
    std::sort(output.begin(), output.end());
  }

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
  bool found = false;

  if (CurrLayer) {

    // Check the requested time against the available times.
    std::vector<miutil::miTime>::const_iterator it;
    std::vector<miutil::miTime> times = getTimes();

    for (it = times.begin(); it != times.end(); ++it) {
      if (*it == time) {
        found = true;
        break;
      }
    }

    // Change the visibility of items in the editor.
    for (int i = 0; i < CurrLayer->itemCount(); ++i) {
      std::string time_str;
      std::string time_prop = timeProperty(CurrLayer->item(i)->propertiesRef(), time_str);
      if (time_prop.empty() || isEditing())
        CurrLayer->item(i)->setProperty("visible", true);
      else {
        bool visible = (time_str.empty() | ((time.isoTime("T") + "Z") == time_str));
        CurrLayer->item(i)->setProperty("visible", visible);
      }
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

  const QList<QSharedPointer<EditItems::Layer> > &layers = EditItems::LayerManager::instance()->orderedLayers();
  for (int i = layers.size() - 1; i >= 0; --i) {

    const QSharedPointer<EditItems::Layer> layer = layers.at(i);
    if (layer->isActive() && layer->isVisible()) {

      QList<QSharedPointer<DrawingItemBase> > items = layer->items();
      qStableSort(items.begin(), items.end(), DrawingManager::itemCompare());

      foreach (const QSharedPointer<DrawingItemBase> item, items) {
        if (item->property("visible", true).toBool()) {
          applyPlotOptions(item);
          setFromLatLonPoints(*item, item->getLatLonPoints());
          item->draw();
        }
      }
    }

  }
  glPopMatrix();
}

int DrawingManager::itemCount() const
{
  return CurrLayer ? CurrLayer->itemCount() : 0;
}

QSet<QString> &DrawingManager::getDrawings()
{
  return drawings_;
}

QSet<QString> &DrawingManager::getLoaded()
{
  return loaded_;
}

QString DrawingManager::getWorkDir() const
{
  return workDir;
}

void DrawingManager::drawSymbol(const QString &name, float x, float y, int width, int height)
{
  if (!symbols.contains(name))
    return;

  GLuint texture = 0;
#if defined(USE_PAINTGL)
  PaintGLContext *glctx = const_cast<PaintGLContext *>(PaintGLContext::currentContext());
#else
  QGLContext *glctx = const_cast<QGLContext *>(QGLContext::currentContext());
#endif
  QImage image(width, height, QImage::Format_ARGB32);

  // If an existing image is cached then delete the texture and prepare to
  // create another.
  bool found = false;
  QString key = QString("%1 %2x%3").arg(name).arg(width).arg(height);

  if (imageCache.contains(key)) {
    image = imageCache[key];
    texture = symbolTextures[key];
    found = true;
  }

  if (!found) {
    image = getSymbolImage(name, width, height);

    texture = glctx->bindTexture(image.mirrored());
    symbolTextures[key] = texture;
    imageCache[key] = image;
  }

  glPushAttrib(GL_COLOR_BUFFER_BIT);
  glEnable(GL_BLEND);
  glctx->drawTexture(QPointF(x, y), texture);
  glPopAttrib();
}

QStringList DrawingManager::symbolNames() const
{
  return symbols.keys();
}

QImage DrawingManager::getSymbolImage(const QString &name, int width, int height)
{
  QImage image(width, height, QImage::Format_ARGB32);
  if (width == 0 || height == 0)
    return image;

  QSvgRenderer renderer(symbols.value(name));
  image.fill(QColor(0, 0, 0, 0).rgba());
  QPainter painter;
  painter.begin(&image);
  renderer.render(&painter);
  painter.end();

  return image;
}

void DrawingManager::applyPlotOptions(const QSharedPointer<DrawingItemBase> &item) const
{
  const bool antialiasing = item->property("antialiasing", true).toBool();
  if (antialiasing)
    glEnable(GL_MULTISAMPLE);
  else
    glDisable(GL_MULTISAMPLE);
}
