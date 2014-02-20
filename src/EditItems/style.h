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

#ifndef EDITITEMSSTYLE_H
#define EDITITEMSSTYLE_H

#include <QDialog>
#include <QComboBox>
#include <QSet>
#include <QVariantMap>

class DrawingItemBase;
class QWidget;

namespace EditItemsStyle {

class StylePropertyEditor : public QObject
{
  Q_OBJECT
public:
  static StylePropertyEditor *create(const QString &, const QSet<DrawingItemBase *> &, const QVariantMap &);
  virtual QString labelText() const = 0;
  QComboBox *comboBox();
  void reset();
protected:
  StylePropertyEditor();
private:
  virtual QComboBox *createComboBox() = 0;
  virtual QString name() const = 0;
  QSet<DrawingItemBase *> items_;
  QComboBox *comboBox_;
  QVariant origInitVal_;
  void init(bool, const QSet<DrawingItemBase *> &, const QVariant &);
  void setCurrentIndex(const QVariant &);
private slots:
  void handleCurrentIndexChanged(int);
};

class LineTypeEditor : public StylePropertyEditor
{
public:
  virtual QString name() const { return "lineType"; }
private:
  virtual QString labelText() const { return "line type"; }
  virtual QComboBox *createComboBox();
};

class LineWidthEditor : public StylePropertyEditor
{
public:
  virtual QString name() const { return "lineWidth"; }
private:
  virtual QString labelText() const { return "line width"; }
  virtual QComboBox *createComboBox();
};

class LineColorEditor : public StylePropertyEditor
{
public:
  virtual QString name() const { return "lineColor"; }
private:
  virtual QString labelText() const { return "line color"; }
  virtual QComboBox *createComboBox();
};

class StyleEditor : public QDialog
{
  Q_OBJECT
public:
  static StyleEditor *instance();
  void edit(const QSet<DrawingItemBase *> &);
private:
  StyleEditor();
  static StyleEditor *instance_;
  QWidget *formWidget_;
  QSet<DrawingItemBase *> items_;
  QMap<DrawingItemBase *, QVariantMap> savedProps_;
  QList<QSharedPointer<StylePropertyEditor> > editors_;
private slots:
  void reset();
};

} // namespace

#endif // EDITITEMSSTYLE_H
