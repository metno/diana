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

#include <EditItems/edititembase.h>
#include <EditItems/style.h>
#include <diEditItemManager.h>

#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVariantMap>
#include <QComboBox>
#include <QSpinBox>
#include "qtUtility.h"

namespace EditItemsStyle {

EditStyleProperty::EditStyleProperty(const QString &labelText)
{
  this->labelText = labelText;
}

QWidget *EditStyleProperty::createEditor(const QVariant &value)
{
  editor = new QLineEdit();
  editor->setText(value.toString());
  connect(editor, SIGNAL(textChanged(QString)), SLOT(updateValue(QString)));
  oldValue = value;
  newValue = value;
  return editor;
}

bool EditStyleProperty::hasChanged() const
{
  return oldValue != newValue;
}

void EditStyleProperty::reset()
{
  editor->setText(oldValue.toString());
  updateValue(oldValue.toString());
}

void EditStyleProperty::updateValue(const QString &value)
{
  newValue = value;
  emit updated();
}

ESP_Int::ESP_Int(const QString &labelText, int min, int max)
 : EditStyleProperty(labelText)
{
  this->min = min;
  this->max = max;
}

QWidget *ESP_Int::createEditor(const QVariant &value)
{
  editor = new QSpinBox();
  editor->setRange(min, max);
  editor->setValue(value.toInt());
  connect(editor, SIGNAL(valueChanged(int)), SLOT(updateValue(int)));
  oldValue = value;
  newValue = value;
  return editor;
}

void ESP_Int::reset()
{
  editor->setValue(oldValue.toInt());
  updateValue(oldValue.toInt());
}

void ESP_Int::updateValue(int value)
{
  newValue = value;
  emit updated();
}

ESP_Float::ESP_Float(const QString &labelText, float min, float max)
 : EditStyleProperty(labelText)
{
  this->min = min;
  this->max = max;
}

QWidget *ESP_Float::createEditor(const QVariant &value)
{
  editor = new QDoubleSpinBox();
  editor->setRange(min, max);
  editor->setValue(value.toDouble());
  connect(editor, SIGNAL(currentIndexChanged(QString)), SLOT(updateValue(QString)));
  oldValue = value;
  newValue = value;
  return editor;
}

void ESP_Float::reset()
{
  editor->setValue(oldValue.toDouble());
  updateValue(oldValue.toDouble());
}

void ESP_Float::updateValue(float value)
{
  newValue = value;
  emit updated();
}

QWidget *ESP_Boolean::createEditor(const QVariant &value)
{
  editor = new QCheckBox();
  editor->setChecked(value.toBool());
  connect(editor, SIGNAL(stateChanged(int)), SLOT(updateValue(int)));
  oldValue = value;
  newValue = value;
  return editor;
}

void ESP_Boolean::reset()
{
  editor->setChecked(oldValue.toBool());
  updateValue(oldValue.toBool() ? Qt::Checked : Qt::Unchecked);
}

void ESP_Boolean::updateValue(int value)
{
  newValue = (value == Qt::Checked);
  emit updated();
}

void ESP_Choice::reset()
{
  editor->setCurrentIndex(editor->findData(oldValue));
  updateValue(oldValue.toString());
}

QWidget *ESP_Colour::createEditor(const QVariant &value)
{
  editor = ColourBox(0, true, 0, "", true);
  editor->setCurrentIndex(editor->findData(value));
  connect(editor, SIGNAL(currentIndexChanged(QString)), SLOT(updateValue(QString)));
  oldValue = value;
  newValue = value;
  return editor;
}

void ESP_Colour::updateValue(const QString &value)
{
  // Map the colour name sent by the editor to its value and set that as the
  // new value.
  int index = editor->findText(value);
  QString s;

  if (index != -1)
    s = editor->itemData(index).value<QColor>().name();
  else
    s = "black";

  ESP_Choice::updateValue(s);
}

QWidget *ESP_Width::createEditor(const QVariant &value)
{
  editor = LinewidthBox(0, true);
  editor->setCurrentIndex(editor->findData(value));
  connect(editor, SIGNAL(currentIndexChanged(QString)), SLOT(updateValue(QString)));
  oldValue = value;
  newValue = value;
  return editor;
}

QWidget *ESP_LinePattern::createEditor(const QVariant &value)
{
  editor = LinetypeBox(0, true);
  editor->setCurrentIndex(editor->findData(value));
  connect(editor, SIGNAL(currentIndexChanged(QString)), SLOT(updateValue(QString)));
  oldValue = value;
  newValue = value;
  return editor;
}

QWidget *ESP_Decoration::createEditor(const QVariant &value)
{
  editor = new QComboBox();
  editor->addItem("none", "");
  editor->addItem("triangles", "triangles");
  editor->addItem("arches", "arches");
  editor->addItem("crosses", "crosses");
  editor->addItem("arrow", "arrow");
  editor->addItem("SIGWX", "SIGWX");
  editor->addItem("arches,triangles", "arches,triangles");
  editor->addItem("jetstream", "jetstream");
  int index = editor->findData(value.toString());
  editor->setCurrentIndex(index);
  connect(editor, SIGNAL(currentIndexChanged(QString)), SLOT(updateValue(QString)));
  oldValue = value;
  newValue = value;
  return editor;
}

QWidget *ESP_FillPattern::createEditor(const QVariant &value)
{
  editor = new QComboBox();
  editor->addItem("solid", "");
  editor->addItem("diagleft", "diagleft");
  editor->addItem("zigzag", "zigzag");
  editor->addItem("paralyse", "paralyse");
  editor->addItem("ldiagleft2", "ldiagleft2");
  editor->addItem("vdiagleft", "vdiagleft");
  editor->addItem("vldiagcross_little", "vldiagcross_little");
  editor->setCurrentIndex(editor->findData(value.toString()));
  connect(editor, SIGNAL(currentIndexChanged(QString)), SLOT(updateValue(QString)));
  oldValue = value;
  newValue = value;
  return editor;
}

StyleEditor::StyleEditor()
{
  setWindowTitle(tr("Edit Style"));

  QVBoxLayout *layout = new QVBoxLayout(this);
  formLayout_ = new QFormLayout();
  layout->addLayout(formLayout_);

  buttonBox = new QDialogButtonBox(QDialogButtonBox::Reset | QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
  connect(buttonBox->button(QDialogButtonBox::Reset), SIGNAL(clicked()), this, SLOT(reset()));
  connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
  connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
  layout->addWidget(buttonBox);

  // Define editors for supported style properties.
  registerProperty("style:linecolour", new ESP_Colour(tr("Line colour")));
  registerProperty("style:linealpha", new ESP_Int(tr("Line alpha"), 0, 255));
  registerProperty("style:linewidth", new ESP_Width(tr("Line width")));
  registerProperty("style:linepattern", new ESP_LinePattern(tr("Line pattern")));
  registerProperty("style:linesmooth", new ESP_Boolean(tr("Line smooth")));
  registerProperty("style:fillcolour", new ESP_Colour(tr("Fill colour")));
  registerProperty("style:fillalpha", new ESP_Int(tr("Fill alpha"), 0, 255));
  registerProperty("style:fillpattern", new ESP_FillPattern(tr("Fill pattern")));
  registerProperty("style:closed", new ESP_Boolean(tr("Closed")));
  registerProperty("style:reversed", new ESP_Boolean(tr("Reversed")));
  registerProperty("style:decoration1", new ESP_Decoration(tr("Decoration 1")));
  registerProperty("style:decoration1.colour", new ESP_Colour(tr("Decoration 1 colour")));
  registerProperty("style:decoration1.alpha", new ESP_Int(tr("Decoration 1 alpha"), 0, 255));
  registerProperty("style:decoration1.offset", new ESP_Int(tr("Decoration 1 offset"), 0, 3));
  registerProperty("style:decoration2", new ESP_Decoration(tr("Decoration 2")));
  registerProperty("style:decoration2.colour", new ESP_Colour(tr("Decoration 2 colour")));
  registerProperty("style:decoration2.alpha", new ESP_Int(tr("Decoration 2 alpha"), 0, 255));
  registerProperty("style:decoration2.offset", new ESP_Int(tr("Decoration 2 offset"), 0, 3));
  registerProperty("style:symbolcolour", new ESP_Colour(tr("Symbol colour")));
  registerProperty("style:symbolalpha", new ESP_Int(tr("Symbol alpha"), 0, 255));
  registerProperty("style:textcolour", new ESP_Colour(tr("Text colour")));
  registerProperty("style:textalpha", new ESP_Int(tr("Text alpha"), 0, 255));
  registerProperty("style:cornersegments", new ESP_Int(tr("Corner segments"), 0, 8));
  registerProperty("style:cornerradius", new ESP_Float(tr("Corner radius"), 0, 99));
  registerProperty("style:fontsize", new ESP_Float(tr("Font size"), 1, 99));
}

void StyleEditor::registerProperty(const QString &name, EditStyleProperty *property)
{
  properties_[name] = property;
  connect(property, SIGNAL(updated()), SLOT(updateButtons()));
}

StyleEditor *StyleEditor::instance()
{
  if (!instance_)
    instance_ = new StyleEditor;
  return instance_;
}

StyleEditor *StyleEditor::instance_ = 0;

// Returns the common style properties of \a items.
QMap<QString, QVariant> StyleEditor::commonStyleProps(const QSet<DrawingItemBase *> &items)
{
  QMap<QString, QVariant> common;
  QRegExp rx("style:(.+)");

  foreach (DrawingItemBase *item, items) {

    // Only examine style properties.
    QStringList keys = item->propertiesRef().keys();
    keys = keys.filter(rx);

    // Collect the style properties from this item.
    QMap<QString, QVariant> props;
    foreach (const QString &key, keys)
      props[key] = Drawing(item)->property(key);

    // Keep all the style properties for the first item and keep only
    // common ones for subsequent items.
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

// Opens a modal dialog to edit the style properties of \a items.
void StyleEditor::edit(const QSet<DrawingItemBase *> &items)
{
  if (items.isEmpty())
    return;

  // Record the items so that we can manipulate them.
  items_ = items;

  DrawingItemBase *item = *(items.begin());
  DrawingItemBase::Category itemCategory = item->category();

  // Clear old content.
  while (!formLayout_->isEmpty()) {
    QLayoutItem *child = formLayout_->takeAt(0);
    if (child != 0) {
      delete child->widget();
      delete child;
    }
  }

  QMap<QString, QVariant> commonProps = commonStyleProps(items);

  foreach (const QString &name, commonProps.keys()) {
    // Create an editor for each common style property.
    if (properties_.contains(name)) {
      EditStyleProperty *prop = properties_.value(name);
      editing_.insert(name);

      QWidget *editor = prop->createEditor(commonProps.value(name));
      formLayout_->addRow(prop->labelText, editor);
    }
  }

  // Open the dialog.
  if (exec() != QDialog::Accepted)
    reset();
}

// Restores original values.
void StyleEditor::reset()
{
  // Reset the values of all the registered properties for all the items
  // with those properties.
  foreach (const QString &name, editing_) {
    EditStyleProperty *prop = properties_.value(name);
    prop->reset();

    foreach (DrawingItemBase *item, items_)
      item->setProperty(name, prop->oldValue);
  }

  buttonBox->button(QDialogButtonBox::Reset)->setEnabled(false);
  EditItemManager::instance()->repaint();
}

void StyleEditor::updateButtons()
{
  // Check the values of all the registered properties.
  bool changed = false;

  foreach (const QString &name, editing_) {
    EditStyleProperty *prop = properties_.value(name);

    if (prop->hasChanged())
      changed = true;

    foreach (DrawingItemBase *item, items_)
      item->setProperty(name, prop->newValue);
  }

  buttonBox->button(QDialogButtonBox::Reset)->setEnabled(changed);
  EditItemManager::instance()->repaint();
}

} // namespace
