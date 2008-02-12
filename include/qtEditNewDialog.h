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
#ifndef _editnewdialog_h
#define _editnewdialog_h

#include <qdialog.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <QLabel>
#include <Q3Frame>

#include <vector>
#include <diCommonTypes.h>
#include <diEditSpec.h>

using namespace std;

class Q3VBoxLayout;
class QLabel;
class Q3ListBox;
class QPushButton;
class QRadioButton;
class QComboBox;
class QTabWidget;
class Q3Frame;

class TimeSpinbox;

class Controller;
class EditManager;

/**
   \brief Dialogue for starting a new edit product 
   

*/
class EditNewDialog: public QDialog
{
  Q_OBJECT
public:
  EditNewDialog( QWidget* parent, Controller* llctrl );
  /// load info about edit products
  bool load(editDBinfo& edbi);
  /// true if defining a new product
  bool newActive;
  ///called when the dialog is closed by the window manager
  bool close(bool alsoDelete);
  
private:
  void ConstructorCernel();
  bool load_combine();
  void combineClear();
  bool setNormal();
  bool checkStatus();
  void handleObjectButton(int);
  void handleFieldButton(int);
  bool checkProductFree();
  void setLoginLabel();
  miString savedProd2Str(const savedProduct& sp,
			 const miString undef = "udefinert");
  
private slots:
  void login_clicked(); 

  void ok_clicked(); 
  void help_clicked(); 
  void cancel_clicked();
  void prodBox(int);
  void idBox(int);
  void ebutton0();
  void ebutton1();
  void ebutton2();
  void ebutton3();
  void prodtimechanged(int);
  void combineSelect(int);
  void tabSelected(const QString&);
  void kill_clicked();

signals:
  /// emitted when starting new product
  void EditNewOk(EditProduct&, EditProductId&, miTime&);
  /// emitted when starting new combine product
  void EditNewCombineOk(EditProduct&, EditProductId&, miTime&);
  /// emitted when help clicked
  void EditNewHelp();
  /// emitted when cancel clicked
  void EditNewCancel();
  /// emitted when login clicked
  void newLogin(editDBinfo&);

private:
  enum { maxelements=4 };
  Controller*  m_ctrl;
  EditManager* m_editm;

  bool  first;

  QLabel* prodlabel;
  QComboBox* prodbox;
  QLabel* idlabel;
  QComboBox* idbox;
  QPushButton* loginb;
  QPushButton* killb;
  QLabel* loginlabel;
  QLabel* availlabel;
  QTabWidget* twd;
  Q3Frame* normaltab;
  Q3Frame* combinetab;
  QLabel* normallabel;
  QLabel* combinelabel;
  Q3ListBox *cBox; // list of combine times
  QPushButton *ok;
  QPushButton *help;


  // product element buttons and labels
  QLabel* elab[maxelements];
  QPushButton* ebut[maxelements];
  QLabel* cselectlabel;
  QLabel* cpid2label;
  // current id and time
  bool normal;
  int currprod;
  EditProductId pid;
  miTime prodtime; //time for normal analysis
  miTime combinetime; //time for combined analysis
  // from EditManager
  vector<EditProduct> products;
  editDBinfo dbi;
  bool productfree;
  bool isdata;
  TimeSpinbox* timespin;

  void setObjectLabel();
  void setFieldLabel();

  QString TABNAME_NORMAL;

};

#endif
