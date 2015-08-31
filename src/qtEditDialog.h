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
#ifndef _editdialog_h
#define _editdialog_h


#include <QDialog>
#include <qpixmap.h>

#include <diCommonTypes.h>
#include <diEditSpec.h>

#include <vector>

class QComboBox;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QSlider;
class ToggleButton;
class QButtonGroup;
class QPushButton;
class QRadioButton;
class QTabWidget;
class EditNewDialog;
class EditComment;
class QMessageBox;
class QCheckBox;
class QAction;

class Controller;
class EditManager;
class ObjectManager;


/**
   \brief Dialogue for editing fields and weather objects and for combining
*/
class EditDialog: public QDialog
{
    Q_OBJECT
private:
    enum { prodb = 0, saveb=  1, sendb=  2 };

public:
  EditDialog( QWidget* parent, Controller* llctrl );
  /// check if any unsaved edits before exiting
  bool okToExit();
  /// stop editing and logout from database
  bool cleanupForExit();
  /// show dialog
  void showAll();
  /// hide dialog
  void hideAll();
  /// returns true if currently editing (not pause)
  bool inedit();

protected:
  ///called when the dialog is closed by the window manager
  void closeEvent( QCloseEvent* );

private:
  void ConstructorCernel( const EditDialogInfo mdi );

  void FieldTab();
  void FrontTab();
  void CombineTab();
  void CombineEditMethods();
  void ListWidgetData( QListWidget* list, int mindex, int index);
  void ComboBoxData( QComboBox* box, int mindex);
  bool saveEverything(bool send, bool approved);
  void updateLabels();

private slots:
  // Field slots
  void fgroupClicked( int index );
  void FieldEditMethods( QListWidgetItem * );
  void fieldEllipseChanged( int index );
  void fieldEllipseShape();
  void undoFieldsEnable();
  void undoFieldsDisable();
  void undofield();
  void redofield();
  void changeInfluence( int index );
  void exlineCheckBoxToggled(bool on);
  // Object slots
  //void FrontEditMethods( int index);
  void FrontTabBox( int index );
  void undofront();
  void redofront();
  void undoFrontsEnable();
  void undoFrontsDisable();
  void autoJoinToggled(bool on);
  void FrontEditDoubleClicked();
  void FrontEditClicked();
  void EditMarkedText();
  void DeleteMarkedAnnotation();
  // Combine slots
  void stopCombine();
  void combine_action(int);
  void selectAreas(QListWidgetItem * );
  // Common slots
  void saveClicked();
  void sendClicked();
  void approveClicked();
  void tabSelected( int );
  void commentClicked(bool);
  void pauseClicked(bool);
  void hideComment();
  void exitClicked();
  void helpClicked();
  void EditNewOk(EditProduct&, EditProductId&, miutil::miTime&);
  void EditNewCombineOk(EditProduct&, EditProductId&, miutil::miTime&);
  void EditNewCancel();

public Q_SLOTS:
  ///undo fields or objects depending on mode
  void undoEdit();
  ///redo fields or objects depending on mode
  void redoEdit();
  /// save edits to file
  void saveEdit();

Q_SIGNALS:
  /// hide dialog
  void EditHide();
  /// redraw (update GL)
  void editUpdate();
  /// apply edit commands
  void editApply();
 /// send plot-commands
  void Apply(const std::vector<std::string>& s, bool);
  /// show documentation
  void showsource(const std::string, const std::string="");
  /// emit edit times
  void emitTimes(const std::string&, const std::vector<miutil::miTime>&);
  /// update field dialog
  void emitFieldEditUpdate(std::string);
  /// editing on or off
  void editMode(bool);
  /// resize main window
  void emitResize(int, int);

private:
  Controller*    m_ctrl;
  EditManager*   m_editm;
  ObjectManager* m_objm;
  EditDialogInfo m_EditDI;

  QAction * editAction;
  QAction * deleteAction;

  QComboBox* m_Frontcm;
  QListWidget* m_Fronteditmethods;
  QListWidget* m_Fieldeditmethods;
  QListWidget* m_SelectAreas;

  EditNewDialog* enew;
  EditComment* ecomment;
  QTabWidget* twd;
  QLabel *prodlabel;
  QLabel *lStatus;

  QButtonGroup* bgroupinfluence;
  QRadioButton* rbInfluence[4];

  QLabel* ellipsenumber;
  QSlider* ellipseslider;
  std::vector<float> ellipsenumbers;

  QPushButton*  saveButton;
  QPushButton*  sendButton;
  QPushButton*  approveButton;
  ToggleButton* commentbutton;
  ToggleButton* pausebutton;
  QPushButton* editexit;
  QPushButton* edithelp;
  QPushButton* undoFieldButton;
  QPushButton* redoFieldButton;
  QPushButton* undoFrontButton;
  QPushButton* redoFrontButton;
  QPushButton* stopCombineButton;

  QCheckBox* exlineCheckBox;
  QCheckBox* autoJoin;

  QPixmap openValuePixmap;
  QPixmap lockValuePixmap;

  //toplayout
  // tab widgets
  QWidget* fieldtab;
  QWidget* objecttab;
  QWidget* combinetab;

  enum { maxfields=2 };
  int numfields, fieldIndex;
  QButtonGroup* fgroup;
  QPushButton** fbutton;

  int m_FrontcmIndex; // index of m_Frontcm;
  int m_FronteditIndex; // index of m_Fronteditmethods;
  std::vector <std::string> m_FronteditList;// items in Frontedit

  std::map<std::string,QString> editTranslations;//translate to any language

  int fieldEditToolGroup;  // 0=standard 1=classes 2=numbers
  int numFieldEditTools;
  int currFieldEditToolIndex;
  std::vector<std::string> classNames;
  std::vector<float>    classValues;
  std::vector<bool>     classValuesLocked;

  std::string currMapmode;
  std::string currEditmode;
  std::string currEdittool;
  int combineAction;
  bool inEdit;          // editing is active
  bool productApproved; // product has been approved
  // current production
  EditProduct currprod;
  EditProductId currid;
  miutil::miTime prodtime;

  bool getText(std::string &, Colour::ColourInfo &);
  bool getEditText(std::vector<std::string> &);
  bool getComplexText(std::vector<std::string> &, std::vector<std::string> &);
  bool getComplexColoredText(std::vector<std::string>&, std::vector<std::string>&, Colour::ColourInfo &);


  QString TABNAME_FIELD;
  QString TABNAME_OBJECTS;
  QString TABNAME_COMBINE;
};

#endif
