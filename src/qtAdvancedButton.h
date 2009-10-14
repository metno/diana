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
#ifndef _advancedbutton_h
#define _advancedbutton_h

#include <qwidget.h>
#include <qpushbutton.h>
//Added by qt3to4:
#include <QMouseEvent>
#include <puTools/miString.h>


using namespace std; 

/**

  \brief Sending signal when right mouse button clicked
    
  Subclass of QPushButton sending the signal rightButtonClicked 
  when the right mouse button is released

*/
class AdvancedButton : public QPushButton
{
    Q_OBJECT

 public: 

 AdvancedButton( QWidget* parent,
	       miString name);



 signals:
    void rightButtonClicked(AdvancedButton*);

private:
  QPalette outPalette;
  QPalette inPalette;


 protected:
 virtual void mouseReleaseEvent( QMouseEvent * );
 
public slots:
  void setDefaultPalette(bool in=true );  


};



#endif  
