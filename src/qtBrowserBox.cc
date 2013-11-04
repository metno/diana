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

#include "qtBrowserBox.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QKeyEvent>


BrowserBox::BrowserBox(QWidget* parent)
  : QDialog(parent)
// , "browserbox", true,
// 	    Qt::WStyle_Customize | Qt::WStyle_NoBorder)
  //	    WStyle_Customize | WStyle_NoBorderEx) | Qt::WX11BypassWM
{
  QHBoxLayout* b= new QHBoxLayout(this);

  QFrame* frame= new QFrame(this);
  frame->setFrameStyle(QFrame::Panel | QFrame::Sunken);

  QVBoxLayout* vb= new QVBoxLayout(frame);
  listname= new QLabel("",frame);
  listname->setAlignment(Qt::AlignCenter);
  listname->setFrameStyle(QFrame::Panel | QFrame::Raised);
  vb->addWidget(listname);

  numlabel= new QLabel("99",frame);
  label= new QLabel("",frame);
  label->setAlignment(Qt::AlignCenter);
  label->setWordWrap(true);

  vb->addWidget(numlabel);
  vb->addWidget(label);
  
  vb->activate();

  b->addWidget(frame);
  b->activate();

  resize(550,150);
}

void BrowserBox::upDate(const std::string& name,
			const int num,
			const std::string& item)
{
  std::string caption= "<em><b>" + name + "</b></em>";
  listname->setText(caption.c_str());
  numlabel->setNum(num+1);
  label->setText(item.c_str());
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



