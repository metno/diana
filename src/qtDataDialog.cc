/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013-2018 met.no

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

#include "qtDataDialog.h"

#include "qtUtility.h"

#include <QAction>
#include <QApplication>
#include <QLayout>
#include <QPushButton>
#include <QToolButton>

#include "apply.xpm"
#include "help.xpm"
#include "hideapply.xpm"
#include "loop.xpm"
#include "paint_hide.xpm"

ShowMoreDialog::ShowMoreDialog(QWidget* parent)
  : QDialog(parent)
  , orientation(Qt::Horizontal)
{
}

bool ShowMoreDialog::showsMore()
{
  return false;
}

void ShowMoreDialog::showMore(bool more)
{
  const QSize before = size();
  if (more)
    sizeLess = before;
  else
    sizeMore = before;

  doShowMore(more);

  layout()->invalidate();
  qApp->processEvents();

  QSize s = more ? sizeMore : sizeLess;
  if (s.width() == 0)
    s = minimumSize();
  if (orientation == Qt::Horizontal)
    s.setHeight(before.height());
  else
    s.setWidth(before.width());
  resize(s);
}

void ShowMoreDialog::doShowMore(bool)
{
}

// ========================================================================

DataDialog::DataDialog(QWidget* parent, Controller* ctrl)
    : ShowMoreDialog(parent)
    , m_ctrl(ctrl)
    , m_action(0)
    , applyhideButton(0)
    , applyButton(0)
    , refreshButton(0)
    , helpButton(0)
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
  if (m_action)
    m_action->setChecked(false);
}

void DataDialog::setVisible(bool visible)
{
  ShowMoreDialog::setVisible(visible);
  if (m_action)
    m_action->setChecked(visible);
}

QLayout* DataDialog::createStandardButtons(bool refresh)
{
  QAbstractButton* hideButton = createButton(tr("Hide"), QPixmap(paint_hide_xpm));
  applyhideButton = createButton(tr("Apply + Hide"), QPixmap(hideapply_xpm));
  applyButton = new QPushButton(tr("Apply"), this);
  applyButton->setIcon(QPixmap(apply_xpm));
  applyButton->setDefault(true);
  indicateUnappliedChanges(false);

  connect(hideButton, &QAbstractButton::clicked, this, &DataDialog::close);
  connect(applyButton, &QAbstractButton::clicked, this, &DataDialog::applyData);
  connect(applyhideButton, &QAbstractButton::clicked, this, &DataDialog::applyhideClicked);

  QLayout* layout = new QHBoxLayout();
  layout->setSpacing(1);

  if (!helpFileName.empty()) {
    helpButton = createButton(tr("Help"), QPixmap(help_xpm));
    connect(helpButton, &QPushButton::clicked, this, &DataDialog::helpClicked);
    helpButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layout->addWidget(helpButton);
  }
  if (refresh) {
    refreshButton = createButton(tr("Refresh"), QPixmap(loop_xpm));
    connect(refreshButton, &QPushButton::clicked, this, &DataDialog::updateTimes);
    layout->addWidget(refreshButton);
  }

  layout->addWidget(hideButton);
  layout->addWidget(applyhideButton);
  layout->addWidget(applyButton);
  // layout->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum));

  return layout;
}

QAbstractButton* DataDialog::createButton(const QString& tooltip, const QIcon& icon)
{
  QToolButton* button = new QToolButton(this);
  button->setToolTip(tooltip);
  button->setIcon(icon);
  button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  return button;
}

void DataDialog::indicateUnappliedChanges(bool on)
{
  if ((!applyhideButton) || (!applyButton))
    return;

  if (on) {
    applyhideButton->setToolTip(tr("Apply* + Hide"));
    applyButton->setText(tr("Apply*"));
  } else {
    applyhideButton->setToolTip(tr("Apply + Hide"));
    applyButton->setText(tr("Apply"));
  }
}

void DataDialog::setRefreshEnabled(bool enabled)
{
  if (refreshButton)
    refreshButton->setEnabled(enabled);
}

void DataDialog::applyhideClicked()
{
  emit applyData();
  close();
}

void DataDialog::helpClicked()
{
  if (!helpFileName.empty())
    Q_EMIT showsource(helpFileName);
}
