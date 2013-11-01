/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

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
#ifndef _qtShowSatValues_h
#define _qtShowSatValues_h

#include "diCommonTypes.h"

#include <qwidget.h>
#include <QLabel>

class QLabel;
class QComboBox;

/**
  \brief Geo image pixel value in status bar
*/
class ShowSatValues : public QWidget {
  Q_OBJECT
public:
  ShowSatValues(QWidget* parent = 0);

private slots:
  void channelChanged(int index);

public slots:
  /// set channels in combobox
  void SetChannels(const std::vector<std::string>& channel);
  /// show channel and value in status bar
  void ShowValues(const std::vector<SatValues>& satval);

// signals:
//   void calibChannel(std::string);

private:
  std::vector<std::string> tooltip;
  QLabel *sylabel;
  QLabel *chlabel;
  QComboBox *channelbox;
};

#endif
