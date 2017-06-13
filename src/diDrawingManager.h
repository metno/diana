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
  DrawingManager();
  ~DrawingManager();

  // parse DRAWING section of setup file (defines Drawing products)
  virtual bool parseSetup() override;

  virtual bool changeProjection(const Area& newArea) override;
  virtual QString loadDrawing(const QString &name, const QString &fileName);
  virtual bool prepare(const miutil::miTime &time) override;
  virtual void setCanvas(DiCanvas* canvas) override;
  virtual void plot(DiGLPainter* gl, bool under, bool over) override;
  virtual bool processInput(const PlotCommand_cpv& inp) override;
  std::vector<std::string> getAnnotations() const override;

  void sendMouseEvent(QMouseEvent* event, EventResult& res) override;
  void sendKeyboardEvent(QKeyEvent* /*event*/, EventResult& /*res*/) override {}

  QList<DrawingItemBase *> findHitItems(const QPointF &pos,
    QHash<DrawingItemBase::HitType, QList<DrawingItemBase *> > &hitItemTypes,
    QList<DrawingItemBase *> &missedItems) const;

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
  bool loadSymbol(const QString &fileName, const QString &section, const QString &symbol = QString());

  // Dialog-related methods
  QMap<QString, QString> &getDrawings();
  QMap<QString, QString> &getLoaded();

  QString getWorkDir() const;
  void setWorkDir(const QString &dir);

  void setEditRect(Rectangle r);

  virtual std::vector<PlotElement> getPlotElements() override;
  virtual bool enablePlotElement(const PlotElement &) override;
  virtual QString plotElementTag() const override;

  int nextJoinId(bool = true);
  void separateJoinIds(const QList<DrawingItemBase *> &);

  virtual bool isEmpty() const;
  virtual QList<DrawingItemBase *> allItems() const;
  bool matchesFilter(DrawingItemBase *item) const;
  bool isItemVisible(DrawingItemBase *item) const;
  void setFilter(const QHash<QString, QStringList> &filter);

  std::vector<PolyLineInfo> loadCoordsFromKML(const std::string &fileName);
  EditItems::ItemGroup *itemGroup(const QString &name);
  void removeItemGroup(const QString &name);

public slots:
  std::vector<miutil::miTime> getTimes() const override;
  void setAllItemsVisible(bool enable);

signals:
  void drawingLoaded(const QString &name);
  void itemsClicked(const QList<DrawingItemBase *> &items);
  void itemsHovered(const QList<DrawingItemBase *> &items);
  void updated();

protected:
  void applyPlotOptions(DiGLPainter *gl, const DrawingItemBase *) const;

  static Rectangle editRect_;

  // Drawing definitions held in the setup file and those that have been loaded.
  QMap<QString, QString> drawings_;
  QMap<QString, QString> loaded_;

  QMap<QString, EditItems::ItemGroup *> itemGroups_;
  QMap<QString, QDateTime> lastUpdated_;
  QHash<QString, QStringList> filter_;
  bool allItemsVisible_;

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
