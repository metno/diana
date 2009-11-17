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
#include "qtAdvancedButton.h"
//Added by qt3to4:
#include <QMouseEvent>
#include <stdio.h>
#include <iostream>




AdvancedButton::AdvancedButton(  QWidget* parent,
				 miutil::miString name)
  :QPushButton( name.c_str(), parent){

  inPalette = QPalette( QColor( 100,200,200));
  outPalette = this->palette();

}

void AdvancedButton::mouseReleaseEvent( QMouseEvent *e )
{
  if ( e->button() == Qt::RightButton ){
    //    emit rightClicked(this);
    emit rightButtonClicked(this);
  }

  QPushButton::mouseReleaseEvent(e);
}



void AdvancedButton::setDefaultPalette(bool in)
{
  if( in )
   this->setPalette( inPalette );
  else
   this->setPalette( outPalette );
}




