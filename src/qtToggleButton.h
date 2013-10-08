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
#ifndef _togglebutton_h
#define _togglebutton_h

#include <qwidget.h>
#include <qpushbutton.h>
#include <qpalette.h>
#include <qpixmap.h>
#include <QMouseEvent>
#include <puTools/miString.h>



/**

  \brief Toggle button

  Button changes colour ( and optionally text ) when toggled

*/
class ToggleButton : public QPushButton
{
    Q_OBJECT

public:

 ToggleButton( QWidget* parent,
     const miutil::miString& name,
     QColor* color = 0 );

 ToggleButton( QWidget* parent,
     const std::string* name,
     QColor* color = 0 );

 ToggleButton( QWidget* parent,
     const miutil::miString* name,
     QColor* color = 0 );

 ToggleButton( QWidget* parent,
	       const QPixmap& pixmap,
               QColor* color = 0 );

signals:
  void rightButtonClicked(ToggleButton*);

public slots:
 void Toggled( bool on );

protected:
 virtual void mouseReleaseEvent( QMouseEvent * );
 
private:
  bool usePalette;
  bool NameChange;
  miutil::miString m_outName;
  miutil::miString m_inName;
  QPalette outPalette;
  QPalette inPalette;
};

#endif
