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
#include <qtBrowserBox.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <QLabel>
#include <Q3Frame>
#include <QKeyEvent>


BrowserBox::BrowserBox(QWidget* parent)
  : QDialog(parent, "browserbox", true,
	    Qt::WStyle_Customize | Qt::WStyle_NoBorder)
  //	    WStyle_Customize | WStyle_NoBorderEx) | Qt::WX11BypassWM
{
  Q3HBoxLayout* b= new Q3HBoxLayout(this, 10, 10, "top_hlayout");

  frame= new Q3Frame(this);
  frame->setFrameStyle(Q3Frame::Panel | Q3Frame::Sunken);
  //frame->setPalette( QPalette( QColor(255, 255, 255) ) );

  Q3VBoxLayout* vb= new Q3VBoxLayout(frame, 10, 10, "vlayout");
  listname= new QLabel("",frame);
  listname->setAlignment(Qt::AlignCenter);
  listname->setFrameStyle(Q3Frame::Panel | Q3Frame::Raised);
  //listname->setPalette( QPalette( QColor(175, 175, 175) ) );
  vb->addWidget(listname,0);

  Q3HBoxLayout* hb= new Q3HBoxLayout();
  numlabel= new QLabel("99",frame);
  label= new QLabel("",frame);
  label->setAlignment(Qt::AlignCenter);

  hb->addWidget(numlabel,0);
  hb->addWidget(label,1);
  
  vb->addLayout(hb,1);
  vb->activate();

  b->addWidget(frame);
  b->activate();

  resize(550,150);
}

void BrowserBox::upDate(const miString& name,
			const int num,
			const miString& item)
{
  miString caption= "<em><b>" + name + "</b></em>";
  listname->setText(caption.cStr());
  numlabel->setNum(num+1);
  label->setText(item.cStr());
  
  QRect r= geometry();
  QRect pr= parentWidget()->geometry();
  move(pr.x()+(pr.width()-r.width())/2,pr.y()+(pr.height()-r.height())/2);
  //setActiveWindow();
  //setFocus();
}

void BrowserBox::keyPressEvent(QKeyEvent *me)
{
  switch (me->key()){
  case Qt::Key_F10:
  case Qt::Key_Left:
    emit(prevplot());
    break;
  case Qt::Key_F11:
  case Qt::Key_Right:
    emit(nextplot());
    break;
  case Qt::Key_Up:
    emit(prevlist());
    break;
  case Qt::Key_Down:
    emit(nextlist());
    break;
  case Qt::Key_Escape:
    emit(cancel());
    break;
  }
}

void BrowserBox::keyReleaseEvent(QKeyEvent *me)
{
  switch (me->key()){
  case Qt::Key_Alt:
    emit(selectplot());
    break;
  }
}

