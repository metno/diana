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
#include <miString.h>

using namespace std; 

class Controller;
class ObjectManager;


/**
   \brief Dialogue for adding a new choice (name/file)
   
*/
class AddtoDialog :public QDialog
{
  Q_OBJECT
public:
  
  AddtoDialog( QWidget* parent, Controller* llctrl);
  /// get name from dialog
  miString getName();
  /// get file from dialog
  miString getFile();
  /// put name in dialog
  void putName(const miString &);
  /// put file in dialog
  void putFile(const miString &);
    
private:
  Controller*    m_ctrl;
  ObjectManager* m_objm;

  QLineEdit *name;
  QLineEdit *file;

signals:


private slots:


public slots:

};

#endif






