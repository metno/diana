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
#include <QMap>
#include <QPointF>
#include <QSet>
#include <QString>
#include <QVariantMap>
//#define QT_SHAREDPOINTER_TRACK_POINTERS
#include <QSharedPointer>
#include <vector>

#if !defined(USE_PAINTGL)
#include <qgl.h>
#else
#include "PaintGL/paintgl.h"
#define QGLWidget PaintGLWidget
#endif

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

  virtual void sendMouseEvent(QMouseEvent* event, EventResult& res);
  virtual void sendKeyboardEvent(QKeyEvent* event, EventResult& res) {}

  QList<QSharedPointer<DrawingItemBase> > findHitItems(
    const QPointF &pos, QList<QSharedPointer<DrawingItemBase> > *missedItems) const;

  QList<QPointF> getLatLonPoints(const DrawingItemBase &item) const;
  void setFromLatLonPoints(DrawingItemBase &item, const QList<QPointF> &latLonPoints) const;
  QList<QPointF> PhysToGeo(const QList<QPointF> &points) const;
  QList<QPointF> GeoToPhys(const QList<QPointF> &latLonPoints) const;

  virtual DrawingItemBase *createItem(const QString &type);
  virtual QSharedPointer<DrawingItemBase> createItemFromVarMap(const QVariantMap &vmap, QString *error);

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
  EditItems::LayerManager *getLayerManager();
  EditItems::LayerManager *getAuxLayerManager();

  QString getWorkDir() const;
  void setWorkDir(const QString &dir);

  void setEditRect(Rectangle r);

  virtual std::vector<PlotElement> getPlotElements() const;
  virtual QString plotElementTag() const;
  void enablePlotElement(const PlotElement &);

  int nextJoinId(bool = true);
  void separateJoinIds(const QList<QSharedPointer<DrawingItemBase> > &);

public slots:
  std::vector<miutil::miTime> getTimes() const;

signals:
  void itemsClicked(const QList<QSharedPointer<DrawingItemBase> > &items);
  void itemsHovered(const QList<QSharedPointer<DrawingItemBase> > &items);

protected:
  virtual void addItem_(const QSharedPointer<DrawingItemBase> &);
  virtual void removeItem_(const QSharedPointer<DrawingItemBase> &);
  void applyPlotOptions(const QSharedPointer<DrawingItemBase> &) const;

  static Rectangle editRect_;

  QMap<QString, QString> drawings_;
  QMap<QString, QString> loaded_;

  EditItems::LayerManager *layerMgr_; // Read by DrawingManager::plot() and EditItemManager::plot(). Read/written by EditDrawingDialog.

  mutable QHash<int, QSharedPointer<EditItems::Layer> > plotElems_;

private:

  GridConverter gc_;
  QString workDir_;

  QMap<QString, QSet<QString> > symbolSections_;
  QMap<QString, QByteArray> symbols_;
  mutable QHash<QString, QImage> imageCache_;
  DrawingStyleManager *styleManager_;

  static int nextJoinId_;
  void setNextJoinId(int);

  static DrawingManager *self_;  // singleton instance pointer
};

#endif // _diDrawingManager_h
