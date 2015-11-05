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
#include <QSharedPointer>
#include <QPair>
#include <QCheckBox>
#include <EditItems/drawingstylemanager.h>
#include <EditItems/drawingitembase.h>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QFormLayout;
class QLineEdit;
class QSpinBox;
class QWidget;

namespace EditItemsStyle {

class EditStyleProperty;    // Defined below.

class StyleEditor : public QDialog
{
  Q_OBJECT

public:
  static StyleEditor *instance();
  void edit(const QSet<DrawingItemBase *> &);

private slots:
  void reset();
  void updateButtons();

private:
  StyleEditor();
  QMap<QString, QVariant> commonStyleProps(const QSet<DrawingItemBase *> &items);
  void registerProperty(const QString &name, EditStyleProperty *property);

  static StyleEditor *instance_;
  QFormLayout *formLayout_;
  QSet<DrawingItemBase *> items_;
  QHash<QString, EditStyleProperty *> properties_;
  QSet<QString> editing_;
  QDialogButtonBox *buttonBox;
};

class EditStyleProperty : public QObject
{
  Q_OBJECT

public:
  EditStyleProperty(const QString &labelText);
  virtual QWidget *createEditor(const QVariant &value);
  bool hasChanged() const;
  virtual void reset();

  QVariant oldValue;
  QVariant newValue;
  QString labelText;

signals:
  void updated();

protected slots:
  void updateValue(const QString &value);

private:
  QLineEdit *editor;
};

class ESP_Int : public EditStyleProperty
{
  Q_OBJECT

public:
  ESP_Int(const QString &labelText, int min, int max);
  virtual QWidget *createEditor(const QVariant &value);
  virtual void reset();

private slots:
  void updateValue(int value);

private:
  int min, max;
  QSpinBox *editor;
};

class ESP_Float : public EditStyleProperty
{
  Q_OBJECT

public:
  ESP_Float(const QString &labelText, float min, float max);
  virtual QWidget *createEditor(const QVariant &value);
  virtual void reset();

private slots:
  void updateValue(float value);

private:
  float min, max;
  QDoubleSpinBox *editor;
};

class ESP_Boolean : public EditStyleProperty
{
  Q_OBJECT

public:
  ESP_Boolean(const QString &labelText) : EditStyleProperty(labelText) {}
  virtual QWidget *createEditor(const QVariant &value);
  virtual void reset();

private slots:
  void updateValue(int value);

private:
  QCheckBox *editor;
};

class ESP_Choice : public EditStyleProperty
{
public:
  ESP_Choice(const QString &labelText) : EditStyleProperty(labelText) {}
  virtual void reset();

protected:
  QComboBox *editor;
};

class ESP_Colour : public ESP_Choice
{
  Q_OBJECT

public:
  ESP_Colour(const QString &labelText) : ESP_Choice(labelText) {}
  virtual QWidget *createEditor(const QVariant &value);

private slots:
  void updateValue(const QString &value);
};

class ESP_Width : public ESP_Choice
{
public:
  ESP_Width(const QString &labelText) : ESP_Choice(labelText) {}
  virtual QWidget *createEditor(const QVariant &value);
};

class ESP_LinePattern : public ESP_Choice
{
public:
  ESP_LinePattern(const QString &labelText) : ESP_Choice(labelText) {}
  virtual QWidget *createEditor(const QVariant &value);
};

class ESP_Decoration : public ESP_Choice
{
public:
  ESP_Decoration(const QString &labelText) : ESP_Choice(labelText) {}
  virtual QWidget *createEditor(const QVariant &value);
};

class ESP_FillPattern : public ESP_Choice
{
public:
  ESP_FillPattern(const QString &labelText) : ESP_Choice(labelText) {}
  virtual QWidget *createEditor(const QVariant &value);
};

} // namespace

#endif // EDITITEMSSTYLE_H
