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
#include <qtTextDialog.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qtextbrowser.h>
#include <qprinter.h>
#include <qfiledialog.h>
#include <qcheckbox.h>

#include <qtUtility.h>
#include <qmime.h> 
#include <iostream>

#include <kill.xpm>
#include <fileopen.xpm>

/*********************************************/
TextDialog::TextDialog( QWidget* parent, const InfoFile ifile)
  : QDialog(parent,"textdialog",false,
	    WStyle_Customize | WStyle_NormalBorder |
	    WStyle_Title | WStyle_SysMenu | WDestructiveClose)
{
  tb = new QTextBrowser( this ); 

  QPushButton* cb= new QPushButton( QPixmap(kill_xpm),
				    tr("Close window"), this );
  connect( cb, SIGNAL( clicked()), this, SLOT(finish()) );

  QPushButton* ob= new QPushButton( QPixmap(fileopen_xpm),
				    tr("Open file.."), this );
  connect( ob, SIGNAL( clicked()), this, SLOT(openwild()) );
  
  fixedb= new QCheckBox(tr("Use fixed font"), this);
  connect( fixedb, SIGNAL(clicked()), this, SLOT(fixedfont()));

  hlayout = new QHBoxLayout( 10 );
  hlayout->addStretch();
  hlayout->addWidget( cb );
  hlayout->addStretch();
  hlayout->addWidget( ob );
  hlayout->addStretch();
  hlayout->addWidget( fixedb );
  hlayout->addStretch();

  vlayout = new QVBoxLayout( this, 5, 5 );
  vlayout->addWidget( tb );
  vlayout->addLayout( hlayout );
  
  setSource(ifile);

  vlayout->activate(); 

  resize(600,400);
}


void TextDialog::setSource(const InfoFile ifile){

  const miString xml_type= "text/xml;charset=UTF-8";
  const miString txt_type= "text/plain";
  const miString htm_type= "text/html;charset=iso8859-1";
  
  infofile= ifile;
  if (infofile.name.exists()){
    setCaption(infofile.name.cStr());

    miString ext, file;
    path= "";
    // find filename-extension
    vector<miString> vs= infofile.filename.split(".");
    if (vs.size()>1){
      ext= vs[vs.size()-1];
      ext.trim();
    }
    if ((ext=="xml" && infofile.doctype=="auto") ||
	infofile.doctype=="xml"){
      tb->mimeSourceFactory()->
	setExtensionType(ext.cStr(),xml_type.cStr());
    } else if (((ext=="txt" || ext=="text") && infofile.doctype=="auto") ||
	       infofile.doctype=="text"){
      tb->mimeSourceFactory()->
	setExtensionType(ext.cStr(),txt_type.cStr());
    } else if (ext!="htm" && ext!="html" && infofile.doctype=="auto"){
      tb->mimeSourceFactory()->
	setExtensionType(ext.cStr(),txt_type.cStr());
    } else if (infofile.doctype=="html"){
      tb->mimeSourceFactory()->
	setExtensionType(ext.cStr(),htm_type.cStr());
    }
    // find path and filename
    if (infofile.filename.contains("/")){
      vs= infofile.filename.split("/");
      if (infofile.filename[0]=='/') path= "/";
      for (int i=0; i<vs.size()-1; i++)
	path+= (vs[i] + "/");
      file = vs[vs.size()-1];
    } else {
      path = "./";
      file= infofile.filename;
    }
    tb->mimeSourceFactory()->addFilePath(path.cStr());
    
    // set courier-font if fixed font selected
    if (infofile.fonttype=="fixed"){
      int psize= font().pointSize();
      tb->setFont(QFont("Courier", psize, QFont::Normal));
    } else {
      //tb->setFont(QFont("Helvetica", 10, QFont::Normal));
      tb->unsetFont();
    }

    tb->setSource(infofile.filename.cStr());

    tb->update();

    if (!fixedb->isChecked() && infofile.fonttype=="fixed"){
      fixedb->setChecked(true);
    } else if (fixedb->isChecked() && infofile.fonttype!="fixed"){
      fixedb->setChecked(false);
    }
  }
}

void TextDialog::finish()
{
  done(0);
}

void TextDialog::fixedfont()
{
  if (fixedb->isChecked()){
    //cerr << "Setter FIXED" << endl;
    int psize= font().pointSize();
    tb->setFont(QFont("Courier", psize, QFont::Normal));
  } else {
    tb->unsetFont();
    //tb->setFont(QFont("Helvetica", 10, QFont::Normal));
  }
  tb->update();

  InfoFile f= infofile;
  if (fixedb->isChecked()){
    f.fonttype="fixed";
  } else {
    f.fonttype="auto";
  }
  setSource(f);
}

void TextDialog::openwild()
{
  InfoFile f;
  QString filter= tr("Textfiles (*.txt *.text *.html);;All (*.*)");
  QString s(QFileDialog::getOpenFileName(path.cStr(),filter,
					 this, "openfile",
					 tr("Open file")));
  if ( s.isEmpty() )
    return;
  
  f.name= s.latin1();
  f.filename= s.latin1();
  f.doctype= "auto";
  f.fonttype= infofile.fonttype;
  setSource(f);
}

// void TextDialog::print()
// {
//   QPrinter printer;
//   if (printer.setup()){
//     QPainter p;
//     if( !p.begin( &printer ) )
//       return; // paint on printer
    
// //     tb->draw(&p);
// //     tb->drawFrame(&p);
// //     tb->drawContents(&p);
    
//     p.end();  // send job to printer
//   }
// }


