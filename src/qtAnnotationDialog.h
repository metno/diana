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
#ifndef _annotationdialog_h
#define _annotationdialog_h

#include <diController.h>
#include <QDialog>

class QPushButton;
class QComboBox;
class QTextEdit;
/**
  \brief Annotation dialog

   Dialog for selection of labels
 */
class AnnotationDialog: public QDialog
{
  Q_OBJECT
public:

  AnnotationDialog( QWidget* parent, Controller* llctrl );
  void parseSetup();
  ///return command strings
  std::vector<std::string> getOKString();
  ///insert command strings
  void putOKString(const std::vector<std::string>& vstr);

  std::vector<std::string> writeLog();
  void readLog(const std::vector<std::string>& vstr,
      const std::string& thisVersion, const std::string& logVersion);

  ///called when the dialog is closed by the window manager
protected:
  void closeEvent( QCloseEvent* );

public slots:
void applyhideClicked();

private slots:
void annoBoxActivated(int);
void defaultButtonClicked();

signals:
void AnnotationApply();
void AnnotationHide();

private:

//ATTRIBUTES
QComboBox* annoBox;
QPushButton* defaultButton;
QTextEdit* textedit;
QPushButton* annotationapply;
QPushButton* annotationhide;
QPushButton* annotationapplyhide;
Controller* m_ctrl;
std::map<QString,QString> current_annoStrings;
std::map<QString,QString> setup_annoStrings;
QStringList annoNames;
};

#endif
