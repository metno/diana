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
#ifndef _diDrawingStyleManager_h
#define _diDrawingStyleManager_h

#include <QHash>
#include <QList>
#include <QPainterPath>
#include <QPointF>
#include <QSet>
#include <QString>
#include <QVariantMap>
#include <QColor>

class DrawingItemBase;

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

  void setStyle(DrawingItemBase *, const QHash<QString, QString> &, const QString & = QString()) const;
  void setDefaultStyle(DrawingItemBase *) const;

  void beginLine(DrawingItemBase *item);
  void endLine(DrawingItemBase *item);
  void beginFill(DrawingItemBase *item);
  void endFill(DrawingItemBase *item);
  void beginText(DrawingItemBase *item);
  void endText(DrawingItemBase *item);

  void drawLines(const DrawingItemBase *item, const QList<QPointF> &points, int z = 0) const;
  void fillLoop(const DrawingItemBase *item, const QList<QPointF> &points) const;

  static const QPainterPath interpolateToPath(const QList<QPointF> &points, bool closed);
  static const QList<QPointF> interpolateToPoints(const QList<QPointF> &points, bool closed);
  static const QList<QPointF> getDecorationLines(const QList<QPointF> &points, qreal lineLength);

  bool contains(const QString &name) const;
  QStringList names() const;
  QVariantMap getStyle(DrawingItemBase *item) const;
  QVariantMap getStyle(const DrawingItemBase *item) const;
  QVariantMap getStyle(const QString &name) const;

  QList<QString> styleNames() const;

  static QString variantToString(const QVariant &);

  static DrawingStyleManager *instance();

private:
  void drawDecoration(const QVariantMap &style, const QString &decoration, bool closed,
                      const Side &side, const QList<QPointF> &points, int z,
                      unsigned int offset = 0) const;
  QVariantMap parse(const QHash<QString, QString> &definition) const;
  QColor parseColour(const QString &text) const;

  QMap<QString, QVariantMap> styles;
  static DrawingStyleManager *self;  // singleton instance pointer
};

#endif // _diDrawingStyleManager_h
