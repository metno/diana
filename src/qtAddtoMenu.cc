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
#include <qtAddtoMenu.h>
#include <qtQuickMenu.h>
#include <q3frame.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlayout.h>
#include <q3listbox.h>
#include <qinputdialog.h>
//Added by qt3to4:
#include <QPixmap>
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>

#include <miString.h>
#include <filenew.xpm>

using namespace std;

AddtoMenu::AddtoMenu(QWidget* parent, QuickMenu* qm)
  : QDialog(parent, "addtomenu", true), quick(qm)
{
  Q3HBoxLayout* b= new Q3HBoxLayout(this, 10, 10, "top_hlayout");

  frame= new Q3Frame(this);
  frame->setFrameStyle(Q3Frame::Panel | Q3Frame::Sunken);

  QString t= "<em><b>"+tr("Add current plot to a private quickmenu")+"</b></em>";
  QLabel* label= new QLabel(t, frame);
  label->setFrameStyle(Q3Frame::Panel | Q3Frame::Raised);
  
  list= new Q3ListBox(frame,"menulist");
  connect(list, SIGNAL(highlighted(int)),SLOT(menuSelected(int)));
  connect(list, SIGNAL(selected(int)),SLOT(okClicked()));
  list->setMinimumWidth(100);
  
  newButton = new QPushButton(QPixmap(filenew_xpm), tr("&Make new"), frame );
  newButton->setEnabled( true );
  connect( newButton, SIGNAL(clicked()), SLOT(newClicked()));
  
  Q3HBoxLayout* hl= new Q3HBoxLayout(5);
  hl->addWidget(list);
  hl->addWidget(newButton);

  // a horizontal frame line
  Q3Frame* line = new Q3Frame( frame );
  line->setFrameStyle( Q3Frame::HLine | Q3Frame::Sunken );

  // last row of buttons
  okButton= new QPushButton( tr("&OK"), frame );
  okButton->setEnabled(false);
  QPushButton* cancel= new QPushButton( tr("&Cancel"), frame );
  connect( okButton, SIGNAL(clicked()), SLOT(okClicked()) );
  connect( cancel, SIGNAL(clicked()), SLOT(reject()) );

  Q3HBoxLayout* hl2= new Q3HBoxLayout(5);
  hl2->addWidget(okButton);
  hl2->addWidget(cancel);
  
  Q3VBoxLayout* vl= new Q3VBoxLayout(frame, 5,5);
  vl->addWidget(label);
  vl->addLayout(hl);
  vl->addWidget(line);
  vl->addLayout(hl2);
  vl->activate();

  b->addWidget(frame);
  b->activate();
 
  resize(350,200);
  fillMenu();
  list->setFocus();
}

void AddtoMenu::fillMenu()
{
  list->clear();
  if (!quick) return;
  vector<miString> vs= quick->getCustomMenues();

  for (int i=0; i<vs.size(); i++){
    list->insertItem(vs[i].cStr());
  }
  if (vs.size()>0){
    list->setCurrentItem(0);
    list->setSelected(0,true);
  }
}

void AddtoMenu::okClicked()
{
  int idx= list->currentItem();
  if (quick->addToMenu(idx))
    accept();
}

void AddtoMenu::newClicked()
{
  bool ok = FALSE;
  QString text = QInputDialog::getText(tr("New Menu"),
				       tr("Make new menu with name:"),
				       QLineEdit::Normal,
				       QString::null, &ok, this );
  if ( ok && !text.isEmpty() ){
    if (quick->addMenu(text.latin1())){
      fillMenu();
      list->setCurrentItem(list->count()-1);
      list->setSelected(list->count()-1, true);
    }
  }
}

void AddtoMenu::menuSelected(int i)
{
  okButton->setEnabled(true);
}
