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

#include <QDialog>

#include <diEditSpec.h>

#include <vector>
#include <map>

class Controller;
class EditManager;
class QLabel;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QPushButton;
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

  bool fieldSelected()
    {return not selectedfield.empty();}
  bool productSelected()
    {return (vselectedprod.size() && (not vselectedprod[0].filename.empty()));}
  const std::string& selectedField()
    {return selectedfield;}
  const std::vector<savedProduct>& vselectedProd()
    {return vselectedprod;}

private:
  std::vector <std::string> getProductNames();
  void fillList();
  void updateFilenames();
  std::string selectedObjectTypes(); //fronts /symbols/areas ?
  void setCheckedCbs(std::map<std::string,bool> useEditobject);
  void initCbs();

private slots:
  void prodnameActivated(int);
  void fieldselect(QListWidgetItem*);
  void filenameSlot(QListWidgetItem*);
  void cbsClicked();

  void DeleteClicked();
  void Refresh();

private:
  Controller* m_ctrl;
  EditManager* m_editm;

  QComboBox* prodnamebox;   //list of available products
  QListWidget *fBox;           // list of available fields

 // the box showing which files have been choosen
  QLabel* filesLabel;
  QListWidget* filenames;

  //delete and refresh buttons
  QPushButton* Delete;
  QPushButton* refresh;

  QPushButton *ok;

  //Checkboxes for selecting fronts/symbols/areas
  QCheckBox *cbs0;
  QCheckBox *cbs1;
  QCheckBox *cbs2;
  QCheckBox *cbs3;

  EditProduct EdProd;
  std::string currentProductName;
  std::string fieldname;
  std::vector<std::string> fields;
  int num;
  int selectedProdIndex;
  std::map <std::string, std::vector<savedProduct> > pmap;
  std::string selectedfield;
  std::vector <savedProduct> vselectedprod;
  std::vector<std::string> productNames;  //list of products in prodnamebox

  std::string MODELFIELDS;
};

#endif
