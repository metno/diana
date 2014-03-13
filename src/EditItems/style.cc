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

#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVariantMap>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <EditItems/edititembase.h>
#include <EditItems/style.h>
#include "qtUtility.h"

namespace EditItemsStyle {

ComboBoxEditor::ComboBoxEditor(QComboBox *comboBox) : comboBox_(comboBox)
{
  connect(comboBox_, SIGNAL(currentIndexChanged(int)), SIGNAL(currentIndexChanged(int)));
}

ComboBoxEditor::~ComboBoxEditor()
{
  delete comboBox_;
}

QWidget *ComboBoxEditor::widget() { return comboBox_; }

int ComboBoxEditor::count() const { return comboBox_->count(); }

QVariant ComboBoxEditor::itemData(int i) const { return comboBox_->itemData(i); }

void ComboBoxEditor::setCurrentIndex(int i) { comboBox_->setCurrentIndex(i); }


CheckBoxEditor::CheckBoxEditor() : checkBox_(new QCheckBox)
{
  connect(checkBox_, SIGNAL(clicked(bool)), SLOT(handleClicked(bool)));
}

CheckBoxEditor::~CheckBoxEditor()
{
  delete checkBox_;
}

QWidget *CheckBoxEditor::CheckBoxEditor::widget() { return checkBox_; }

int CheckBoxEditor::count() const { return 2; }

QVariant CheckBoxEditor::itemData(int i) const { return QVariant(i == 1); }

void CheckBoxEditor::setCurrentIndex(int i) { checkBox_->setChecked(i == 1); }

void CheckBoxEditor::handleClicked(bool checked) { emit currentIndexChanged(checked ? 1 : 0); }


IntRangeEditor::IntRangeEditor(int lo, int hi) : spinBox_(new QSpinBox)
{
  spinBox_->setRange(lo, hi);
  connect(spinBox_, SIGNAL(valueChanged(int)), SLOT(handleValueChanged(int)));
}

IntRangeEditor::~IntRangeEditor()
{
  delete spinBox_;
}

QWidget *IntRangeEditor::IntRangeEditor::widget() { return spinBox_; }

int IntRangeEditor::count() const { return (spinBox_->maximum() - spinBox_->minimum()) + 1; }

QVariant IntRangeEditor::itemData(int i) const { return QVariant(spinBox_->minimum() + i); }

void IntRangeEditor::setCurrentIndex(int i) { spinBox_->setValue(spinBox_->minimum() + i); }

void IntRangeEditor::handleValueChanged(int val) { emit currentIndexChanged(val - spinBox_->minimum()); }


StylePropertyEditor::StylePropertyEditor()
  : editor_(0)
{
}

StylePropertyEditor::~StylePropertyEditor()
{
  if (editor_)
    delete editor_;
}

QWidget * StylePropertyEditor::widget() { return editor_ ? editor_->widget() : 0; }

void StylePropertyEditor::reset()
{
  if (editor_)
    setCurrentIndex(origInitVal_);
}

void StylePropertyEditor::init(bool applicable, const QSet<QSharedPointer<DrawingItemBase> > &items, const QVariant &initVal)
{
  items_ = items;
  if (applicable) {
    editor_ = createEditor();
    connect(editor_, SIGNAL(currentIndexChanged(int)), SLOT(handleCurrentIndexChanged(int)));
    setCurrentIndex(initVal);
    origInitVal_ = initVal;
  }
}

// Sets the current index of the editor to the one matching valid user data \a val, or to -1 otherwise.
void StylePropertyEditor::setCurrentIndex(const QVariant &val)
{
  Q_ASSERT(editor_);
  if (val.isValid()) {
    const QString valS = val.toString();
    for (int i = 0; i < editor_->count(); ++i) {
      const QVariant itemData = editor_->itemData(i);
      if (itemData.toString() == valS) {
        editor_->setCurrentIndex(i);
        return;
      }
    }
  }
  editor_->setCurrentIndex(-1);
}

void StylePropertyEditor::handleCurrentIndexChanged(int index)
{
  IndexedEditor *editor = qobject_cast<IndexedEditor *>(sender());
  if (!editor) {
    qWarning() << "StylePropertyEditor::handleCurrentIndexChanged(): sender() not an IndexedEditor";
    return;
  }

  const QVariant userDataVar = editor->itemData(index);
  if (!userDataVar.isValid())
    return;
  const QString userData = userDataVar.toString();

  const QString fullName = QString("style:%1").arg(name());

  foreach (QSharedPointer<DrawingItemBase> item, items_) {
    item->propertiesRef().insert(fullName, userData);
  }
  EditItemManager::instance()->repaint();
}

class SPE_linecolour : public StylePropertyEditor
{
public:
  virtual QString name() const { return DSP_linecolour::name(); }
private:
  virtual QString labelText() const { return "line colour"; }
  virtual IndexedEditor *createEditor() { return new ComboBoxEditor(ColourBox(0, true, 0, tr("off").toStdString(),true)); }
};

class SPE_linetransparency : public StylePropertyEditor
{
public:
  virtual QString name() const { return DSP_linetransparency::name(); }
private:
  virtual QString labelText() const { return "line transparency"; }
  virtual IndexedEditor *createEditor() { return new IntRangeEditor(0, 255); }
};

class SPE_linewidth : public StylePropertyEditor
{
public:
  virtual QString name() const { return DSP_linewidth::name(); }
private:
  virtual QString labelText() const { return "line width"; }
  virtual IndexedEditor *createEditor() { return new ComboBoxEditor(LinewidthBox(0, true)); }
};

class SPE_linepattern : public StylePropertyEditor
{
public:
  virtual QString name() const { return DSP_linepattern::name(); }
private:
  virtual QString labelText() const { return "line pattern"; }
  virtual IndexedEditor *createEditor() { return new ComboBoxEditor(LinetypeBox(0, true)); }
};

class SPE_linesmooth : public StylePropertyEditor
{
public:
  virtual QString name() const { return DSP_linesmooth::name(); }
private:
  virtual QString labelText() const { return "line smooth"; }
  virtual IndexedEditor *createEditor() { return new CheckBoxEditor(); }
};

class SPE_fillcolour : public StylePropertyEditor
{
public:
  virtual QString name() const { return DSP_fillcolour::name(); }
private:
  virtual QString labelText() const { return "fill colour"; }
  virtual IndexedEditor *createEditor() { return new ComboBoxEditor(ColourBox(0, true, 0, tr("off").toStdString(),true)); }
};

class SPE_filltransparency : public StylePropertyEditor
{
public:
  virtual QString name() const { return DSP_filltransparency::name(); }
private:
  virtual QString labelText() const { return "fill transparency"; }
  virtual IndexedEditor *createEditor() { return new IntRangeEditor(0, 255); }
};

class SPE_closed : public StylePropertyEditor
{
public:
  virtual QString name() const { return DSP_closed::name(); }
private:
  virtual QString labelText() const { return "closed"; }
  virtual IndexedEditor *createEditor() { return new CheckBoxEditor(); }
};

// ... editors for more subtypes

class EditStyleProperty
{
public:
  StylePropertyEditor *createEditor(const QString &propName, const QSet<QSharedPointer<DrawingItemBase> > &items, const QVariantMap &sprops)
  {
    StylePropertyEditor *editor = createSpecialEditor();
    if (editor)
      editor->init(sprops.contains(propName), items, sprops.value(propName));
    return editor;
  }

protected:
  virtual StylePropertyEditor *createSpecialEditor() const = 0;
};

class ESP_linecolour : public EditStyleProperty
{
private:
  virtual StylePropertyEditor *createSpecialEditor() const { return new SPE_linecolour; }
};

class ESP_linetransparency : public EditStyleProperty
{
private:
  virtual StylePropertyEditor *createSpecialEditor() const { return new SPE_linetransparency; }
};

class ESP_linewidth : public EditStyleProperty
{
private:
  virtual StylePropertyEditor *createSpecialEditor() const { return new SPE_linewidth; }
};

class ESP_linepattern : public EditStyleProperty
{
private:
  virtual StylePropertyEditor *createSpecialEditor() const { return new SPE_linepattern; }
};

class ESP_linesmooth : public EditStyleProperty
{
private:
  virtual StylePropertyEditor *createSpecialEditor() const { return new SPE_linesmooth; }
};

class ESP_fillcolour : public EditStyleProperty
{
private:
  virtual StylePropertyEditor *createSpecialEditor() const { return new SPE_fillcolour; }
};

class ESP_filltransparency : public EditStyleProperty
{
private:
  virtual StylePropertyEditor *createSpecialEditor() const { return new SPE_filltransparency; }
};

class ESP_closed : public EditStyleProperty
{
private:
  virtual StylePropertyEditor *createSpecialEditor() const { return new SPE_closed; }
};

StyleEditor::StyleEditor()
{
  setWindowTitle(tr("Item Style"));

  QVBoxLayout *layout = new QVBoxLayout(this);
  formWidget_ = new QWidget;
  layout->addWidget(formWidget_);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Reset | QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
  connect(buttonBox->button(QDialogButtonBox::Reset), SIGNAL(clicked()), this, SLOT(reset()));
  connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
  connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
  layout->addWidget(buttonBox);

  // define editors for supported style properties
  properties_.insert(DSP_linecolour::name(), new ESP_linecolour);
  properties_.insert(DSP_linetransparency::name(), new ESP_linetransparency);
  properties_.insert(DSP_linewidth::name(), new ESP_linewidth);
  properties_.insert(DSP_linepattern::name(), new ESP_linepattern);
  properties_.insert(DSP_linesmooth::name(), new ESP_linesmooth);
  // lineshape ... TBD
  properties_.insert(DSP_fillcolour::name(), new ESP_fillcolour);
  properties_.insert(DSP_filltransparency::name(), new ESP_filltransparency);
  // fillpattern ... TBD
  properties_.insert(DSP_closed::name(), new ESP_closed);
  // decoration1 ... TBD
  // decoration1.colour ... TBD
  // decoration1.offset ... TBD
  // decoration2 ... TBD
  // decoration2.colour ... TBD
  // decoration2.offset ... TBD
}

StyleEditor *StyleEditor::instance()
{
  if (!instance_)
    instance_ = new StyleEditor;
  return instance_;
}

StyleEditor *StyleEditor::instance_ = 0;

// Returns the union of all style properties of customizable items in \a items.
// Style properties that differ in at least two items will be indicated as invalid (i.e. a default QVariant).
static QVariantMap getCustomStylePropsUnion(const QSet<QSharedPointer<DrawingItemBase> > &items)
{
  QVariantMap sprops;
  QRegExp rx("style:(.+)");

  // loop over customizable items
  foreach (QSharedPointer<DrawingItemBase> item, items) {
    const QVariantMap &props = item->propertiesRef();
    if (props.value("style:type") != "Custom")
      continue;

    // loop over style properties
    foreach (const QString &key, props.keys()) {
      if (rx.indexIn(key) < 0)
        continue;

      const QVariant val = props.value(key);
      const QString skey = rx.cap(1);
      Q_ASSERT(val.isValid());
      if (!sprops.contains(skey)) {
        sprops.insert(skey, val); // insert first value for key
      } else {
        const QVariant existingVal = sprops.value(skey);
        if (existingVal.isValid() && (existingVal != val))
          sprops.insert(skey, QVariant()); // invalidate existing value for key
      }
    }
  }

  return sprops;
}

// Returns the style properties of customizable items in \a items.
static QMap<DrawingItemBase *, QVariantMap> getCustomStyleProps(const QSet<QSharedPointer<DrawingItemBase> > &items)
{
  QMap<DrawingItemBase *, QVariantMap> spropsMap;
  QRegExp rx("style:(.+)");

  // loop over customizable items
  foreach (QSharedPointer<DrawingItemBase> item, items) {
    const QVariantMap &props = item->propertiesRef();
    if (props.value("style:type") != "Custom")
      continue;

    QVariantMap sprops;

    // loop over properties and register only style properties
    foreach (const QString &key, props.keys()) {
      if (rx.indexIn(key) >= 0)
        sprops.insert(key, props.value(key));
    }

    spropsMap.insert(item.data(), sprops);
  }

  return spropsMap;
}

// Opens a modal dialog to edit the style properties of \a items.
void StyleEditor::edit(const QSet<QSharedPointer<DrawingItemBase> > &items)
{
  // only allow editing for items with style type 'Custom'
  foreach (QSharedPointer<DrawingItemBase> item, items) {
    const QString typeName = item->propertiesRef().value("style:type").toString();
    if (typeName != "Custom") {
      QMessageBox::warning(
            0, "Warning", QString(
              "At least one non-custom style type found: %1.\n"
              "Please convert to custom style type.").arg(typeName));
      return;
    }
  }

  // get initial values
  savedProps_ = getCustomStyleProps(items);
  QVariantMap sprops = getCustomStylePropsUnion(items);

  // clear old content
  editors_.clear();
  formLabels_.clear();
  if (formWidget_->layout())
    delete formWidget_->layout();

  // set new content and initial values for the properties that we would like to support
  QGridLayout *gridLayout = new QGridLayout;
  formWidget_->setLayout(gridLayout);
  QStringList propNames = DrawingStyleManager::instance()->properties();
  qSort(propNames);
  int row = 0;
  foreach (QString propName, propNames) {
    StylePropertyEditor *editor = 0;
    if (properties_.contains(propName))
      editor = properties_.value(propName)->createEditor(propName, items, sprops);
    if (editor) {
      editors_.append(QSharedPointer<StylePropertyEditor>(editor));
      QWidget *editorWidget = editor->widget();
      if (!editorWidget) {
        editorWidget = new QLabel("n/a"); // i.e. not found in any of the selected items
        formLabels_.append(QSharedPointer<QLabel>(qobject_cast<QLabel *>(editorWidget)));
      }
      QLabel *label = new QLabel(editor->labelText());
      formLabels_.append(QSharedPointer<QLabel>(label));
      gridLayout->addWidget(label, row, 0);
      gridLayout->addWidget(editorWidget, row, 1);
    } else { // property name not recognized at all
      QLabel *label = new QLabel(QString("%1: UNSUPPORTED").arg(propName));
      formLabels_.append(QSharedPointer<QLabel>(label));
      gridLayout->addWidget(label, row, 0, 1, -1);
    }

    row++;
  }

  // open dialog
  setWindowTitle(QString("%1 (%2 %3)").arg(tr("Item Style")).arg(items.size()).arg(items.size() == 1 ? tr("item") : tr("items")));
  if (exec() != QDialog::Accepted)
    reset();
}

// Restores original values.
void StyleEditor::reset()
{
  if (isVisible()) {
    // reset editors
    foreach (QSharedPointer<StylePropertyEditor> editor, editors_) {
      if (!editor.isNull())
        editor->reset();
    }
  }

  // reset items
  foreach (DrawingItemBase *item, savedProps_.keys()) {
    const QVariantMap sprops = savedProps_.value(item);
    foreach (QString name, sprops.keys()) {
      item->propertiesRef().insert(name, sprops.value(name));
    }
  }
  EditItemManager::instance()->repaint();
}

} // namespace
