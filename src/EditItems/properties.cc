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

#include <diEditItemManager.h>
#include <EditItems/drawingstylemanager.h>
#include <EditItems/drawingitembase.h>
#include <EditItems/properties.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QVariantMap>
#include <QVBoxLayout>
#include "qtUtility.h"

#define MILOGGER_CATEGORY "diana.EditItems.Properties"
#include <miLogger/miLogging.h>

namespace Properties {

static const QString PROPERTIES("properties");

EditProperty::EditProperty(const QString &labelText)
{
  this->labelText = labelText;
}

QWidget *EditProperty::createEditor(const QVariant &value)
{
  editor = new QLineEdit();
  editor->setText(value.toString());
  connect(editor, SIGNAL(textChanged(QString)), SLOT(updateValue(QString)));
  oldValue = value;
  newValue = value;
  return editor;
}

bool EditProperty::hasChanged() const
{
  return oldValue != newValue;
}

void EditProperty::reset()
{
  editor->setText(oldValue.toString());
  updateValue(oldValue.toString());
}

void EditProperty::updateValue(const QString &value)
{
  newValue = value;
  emit updated();
}

EP_Int::EP_Int(const QString &labelText, int min, int max)
 : EditProperty(labelText)
{
  this->min = min;
  this->max = max;
}

QWidget *EP_Int::createEditor(const QVariant &value)
{
  editor = new QSpinBox();
  editor->setRange(min, max);
  editor->setValue(value.toInt());
  connect(editor, SIGNAL(valueChanged(int)), SLOT(updateValue(int)));
  oldValue = value;
  newValue = value;
  return editor;
}

void EP_Int::reset()
{
  editor->setValue(oldValue.toInt());
  updateValue(oldValue.toInt());
}

void EP_Int::updateValue(int value)
{
  newValue = value;
  emit updated();
}

EP_Float::EP_Float(const QString &labelText, float min, float max)
 : EditProperty(labelText)
{
  this->min = min;
  this->max = max;
}

QWidget *EP_Float::createEditor(const QVariant &value)
{
  editor = new QDoubleSpinBox();
  editor->setRange(min, max);
  editor->setValue(value.toDouble());
  connect(editor, SIGNAL(valueChanged(double)), SLOT(updateValue(double)));
  oldValue = value;
  newValue = value;
  return editor;
}

void EP_Float::reset()
{
  editor->setValue(oldValue.toDouble());
  updateValue(oldValue.toDouble());
}

void EP_Float::updateValue(double value)
{
  newValue = value;
  emit updated();
}

QWidget *EP_Boolean::createEditor(const QVariant &value)
{
  editor = new QCheckBox();
  editor->setChecked(value.toBool());
  connect(editor, SIGNAL(stateChanged(int)), SLOT(updateValue(int)));
  oldValue = value;
  newValue = value;
  return editor;
}

void EP_Boolean::reset()
{
  editor->setChecked(oldValue.toBool());
  updateValue(oldValue.toBool() ? Qt::Checked : Qt::Unchecked);
}

void EP_Boolean::updateValue(int value)
{
  newValue = (value == Qt::Checked);
  emit updated();
}

QWidget *EP_Time::createEditor(const QVariant &value)
{
  editor = new QDateTimeEdit();
  QDateTime dateTime = QDateTime::fromString(value.toString(), "yyyy-MM-ddThh:mm:ssZ");
  editor->setDateTime(dateTime);
  connect(editor, SIGNAL(dateTimeChanged(QDateTime)), SLOT(updateValue(QDateTime)));
  oldValue = value;
  newValue = value;
  return editor;
}

void EP_Time::reset()
{
  QDateTime dateTime = QDateTime::fromString(oldValue.toString(), "yyyy-MM-ddThh:mm:ssZ");
  editor->setDateTime(dateTime);
  updateValue(dateTime);
}

void EP_Time::updateValue(const QDateTime &value)
{
  newValue = value.toString(Qt::ISODate) + "Z";
  emit updated();
}

void EP_Choice::reset()
{
  editor->setCurrentIndex(editor->findData(oldValue));
  updateValue(oldValue.toString());
}

QWidget *EP_Colour::createEditor(const QVariant &value)
{
  editor = ColourBox(0, true, 0, "", true);
  editor->setCurrentIndex(editor->findData(value));
  connect(editor, SIGNAL(currentIndexChanged(QString)), SLOT(updateValue(QString)));
  oldValue = value;
  newValue = value;
  return editor;
}

void EP_Colour::updateValue(const QString &value)
{
  // Map the colour name sent by the editor to its value and set that as the
  // new value.
  int index = editor->findText(value);
  QString s;

  if (index != -1)
    s = editor->itemData(index).value<QColor>().name();
  else
    s = "black";
  newValue = s;
  EP_Choice::updateValue(s);
}

QWidget *EP_Width::createEditor(const QVariant &value)
{
  editor = LinewidthBox(0, true);
  editor->setCurrentIndex(editor->findData(value));
  connect(editor, SIGNAL(currentIndexChanged(QString)), SLOT(updateValue(QString)));
  oldValue = value;
  newValue = value;
  return editor;
}

QWidget *EP_LinePattern::createEditor(const QVariant &value)
{
  editor = LinetypeBox(0, true);
  editor->setCurrentIndex(editor->findData(value));
  connect(editor, SIGNAL(currentIndexChanged(QString)), SLOT(updateValue(QString)));
  oldValue = value;
  newValue = value;
  return editor;
}

QWidget *EP_Decoration::createEditor(const QVariant &value)
{
  editor = new QComboBox();
  editor->addItem("none", "");
  editor->addItem("triangles", "triangles");
  editor->addItem("arches", "arches");
  editor->addItem("crosses", "crosses");
  editor->addItem("arrow", "arrow");
  editor->addItem("SIGWX", "SIGWX");
  editor->addItem("arches,triangles", "arches,triangles");
  editor->addItem("jetstream", "arrow,jetstream");
  editor->addItem("fishbone", "fishbone");
  int index = editor->findData(value.toString());
  editor->setCurrentIndex(index);
  connect(editor, SIGNAL(currentIndexChanged(QString)), SLOT(updateValue(QString)));
  oldValue = value;
  newValue = value;
  return editor;
}

QWidget *EP_FillPattern::createEditor(const QVariant &value)
{
  editor = new QComboBox();
  editor->addItem("solid", "");
  editor->addItem("diagleft", "diagleft");
  editor->addItem("zigzag", "zigzag");
  editor->addItem("paralyse", "paralyse");
  editor->addItem("ldiagleft2", "ldiagleft2");
  editor->addItem("vdiagleft", "vdiagleft");
  editor->addItem("vldiagcross_little", "vldiagcross_little");
  editor->addItem("snow", "snow");
  editor->addItem("rain", "rain");
  editor->setCurrentIndex(editor->findData(value.toString()));
  connect(editor, SIGNAL(currentIndexChanged(QString)), SLOT(updateValue(QString)));
  oldValue = value;
  newValue = value;
  return editor;
}

QWidget *EP_StringList::createEditor(const QVariant &value)
{
  editor = new QTextEdit();
  editor->setPlainText(value.toStringList().join("\n"));
  connect(editor, SIGNAL(textChanged()), SLOT(updateValue()));
  oldValue = value;
  newValue = value;
  return editor;
}

void EP_StringList::updateValue()
{
  // Split the lines of text.
  QStringList s = editor->toPlainText().split("\n");
  newValue = QVariant(s);
  emit updated();
}

void EP_StringList::reset()
{
  editor->setPlainText(oldValue.toStringList().join("\n"));
  updateValue();
}

PropertiesEditor::PropertiesEditor()
{
  setWindowTitle(tr("Item Properties"));

  QVBoxLayout *layout = new QVBoxLayout(this);
  formLayout_ = new QFormLayout();
  layout->addLayout(formLayout_);
  layout->addStretch();

  buttonBox = new QDialogButtonBox(QDialogButtonBox::Reset | QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
  connect(buttonBox->button(QDialogButtonBox::Reset), SIGNAL(clicked()), this, SLOT(reset()));
  connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
  connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
  layout->addWidget(buttonBox);

  // Define editors for supported properties.
  registerProperty("met:info:speed", new EP_Int(tr("Wind speed (knots)"), 0, 500));
  registerProperty("TimeSpan:begin", new EP_Time(tr("Beginning")));
  registerProperty("TimeSpan:end", new EP_Time(tr("Ending")));
  registerProperty("time", new EP_Time(tr("Time")));
  registerProperty("text", new EP_StringList(tr("Text")));

  // Define editors for supported style properties.
  registerProperty("style:linecolour", new EP_Colour(tr("Line colour")));
  registerProperty("style:linealpha", new EP_Int(tr("Line alpha"), 0, 255));
  registerProperty("style:linewidth", new EP_Width(tr("Line width")));
  registerProperty("style:linepattern", new EP_LinePattern(tr("Line pattern")));
  registerProperty("style:linesmooth", new EP_Boolean(tr("Line smooth")));
  registerProperty("style:fillcolour", new EP_Colour(tr("Fill colour")));
  registerProperty("style:fillalpha", new EP_Int(tr("Fill alpha"), 0, 255));
  registerProperty("style:fillpattern", new EP_FillPattern(tr("Fill pattern")));
  registerProperty("style:closed", new EP_Boolean(tr("Closed")));
  registerProperty("style:reversed", new EP_Boolean(tr("Reversed")));
  registerProperty("style:decoration1", new EP_Decoration(tr("Decoration 1")));
  registerProperty("style:decoration1.colour", new EP_Colour(tr("Decoration 1 colour")));
  registerProperty("style:decoration1.alpha", new EP_Int(tr("Decoration 1 alpha"), 0, 255));
  registerProperty("style:decoration1.offset", new EP_Int(tr("Decoration 1 offset"), 0, 3));
  registerProperty("style:decoration2", new EP_Decoration(tr("Decoration 2")));
  registerProperty("style:decoration2.colour", new EP_Colour(tr("Decoration 2 colour")));
  registerProperty("style:decoration2.alpha", new EP_Int(tr("Decoration 2 alpha"), 0, 255));
  registerProperty("style:decoration2.offset", new EP_Int(tr("Decoration 2 offset"), 0, 3));
  registerProperty("style:symbolcolour", new EP_Colour(tr("Symbol colour")));
  registerProperty("style:symbolalpha", new EP_Int(tr("Symbol alpha"), 0, 255));
  registerProperty("style:textcolour", new EP_Colour(tr("Text colour")));
  registerProperty("style:textalpha", new EP_Int(tr("Text alpha"), 0, 255));
  registerProperty("style:cornersegments", new EP_Int(tr("Corner segments"), 0, 8));
  registerProperty("style:cornerradius", new EP_Float(tr("Corner radius"), 0, 20));
  registerProperty("style:fontsize", new EP_Float(tr("Font size"), 1, 99));
}

PropertiesEditor *PropertiesEditor::instance()
{
  if (!instance_)
    instance_ = new PropertiesEditor;
  return instance_;
}

PropertiesEditor *PropertiesEditor::instance_ = 0;

void PropertiesEditor::registerProperty(const QString &name, EditProperty *property)
{
  editors_[name] = property;
  connect(property, SIGNAL(updated()), SLOT(updateButtons()));
}

/**
 * Returns the common properties of \a items, including those defined as
 * editable in each item's style.
 */
QMap<QString, QVariant> PropertiesEditor::commonProperties(const QList<DrawingItemBase *> &items) const
{
  QMap<QString, QVariant> common;

  foreach (DrawingItemBase *item, items) {

    // Each item's style has a list of properties that are relevant to it.
    QVariantMap style = DrawingStyleManager::instance()->getStyle(item);
    QStringList editable = style.value(PROPERTIES).toStringList();

    // this makes all style properties editable
    foreach (const QString &key, style.keys()) {
      if (!key.isEmpty() && key != PROPERTIES)
        editable << ("style:" + key);
    }
    editable.removeAll("");

    // Collect the properties defined as editable in its style, using default
    // values if not defined for the item.
    QMap<QString, QVariant> props;
    foreach (const QString &key, editable)
      props[key] = Drawing(item)->property(key);

    // Collect the common properties that all items can have.
    foreach (const QString &key, rules_.value("common-properties")) {
      QVariant value = Drawing(item)->property(key);
      if (value.isValid())
        props[key] = value;
    }

    // Keep all the properties for the first item but only common ones for
    // subsequent items.
    if (common.isEmpty())
      common = props;
    else {
      foreach (const QString &key, common.keys()) {
        if (!props.contains(key) || (common.value(key) != props.value(key)))
          common.remove(key);
      }
    }
  }

  return common;
}

/**
 * Opens a dialog to show the properties of \a item.
 * The properties may be modified if \a readOnly is false (the default).
 * The dialog is modal if \a modal is true (the default).
 */
void PropertiesEditor::edit(const QList<DrawingItemBase *> &items, bool readOnly, bool modal)
{
  // Record the items so that we can manipulate them in slots.
  items_ = items;

  // Clear old content.
  while (!formLayout_->isEmpty()) {
    QLayoutItem *child = formLayout_->takeAt(0);
    if (child != 0) {
      delete child->widget();
      delete child;
    }
  }

  // Clear the previous set of editable properties and obtain the properties
  // that are common to all items being edited.
  editing_.clear();

  QMap<QString, QVariant> common = commonProperties(items);

  foreach (const QString &name, common.keys()) {
    // Create an editor for each common style property.
    if (editors_.contains(name)) {
      EditProperty *prop = editors_.value(name);
      editing_.insert(name);

      QWidget *editor = prop->createEditor(common.value(name));
      formLayout_->addRow(prop->labelText, editor);
    }
  }

  if (editing_.isEmpty()) {
    formLayout_->addRow(new QLabel(tr("No editable properties")));
    buttonBox->button(QDialogButtonBox::Reset)->hide();
    buttonBox->button(QDialogButtonBox::Ok)->hide();
  } else {
    buttonBox->button(QDialogButtonBox::Reset)->show();
    buttonBox->button(QDialogButtonBox::Ok)->show();
  }

  // Open the dialog.
  if (exec() != QDialog::Accepted)
    reset();
}

/**
 * Returns true if any of the items contain properties that can be edited;
 * otherwise returns false.
 */
bool PropertiesEditor::canEdit(const QList<DrawingItemBase *> &items) const
{
  QMap<QString, QVariant> common = commonProperties(items);
  foreach (const QString &name, common.keys()) {
    if (editors_.contains(name))
      return true;
  }
  return false;
}

QStringList PropertiesEditor::propertyRules(const QString &name) const
{
  if (rules_.contains(name))
    return rules_.value(name);
  else
    return QStringList();
}

void PropertiesEditor::setPropertyRules(const QString &name, const QStringList &values)
{
  rules_[name] = values;
}

/**
 * Restores original values in the dialog.
 */
void PropertiesEditor::reset()
{
  // Reset the values of all the registered properties for all the items
  // with those properties.
  foreach (const QString &name, editing_) {
    EditProperty *prop = editors_.value(name);
    prop->reset();

    foreach (DrawingItemBase *item, items_)
      item->setProperty(name, prop->oldValue);
  }

  buttonBox->button(QDialogButtonBox::Reset)->setEnabled(false);
  EditItemManager::instance()->repaint();
}

void PropertiesEditor::updateButtons()
{
  // Check the values of all the registered properties.
  bool changed = false;

  foreach (const QString &name, editing_) {
    EditProperty *prop = editors_.value(name);

    if (prop->hasChanged())
      changed = true;

    foreach (DrawingItemBase *item, items_)
      item->setProperty(name, prop->newValue);
  }

  buttonBox->button(QDialogButtonBox::Reset)->setEnabled(changed);
  EditItemManager::instance()->repaint();
}

} // namespace
