/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006 met.no

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
#ifndef _buttonlayout_h
#define _buttonlayout_h

#include <qbuttongroup.h>
#include <qlayout.h>
#include <qpalette.h>
#include <qpushbutton.h>
#include <qwidget.h>

#include "diObsDialogInfo.h"
#include "diPlotCommand.h"
#include "qtToggleButton.h"

#include <string>
#include <vector>

/**
  \brief Grid of buttons
*/
class ButtonLayout : public QWidget
{
  Q_OBJECT

public:
  ButtonLayout(QWidget* parent, const std::vector<ObsDialogInfo::Button>& button_infos,
               int nr_col = 3 // number of columns
  );

  void setEnabled(bool enabled);
  bool noneChecked();
  void setButtonOn(const std::string& buttonName);

  std::vector<std::string> getOKString();

public Q_SLOTS:
  void ALLClicked();
  void NONEClicked();

private Q_SLOTS:
  void rightButtonClicked(ToggleButton* butto);

Q_SIGNALS:
  void rightClickedOn(const std::string&);
  void buttonClicked(int id);

private:
  QButtonGroup* bgroup;
  std::vector<ObsDialogInfo::Button> button_infos_;
  std::vector<ToggleButton*> togglebuttons_;
};

#endif
