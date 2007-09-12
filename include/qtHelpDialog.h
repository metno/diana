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
#ifndef _helpdialog_h
#define _helpdialog_h


#include <qdialog.h>
#include <qfont.h>
#include <qpalette.h>
#include <miString.h>
#include <vector>

using namespace std;

class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QTextBrowser;
class QLabel;


/**
   \brief the documentation viewer

   the help/documentation viewer includes
   - simple html-like document syntax
   - hypertext capabilities
   - navigation buttons

*/

class HelpDialog: public QDialog {
  Q_OBJECT
public:
  HelpDialog( QWidget* parent, const miString p, const miString s );

  /// set the path to all document files
  void addFilePath( const miString& filepath );
  /// set new source
  void setSource( const miString& source );
  /// return current documentation path
  miString helpPath() const {return path;}

public slots:
  /// show the named document 
  void showdoc(const miString doc);
  /// jump to tag in document
  void jumpto(const miString tag);

private slots:
  void hideHelp();
  void printHelp();
  void backwardAvailable(bool b);
  void forwardAvailable(bool b);
   
private:
  void ConstructorCernel(const miString& filepath, const miString& source);
  
  QTextBrowser* tb;

  bool firstdoc;
  miString path;

  QPushButton* backwardbutton;
  QPushButton* forwardbutton;
  QPushButton* closebutton;
  QPushButton* printbutton;
  QLabel* plabel; 

  QVBoxLayout* vlayout;
  QHBoxLayout* hlayout;

  QFont m_font; 
};


#endif
