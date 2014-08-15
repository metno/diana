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

#include <diController.h>
#include <diEditItemManager.h>
#include <EditItems/toolbar.h>
#include <EditItems/eimtestdialog.h>
#include <QAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QScrollBar>
#include <QMessageBox>

#include <QDebug>

EIMTestDialog::EIMTestDialog(QWidget *parent)
  : QDialog(parent)
  , nEvents_(0)
{
  setWindowTitle("Edit Item Manager Test Dialog");
  setFocusPolicy(Qt::StrongFocus);
  QVBoxLayout *mainLayout = new QVBoxLayout;
  setLayout(mainLayout);
  textEdit_ = new QTextEdit;
  textEdit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  mainLayout->addWidget(textEdit_);

  enabledCheckBox_ = new QCheckBox("Enabled");
  connect(enabledCheckBox_, SIGNAL(stateChanged(int)), SLOT(changeEnabledState(int)));
  mainLayout->addWidget(enabledCheckBox_);

  QHBoxLayout *selectLayout = new QHBoxLayout;
  QPushButton *selectButton = new QPushButton("Select item:");
  connect(selectButton, SIGNAL(clicked()), SLOT(selectItem()));
  selectLayout->addWidget(selectButton);
  selectComboBox_ = new QComboBox;
  selectLayout->addWidget(selectComboBox_);
  mainLayout->addLayout(selectLayout);

  QPushButton *clearButton = new QPushButton("Clear text edit");
  connect(clearButton, SIGNAL(clicked()), SLOT(clear()));
  mainLayout->addWidget(clearButton);

  QPushButton *pcspButton = new QPushButton("Prepare cross section placement");
  connect(pcspButton, SIGNAL(clicked()), SLOT(prepareCrossSectionPlacement()));
  mainLayout->addWidget(pcspButton);
}

void EIMTestDialog::changeEnabledState(int state)
{
  if (state == Qt::Checked) {
    EditItemManager::instance()->enableItemChangeNotification();
    EditItemManager::instance()->setItemChangeFilter("Cross section");
    connect(EditItemManager::instance(), SIGNAL(itemChanged(const QVariantMap &)), SLOT(handleItemChange(const QVariantMap &)), Qt::UniqueConnection);
    connect(EditItemManager::instance(), SIGNAL(itemRemoved(int)), SLOT(handleItemRemoval(int)), Qt::UniqueConnection);
    EditItemManager::instance()->emitItemChanged();
  } else {
    disconnect(EditItemManager::instance(), SIGNAL(itemChanged(const QVariantMap &)), this, SLOT(handleItemChange(const QVariantMap &)));
    disconnect(EditItemManager::instance(), SIGNAL(itemRemoved(int)), this, SLOT(handleItemRemoval(int)));
    EditItemManager::instance()->enableItemChangeNotification(false);
  }
}

void EIMTestDialog::clear()
{
  textEdit_->clear();
}

void EIMTestDialog::prepareCrossSectionPlacement()
{
  if (!enabledCheckBox_->isChecked())
    enabledCheckBox_->setChecked(true);
  EditItems::ToolBar::instance()->setCreatePolyLineAction("Cross section");
  EditItems::ToolBar::instance()->show();
}

void EIMTestDialog::appendText(const QString &s)
{
  textEdit_->setPlainText(textEdit_->toPlainText() + s);
  textEdit_->verticalScrollBar()->setValue(textEdit_->verticalScrollBar()->maximum());
}

void EIMTestDialog::handleItemChange(const QVariantMap &props)
{
  nEvents_++;
  QString s;
  if (props.isEmpty()) {
    s += QString("*** no matching item found (event # %1) ***\n").arg(nEvents_);
  } else {
    s += QString("*** potential item change (event # %1) ***\n").arg(nEvents_);
    s += QString("properties:\n");
    foreach (QString key, props.keys()) {
      if (key == "latLonPoints") {
        s += "  lat/lon points:\n";
        foreach (QVariant v, props.value(key).toList()) {
          const QPointF p = v.toPointF();
          s += QString("    %1  %2\n").arg(p.x()).arg(p.y());
        }
      } else {
        s += QString("  %1: %2\n").arg(key).arg(props.value(key).toString());
        if (key == "id") {
          itemIds_.insert(props.value(key).toInt());
          updateSelectComboBox();
        }
      }
    }
  }
  s += "\n";
  appendText(s);
}

void EIMTestDialog::handleItemRemoval(int id)
{
  nEvents_++;
  appendText(QString("*** item removed - id: %1 (event # %2) ***\n").arg(id).arg(nEvents_));
  itemIds_.remove(id);
  updateSelectComboBox();
}

void EIMTestDialog::updateSelectComboBox()
{
  QStringList ids;
  foreach (int id, itemIds_.values())
    ids.append(QString("%1").arg(id));
  selectComboBox_->clear();
  selectComboBox_->addItems(ids);
  selectComboBox_->addItem("1234567890"); // for testing handling of an invalid ID
}

void EIMTestDialog::selectItem()
{
  const QString id_s = selectComboBox_->currentText();
  bool ok;
  const int id = id_s.toInt(&ok);
  if (ok) {
    if (!EditItemManager::instance()->selectItem(id, true))
      QMessageBox::warning(this, "Item not found", "Item not found", QMessageBox::Ok);
  } else {
    qDebug() << "failed to convert" << id_s << "to integer";
  }
}
