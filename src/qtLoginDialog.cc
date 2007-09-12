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


LoginDialog::LoginDialog(editDBinfo& d,  QWidget* parent)
  : QDialog( parent, "login", true ), dbi(d)
{

  top_vlayout = new QVBoxLayout(this, 10, 10, "top_vlayout");

  ff = new QFrame( this, "loginframe" );
  ff->setFrameStyle( QFrame::Sunken | QFrame::Panel );
  ff->setLineWidth( 1 );
  top_vlayout->addWidget( ff, 1 );

  f_vlayout=  new QVBoxLayout(ff, 10, 10, "f_vlayout");

   // Create a layout manager for the label
  h_hlayout = new QHBoxLayout(20, "h_hlayout");
  QLabel* label= new QLabel(tr("Diana db login"), ff,"label");
  label->setFrameStyle( QFrame::Panel | QFrame::Raised );
  label->setFont(QFont( "Helvetica", 14, QFont::Normal, true ));
  label->setPalette( QPalette( QColor(255, 89, 0) ) );
  h_hlayout->addWidget(label,0);
  f_vlayout->addLayout(h_hlayout, 0);

  QGridLayout* glayout = new QGridLayout(3,2,5,"loglayout");
  
  int startwidget= 0;
  QLabel* server= new QLabel(tr("Database server:"), ff,"server"); 
  QLabel* name= new QLabel(tr("Username:"), ff,"name"); 
  QLabel* pwd= new QLabel(tr("Password:"), ff,"pwd");
  dbserver  = new QLineEdit(ff,"dbserver");
  dbserver->setMinimumWidth(200);
  if (dbi.host.length()>0){
    dbserver->setText(dbi.host.c_str());
    startwidget=1;
  }
  username  = new QLineEdit(ff,"username");
  if (dbi.user.length()>0){
    username->setText(dbi.user.c_str());
    startwidget= 2;
  }
  passwd  = new QLineEdit(ff,"passwd");
  passwd->setEchoMode(QLineEdit::Password);

  glayout->addWidget(server, 1,1);
  glayout->addWidget(name, 2,1);
  glayout->addWidget(pwd, 3,1);
  glayout->addWidget(dbserver, 1,2);
  glayout->addWidget(username, 2,2);
  glayout->addWidget(passwd, 3,2);
  f_vlayout->addLayout(glayout, 0);
 
  okb= new QPushButton(tr("Log in"),ff, "okb");
  okb->setDefault(true);
  connect(okb, SIGNAL(clicked()), SLOT(accept()));
  quitb= new QPushButton(tr("Cancel"),ff, "quitb");
  connect(quitb, SIGNAL(clicked()), SLOT(reject()));

  // buttons layout
  b_hlayout = new QHBoxLayout(20, "b_hlayout");
  b_hlayout->addWidget(okb, 10);
  b_hlayout->addWidget(quitb, 10);

  f_vlayout->addLayout(b_hlayout,0);

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
  dbi.host= dbserver->text().latin1();
  dbi.user= username->text().latin1();
  dbi.pass= passwd->text().latin1();
  return dbi;
}
