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

#include <QAction>
#include <QLayout>
#include <QPushButton>

#include "qtDataDialog.h"
#include "qtUtility.h"

DataDialog::DataDialog(QWidget *parent, Controller *ctrl)
  : QDialog(parent), applyhideButton(0), applyButton(0), m_ctrl(ctrl), m_action(0)
{
  connect(this, SIGNAL(finished(int)), SLOT(unsetAction()));
}

DataDialog::~DataDialog()
{
}

QAction *DataDialog::action() const
{
  return m_action;
}

void DataDialog::closeEvent(QCloseEvent *event)
{
  QDialog::closeEvent(event);
  unsetAction();
}

void DataDialog::unsetAction()
{
  if (m_action) m_action->setChecked(false);
}

QLayout *DataDialog::createStandardButtons()
{
  QPushButton *helpButton = NormalPushButton(tr("Help"), this);
  QPushButton *refreshButton = NormalPushButton(tr("Refresh"), this);
  QPushButton *hideButton = NormalPushButton(tr("Hide"), this);

  applyhideButton = NormalPushButton("", this);
  applyButton = NormalPushButton("", this);
  indicateUnappliedChanges(false);

  applyButton->setDefault(true);

  connect(hideButton, SIGNAL(clicked()), SLOT(close()));
  connect(applyButton, SIGNAL(clicked()), SIGNAL(applyData()));
  connect(refreshButton, SIGNAL(clicked()), SLOT(updateTimes()));
  connect(applyhideButton, SIGNAL(clicked()), SLOT(applyhideClicked()));
  connect(helpButton, SIGNAL(clicked()), SLOT(helpClicked()));

  QHBoxLayout* helplayout = new QHBoxLayout();
  helplayout->addWidget(helpButton);
  helplayout->addWidget(refreshButton);

  QHBoxLayout* applylayout = new QHBoxLayout();
  applylayout->addWidget(hideButton);
  applylayout->addWidget(applyhideButton);
  applylayout->addWidget(applyButton);

  QVBoxLayout* vlayout = new QVBoxLayout();
  vlayout->setSpacing(1);
  vlayout->addLayout(helplayout);
  vlayout->addLayout(applylayout);

  return vlayout;
}

void DataDialog::indicateUnappliedChanges(bool on)
{
  if ((!applyhideButton) || (!applyButton))
    return;

  if (on) {
    applyhideButton->setText(tr("Apply* + Hide"));
    applyButton->setText(tr("Apply*"));
  } else {
    applyhideButton->setText(tr("Apply + Hide"));
    applyButton->setText(tr("Apply"));
  }
}

void DataDialog::applyhideClicked()
{
  emit applyData();
  close();
}

void DataDialog::helpClicked()
{
  emit showsource(helpFileName);
}
