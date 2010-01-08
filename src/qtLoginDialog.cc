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

#include <qtLoginDialog.h>

#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>


LoginDialog::LoginDialog(editDBinfo& d,  QWidget* parent)
  : QDialog( parent), dbi(d)
{

  setModal(true);

  QVBoxLayout* top_vlayout = new QVBoxLayout(this);

  QFrame* ff = new QFrame( this);
  ff->setFrameStyle( QFrame::Sunken | QFrame::Panel );
  ff->setLineWidth( 1 );
  top_vlayout->addWidget( ff);

  QVBoxLayout* f_vlayout=  new QVBoxLayout(ff);

   // Create a layout manager for the label
  QHBoxLayout* h_hlayout = new QHBoxLayout();
  QLabel* label= new QLabel(tr("Diana db login"), ff);
  label->setFrameStyle( QFrame::Panel | QFrame::Raised );
  label->setFont(QFont( "Helvetica", 14, QFont::Normal, true ));
  label->setPalette( QPalette( QColor(255, 89, 0) ) );
  h_hlayout->addWidget(label);
  f_vlayout->addLayout(h_hlayout);

  QGridLayout* glayout = new QGridLayout();

  int startwidget= 0;
  QLabel* server= new QLabel(tr("Database server:"), ff);
  QLabel* name= new QLabel(tr("Username:"), ff);
  QLabel* pwd= new QLabel(tr("Password:"), ff);
  dbserver  = new QLineEdit(ff);
  dbserver->setMinimumWidth(200);
  if (dbi.host.length()>0){
    dbserver->setText(dbi.host.c_str());
    startwidget=1;
  }
  username  = new QLineEdit(ff);
  if (dbi.user.length()>0){
    username->setText(dbi.user.c_str());
    startwidget= 2;
  }
  passwd  = new QLineEdit(ff);
  passwd->setEchoMode(QLineEdit::Password);

  glayout->addWidget(server, 1,1);
  glayout->addWidget(name, 2,1);
  glayout->addWidget(pwd, 3,1);
  glayout->addWidget(dbserver, 1,2);
  glayout->addWidget(username, 2,2);
  glayout->addWidget(passwd, 3,2);
  f_vlayout->addLayout(glayout, 0);

  okb= new QPushButton(tr("Log in"),ff);
  okb->setDefault(true);
  connect(okb, SIGNAL(clicked()), SLOT(accept()));
  quitb= new QPushButton(tr("Cancel"),ff);
  connect(quitb, SIGNAL(clicked()), SLOT(reject()));

  // buttons layout
  QHBoxLayout* b_hlayout = new QHBoxLayout();
  b_hlayout->addWidget(okb);
  b_hlayout->addWidget(quitb);

  f_vlayout->addLayout(b_hlayout);

  // Start the geometry management
  f_vlayout->activate();
  top_vlayout->activate();

  if (startwidget==0){
    dbserver->end(true);
    dbserver->setFocus();
  } else if (startwidget==1){
    username->end(true);
    username->setFocus();
  } else {
    passwd->end(true);
    passwd->setFocus();
  }
}

editDBinfo LoginDialog::getDbInfo()
{
  dbi.host= dbserver->text().toStdString();
  dbi.user= username->text().toStdString();
  dbi.pass= passwd->text().toStdString();
  return dbi;
}








