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
/*
  Saving and editing comments
  Updates: 
  March 2001
  August 2001, two windows 
*/

#include <fstream>
#include <qapplication.h>
#include <qwidget.h>
#include <qlayout.h>
#include <qsplitter.h>
#include <qtEditComment.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <miString.h>
#include <qstring.h>
#include <q3multilineedit.h>
#include <qtToggleButton.h>
#include <iostream>
#include <diController.h>
#include <diObjectManager.h>


/*********************************************/
EditComment::EditComment( QWidget* parent, Controller* llctrl, 
			  bool edit)
: QDialog(parent), m_ctrl(llctrl), m_objm(0), mEdit(0), mEdit2(0)
{
#ifdef dEdit 
  cout<<"EditComment::EditComment called"<<endl;
#endif



  //initialization, font, colours etc
   
  m_objm= m_ctrl->getObjectManager();
  inEditSession = edit;
  inComment = false;

  //one window mEdit for editing new comments
  // one window mEdit2 for showing old comments (also used in objectDialog)
  if (inEditSession){
    setGeometry(100,100,480,480);
    setCaption(tr("Comments-editing"));
    split = new QSplitter(Qt::Vertical,this);
    split->setGeometry(10,10,460,380);
    mEdit = new Q3MultiLineEdit(split);
    mEdit2 = new Q3MultiLineEdit(split);
    mEdit2->hide();
    showOld = new ToggleButton(this,tr("Show previous comments").latin1());
    showOld->setOn(false);
    connect(showOld, SIGNAL( toggled(bool)),SLOT( showOldToggled( bool ) ));
    Q3VBoxLayout * vlayout = new Q3VBoxLayout( this);
    vlayout->addWidget(split);
    vlayout->addWidget(showOld);
  }
  else {
    setGeometry(100,100,480,400);
    setMinimumSize(480,400);
    setMaximumSize(480,400);
    setCaption(tr("Comments"));
    mEdit2 = new Q3MultiLineEdit(this);
    mEdit2->setGeometry(10,10,460,380);
  }

}



/*********************************************/


void EditComment::startComment()
{
  //start comments for editing
  if (inComment) return;
  mEdit->clear();
  miString comments = m_objm->getComments();
  mEdit->setText(comments.c_str());
  mEdit->insertLine("\n");
  int n = mEdit->numLines();
  mEdit->setCursorPosition(n,0);
  mEdit->setEdited(false);
  inComment = true;
  if (showOld->isOn()) readComment();
}



void EditComment::readComment()
{
  //read comments only
  mEdit2->clear();
  miString comments = m_objm->readComments(inEditSession);
  mEdit2->setText(comments.c_str());
  mEdit2->insertLine("\n");
  int n = mEdit2->numLines();
  mEdit2->setCursorPosition(n,0);
  mEdit2->setReadOnly(true);
}



void EditComment::saveComment()
{
  if (inComment && mEdit->edited()){
    miString comments = miString(mEdit->text().latin1());
    //put comments into plotm->editobjects->comments;
    m_objm->putComments(comments);
    mEdit->setEdited(false);
  }
}


void EditComment::stopComment(){
  if (inEditSession){
    mEdit->clear();
    mEdit->setEdited(false);
  }
  mEdit2->clear();  
  inComment = false;
}


void EditComment::showOldToggled(bool on){
  if (on){
    mEdit2->show();
    readComment();
  }
  else
    mEdit2->hide();
}


bool EditComment::close(bool alsoDelete){
  emit CommentHide();
  return true;
}




