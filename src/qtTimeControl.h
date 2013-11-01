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
#ifndef _qtTimeControl_h
#define _qtTimeControl_h

#include <qdialog.h>
#include <QLabel>
#include <puTools/miTime.h>

#include <map>
#include <vector>


class QLabel;
class QSlider;
class QComboBox;
class QCheckBox;
class QRadioButton;

/**
  \brief Misc plot time settings

  Dialogue for selection of
  -animation interval
  -animation speed
  -data basis for main plot time
*/
class TimeControl : public QDialog {
  Q_OBJECT

public:

  TimeControl(QWidget*);
  /// Set data basis for time slider
  void useData(std::string type, int id=-1);
  /// Remove one data type
  std::vector<std::string> deleteType(int id);

  signals:
  ///Animation speed (sec)
  void timeoutChanged(float);
  ///Animation interval
  void minmaxValue(const miutil::miTime&, const miutil::miTime&);
  ///Animation interval cleared
  void clearMinMax();
  /// Data basis for time slider
  void data(std::string);
  void timecontrolHide();

public slots:
  /// Set times for start/stop sliders
  void setTimes( std::vector<miutil::miTime>& times );

private slots:
  void timeoffsetCheckBoxClicked();
  void timerangeCheckBoxClicked();
  void StartValue(int);
  void StopValue(int);
  void timeoutSlot(int);
  void dataSlot(int);
  void minmaxSlot();
  void OffsetValue(int);

private:

  std::vector<std::string> dataname;
  std::map<int,std::string> external_id;

  std::vector<miutil::miTime> m_times;
  QFont m_font;
  QCheckBox* timerangeCheckBox;
  QLabel* startTimeLabel;
  QLabel* stopTimeLabel;
  QSlider* startSlider;
  QSlider* stopSlider;
  QComboBox* timeoutBox;
  std::vector<float> timeouts;
  QComboBox* dataBox;
  QCheckBox* timeoffsetCheckBox;
  QLabel* offsetTimeLabel;
  QSlider* offsetSlider;

};

#endif
