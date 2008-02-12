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
#ifndef _editDefineFielddialog_h
#define _editDefineFielddialog_h

#include <qdialog.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <QLabel>

#include <diEditSpec.h>
#include <miString.h>
#include <vector>
#include <map>

using namespace std;

class Controller;
class EditManager;
class Q3VBoxLayout;
class QLabel;
class QComboBox;
class Q3ListBox;
class QPushButton;
class Q3VButtonGroup;
class QCheckBox;


/**
   \brief Dialogue for selecting fields for editing

   Access to fields chosen in FieldDialog and fields
   previously edited and saved (locally or via the product database system).
*/
class EditDefineFieldDialog: public QDialog
{
  Q_OBJECT

public:
  EditDefineFieldDialog(QWidget* parent, Controller*,int n, EditProduct ep);

  bool fieldSelected(){return selectedfield.exists();}
  bool productSelected(){return (vselectedprod.size()
				 && vselectedprod[0].filename.exists());}
  miString selectedField(){return selectedfield;}
  vector <savedProduct> vselectedProd(){return vselectedprod;}

private:
  vector <miString> getProductNames();
  void fillList();
  void updateFilenames();
  miString selectedObjectTypes(); //fronts /symbols/areas ?
  void setCheckedCbs(map<miString,bool> useEditobject);
  void initCbs();

private slots:
  void prodnameActivated(int);
  void fieldselect();
  void filenameSlot();
  void cbsClicked();

  void DeleteClicked();
  void Refresh();
  void help_clicked();

signals:
  void EditDefineHelp();

private:
  Controller* m_ctrl;
  EditManager* m_editm;

  QComboBox* prodnamebox;   //list of available products
  Q3ListBox *fBox;           // list of available fields

 // the box showing which files have been choosen
  QLabel* filesLabel;
  Q3ListBox* filenames;

  //delete and refresh buttons
  QPushButton* Delete;
  QPushButton* refresh;

  QPushButton *ok;
  QPushButton *help;

  //Checkboxes for selecting fronts/symbols/areas
  Q3VButtonGroup * bgroupobjects;
  QCheckBox *cbs0;
  QCheckBox *cbs1;
  QCheckBox *cbs2;
  QCheckBox *cbs3;

  EditProduct EdProd;
  miString currentProductName;
  miString fieldname;
  vector<miString> fields;
  int num;
  int selectedProdIndex;
  map <miString, vector<savedProduct> > pmap;
  miString selectedfield;
  vector <savedProduct> vselectedprod;
  vector<miString> productNames;  //list of products in prodnamebox

  miString MODELFIELDS;

};

#endif









