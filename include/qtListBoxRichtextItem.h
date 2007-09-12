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
#ifndef _ListBoxRichtextItem_h
#define _ListBoxRichtextItem_h

#include <qlistbox.h>
#include <qsimplerichtext.h>

/**
   \brief Qt listbox richtext item
   
   This is a Qt listbox item with richtext capabilities

*/

class ListBoxRichtextItem : public QListBoxItem {
public:
  ListBoxRichtextItem(QSimpleRichText* t, QBrush* b, QString st,
		      QListBox * listbox = 0 )
    : QListBoxItem(listbox), srt(t), selectb(b)
  {init(st);}
  ListBoxRichtextItem(QSimpleRichText* t, QBrush* b, QString st,
		      QListBox * listbox, QListBoxItem * after)
    : QListBoxItem (listbox,after), srt(t), selectb(b)
  {init(st);}
  ~ListBoxRichtextItem()
  {delete srt; delete selectb;}
  
  virtual int height ( const QListBox * ) const
  {return srt->height();}
  virtual int width ( const QListBox * ) const
  {return srt->width();}

protected:
  QSimpleRichText* srt;
  QBrush* selectb;

  void init(QString& sortt)
  {
    setCustomHighlighting(true);
    setText(sortt);
  }

  virtual void paint( QPainter * p)
  { 
    QRect clip(0,0,srt->width(),srt->height());
    srt->draw(p,0,0,clip,listBox()->colorGroup(),(selected()? selectb : 0));
  }
};

#endif
