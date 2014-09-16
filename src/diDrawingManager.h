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
#ifndef _diDrawingManager_h
#define _diDrawingManager_h

#include "diManager.h"
#include <diField/diGridConverter.h>
#include <EditItems/drawingitembase.h>
#include <EditItems/layermanager.h>
#include <EditItems/layer.h>
#include <QHash>
#include <QList>
#include <QPointF>
#include <QSet>
#include <QString>
#include <QVariantMap>
//#define QT_SHAREDPOINTER_TRACK_POINTERS
#include <QSharedPointer>
#include <vector>

class PlotModule;
class ObjectManager;

class QKeyEvent;
class QMouseEvent;

class DrawingStyleManager;

/**
  \brief Manager for drawing areas and annotations.
*/
class DrawingManager : public Manager
{
  Q_OBJECT

public:
  class itemCompare
  {
  public:
      inline bool operator()(const QSharedPointer<DrawingItemBase> t1, const QSharedPointer<DrawingItemBase> t2) const
      {
          return (t1->id() < t2->id());
      }
  };

  DrawingManager();
  ~DrawingManager();

  // parse DRAWING section of setup file (defines Drawing products)
  virtual bool parseSetup();

  virtual bool changeProjection(const Area& newArea);
  virtual bool loadDrawing(const QString &fileName);
  virtual bool prepare(const miutil::miTime &time);
  virtual void plot(bool under, bool over);
  virtual bool processInput(const std::vector<std::string>& inp);
  virtual std::vector<std::string> getAnnotations() const;

  virtual void sendMouseEvent(QMouseEvent* event, EventResult& res) {}
  virtual void sendKeyboardEvent(QKeyEvent* event, EventResult& res) {}

  QList<QPointF> getLatLonPoints(const DrawingItemBase &item) const;
  void setFromLatLonPoints(DrawingItemBase &item, const QList<QPointF> &latLonPoints) const;
  QList<QPointF> PhysToGeo(const QList<QPointF> &points) const;
  QList<QPointF> GeoToPhys(const QList<QPointF> &latLonPoints) const;

  virtual QSharedPointer<DrawingItemBase> createItemFromVarMap(const QVariantMap &vmap, QString *error);

  template<typename BaseType, typename PolyLineType, typename SymbolType,
           typename TextType, typename CompositeType>
  inline BaseType *createItemFromVarMap_(const QVariantMap &vmap, QString *error)
  {
    Q_ASSERT(!vmap.empty());
    Q_ASSERT(vmap.contains("type"));
    Q_ASSERT(vmap.value("type").canConvert(QVariant::String));
    BaseType *item = 0;
    *error = QString();
    if (vmap.value("type").toString().endsWith("PolyLine")) {
      item = new PolyLineType();
    } else if (vmap.value("type").toString().endsWith("Symbol")) {
      item = new SymbolType();
    } else if (vmap.value("type").toString().endsWith("Text")) {
      item = new TextType();
    } else if (vmap.value("type").toString().endsWith("Composite")) {
      item = new CompositeType();
    } else {
      *error = QString("unsupported item type: %1")
          .arg(vmap.value("type").toString());
      return 0;
    }

    item->setProperties(vmap);
    setFromLatLonPoints(*item, Drawing(item)->getLatLonPoints());

    CompositeType *c = dynamic_cast<CompositeType *>(item);
    if (c)
      c->createElements();

    return item;
  }

  static DrawingManager *instance();

  // Resource handling
  void drawSymbol(const QString &name, float x, float y, int width, int height);
  QStringList symbolNames() const;
  QImage getSymbolImage(const QString &name, int width, int height);
  QSize getSymbolSize(const QString &name);

  // Dialog-related methods
  QSet<QString> &getDrawings();
  QSet<QString> &getLoaded();
  EditItems::LayerManager *getLayerManager();
  EditItems::LayerManager *getAuxLayerManager();

  QString getWorkDir() const;
  void setWorkDir(const QString &dir);

  void setPlotRect(Rectangle r);
  void setEditRect(Rectangle r);

public slots:
  std::vector<miutil::miTime> getTimes() const;

protected:
  virtual void addItem_(const QSharedPointer<DrawingItemBase> &);
  virtual void removeItem_(const QSharedPointer<DrawingItemBase> &);
  void applyPlotOptions(const QSharedPointer<DrawingItemBase> &) const;
  std::string timeProperty(const QVariantMap &properties, std::string &time_str) const;

  static Rectangle plotRect;
  static Rectangle editRect;
  Area currentArea;

  // ### are these needed any longer?
  QSet<QString> drawings_;
  QSet<QString> loaded_;

  EditItems::LayerManager *layerMgr_; // Read by DrawingManager::plot() and EditItemManager::plot(). Read/written by EditDrawingDialog.

private:

  GridConverter gc;
  QString workDir;

  QMap<QString, QByteArray> symbols;
  QHash<QString, GLuint> symbolTextures;
  QHash<QString, QImage> imageCache;
  DrawingStyleManager *styleManager;

  static DrawingManager *self;  // singleton instance pointer
};

#endif // _diDrawingManager_h
