/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013-2015 met.no

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

#include <QApplication>
#include <QButtonGroup>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QIcon>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

namespace EditItems {

TextEditor::TextEditor(const QString &text, int fontSize, bool readOnly)
{
  setWindowTitle("Text Editor");

  sizeBox = new QSpinBox();
  sizeBox->setMinimum(1);
  sizeBox->setMaximum(56);
  sizeBox->setValue(fontSize);

  textEdit_ = new QPlainTextEdit;
  textEdit_->setPlainText(text);
  textEdit_->setReadOnly(readOnly);
  textFont = QFont();
  textFont.setPointSize(sizeBox->value());
  textEdit_->setFont(textFont);
  textEdit_->installEventFilter(this);

  connect(sizeBox, SIGNAL(valueChanged(int)), SLOT(setFontSize(int)));

  if (readOnly) {
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), SLOT(reject()));
  } else {
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
    connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), SLOT(reject()));
    connect(buttonBox->button(QDialogButtonBox::Save), SIGNAL(clicked()), SLOT(accept()));
  }

  QHBoxLayout *fontLayout = new QHBoxLayout();
  fontLayout->addWidget(new QLabel(tr("Font size:")));
  fontLayout->addWidget(sizeBox);
  fontLayout->addStretch();

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addWidget(textEdit_);
  layout->addLayout(fontLayout);
  layout->addWidget(buttonBox);
}

TextEditor::~TextEditor()
{
  delete textEdit_;
}

bool TextEditor::eventFilter(QObject *watched, QEvent *event)
{
  if (event->type() == QEvent::KeyPress) {
    if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Tab) {
      sizeBox->setFocus(Qt::TabFocusReason);
      return true;
    }
  }
  return false;
}

QString TextEditor::text() const
{
  return textEdit_->toPlainText();
}

int TextEditor::fontSize() const
{
  return textEdit_->font().pointSize();
}

void TextEditor::setFontSize(int size)
{
  textFont.setPointSize(size);
  textEdit_->setFont(textFont);
}

} // namespace
