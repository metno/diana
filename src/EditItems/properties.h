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

#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <QDateTime>
#include <QDialog>
#include <QHash>
#include <QLineEdit>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QVariantMap>

class QCheckBox;
class QComboBox;
class QDateTimeEdit;
class QDialogButtonBox;
class QDoubleSpinBox;
class QFormLayout;
class QLineEdit;
class QSpinBox;
class QWidget;

class DrawingItemBase;

namespace Properties {

class EditProperty;     // Defined below.

class PropertiesEditor : public QDialog
{
  Q_OBJECT

public:
  static PropertiesEditor *instance();
  bool canEdit(const QList<DrawingItemBase *> &items) const;
  void edit(const QList<DrawingItemBase *> &, bool readOnly = false, bool modal = true);
  QStringList propertyRules(const QString &rule) const;
  void setPropertyRules(const QString &rule, const QStringList &values);

private slots:
  void reset();
  void updateButtons();

private:
  PropertiesEditor();
  QMap<QString, QVariant> commonProperties(const QList<DrawingItemBase *> &items) const;
  void registerProperty(const QString &name, EditProperty *property);

  static PropertiesEditor *instance_;
  QHash<QString, QStringList> rules_;

  QFormLayout *formLayout_;
  QList<DrawingItemBase *> items_;
  QHash<QString, EditProperty *> editors_;
  QSet<QString> editing_;
  QDialogButtonBox *buttonBox;
};

class EditProperty : public QObject
{
  Q_OBJECT

public:
  EditProperty(const QString &labelText);
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

class EP_Int : public EditProperty
{
  Q_OBJECT

public:
  EP_Int(const QString &labelText, int min, int max);
  virtual QWidget *createEditor(const QVariant &value);
  virtual void reset();

private slots:
  void updateValue(int value);

private:
  int min, max;
  QSpinBox *editor;
};

class EP_Float : public EditProperty
{
  Q_OBJECT

public:
  EP_Float(const QString &labelText, float min, float max);
  virtual QWidget *createEditor(const QVariant &value);
  virtual void reset();

private slots:
  void updateValue(double value);

private:
  float min, max;
  QDoubleSpinBox *editor;
};

class EP_Boolean : public EditProperty
{
  Q_OBJECT

public:
  EP_Boolean(const QString &labelText) : EditProperty(labelText) {}
  virtual QWidget *createEditor(const QVariant &value);
  virtual void reset();

private slots:
  void updateValue(int value);

private:
  QCheckBox *editor;
};

class EP_Time : public EditProperty
{
  Q_OBJECT

public:
  EP_Time(const QString &labelText) : EditProperty(labelText) {}
  virtual QWidget *createEditor(const QVariant &value);
  virtual void reset();

private slots:
  void updateValue(const QDateTime &value);

private:
  QDateTimeEdit *editor;
};

class EP_Choice : public EditProperty
{
public:
  EP_Choice(const QString &labelText) : EditProperty(labelText) {}
  virtual void reset();

protected:
  QComboBox *editor;
};

class EP_Colour : public EP_Choice
{
  Q_OBJECT

public:
  EP_Colour(const QString &labelText) : EP_Choice(labelText) {}
  virtual QWidget *createEditor(const QVariant &value);

private slots:
  void updateValue(const QString &value);
};

class EP_Width : public EP_Choice
{
public:
  EP_Width(const QString &labelText) : EP_Choice(labelText) {}
  virtual QWidget *createEditor(const QVariant &value);
};

class EP_LinePattern : public EP_Choice
{
public:
  EP_LinePattern(const QString &labelText) : EP_Choice(labelText) {}
  virtual QWidget *createEditor(const QVariant &value);
};

class EP_Decoration : public EP_Choice
{
public:
  EP_Decoration(const QString &labelText) : EP_Choice(labelText) {}
  virtual QWidget *createEditor(const QVariant &value);
};

class EP_FillPattern : public EP_Choice
{
public:
  EP_FillPattern(const QString &labelText) : EP_Choice(labelText) {}
  virtual QWidget *createEditor(const QVariant &value);
};

class EP_StringList : public EditProperty
{
  Q_OBJECT

public:
  EP_StringList(const QString &labelText) : EditProperty(labelText) {}
  virtual QWidget *createEditor(const QVariant &value);
  virtual void reset();

private slots:
  void updateValue();

private:
  QTextEdit *editor;
};

} // namespace

#endif // PROPERTIES_H
