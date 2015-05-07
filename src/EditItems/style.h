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
#include <QSet>
#include <QList>
#include <QMap>
#include <QVariantMap>
#include <QLabel>
//#define QT_SHAREDPOINTER_TRACK_POINTERS
#include <QSharedPointer>
#include <QPair>
#include <QCheckBox>
#include <EditItems/drawingstylemanager.h>
#include <EditItems/drawingitembase.h>

class QWidget;
class QComboBox;
class QCheckBox;
class QSpinBox;

namespace EditItemsStyle {

class EditStyleProperty;

class IndexedEditor : public QObject
{
  Q_OBJECT
public:
  virtual QWidget *widget() = 0;
  virtual int count() const = 0;
  virtual QVariant itemData(int) const = 0;
  virtual void setCurrentIndex(int) = 0;
signals:
  void currentIndexChanged(int);
};

class ComboBoxEditor : public IndexedEditor
{
public:
  ComboBoxEditor(QComboBox *);
  ~ComboBoxEditor();
private:
  QComboBox *comboBox_;
  virtual QWidget *widget();
  virtual int count() const;
  virtual QVariant itemData(int i) const;
  virtual void setCurrentIndex(int i);
};

class CheckBoxEditor : public IndexedEditor
{
  Q_OBJECT
public:
  CheckBoxEditor();
  ~CheckBoxEditor();
private:
  QCheckBox *checkBox_;
  virtual QWidget *widget();
  virtual int count() const;
  virtual QVariant itemData(int i) const;
  virtual void setCurrentIndex(int i);
private slots:
  void handleClicked(bool);
};

class IntRangeEditor : public IndexedEditor
{
  Q_OBJECT
public:
  IntRangeEditor(int, int);
  ~IntRangeEditor();
private:
  QSpinBox *spinBox_;
  virtual QWidget *widget();
  virtual int count() const;
  virtual QVariant itemData(int i) const;
  virtual void setCurrentIndex(int i);
private slots:
  void handleValueChanged(int);
};

class StylePropertyEditor : public QObject
{
  Q_OBJECT
public:
  virtual ~StylePropertyEditor();
  void init(bool, const QSet<DrawingItemBase *> &, const QVariant &);
  virtual QString labelText() const = 0;
  QWidget *widget();
  void reset();
  void setCurrentIndex(int);
  void updateItems(int);
  virtual DrawingStyleManager::LockCategory lockCategory() const;
protected:
  StylePropertyEditor();
private:
  virtual IndexedEditor *createEditor() = 0;
  virtual QString name() const = 0;
  QSet<DrawingItemBase *> items_;
  IndexedEditor *editor_;
  QVariant origInitVal_;
  bool lockingEnabled_;
  void setCurrentIndex(const QVariant &);
private slots:
  void handleCurrentIndexChanged(int);
};

class StyleEditor : public QDialog
{
  Q_OBJECT
public:
  static StyleEditor *instance();
  void edit(const QSet<DrawingItemBase *> &);
  QList<QSharedPointer<StylePropertyEditor> > lockedEditors(StylePropertyEditor *);
private:
  StyleEditor();
  static StyleEditor *instance_;
  QWidget *formWidget_;
  QSet<DrawingItemBase *> items_;
  QMap<DrawingItemBase *, QVariantMap> savedProps_;
  QList<QSharedPointer<StylePropertyEditor> > editors_;
  QList<QSharedPointer<QLabel> > formLabels_;
  QMap<QString, QCheckBox *> lockedCheckBoxes_;
  QHash<QString, EditStyleProperty *> properties_;
  QHash<DrawingStyleManager::LockCategory, QList<QPair<QSharedPointer<StylePropertyEditor>, QCheckBox *> > > lockedEditors_;
private slots:
  void reset();
};

} // namespace

#endif // EDITITEMSSTYLE_H
