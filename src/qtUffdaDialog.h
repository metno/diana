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
#ifndef _uffdaDialog_h
#define _uffdaDialog_h

#include <QDialog>
#include <QToolTip>

#include <diController.h>
#include <deque>
#include <vector>

class PushButton;
class QListWidget;
class QListWidgetItem;
class QLabel;

/**
  \brief Dynamic tooltip
*/
class DynamicTip: public QToolTip
{
public:
  DynamicTip(QWidget * parent, std::vector <std::string> tips);
protected:
  void maybeTip(const QPoint &);
private:
  std::vector <QString> vClasstips;
};

/**

  \brief Dialog used for sending information to the Uffda project
  
A tool for collection of AVHRR based products. User clicks on points on the image and classifies them.Info is sent to uffda mail adress.

*/
class UffdaDialog: public QDialog
{
  Q_OBJECT

public:

  //the constructor
  UffdaDialog( QWidget* parent, Controller* llctrl );
  /// called when the dialog is closed by the window manager
  /// add position x,y to the list
  void addPosition(float,float);
  /// called when user clicked on uffda station on map
  void pointClicked(std::string);
  /// check if all Uffda data sent before exiting
  bool okToExit();
  /// deselects all items
  void clearSelection();

protected:
  void closeEvent( QCloseEvent* );

/**

  \brief One Uffda point with classification
  

*/
  struct uffdaElement{
    float lat,lon; //position
    QString satclass; //uffda class
    QString sattime;  //satelitte name and time
    QString posstring; //position string
    bool ok; 
  };

private:

  Controller* m_ctrl;
  DynamicTip * t;

//************** q tWidgets that appear in the dialog  *******************

  // listbox for selecting uffda class
  QListWidget * classlist;

  //list of times/files
  QListWidget* satlist; 

 // the box showing which positions have been choosen
  QLabel* posLabel;
  QListWidget* poslist;  
 

  //delete and store buttons
  QPushButton* Deleteb;
  QPushButton* DeleteAllb;
  QPushButton* storeb;

  //push buttons for help/hide/send
  QPushButton* helpb; 
  QPushButton* hideb;
  QPushButton* sendb;

  //deques of satellite positions , time,classes etc.
  std::deque <uffdaElement> v_uffda; 
  StationPlot * sp;

  std::string mailto; //mailadress to send Uffdastring to
  void updatePoslist(uffdaElement &ue, int, bool);
  void updateStationPlot();

  std::string getUffdaString();
  void send(const std::string& to, const std::string& subject,const std::string& body);

  int satIndex; //index of sat list
  int classIndex; //index of class list

private Q_SLOTS:
  void DeleteClicked();
  void DeleteAllClicked();
  void helpClicked();
  void storeClicked();
  void sendClicked();
  void classlistSlot(QListWidgetItem*);
  void satlistSlot(QListWidgetItem*);
  void poslistSlot(QListWidgetItem*);


Q_SIGNALS:
  void uffdaHide();
  void showsource(const std::string, const std::string=""); // activate help
  void stationPlotChanged();
};

#endif
