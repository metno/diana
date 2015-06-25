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
#ifndef _diDrawingManager_h
#define _diDrawingManager_h

#include "diManager.h"
#include "EditItems/drawingitembase.h"

#include <diField/diGridConverter.h>
#include <EditItems/drawingitembase.h>
#include <EditItems/itemgroup.h>
#include <QHash>
#include <QList>
#include <QMap>
#include <QPointF>
#include <QSet>
#include <QString>
#include <QVariantMap>
#include <vector>

class PlotModule;
class ObjectManager;

class QKeyEvent;
class QMouseEvent;

class DrawingStyleManager;
struct PlotElement;

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
      inline bool operator()(const DrawingItemBase *t1, const DrawingItemBase *t2) const
      {
          return (t1 < t2);
      }
  };

  DrawingManager();
  ~DrawingManager();

  // parse DRAWING section of setup file (defines Drawing products)
  virtual bool parseSetup();

  virtual bool changeProjection(const Area& newArea);
  virtual QString loadDrawing(const QString &name, const QString &fileName);
  virtual bool prepare(const miutil::miTime &time);
  virtual void setCanvas(DiCanvas* canvas) /* Q_DECL_OVERRIDE*/;
  virtual void plot(DiGLPainter* gl, bool under, bool over);
  virtual bool processInput(const std::vector<std::string>& inp);
  virtual std::vector<std::string> getAnnotations() const;

  virtual void sendMouseEvent(QMouseEvent* event, EventResult& res);
  virtual void sendKeyboardEvent(QKeyEvent* event, EventResult& res) {}

  QList<DrawingItemBase *> findHitItems(const QPointF &pos, QList<DrawingItemBase *> &missedItems) const;

  QList<QPointF> getLatLonPoints(const DrawingItemBase *item) const;
  void setFromLatLonPoints(DrawingItemBase *item, const QList<QPointF> &latLonPoints) const;
  QList<QPointF> PhysToGeo(const QList<QPointF> &points) const;
  QList<QPointF> GeoToPhys(const QList<QPointF> &latLonPoints) const;

  virtual DrawingItemBase *createItem(const QString &type);
  virtual DrawingItemBase *createItemFromVarMap(const QVariantMap &vmap, QString &error);

  static DrawingManager *instance();

  // Resource handling
  QStringList symbolNames(const QString &section = QString()) const;
  QStringList symbolSectionNames() const;
  QImage getCachedImage(const QString &, int, int) const;
  QImage getSymbolImage(const QString &, int, int) const;
  QSize getSymbolSize(const QString &) const;

  // Dialog-related methods
  QMap<QString, QString> &getDrawings();
  QMap<QString, QString> &getLoaded();

  QString getWorkDir() const;
  void setWorkDir(const QString &dir);

  void setEditRect(Rectangle r);

  virtual std::vector<PlotElement> getPlotElements();
  virtual QString plotElementTag() const;
  void enablePlotElement(const PlotElement &);

  int nextJoinId(bool = true);
  void separateJoinIds(const QList<DrawingItemBase *> &);

  virtual QList<DrawingItemBase *> allItems() const;
  bool matchesFilter(DrawingItemBase *item) const;
  bool isItemVisible(DrawingItemBase *item) const;
  void setFilter(const QPair<QStringList, QSet<QString> > &filter);

  std::vector<PolyLineInfo> loadCoordsFromKML(const std::string &fileName);
  EditItems::ItemGroup *itemGroup(const QString &name);
  void removeItemGroup(const QString &name);

public slots:
  std::vector<miutil::miTime> getTimes() const;

signals:
  void drawingLoaded(const QString &name);
  void itemsClicked(const QList<DrawingItemBase *> &items);
  void itemsHovered(const QList<DrawingItemBase *> &items);
  void updated();

protected:
  virtual void addItem_(DrawingItemBase *, EditItems::ItemGroup *group);
  virtual void removeItem_(DrawingItemBase *, EditItems::ItemGroup *group);
  void applyPlotOptions(DiGLPainter *gl, const DrawingItemBase *) const;

  static Rectangle editRect_;

  // Drawing definitions held in the setup file and those that have been loaded.
  QMap<QString, QString> drawings_;
  QMap<QString, QString> loaded_;

  QMap<QString, EditItems::ItemGroup *> itemGroups_;
  QPair<QStringList, QSet<QString> > filter_;

private:
  GridConverter gc_;
  QString workDir_;

  QMap<QString, QSet<QString> > symbolSections_;
  QMap<QString, QByteArray> symbols_;
  mutable QHash<QString, QImage> imageCache_;
  DrawingStyleManager *styleManager_;

  static int nextJoinId_;
  void setNextJoinId(int);

  QHash<QString, EditItems::ItemGroup *> plotElements_;

  static DrawingManager *self_;  // singleton instance pointer
};

#endif // _diDrawingManager_h
