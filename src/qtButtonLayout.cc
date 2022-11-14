/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#include "qtButtonLayout.h"

#include <puTools/miStringFunctions.h>

#include <QGridLayout>
#include <QButtonGroup>

#define MILOGGER_CATEGORY "diana.ButtonLayout"
#include <miLogger/miLogging.h>


ButtonLayout::ButtonLayout(QWidget* parent, const std::vector<ObsDialogInfo::Button>& button_infos, int nr_col)
    : QWidget(parent)
    , bgroup(new QButtonGroup(this))
    , button_infos_(button_infos)
{
  QGridLayout *layoutgrid = new QGridLayout(this);
  layoutgrid->setSpacing(1);

  bgroup->setExclusive(false);
  togglebuttons_.reserve(button_infos_.size());

  int col = 0, row = 0, id = 0;
  for (const ObsDialogInfo::Button& binfo : button_infos_) {
    ToggleButton* tb = new ToggleButton(this, QString::fromStdString(binfo.name));
    connect(tb, &ToggleButton::rightButtonClicked, this, &ButtonLayout::rightButtonClicked);
    bgroup->addButton(tb, id++);
    tb->setToolTip(QString::fromStdString(binfo.tooltip));
    tb->setCheckable(true);
    tb->setEnabled(true);
    togglebuttons_.push_back(tb);

    layoutgrid->addWidget(tb, row, col);
    col += 1;
    if (col >= nr_col) {
      col = 0;
      row += 1;
    }
  }

  connect(bgroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &ButtonLayout::buttonClicked);
}

void ButtonLayout::setEnabled(bool enabled)
{
  for (ToggleButton* tb : togglebuttons_)
    tb->setEnabled(enabled);
}

void ButtonLayout::ALLClicked()
{
  for (ToggleButton* tb : togglebuttons_)
    tb->setChecked(true);
}

void ButtonLayout::NONEClicked()
{
  for (ToggleButton* tb : togglebuttons_)
    tb->setChecked(false);
}

bool ButtonLayout::noneChecked()
{
  for (ToggleButton* tb : togglebuttons_) {
    if (tb->isChecked())
      return false;
  }
  return true;
}

void ButtonLayout::setButtonOn(const std::string& buttonName)
{
  const std::string buttonName_l = miutil::to_lower(buttonName);
  for (size_t j = 0; j < button_infos_.size(); ++j) {
    if (buttonName_l == miutil::to_lower(button_infos_[j].name)) {
      togglebuttons_[j]->setChecked(true);
      break;
    }
  }
}

std::vector<std::string> ButtonLayout::getOKString()
{
  std::vector<std::string> str;
  for (size_t j = 0; j < button_infos_.size(); ++j) {
    if (togglebuttons_[j]->isChecked())
      str.push_back(button_infos_[j].name);
  }
  return str;
}

void ButtonLayout::rightButtonClicked(ToggleButton* butto  )
{
  const int id = bgroup->id(butto);
  if (id >= 0 && id < (int)button_infos_.size()) {
    const std::string& name = button_infos_[id].name;
    Q_EMIT rightClickedOn(name);
  }
}
