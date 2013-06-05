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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtToggleButton.h"

#include <QMouseEvent>
#include <QPixmap>
#include <iostream>


ToggleButton::ToggleButton( QWidget* parent,
			    miutil::miString name,
			    QColor* color )
  : QPushButton( name.c_str(), parent)
{
  NameChange = false;

  if (color && &color[0] && &color[1]) {
    inPalette = QPalette( color[0], color[1] );
    outPalette = this->palette();
    this->setPalette( outPalette );
    usePalette  = true;
  } else {
    usePalette    = false;
  }

  this->setCheckable ( TRUE );

  connect( this, SIGNAL( toggled( bool )),this, SLOT(Toggled( bool ) ) );

}


ToggleButton::ToggleButton( QWidget* parent,
          std::string* name,
          QColor* color )
  : QPushButton( (name[1]).c_str(),  parent )
{


  if (color && &color[0] && &color[1]) {
    inPalette = QPalette( color[0], color[1] );
    outPalette = this->palette();
    this->setPalette( outPalette );
    usePalette  = true;
  } else {
    usePalette  = false;
  }


  NameChange = true;

  if ( name ) {
    m_inName  = name[0];
    m_outName = name[1];
  }

  this->setCheckable ( TRUE );

  connect( this, SIGNAL( toggled( bool )),this, SLOT(Toggled( bool ) ) );

}


ToggleButton::ToggleButton( QWidget* parent,
          miutil::miString* name,
          QColor* color )
  : QPushButton( (name[1]).c_str(),  parent )
{


  if (color && &color[0] && &color[1]) {
    inPalette = QPalette( color[0], color[1] );
    outPalette = this->palette();
    this->setPalette( outPalette );
    usePalette  = true;
  } else {
    usePalette  = false;
  }


  NameChange = true;

  if ( name ) {
    m_inName  = name[0];
    m_outName = name[1];
  }

  this->setCheckable ( TRUE );

  connect( this, SIGNAL( toggled( bool )),this, SLOT(Toggled( bool ) ) );

}


ToggleButton::ToggleButton( QWidget* parent,
			    const QPixmap& pixmap,
			    QColor* color )
  : QPushButton( pixmap, QString(""), parent)
{
  NameChange = false;

  if (color && &color[0] && &color[1]) {
    inPalette = QPalette( color[0], color[1] );
    outPalette = this->palette();
    this->setPalette( outPalette );
    usePalette  = true;
  } else {
    usePalette  = false;
  }

  this->setCheckable ( TRUE );

  connect( this, SIGNAL( toggled( bool )),this, SLOT(Toggled( bool ) ) );

}


void ToggleButton::Toggled( bool on )
{
  if( on ){
    if( usePalette )
      this->setPalette( inPalette );
    if( NameChange )
      this->setText( m_inName.c_str() );
  } else {
    if( usePalette )
      this->setPalette( outPalette );
    if( NameChange )
      this->setText( m_outName.c_str() );
  }

}

void ToggleButton::mouseReleaseEvent( QMouseEvent *e )
{

  if ( e->button() == Qt::RightButton ){
    emit rightButtonClicked(this);
  }

  QPushButton::mouseReleaseEvent(e);
}
