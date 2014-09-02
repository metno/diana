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

#include <QObject>
#include <QHash>
#include <QList>
#include <QPainterPath>
#include <QPointF>
#include <QSet>
#include <QString>
#include <QVariantMap>
#include <QColor>

#include "diFontManager.h"
#include "diPlotOptions.h"
#include "EditItems/drawingitembase.h"

class DrawingItemBase;

class DrawingStyleProperty
{
public:
  virtual QVariant parse(const QHash<QString, QString> &) const = 0;
protected:
  static QString lineColour(const QHash<QString, QString> &);
  static QString fillColour(const QHash<QString, QString> &);
  static QString textColour(const QHash<QString, QString> &);
};

class DSP_linecolour : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_linetransparency : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_linewidth : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_linepattern : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_linesmooth : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_lineshape : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_fillcolour : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_filltransparency : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_fillpattern : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_closed : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_decoration1 : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_decoration1_colour : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_decoration1_offset : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_decoration2 : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_decoration2_colour : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_decoration2_offset : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_textcolour : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_fontname : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_fontface : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_fontsize : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_objects : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_values : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_styles : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

class DSP_layout : public DrawingStyleProperty
{
public:
  static QString name();
private:
  virtual QVariant parse(const QHash<QString, QString> &) const;
};

/**
  \brief Manager for drawing styles.
*/
class DrawingStyleManager
{
public:
  enum Side { Inside, Outside };

  DrawingStyleManager();
  virtual ~DrawingStyleManager();
  void addStyle(const DrawingItemBase::Category &category, const QHash<QString, QString> &definition);

  void setStyle(DrawingItemBase *, const QHash<QString, QString> &, const QString & = QString()) const;
  void setStyle(DrawingItemBase *, const QVariantMap &, const QString & = QString()) const;
  void setDefaultStyle(DrawingItemBase *) const;

  void beginLine(DrawingItemBase *item);
  void endLine(DrawingItemBase *item);
  void beginFill(DrawingItemBase *item);
  void endFill(DrawingItemBase *item);
  void beginText(const DrawingItemBase *item, FontManager *fp, float scale, const PlotOptions &poptions);
  void endText(const DrawingItemBase *item);

  void drawLines(const DrawingItemBase *item, const QList<QPointF> &points, int z = 0) const;
  void fillLoop(const DrawingItemBase *item, const QList<QPointF> &points) const;
  void setFont(const DrawingItemBase *item, FontManager *fp, float scale, const PlotOptions &poptions);

  static const QPainterPath interpolateToPath(const QList<QPointF> &points, bool closed);
  static const QList<QPointF> interpolateToPoints(const QList<QPointF> &points, bool closed);
  static const QList<QPointF> getDecorationLines(const QList<QPointF> &points, qreal lineLength);

  bool containsStyle(const DrawingItemBase::Category &category, const QString &name) const;
  QStringList styles(const DrawingItemBase::Category &category) const;
  QStringList properties(const DrawingItemBase::Category &category) const;
  QVariantMap getStyle(DrawingItemBase *item) const;
  QVariantMap getStyle(const DrawingItemBase *item) const;
  QVariantMap getStyle(const DrawingItemBase::Category &category, const QString &name) const;

  static DrawingStyleManager *instance();

private:
  void drawDecoration(const QVariantMap &style, const QString &decoration, bool closed,
                      const Side &side, const QList<QPointF> &points, int z,
                      unsigned int offset = 0) const;
  QVariantMap parse(const DrawingItemBase::Category &category,
                    const QHash<QString, QString> &definition) const;

  // Record the style maps for each category of object.
  QHash<DrawingItemBase::Category, QHash<QString, QVariantMap> > styles_;
  QHash<DrawingItemBase::Category, QHash<QString, DrawingStyleProperty *> > properties_;
  static DrawingStyleManager *self;  // singleton instance pointer
};

#endif // _diDrawingStyleManager_h
