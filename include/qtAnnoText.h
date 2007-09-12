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
#ifndef _addtoDialog_h
#define _addtODialog_h

#include <qdialog.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qvalidator.h>
#include <miString.h>

using namespace std; 

class Controller;

/**
   \brief Dialogue for editing annotation texts
   

*/
class AnnoText :public QDialog
{
  Q_OBJECT
public:

  /// constructor for annotation text fpr product prodname, symbolText and xText are texts to be edited   
  AnnoText( QWidget* parent, Controller* llctrl, miString prodname,
vector <miString> & symbolText, vector <miString> & xText);
  /// get edited annotation text in symbolText and xtext
  void getAnnoText(vector <miString> & symbolText, vector <miString>
		      & xText);
  ~AnnoText();

    
private:
  Controller*    m_ctrl;

  vector <QComboBox*> vSymbolEdit;
  QPushButton* quitb;

  miString productname;

signals:
  /// redraw (update GL)
  void editUpdate();


private slots:
  void textChanged(const QString &);
  void textSelected();
  void stop();

public slots:

protected:
  void keyReleaseEvent(QKeyEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void mousePressEvent(QMouseEvent*);

};

#endif






