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

#include <diDrawingManager.h>
#include <EditItems/dialogcommon.h>
#include <EditItems/kml.h>
#include <EditItems/editpolyline.h>
#include <EditItems/editsymbol.h>
#include <EditItems/edittext.h>
#include <EditItems/editcomposite.h>
#include <EditItems/layermanager.h>
#include <EditItems/layer.h>

#include <QApplication>
#include <QButtonGroup>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QIcon>
#include <QRadioButton>
#include <QToolButton>
#include <QVBoxLayout>

#include "empty.xpm"

namespace EditItems {

CheckableLabel::CheckableLabel(bool checked, const QPixmap &pixmap, const QString &checkedToolTip, const QString &uncheckedToolTip, bool clickable)
  : checked_(checked)
  , pixmap_(pixmap)
  , checkedToolTip_(checkedToolTip)
  , uncheckedToolTip_(uncheckedToolTip)
  , clickable_(clickable)
{
  setMargin(0);
  setChecked(checked_);
}

void CheckableLabel::setChecked(bool enabled)
{
  checked_ = enabled;
  if (checked_) {
    setPixmap(pixmap_);
    setToolTip(checkedToolTip_);
  } else {
    setPixmap(QPixmap(empty_xpm));
    setToolTip(uncheckedToolTip_);
  }
}

void CheckableLabel::checkAndNotify(bool enabled)
{
  setChecked(enabled);
  emit checked(enabled);
}

void CheckableLabel::mousePressEvent(QMouseEvent *event)
{
  if (clickable_ && (event->button() & Qt::LeftButton))
    setChecked(!checked_);
  emit mouseClicked(event);
  emit checked(checked_);
}

ClickableLabel::ClickableLabel(const QString &name)
  : QLabel(name)
{
  setMargin(0);
}

void ClickableLabel::mousePressEvent(QMouseEvent *event)
{
  emit mouseClicked(event);
}

void ClickableLabel::mouseDoubleClickEvent(QMouseEvent *event)
{
  emit mouseDoubleClicked(event);
}

ScrollArea::ScrollArea(QWidget *parent)
  : QScrollArea(parent)
{
}

void ScrollArea::keyPressEvent(QKeyEvent *event)
{
  event->ignore();
}

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

QToolButton *createToolButton(const QIcon &icon, const QString &toolTip, const QObject *object, const char *method)
{
  QToolButton *button = new QToolButton;
  button->setIcon(icon);
  button->setToolTip(toolTip);
  QObject::connect(button, SIGNAL(clicked()), object, method);
  return button;
}

// Returns a list of layers from \a fileName. Upon failure, a reason is passed in \a error.
QList<QSharedPointer<Layer> > createLayersFromFile(const QString &fileName, LayerManager *layerManager, bool ensureUniqueNames, QString *error)
{
  *error = QString();

  const QList<QSharedPointer<Layer> > layers = KML::createFromFile(layerManager, fileName, error);
  if (ensureUniqueNames) {
    foreach (const QSharedPointer<Layer> &layer, layers)
      layerManager->ensureUniqueLayerName(layer);
  }

  return error->isEmpty() ? layers : QList<QSharedPointer<Layer> >();
}

// Asks the user for a file name and returns a list of layers from this file. Upon failure, a reason is passed in \a error.
QList<QSharedPointer<Layer> > createLayersFromFile(LayerManager *layerManager, bool ensureUniqueNames, QString *error)
{
  *error = QString();

  const QString fileName = QFileDialog::getOpenFileName(0, QObject::tr("Open File"),
    DrawingManager::instance()->getWorkDir(), QObject::tr("KML files (*.kml);; All files (*)"));
  if (fileName.isEmpty())
    return QList<QSharedPointer<Layer> >();

  QApplication::setOverrideCursor(Qt::WaitCursor);
  const QList<QSharedPointer<Layer> > layers = createLayersFromFile(fileName, layerManager, ensureUniqueNames, error);
  QApplication::restoreOverrideCursor();

  QFileInfo fi(fileName);
  DrawingManager::instance()->setWorkDir(fi.dir().absolutePath());

  return error->isEmpty() ? layers : QList<QSharedPointer<Layer> >();
}

class StringSelector : public QDialog
{
public:
  StringSelector(const QString &title, const QString &noneTitle, const QString &defaultString, const QStringList &strings)
    : noneButton_(0)
  {
    setWindowTitle(title);
    QVBoxLayout *vboxLayout = new QVBoxLayout;

    foreach (const QString &s, strings) {
      QRadioButton *button = new QRadioButton(s);
      button->setChecked(s == defaultString);
      bgroup_.addButton(button);
      vboxLayout->addWidget(button);
    }

    noneButton_ = new QRadioButton(noneTitle);
    noneButton_->setChecked(!(bgroup_.checkedButton()));
    bgroup_.addButton(noneButton_);
    vboxLayout->addWidget(noneButton_);

    QDialogButtonBox *dialogBBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(dialogBBox, SIGNAL(accepted()), SLOT(accept()));
    connect(dialogBBox, SIGNAL(rejected()), SLOT(reject()));
    vboxLayout->addWidget(dialogBBox);

    setLayout(vboxLayout);
  }

  ~StringSelector()
  {
    foreach (QAbstractButton *button, bgroup_.buttons())
      delete button;
  }

  QString selection() const
  {
    QAbstractButton *button = bgroup_.checkedButton();
    if (button && (button != noneButton_))
      return button->text();
    return QString();
  }

private:
  QButtonGroup bgroup_;
  QAbstractButton *noneButton_;
};

// Opens a modal dialog that lets the user select one of the candidate strings or none.
// Returns either the selected candidate string or an empty string.
QString selectString(const QString &title, const QString &noneTitle, const QString &defaultString, const QStringList &strings, bool &cancel)
{
  StringSelector stor(title, noneTitle, defaultString, strings);
  cancel = false;
  if (stor.exec() == QDialog::Rejected) {
    cancel = true;
    return QString();
  }
  return stor.selection();
}

} // namespace
