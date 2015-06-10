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
#ifndef TRAJECTORYDIALOG_H
#define TRAJECTORYDIALOG_H

#include <diController.h>
#include <QDialog>
#include <vector>

class QComboBox;
class QListWidget;
class QPushButton;
class QSpinBox;
class QLabel;
class GeoPosLineEdit;
class QCheckBox;

/**
  \brief Dialogue for 2D-trajectories

   -select of start positions
   -select colours, lines etc
   -start computation
*/
class TrajectoryDialog: public QDialog
{
  Q_OBJECT

public:
  TrajectoryDialog(QWidget* parent, Controller* llctrl);

  ///add position to list of positions
  void mapPos(float lat, float lon);

  //send all positions to TrajectoryPlotdiObsDi
  void sendAllPositions();

  std::vector<std::string> writeLog();
  void readLog(const std::vector<std::string>& vstr,
      const std::string& thisVersion, const std::string& logVersion);

protected:
  void closeEvent( QCloseEvent* );

private:
  Controller* contr;

  struct posStruct {
    double lat,lon;
  };
  std::vector<posStruct> positionVector;

  std::vector<Colour::ColourInfo> colourInfo;
  std::vector<std::string> linetypes;

  QLabel* fieldName;
  QComboBox* colbox;
  QComboBox* lineWidthBox;
  QComboBox* lineTypeBox;
  GeoPosLineEdit* edit;
  QListWidget* posList;
  QCheckBox* posButton;
  QPushButton* deleteButton;
  QPushButton* deleteAllButton;
  QPushButton* startCalcButton;

private:
  void update_posList(float lat, float lon);

private Q_SLOTS:
  void colourSlot(int);
  void lineWidthSlot(int);
  void lineTypeSlot(int);
  void posButtonToggled(bool);
  void posListSlot();
  void deleteClicked();
  void deleteAllClicked();
  void startCalcButtonClicked();
  void helpClicked();
  void printClicked();
  void applyhideClicked();
  void quitClicked();
  void editDone();

public Q_SLOTS:
  void showplus();

Q_SIGNALS:
  void markPos(bool);
  void TrajHide();
  void updateTrajectories();
  void showsource(const std::string, const std::string="");
};

#endif
