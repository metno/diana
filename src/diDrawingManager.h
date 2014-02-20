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

#include <GL/gl.h>

#include <EditItems/drawingitembase.h>

#include <diCommonTypes.h>
#include <diDrawingTypes.h>
#include "diManager.h"
#include <diMapMode.h>

#include <diField/diGridConverter.h>

#include <QHash>
#include <QList>
#include <QObject>
#include <QPainterPath>
#include <QPointF>
#include <QSet>
#include <QString>
#include <QVariantMap>

#include <vector>

class DrawingItemBase;
class PlotModule;
class ObjectManager;

class QAction;
class QKeyEvent;
class QMouseEvent;

#if defined(USE_PAINTGL)
#include "PaintGL/paintgl.h"
#define QGLContext PaintGLContext
#endif

/**
  \brief Manager for drawing styles.
*/
class DrawingStyleManager
{
public:
  enum Side { Inside, Outside };

  DrawingStyleManager();
  virtual ~DrawingStyleManager();
  void addStyle(const QHash<QString, QString> &definition);

  void beginLine(DrawingItemBase *item);
  void endLine(DrawingItemBase *item);
  void beginFill(DrawingItemBase *item);
  void endFill(DrawingItemBase *item);

  void drawLines(const DrawingItemBase *item, const QList<QPointF> &points, int z = 0) const;
  void fillLoop(const DrawingItemBase *item, const QList<QPointF> &points) const;

  static const QPainterPath interpolateToPath(const QList<QPointF> &points, bool closed);
  static const QList<QPointF> interpolateToPoints(const QList<QPointF> &points, bool closed);
  static const QList<QPointF> getDecorationLines(const QList<QPointF> &points, qreal lineLength);

  bool contains(const QString &name) const;
  QVariantMap getStyle(DrawingItemBase *item) const;
  QVariantMap getStyle(const DrawingItemBase *item) const;
  QVariantMap getStyle(const QString &name) const;

  static DrawingStyleManager *instance();

private:
  void drawDecoration(const QVariantMap &style, const QString &decoration, bool closed,
                      const Side &side, const QList<QPointF> &points, int z,
                      unsigned int offset = 0) const;
  QVariantMap parse(const QHash<QString, QString> &definition) const;
  QColor parseColour(const QString &text) const;

  QHash<QString, QVariantMap> styles;
  static DrawingStyleManager *self;  // singleton instance pointer
};

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
      inline bool operator()(const DrawingItemBase* t1, const DrawingItemBase* t2) const
      {
          return (t1->id() < t2->id());
      }
  };

  DrawingManager();
  ~DrawingManager();

  /// parse DRAWING section of setup file (defines Drawing products)
  bool parseSetup();

  virtual bool changeProjection(const Area& newArea);
  virtual bool loadItems(const QString &fileName);
  virtual bool prepare(const miutil::miTime &time);
  virtual void plot(bool under, bool over);
  virtual bool processInput(const std::vector<std::string>& inp);
  virtual std::vector<std::string> getAnnotations() const;

  virtual void sendMouseEvent(QMouseEvent* event, EventResult& res) {}
  virtual void sendKeyboardEvent(QKeyEvent* event, EventResult& res) {}

  QList<QPointF> getLatLonPoints(DrawingItemBase* item) const;
  void setFromLatLonPoints(DrawingItemBase* item, const QList<QPointF> &latLonPoints);
  QList<QPointF> PhysToGeo(const QList<QPointF> &points) const;
  QList<QPointF> GeoToPhys(const QList<QPointF> &latLonPoints);

  QSet<DrawingItemBase *> getItems() const;

  virtual DrawingItemBase *createItemFromVarMap(const QVariantMap &vmap, QString *error);

  template<typename BaseType, typename PolyLineType, typename SymbolType>
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
    } else {
      *error = QString("unsupported item type: %1, expected %2 or %3")
          .arg(vmap.value("type").toString()).arg("*PolyLine").arg("*Symbol");
      return 0;
    }

    item->setProperties(vmap);
    setFromLatLonPoints(Drawing(item), Drawing(item)->getLatLonPoints());
    return item;
  }

  static DrawingManager *instance();

  // Resource handling
  void drawSymbol(const QString &name, float x, float y, int width, int height);

  // Dialog-related methods
  QSet<QString> &getDrawings();
  QSet<QString> &getLoaded();

  QString getWorkDir() const;

public slots:
  std::vector<miutil::miTime> getTimes() const;

protected:
  virtual void addItem_(DrawingItemBase *);
  virtual void removeItem_(DrawingItemBase *item);
  void applyPlotOptions(DrawingItemBase *item) const;

  Rectangle plotRect;
  Rectangle editRect;
  Area currentArea;

  QSet<DrawingItemBase *> items_; // ### >>> move to layers.h
  QSet<QString> drawings_;
  QSet<QString> loaded_;

private:
  std::string timeProperty(const QVariantMap &properties, std::string &time_str) const;

  GridConverter gc;
  QString workDir;

  QHash<QString, QByteArray> symbols;
  QHash<QString, GLuint> symbolTextures;
  QHash<QString, QImage> imageCache;
  DrawingStyleManager styleManager;

  static DrawingManager *self;  // singleton instance pointer
};

#endif
