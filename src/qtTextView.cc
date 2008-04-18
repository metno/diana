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
#include <qtTextView.h>

#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QScrollArea>
#include <QVBoxLayout>

TextWidget::TextWidget(QWidget* parent, const miString& text, int id_)
  : QWidget(parent)
{ 
  id = id_;

  label = new QLabel(QString(text.c_str()),this);
  QScrollArea* scroll = new QScrollArea(this);
  scroll->setWidget( label );

  QVBoxLayout *vlayout = new QVBoxLayout( this, 5, 5);
  vlayout->addWidget( scroll );

}

void TextWidget::setText(miString text)
{
  QString str = text.c_str();
  if(!str.isNull())
    label->setText(str);
}

TextView::TextView(QWidget* parent)
  : QDialog(parent)
{ 

  tabwidget = new QTabWidget(this);
  QPushButton* printButton = new QPushButton(tr("Print"),this);
  connect( printButton,SIGNAL(clicked()), SLOT( printSlot() ));
  QPushButton* hideButton = new QPushButton(tr("Hide"),this);
  connect( hideButton,SIGNAL(clicked()), hideButton,SLOT( hide() ));

}

void TextView::setText(int id, const miString& name,
		      const miString& text)
{
  if(!idmap.count(id)){
    TextWidget* widget = new TextWidget(this,text,id);
    idmap[id]=widget;
    tabwidget->addTab(widget,name.cStr());
  } else {
    idmap[id]->setText(text);
  }
}

void TextView::deleteTab(int id)
{

  if(idmap.count(id)){
    tabwidget->removePage(idmap[id]);
    idmap.erase(id);
  }

  if(idmap.size()==0)
    hide();
}

void TextView::printSlot()
{
  emit printClicked(tabwidget->currentIndex());
}



