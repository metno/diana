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

#include <vector>
#include <puTools/miString.h>
#include <diCommonTypes.h>
#include <diDrawingTypes.h>
#include <diField/diGridConverter.h>
#include "diManager.h"
#include <diMapMode.h>
#include <QHash>
#include <QList>
#include <QObject>
#include <QPointF>
#include <QSet>
#include <QString>
#include <QVariantMap>

using namespace std;

class DrawingItemBase;
class PlotModule;
class ObjectManager;

class QAction;
class QKeyEvent;
class QMouseEvent;

/**
  \brief Manager for drawing areas and annotations.
*/

class DrawingManager : public Manager
{
  Q_OBJECT

public:
  DrawingManager();
  ~DrawingManager();

  /// parse DRAWING section of setup file (defines Drawing products)
  bool parseSetup();

  std::vector<miutil::miTime> getTimes() const;

  virtual bool changeProjection(const Area& newArea);
  bool prepare(const miutil::miTime &time);
  virtual void plot(bool under, bool over);
  virtual bool processInput(const std::vector<std::string>& inp);

  virtual void sendMouseEvent(QMouseEvent* event, EventResult& res) {}
  virtual void sendKeyboardEvent(QKeyEvent* event, EventResult& res) {}

  QList<QPointF> getLatLonPoints(DrawingItemBase* item) const;
  void setFromLatLonPoints(DrawingItemBase* item, const QList<QPointF> &latLonPoints);
  QList<QPointF> PhysToGeo(const QList<QPointF> &points) const;
  QList<QPointF> GeoToPhys(const QList<QPointF> &latLonPoints);

  QSet<DrawingItemBase *> getItems() const;

  static DrawingManager *instance();

protected:
  virtual void addItem_(DrawingItemBase *);
  virtual DrawingItemBase *createItemFromVarMap(const QVariantMap &, QString *);
  virtual void loadItemsFromFile(const QString &fileName);
  virtual void removeItem_(DrawingItemBase *item);

  Rectangle plotRect;
  Rectangle editRect;
  Area currentArea;

  QSet<DrawingItemBase *> items_;

private:
  GridConverter gc;   // gridconverter class

  static DrawingManager *self;  // singleton instance pointer
};

#endif
