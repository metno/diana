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

#include <diEditItemManager.h>
#include <EditItems/editdrawingdialog.h>
#include <EditItems/toolbar.h>
#include <EditItems/layergroup.h>
#include <editdrawing.xpm> // ### for now

#include <QAction>
#include <QPushButton>
#include <QVBoxLayout>

#include <qtUtility.h>

namespace EditItems {

EditDrawingDialog::EditDrawingDialog(QWidget *parent, Controller *ctrl)
  : DataDialog(parent, ctrl)
{
  editm_ = EditItemManager::instance();

  // Create an action that can be used to open the dialog from within a menu or toolbar.
  m_action = new QAction(QIcon(QPixmap(editdrawing_xpm)), tr("Edit Drawing Dialog"), this);
  m_action->setShortcut(Qt::SHIFT + Qt::CTRL + Qt::Key_B);
  m_action->setCheckable(true);
  m_action->setIconVisibleInMenu(true);

  QListView *propertyList = new QListView();
  propertyList->setModel(&propertyModel_);
  propertyList->setSelectionMode(QAbstractItemView::MultiSelection);

  QListView *valueList = new QListView();
  valueList->setModel(&valueModel_);
  valueList->setSelectionMode(QAbstractItemView::MultiSelection);

  QPushButton *resetButton = NormalPushButton(tr("Reset"), this);
  connect(resetButton, SIGNAL(clicked()), SIGNAL(resetChoices()));
  QPushButton *hideButton = NormalPushButton(tr("Hide"), this);
  connect(hideButton, SIGNAL(clicked()), SIGNAL(hideData()));

  QHBoxLayout *viewLayout = new QHBoxLayout;
  viewLayout->addWidget(propertyList);
  viewLayout->addWidget(valueList);

  QHBoxLayout *bottomLayout = new QHBoxLayout;
  bottomLayout->addWidget(resetButton);
  bottomLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding));
  bottomLayout->addWidget(hideButton);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addLayout(viewLayout);
  mainLayout->addLayout(bottomLayout);

  setWindowTitle("Edit Drawing Dialog");
  setFocusPolicy(Qt::StrongFocus);
}

std::string EditDrawingDialog::name() const
{
  return "EDITDRAWING";
}

} // namespace
