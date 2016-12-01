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
#ifndef _diDrawingStyleManager_h
#define _diDrawingStyleManager_h

#include <QObject>
#include <QHash>
#include <QList>
#include <QPainterPath>
#include <QPointF>
#include <QSet>
#include <QString>
#include <QVariantMap>
#include <QColor>

#include "EditItems/drawingitembase.h"

class DrawingStyleProperty;
class DiCanvas;

/**
  \brief Manager for drawing styles.
*/
class DrawingStyleManager
{
public:
  enum Side { Inside, Outside };

  enum StyleCategory { Invalid, General, Line, Area, Decoration, Symbol };
  enum LockCategory { LockNone, LockAlpha, LockColour };

  DrawingStyleManager();
  virtual ~DrawingStyleManager();

  void setCanvas(DiCanvas* canvas)
    { mCanvas = canvas; }
  DiCanvas* canvas() const
    { return mCanvas; }

  void addStyle(const DrawingItemBase::Category &category, const QHash<QString, QString> &definition);

  void setStyle(DrawingItemBase *, const QHash<QString, QString> &, const QString & = QString()) const;
  void setStyle(DrawingItemBase *, const QVariantMap &, const QString & = QString()) const;
  void setComplexTextList(const QStringList &strings);

  void beginLine(DiGLPainter* gl, DrawingItemBase *item);
  void endLine(DiGLPainter* gl, DrawingItemBase *item);
  void beginFill(DiGLPainter* gl, DrawingItemBase *item);
  void endFill(DiGLPainter* gl, DrawingItemBase *item);

  void drawText(DiGLPainter* gl, const DrawingItemBase *) const;

  void highlightPolyLine(DiGLPainter* gl, const DrawingItemBase *, const QList<QPointF> &, int, const QColor &, bool = false) const;
  QList<QPointF> linesForBBox(DrawingItemBase *item) const;
  QList<QPointF> linesForBBox(const QRectF &bbox, int cornerSegments, float cornerRadius, const QString &border) const;

  void drawLines(DiGLPainter* gl, const DrawingItemBase *item, const QList<QPointF> &points, int z = 0, bool = false) const;
  void fillLoop(DiGLPainter* gl, const DrawingItemBase *item, const QList<QPointF> &points) const;
  void setFont(const DrawingItemBase *item) const;

  void drawSymbol(DiGLPainter* gl, const DrawingItemBase *) const;

  static const QPainterPath interpolateToPath(const QList<QPointF> &points, bool closed);
  static const QList<QPointF> interpolateToPoints(const QList<QPointF> &points, bool closed);
  static const QList<QPointF> getDecorationLines(const QList<QPointF> &points, qreal lineLength);

  static QString styleCategoryName(const StyleCategory);

  bool containsStyle(const DrawingItemBase::Category &category, const QString &name) const;
  QStringList styles(const DrawingItemBase::Category &category) const;
  QStringList properties(const DrawingItemBase::Category &category) const;
  QVariantMap getStyle(DrawingItemBase *item) const;
  QVariantMap getStyle(const DrawingItemBase *item) const;
  QVariantMap getStyle(const DrawingItemBase::Category &category, const QString &name) const;
  QStringList getComplexTextList() const;

  QImage toImage(const DrawingItemBase::Category &category, const QString &name,
                 const QString &value = QString()) const;

  static DrawingStyleManager *instance();

private:
  void drawDecoration(DiGLPainter* gl, const DrawingItemBase *item, const QVariantMap &style,
                      const QString &decoration, bool closed, const Side &side,
                      const QList<QPointF> &points, int z, unsigned int offset = 0) const;
  QVariantMap parse(const DrawingItemBase::Category &category,
                    const QHash<QString, QString> &definition) const;

  // Record the style maps for each category of object.
  QHash<DrawingItemBase::Category, QHash<QString, QVariantMap> > styles_;
  QHash<DrawingItemBase::Category, QHash<QString, DrawingStyleProperty *> > properties_;
  static DrawingStyleManager *self_;  // singleton instance pointer
  QStringList complexTextList_;
  DiCanvas* mCanvas;
};

class DrawingStyleProperty
{
public:
  DrawingStyleProperty();
  virtual ~DrawingStyleProperty() { }
  virtual QVariant parse(const QString &text) const;
  QString name;

protected:
  static QString symbolColour(const QHash<QString, QString> &);
  static QString textColour(const QHash<QString, QString> &);
};

class DSP_Colour : public DrawingStyleProperty
{
public:
  DSP_Colour(const QString &text);
  virtual QVariant parse(const QString &text) const;

private:
  QString defaultColour;
};

class DSP_Int : public DrawingStyleProperty
{
public:
  virtual QVariant parse(const QString &text) const;
};

class DSP_Alpha : public DSP_Int
{
public:
  DSP_Alpha(int defaultValue);
  virtual QVariant parse(const QString &text) const;

private:
  int defaultAlpha;
};

class DSP_Float : public DrawingStyleProperty
{
public:
  virtual QVariant parse(const QString &text) const;
};

class DSP_Width : public DSP_Float
{
public:
  virtual QVariant parse(const QString &text) const;
};

class DSP_LinePattern : public DrawingStyleProperty
{
public:
  virtual QVariant parse(const QString &text) const;
};

class DSP_Boolean : public DrawingStyleProperty
{
public:
  virtual QVariant parse(const QString &text) const;
};

class DSP_StringList : public DrawingStyleProperty
{
public:
  DSP_StringList(const QString &text);
  virtual QVariant parse(const QString &text) const;

private:
  QString separator;
};

#endif // _diDrawingStyleManager_h
