/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2017 met.no

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

#include "diana_config.h"

#include "qtTextDialog.h"
#include "qtUtility.h"

#include <puTools/miStringFunctions.h>

#include <QPushButton>
#include <QTextBrowser>
#include <QFileDialog>
#include <QCheckBox>
#include <QPixmap>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "kill.xpm"
#include "fileopen.xpm"


/*********************************************/
TextDialog::TextDialog(QWidget* parent, const InfoFile& ifile)
    : QDialog(parent)
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

  QHBoxLayout* hlayout = new QHBoxLayout();
  hlayout->addStretch();
  hlayout->addWidget( cb );
  hlayout->addStretch();
  hlayout->addWidget( ob );
  hlayout->addStretch();
  hlayout->addWidget( fixedb );
  hlayout->addStretch();

  QVBoxLayout* vlayout = new QVBoxLayout( this);
  vlayout->addWidget( tb );
  vlayout->addLayout( hlayout );

  setSource(ifile);

  vlayout->activate();

  resize(600,400);
}

void TextDialog::setSource(const InfoFile& ifile)
{
  const std::string xml_type= "text/xml;charset=UTF-8";
  const std::string txt_type= "text/plain";
  const std::string htm_type= "text/html;charset=iso8859-1";

  infofile= ifile;
  if (not infofile.name.empty()){
    setWindowTitle(QString::fromStdString(infofile.name));

    std::string ext, file;
    path= "";
    // find filename-extension
    std::vector<std::string> vs= miutil::split(infofile.filename, ".");
    if (vs.size()>1){
      ext= vs[vs.size()-1];
      miutil::trim(ext);
    }

    // find path and filename
    if (miutil::contains(infofile.filename, "/")){
      vs= miutil::split(infofile.filename, "/");
      if (infofile.filename[0]=='/') path= "/";
      for (unsigned int i=0; i<vs.size()-1; i++)
        path+= (vs[i] + "/");
      file = vs[vs.size()-1];
    } else {
      path = "./";
      file= infofile.filename;
    }


    // set courier-font if fixed font selected
    if (infofile.fonttype=="fixed"){
      int psize= font().pointSize();
      tb->setFont(QFont("Courier", psize, QFont::Normal));
    } else {
      tb->setFont(QFont("Helvetica", 10, QFont::Normal));
      //tb->unsetFont();
    }

    tb->setSource(QString(infofile.filename.c_str()));

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
    //    tb->unsetFont();
    tb->setFont(QFont("Helvetica", 10, QFont::Normal));
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
  QString s(QFileDialog::getOpenFileName(this,
      tr("Open file"),
      path.c_str(),
      filter));
  if ( s.isEmpty() )
    return;

  f.name= s.toStdString();
  f.filename= s.toStdString();
  f.doctype= "auto";
  f.fonttype= infofile.fonttype;
  setSource(f);
}
