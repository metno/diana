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
#include <EditItems/modifypropertiescommand.h>
#include <diEditItemManager.h>

#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVariantMap>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include "qtUtility.h"

namespace EditItemsStyle {

class DecorationBox : public QComboBox
{
public:
  DecorationBox(QWidget *parent = 0)
    : QComboBox(parent)
  {
    addItem("none", "");
    addItem("triangles", "triangles");
    addItem("arches", "arches");
    addItem("crosses", "crosses");
    addItem("arrow", "arrow");
    addItem("SIGWX", "SIGWX");
    addItem("arches,triangles", "arches,triangles");
  }
};

class FillPatternBox : public QComboBox
{
public:
  FillPatternBox(QWidget *parent = 0)
    : QComboBox(parent)
  {
    addItem("solid", "");
    addItem("diagleft", "diagleft");
    addItem("zigzag", "zigzag");
    addItem("paralyse", "paralyse");
    addItem("ldiagleft2", "ldiagleft2");
    addItem("vdiagleft", "vdiagleft");
    addItem("vldiagcross_little", "vldiagcross_little");
  }
};

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

  foreach (QSharedPointer<DrawingItemBase> item, items_)
    item->propertiesRef().insert(fullName, userData);

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

class SPE_fillpattern : public StylePropertyEditor
{
public:
  virtual QString name() const { return DSP_fillpattern::name(); }
private:
  virtual QString labelText() const { return "fill pattern"; }
  virtual IndexedEditor *createEditor() { return new ComboBoxEditor(new FillPatternBox); }
};

class SPE_closed : public StylePropertyEditor
{
public:
  virtual QString name() const { return DSP_closed::name(); }
private:
  virtual QString labelText() const { return "closed"; }
  virtual IndexedEditor *createEditor() { return new CheckBoxEditor(); }
};

class SPE_decoration1 : public StylePropertyEditor
{
public:
  virtual QString name() const { return DSP_decoration1::name(); }
private:
  virtual QString labelText() const { return "decoration 1"; }
  virtual IndexedEditor *createEditor() { return new ComboBoxEditor(new DecorationBox); }
};

class SPE_decoration1_colour : public StylePropertyEditor
{
public:
  virtual QString name() const { return DSP_decoration1_colour::name(); }
private:
  virtual QString labelText() const { return "decoration 1 colour"; }
  virtual IndexedEditor *createEditor() { return new ComboBoxEditor(ColourBox(0, true, 0, tr("off").toStdString(),true)); }
};

class SPE_decoration1_offset : public StylePropertyEditor
{
public:
  virtual QString name() const { return DSP_decoration1_offset::name(); }
private:
  virtual QString labelText() const { return "decoration 1 offset"; }
  virtual IndexedEditor *createEditor() { return new IntRangeEditor(0, 100); }
};

class SPE_decoration2 : public StylePropertyEditor
{
public:
  virtual QString name() const { return DSP_decoration2::name(); }
private:
  virtual QString labelText() const { return "decoration 2"; }
  virtual IndexedEditor *createEditor() { return new ComboBoxEditor(new DecorationBox); }
};

class SPE_decoration2_colour : public StylePropertyEditor
{
public:
  virtual QString name() const { return DSP_decoration2_colour::name(); }
private:
  virtual QString labelText() const { return "decoration 2 colour"; }
  virtual IndexedEditor *createEditor() { return new ComboBoxEditor(ColourBox(0, true, 0, tr("off").toStdString(),true)); }
};

class SPE_decoration2_offset : public StylePropertyEditor
{
public:
  virtual QString name() const { return DSP_decoration2_offset::name(); }
private:
  virtual QString labelText() const { return "decoration 2 offset"; }
  virtual IndexedEditor *createEditor() { return new IntRangeEditor(0, 100); }
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

class ESP_fillpattern : public EditStyleProperty
{
private:
  virtual StylePropertyEditor *createSpecialEditor() const { return new SPE_fillpattern; }
};

class ESP_closed : public EditStyleProperty
{
private:
  virtual StylePropertyEditor *createSpecialEditor() const { return new SPE_closed; }
};

class ESP_decoration1 : public EditStyleProperty
{
private:
  virtual StylePropertyEditor *createSpecialEditor() const { return new SPE_decoration1; }
};

class ESP_decoration1_colour : public EditStyleProperty
{
private:
  virtual StylePropertyEditor *createSpecialEditor() const { return new SPE_decoration1_colour; }
};

class ESP_decoration1_offset : public EditStyleProperty
{
private:
  virtual StylePropertyEditor *createSpecialEditor() const { return new SPE_decoration1_offset; }
};

class ESP_decoration2 : public EditStyleProperty
{
private:
  virtual StylePropertyEditor *createSpecialEditor() const { return new SPE_decoration2; }
};

class ESP_decoration2_colour : public EditStyleProperty
{
private:
  virtual StylePropertyEditor *createSpecialEditor() const { return new SPE_decoration2_colour; }
};

class ESP_decoration2_offset : public EditStyleProperty
{
private:
  virtual StylePropertyEditor *createSpecialEditor() const { return new SPE_decoration2_offset; }
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
  properties_.insert(DSP_fillpattern::name(), new ESP_fillpattern);
  properties_.insert(DSP_closed::name(), new ESP_closed);
  properties_.insert(DSP_decoration1::name(), new ESP_decoration1);
  properties_.insert(DSP_decoration1_colour::name(), new ESP_decoration1_colour);
  properties_.insert(DSP_decoration1_offset::name(), new ESP_decoration1_offset);
  properties_.insert(DSP_decoration2::name(), new ESP_decoration2);
  properties_.insert(DSP_decoration2_colour::name(), new ESP_decoration2_colour);
  properties_.insert(DSP_decoration2_offset::name(), new ESP_decoration2_offset);
}

StyleEditor *StyleEditor::instance()
{
  if (!instance_)
    instance_ = new StyleEditor;
  return instance_;
}

StyleEditor *StyleEditor::instance_ = 0;

// Returns the union of all style properties of \a items.
// Style properties that differ in at least two items will be indicated as invalid (i.e. a default QVariant).
static QVariantMap getStylePropsUnion(const QSet<QSharedPointer<DrawingItemBase> > &items)
{
  QVariantMap sprops;
  QRegExp rx("style:(.+)");

  // loop over items
  foreach (QSharedPointer<DrawingItemBase> item, items) {
    const QVariantMap &props = item->propertiesRef();

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

// Returns the style properties of \a items.
static QMap<DrawingItemBase *, QVariantMap> getStyleProps(const QSet<QSharedPointer<DrawingItemBase> > &items)
{
  QMap<DrawingItemBase *, QVariantMap> spropsMap;
  QRegExp rx("style:(.+)");

  // loop over items
  foreach (QSharedPointer<DrawingItemBase> item, items) {
    const QVariantMap &props = item->propertiesRef();

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
  DrawingItemBase::Category category = items.begin()->data()->category();

  // get initial values
  savedProps_ = getStyleProps(items);
  QVariantMap sprops = getStylePropsUnion(items);

  // clear old content
  editors_.clear();
  formLabels_.clear();
  if (formWidget_->layout())
    delete formWidget_->layout();

  // set new content and initial values for the properties that we would like to support
  QGridLayout *gridLayout = new QGridLayout;
  formWidget_->setLayout(gridLayout);
  QStringList propNames = DrawingStyleManager::instance()->properties(category);
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

  // save current properties
  const QList<QSharedPointer<DrawingItemBase> > itemList = items.toList();
  const QList<QVariantMap> oldProperties = DrawingItemBase::properties(itemList);

  // open dialog
  setWindowTitle(QString("%1 (%2 %3)").arg(tr("Item Style")).arg(items.size()).arg(items.size() == 1 ? tr("item") : tr("items")));
  if (exec() == QDialog::Accepted) {
    if (oldProperties != DrawingItemBase::properties(itemList))
      EditItemManager::instance()->undoStack()->push(
            new EditItems::ModifyPropertiesCommand("modify style properties", itemList, oldProperties, DrawingItemBase::properties(itemList)));
  } else {
    reset();
  }
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
