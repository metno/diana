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
#include <EditItems/layer.h>
#include <EditItems/layergroup.h>
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

#include <QDebug>

using namespace std;
using namespace miutil;

DrawingManager *DrawingManager::self_ = 0;
Rectangle DrawingManager::plotRect_;
Rectangle DrawingManager::editRect_;

DrawingManager::DrawingManager()
{
  setEditRect(PLOTM->getPlotSize());
  currentArea_ = PLOTM->getCurrentArea();
  styleManager_ = DrawingStyleManager::instance();
  layerMgr_ = new EditItems::LayerManager();
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
void DrawingManager::separateJoinIds(const QList<QSharedPointer<DrawingItemBase> > &items)
{
  int loAbsJoinId = 0;
  foreach (const QSharedPointer<DrawingItemBase> &item, items) {
    const int absJoinId = qAbs(item->joinId());
    if (absJoinId > 0)
      loAbsJoinId = (loAbsJoinId ? qMin(loAbsJoinId, absJoinId) : absJoinId);
  }

  const int add = (DrawingManager::instance()->nextJoinId(false) - loAbsJoinId);
  int hiAbsJoinId = 0;
  foreach (const QSharedPointer<DrawingItemBase> &item, items) {
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
        drawings_.insert(items["file"]);
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
  if (inp.empty())
    return false;

  loaded_.clear();
  layerMgr_->clear();

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
        if (loadDrawing(value))
          break;
        else
          return false;
      }
    }
  }

  // Only enable this manager if there are specific files loaded, not just
  // if there are items in the layers.
  setEnabled(!loaded_.isEmpty());

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
  Q_ASSERT(!layerMgr_->selectedLayers().isEmpty());
  layerMgr_->selectedLayers().first()->insertItem(item);
}

bool DrawingManager::loadDrawing(const QString &fileName)
{
  // parse file and create item layers
  QString error;
  QList<QSharedPointer<EditItems::Layer> > layers = KML::createFromFile<DrawingItemBase, DrawingItem_PolyLine::PolyLine, DrawingItem_Symbol::Symbol,
      DrawingItem_Text::Text, DrawingItem_Composite::Composite>(layerMgr_, fileName, &error);

  if (!error.isEmpty()) {
    METLIBS_LOG_WARN("Failed to create items from file " << fileName.toStdString() << ": " << error.toStdString());
    return false;
  }
  if (layers.isEmpty()) {
    METLIBS_LOG_WARN("File " << fileName.toStdString() << " contained no items");
    return false;
  }

  // initialize screen coordinates from lat/lon         ### already done in KML::createFromFile() ???
  foreach (QSharedPointer<EditItems::Layer> layer, layers) {
    for (int i = 0; i < layer->itemCount(); ++i)
      setFromLatLonPoints(*(layer->itemRef(i)), layer->item(i)->getLatLonPoints());
  }

  layerMgr_->addToNewLayerGroup(layers, fileName);

  loaded_.insert(fileName);
  return true;
}

void DrawingManager::removeItem_(const QSharedPointer<DrawingItemBase> &item)
{
  layerMgr_->removeItem(item);
}

QList<QPointF> DrawingManager::getLatLonPoints(const DrawingItemBase &item) const
{
  const QList<QPointF> points = item.getPoints();
  return PhysToGeo(points);
}

// Returns geographic coordinates converted from screen coordinates.
QList<QPointF> DrawingManager::PhysToGeo(const QList<QPointF> &points) const
{
  int w, h;
  PLOTM->getPlotWindow(w, h);
  float dx = (plotRect_.x1 - editRect_.x1) * float(w)/plotRect_.width();
  float dy = (plotRect_.y1 - editRect_.y1) * float(h)/plotRect_.height();

  int n = points.size();

  QList<QPointF> latLonPoints;

  for (int i = 0; i < n; ++i) {
    float x, y;
    PLOTM->PhysToGeo(points.at(i).x() - dx,
                     points.at(i).y() - dy,
                     x, y, currentArea_, plotRect_);
    latLonPoints.append(QPointF(x, y));
  }

  return latLonPoints;
}

void DrawingManager::setFromLatLonPoints(DrawingItemBase &item, const QList<QPointF> &latLonPoints) const
{
  if (!latLonPoints.isEmpty()) {
    QList<QPointF> points = GeoToPhys(latLonPoints);
    item.setPoints(points);
  }
}

// Returns screen coordinates converted from geographic coordinates.
QList<QPointF> DrawingManager::GeoToPhys(const QList<QPointF> &latLonPoints) const
{
  int w, h;
  PLOTM->getPlotWindow(w, h);

  QList<QPointF> points;
  int n = latLonPoints.size();

  const Rectangle currPlotRect = PLOTM->getPlotSize();
  for (int i = 0; i < n; ++i) {
    float x, y;
    PLOTM->GeoToPhys(latLonPoints.at(i).x(),
                     latLonPoints.at(i).y(),
                     x, y, currentArea_, currPlotRect);
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

  QList<QSharedPointer<EditItems::Layer> > layers = layerMgr_->orderedLayers();
  for (int i = layers.size() - 1; i >= 0; --i) {

    const QSharedPointer<EditItems::Layer> layer = layers.at(i);
    if (layer->isVisible()) {

      QList<QSharedPointer<DrawingItemBase> > items = layer->items();
      foreach (const QSharedPointer<DrawingItemBase> item, items) {

        std::string time_str;
        std::string prop_str = timeProperty(item->propertiesRef(), time_str);
        if (!time_str.empty())
          times.insert(miutil::miTime(time_str));
      }
    }
  }

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

  // Change the visibility of items.
  const QList<QSharedPointer<EditItems::Layer> > &layers = layerMgr_->orderedLayers();
  for (int i = layers.size() - 1; i >= 0; --i) {

    const QSharedPointer<EditItems::Layer> layer = layers.at(i);
    QList<QSharedPointer<DrawingItemBase> > items = layer->items();

    foreach (const QSharedPointer<DrawingItemBase> item, items) {
      std::string time_str;
      std::string time_prop = timeProperty(item->propertiesRef(), time_str);
      if (time_prop.empty())
        item->setProperty("visible", true);
      else {
        bool visible = (time_str.empty() | ((time.isoTime("T") + "Z") == time_str));
        item->setProperty("visible", visible);
      }
    }
  }

  return found;
}

bool DrawingManager::changeProjection(const Area& newArea)
{
  // Record the new plot rectangle and area.
  // Update the edit rectangle so that objects are positioned consistently.
  Rectangle r = PLOTM->getPlotSize();
  setPlotRect(r);
  setEditRect(r);
  currentArea_ = newArea;
  return true;
}

void DrawingManager::plot(bool under, bool over)
{
  if (!under)
    return;

  // Apply a transformation so that the items can be plotted with screen coordinates
  // while everything else is plotted in map coordinates.
  glPushMatrix();
  Rectangle r = PLOTM->getPlotSize();
  setPlotRect(r);
  int w, h;
  PLOTM->getPlotWindow(w, h);
  glTranslatef(editRect_.x1, editRect_.y1, 0.0);
  glScalef(plotRect_.width()/w, plotRect_.height()/h, 1.0);

  QList<QSharedPointer<EditItems::Layer> > layers = layerMgr_->orderedLayers();
  for (int i = layers.size() - 1; i >= 0; --i) {

    const QSharedPointer<EditItems::Layer> layer = layers.at(i);
    if (layer->isVisible()) {

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
  if (!imageCache_.contains(key))
    imageCache_.insert(key, QGLWidget::convertToGLFormat(getSymbolImage(name, width, height)));
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

void DrawingManager::applyPlotOptions(const QSharedPointer<DrawingItemBase> &item) const
{
  const bool antialiasing = item->property("antialiasing", true).toBool();
  if (antialiasing)
    glEnable(GL_MULTISAMPLE);
  else
    glDisable(GL_MULTISAMPLE);
}

EditItems::LayerManager *DrawingManager::getLayerManager()
{
  return layerMgr_;
}

void DrawingManager::setPlotRect(Rectangle r)
{
  DrawingManager::plotRect_ = Rectangle(r.x1, r.y1, r.x2, r.y2);
}

void DrawingManager::setEditRect(Rectangle r)
{
  DrawingManager::editRect_ = Rectangle(r.x1, r.y1, r.x2, r.y2);
}

std::vector<PlotElement> DrawingManager::getPlotElements() const
{
  std::vector<PlotElement> pel;
  plotElems_.clear();
  int i = 0;
  foreach (const QSharedPointer<EditItems::Layer> &layer, layerMgr_->orderedLayers()) {
    pel.push_back(
          PlotElement(
            plotElementTag().toStdString(), QString("%1").arg(i).toStdString(),
            plotElementTag().toStdString(), layer->isVisible()));
    plotElems_.insert(i, layer);
    i++;
  }
  return pel;
}

QString DrawingManager::plotElementTag() const
{
  return "DRAWING";
}

void DrawingManager::enablePlotElement(const PlotElement &pe)
{
  const QString s = QString::fromStdString(pe.str);
  bool ok = false;
  const int i = s.toInt(&ok);
  if (!ok) {
    qWarning() << "DM::enablePlotElement(): failed to extract int from pe.str:" << s;
    return;
  }

  if (!plotElems_.contains(i)) {
    qWarning() << "DM::enablePlotElement(): key not found in plotElems_:" << i;
    return;
  }

  plotElems_.value(i)->setVisible(pe.enabled, true);
}
