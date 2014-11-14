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

#define MILOGGER_CATEGORY "diana.EditItemManager"
#include <miLogger/miLogging.h>

#include <EditItems/edititembase.h>
#include <EditItems/properties.h>

#include <QAction>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

namespace Properties {

TextEditor::TextEditor(const QString &text, bool readOnly)
{
  setWindowTitle("Text Editor");

  QVBoxLayout *layout = new QVBoxLayout;
  textEdit_ = new QTextEdit;
  textEdit_->setPlainText(text);
  textEdit_->setReadOnly(readOnly);
  layout->addWidget(textEdit_);

  QDialogButtonBox *buttonBox = 0;
  if (readOnly) {
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), SLOT(reject()));
  } else {
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
    connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), SLOT(reject()));
    connect(buttonBox->button(QDialogButtonBox::Save), SIGNAL(clicked()), SLOT(accept()));
  }
  layout->addWidget(buttonBox);

  setLayout(layout);
}

TextEditor::~TextEditor()
{
  delete textEdit_;
}

QString TextEditor::text() const
{
  return textEdit_->toPlainText();
}

SpecialLineEdit::SpecialLineEdit(const QString &pname, bool readOnly)
  : propertyName_(pname)
{
  setReadOnly(readOnly);
}

QString SpecialLineEdit::propertyName() const { return propertyName_; }

void SpecialLineEdit::contextMenuEvent(QContextMenuEvent *event)
{
  QMenu *menu = createStandardContextMenu();
  QAction action(isReadOnly() ? tr("&Show") : tr("&Edit"), 0);
  connect(&action, SIGNAL(triggered()), this, SLOT(openTextEdit()));
  menu->addAction(&action);
  menu->exec(event->globalPos());
  delete menu;
}

void SpecialLineEdit::mouseDoubleClickEvent(QMouseEvent *)
{
  openTextEdit();
}

void SpecialLineEdit::openTextEdit()
{
  TextEditor textEditor(text(), isReadOnly());
  textEditor.setWindowTitle(propertyName());
  if (textEditor.exec() == QDialog::Accepted)
    setText(textEditor.text());
}

static QWidget * createEditor(const QString &propertyName, const QVariant &val, bool readOnly = false)
{
  QWidget *editor = 0;
  if ((val.type() == QVariant::Double) || (val.type() == QVariant::Int) || (val.type() == QVariant::Bool) ||
      (val.type() == QVariant::String) || (val.type() == QVariant::ByteArray)) {
    editor = new SpecialLineEdit(propertyName, readOnly);
    qobject_cast<QLineEdit *>(editor)->setText(val.toString());
  } else if (val.type() == QVariant::DateTime) {
    editor = new QDateTimeEdit(val.toDateTime());
    qobject_cast<QDateTimeEdit *>(editor)->setReadOnly(readOnly);
  } else {
    METLIBS_LOG_WARN("WARNING: unsupported type:" << val.typeName());
  }
  return editor;
}

PropertiesEditor::PropertiesEditor()
{
  setWindowTitle(tr("Item Properties"));

  QVBoxLayout *layout = new QVBoxLayout(this);
  formWidget_ = new QWidget();
  layout->addWidget(formWidget_);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
  connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
  connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
  layout->addWidget(buttonBox);
}

PropertiesEditor *PropertiesEditor::instance()
{
  if (!instance_)
    instance_ = new PropertiesEditor;
  return instance_;
}

PropertiesEditor *PropertiesEditor::instance_ = 0;

// Opens a modal dialog to show the properties of \a item.
// The properties may be modified if \a readOnly is false.
// Returns true iff the properties were changed.
bool PropertiesEditor::edit(QSharedPointer<DrawingItemBase> &item, bool readOnly)
{
  const QVariantMap origProps = item->properties();
  if (origProps.isEmpty()) {
    QMessageBox::information(0, "info", "No properties to edit!");
    return false;
  }

  // clear old content
  qDeleteAll(formWidget_->children());

  // set new content and initial values
  QFormLayout *formLayout = new QFormLayout(formWidget_);
  foreach (const QString key, origProps.keys()) {
    QWidget *editor = createEditor(key, origProps.value(key), readOnly);
    if (editor)
      formLayout->addRow(key, editor);
  }

  // open dialog
  if (exec() == QDialog::Accepted) {
    QVariantMap newProps;
    for (int i = 0; i < formLayout->rowCount(); ++i) {
      QLayoutItem *litem = formLayout->itemAt(i, QFormLayout::LabelRole);
      if (litem) {
        const QString key = qobject_cast<const QLabel *>(litem->widget())->text();
        QWidget *editor = formLayout->itemAt(i, QFormLayout::FieldRole)->widget();
        if (qobject_cast<QLineEdit *>(editor)) {
          newProps.insert(key, qobject_cast<const QLineEdit *>(editor)->text());
        } else if (qobject_cast<QDateTimeEdit *>(editor)) {
          QDateTimeEdit *ed = qobject_cast<QDateTimeEdit *>(editor);
          newProps.insert(key, ed->dateTime());
        }
      }
    }

    if (newProps != origProps) {
      item->setProperties(newProps);
      return true;
    }
  }

  return false;
}

} // namespace
