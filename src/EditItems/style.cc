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
#include <EditItems/edititembase.h>
#include <EditItems/style.h>
#include "qtUtility.h"

namespace EditItemsStyle {

QComboBox * StylePropertyEditor::comboBox() { return comboBox_; }

void StylePropertyEditor::reset()
{
  if (comboBox_)
    setCurrentIndex(origInitVal_);
}

StylePropertyEditor::StylePropertyEditor() : comboBox_(0) { }

void StylePropertyEditor::init(bool applicable, const QSet<DrawingItemBase *> &items, const QVariant &initVal)
{
  items_ = items;
  if (applicable) {
    comboBox_ = createComboBox();
    connect(comboBox_, SIGNAL(currentIndexChanged(int)), this, SLOT(handleCurrentIndexChanged(int)));
    setCurrentIndex(initVal);
    origInitVal_ = initVal;
  }
}

// Sets the current index of the combo box to the one matching valid user data \a val, or to -1 otherwise.
void StylePropertyEditor::setCurrentIndex(const QVariant &val)
{
  if (val.isValid()) {
    for (int i = 0; i < comboBox_->count(); ++i) {
      const QVariant itemData = comboBox_->itemData(i);
      if (itemData == val) {
        comboBox_->setCurrentIndex(i);
        return;
      }
    }
  }
  comboBox_->setCurrentIndex(-1);
}

void StylePropertyEditor::handleCurrentIndexChanged(int index)
{
  QComboBox *cbox = qobject_cast<QComboBox *>(sender());
  if (!cbox) {
    qWarning() << "StylePropertyEditor::handleCurrentIndexChanged(): sender() not a QComboBox";
    return;
  }

  const QVariant userData = cbox->itemData(index);
  if (!userData.isValid())
    return;

  const QString fullName = QString("style:%1").arg(name());

  foreach (DrawingItemBase *item, items_)
    item->propertiesRef().insert(fullName, userData);
  EditItemManager::instance()->repaint();
}

QComboBox *LineTypeEditor::createComboBox() { return LinetypeBox(0, true); }
QComboBox *LineWidthEditor::createComboBox() { return LinewidthBox(0, true); }
QComboBox *LineColorEditor::createComboBox() { return ColourBox(0, true, 0, tr("off").toStdString(),true); }

StylePropertyEditor * StylePropertyEditor::create(const QString &name, const QSet<DrawingItemBase *> &items, const QVariantMap &sprops)
{
  StylePropertyEditor *editor = 0;

  if (name == LineTypeEditor().name())
    editor = new LineTypeEditor;
  else if (name == LineWidthEditor().name())
    editor = new LineWidthEditor;
  else if (name == LineColorEditor().name())
    editor = new LineColorEditor;

  if (editor)
    editor->init(sprops.contains(name), items, sprops.value(name));

  return editor;
}

StyleEditor::StyleEditor()
{
  setWindowTitle(tr("Item Style"));

  QVBoxLayout *layout = new QVBoxLayout(this);
  formWidget_ = new QWidget();
  layout->addWidget(formWidget_);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Reset | QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
  connect(buttonBox->button(QDialogButtonBox::Reset), SIGNAL(clicked()), this, SLOT(reset()));
  connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
  connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
  layout->addWidget(buttonBox);
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
static QVariantMap getCustomStylePropsUnion(const QSet<DrawingItemBase *> &items)
{
  QVariantMap sprops;
  QRegExp rx("style:(.+)");

  // loop over customizable items
  foreach (DrawingItemBase *item, items) {
    const QVariantMap &props = item->propertiesRef();
    if (props.value("style:type") != "custom")
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
static QMap<DrawingItemBase *, QVariantMap> getCustomStyleProps(const QSet<DrawingItemBase *> &items)
{
  QMap<DrawingItemBase *, QVariantMap> spropsMap;
  QRegExp rx("style:(.+)");

  // loop over customizable items
  foreach (DrawingItemBase *item, items) {
    const QVariantMap &props = item->propertiesRef();
    if (props.value("style:type") != "custom")
      continue;

    QVariantMap sprops;

    // loop over properties and register only style properties
    foreach (const QString &key, props.keys()) {
      if (rx.indexIn(key) >= 0)
        sprops.insert(key, props.value(key));
    }

    spropsMap.insert(item, sprops);
  }

  return spropsMap;
}

// Opens a modal dialog to edit the style properties of \a items.
void StyleEditor::edit(const QSet<DrawingItemBase *> &items)
{
  // get initial values
  savedProps_ = getCustomStyleProps(items);
  QVariantMap sprops = getCustomStylePropsUnion(items);

  // clear old content
  qDeleteAll(formWidget_->children());
  editors_.clear();

  // set new content and initial values for the properties that we would like to support
  QFormLayout *formLayout = new QFormLayout(formWidget_);
  QStringList names = QStringList() << "lineType"<< "lineWidth" << "lineColor";
  foreach (QString name, names) {
    QSharedPointer<StylePropertyEditor> editor(StylePropertyEditor::create(name, items, sprops));
    editors_.append(editor);
    if (!editor.isNull()) {
      QWidget *editorWidget = editor->comboBox();
      if (!editorWidget)
        editorWidget = new QLabel("n/a"); // i.e. not found in any of the selected items
      formLayout->addRow(editor->labelText(), editorWidget);
    } else { // property name not recognized at all
      const QString errMsg = QString("%1: UNSUPPORTED").arg(name);
      formLayout->addRow(errMsg, new QLabel(errMsg));
    }
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
