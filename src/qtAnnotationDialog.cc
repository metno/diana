/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#include "qtAnnotationDialog.h"
#include <puTools/miStringFunctions.h>
#include <puTools/miSetupParser.h>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include "qtUtility.h"
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>

#define MILOGGER_CATEGORY "diana.AnnotationDialog"
#include <miLogger/miLogging.h>

using namespace std;

AnnotationDialog::AnnotationDialog( QWidget* parent, Controller* llctrl )
: QDialog(parent)
{
  setWindowTitle(tr("Annotations"));

  m_ctrl=llctrl;

  parseSetup();

  annoBox = new QComboBox(this);
  map<QString,QString>::iterator ip = current_annoStrings.begin();
  for ( ; ip!= current_annoStrings.end(); ++ip) {
    annoBox->addItem(ip->first);
  }
  connect( annoBox, SIGNAL(activated(int)), SLOT(annoBoxActivated(int)));

  defaultButton = NormalPushButton( tr("Default"), this);
  connect( defaultButton, SIGNAL(clicked()), SLOT( defaultButtonClicked() ));

  textedit = new QTextEdit(this);
  textedit->setLineWrapMode(QTextEdit::NoWrap);
  textedit->setFont(QFont("Courier",12,QFont::Normal));
  textedit->setReadOnly(false);
  textedit->setMinimumHeight(150);

  annotationhide = NormalPushButton( tr("Hide"), this);
  annotationapplyhide = NormalPushButton( tr("Apply + Hide"), this );
  annotationapply = NormalPushButton( tr("Apply"), this);
  annotationapply->setDefault( true );

  connect( annotationhide, SIGNAL(clicked()), SIGNAL( AnnotationHide() ));
  connect( annotationapply, SIGNAL(clicked()), SIGNAL( AnnotationApply() ));
  connect( annotationapplyhide, SIGNAL(clicked()), SLOT(applyhideClicked()));

  QHBoxLayout* toplayout = new QHBoxLayout();
  toplayout->addWidget( annoBox );
  toplayout->addWidget( defaultButton );

  QHBoxLayout* applylayout = new QHBoxLayout();
  applylayout->addWidget( annotationhide );
  applylayout->addWidget(annotationapplyhide );
  applylayout->addWidget( annotationapply );

  QVBoxLayout* vlayout= new QVBoxLayout( this);
  vlayout->addLayout( toplayout );
  vlayout->addWidget( textedit );
  vlayout->addLayout( applylayout );

  annoBox->setCurrentIndex(0);
  annoBoxActivated(0);

  this->hide();

}

void AnnotationDialog::annoBoxActivated(int i)
{
  METLIBS_LOG_SCOPE();
  textedit->setText(current_annoStrings[annoBox->currentText()]);
}

void AnnotationDialog::defaultButtonClicked()
{
  METLIBS_LOG_SCOPE();
  textedit->setText(setup_annoStrings[annoBox->currentText()]);
}

void AnnotationDialog::applyhideClicked(){
  METLIBS_LOG_SCOPE();
  emit AnnotationHide();
  emit AnnotationApply();
}

void AnnotationDialog::parseSetup(){
  METLIBS_LOG_SCOPE();

  std::string anno_section="ANNOTATIONS";
  std::string label_section="LABELS";
  vector<std::string> vstr;

  if (miutil::SetupParser::getSection(anno_section,vstr)){
    int nv=0, nvstr=vstr.size();
    std::string key,error;
    vector<std::string> values, vsub;
    bool ok= true;

    while (ok && nv<nvstr) {

      miutil::SetupParser::splitKeyValue(vstr[nv],key,values);
      QString name;
      QString qstr;

      if (key=="annotation" && values.size()==1) {
        name= values[0].c_str();
        nv++;
      } else {
        continue;
      }

      while (nv<nvstr) {
        if (vstr[nv]=="end.annotation") {
          nv++;
          break;
        } else {
          qstr += vstr[nv].c_str();
          qstr += "\n";
          nv++;
        }

      }
      setup_annoStrings[name]=qstr;
      current_annoStrings[name]=qstr;
      if ( name == "default")
        defaultAnno = qstr;
      qstr.clear();
    }

  } else if ( miutil::SetupParser::getSection(label_section,vstr) ) { //obsolete syntax, use annotations section
    METLIBS_LOG_INFO(LOGVAL(label_section));
    QString name = "default";
    QString qstr;
    for(size_t i=0; i<vstr.size(); i++) {
      qstr += vstr[i].c_str();
      qstr += " \n";
    }
    setup_annoStrings[name]=qstr;
    current_annoStrings[name]=qstr;
    defaultAnno = qstr;

  } else {

    METLIBS_LOG_WARN(anno_section << " section not found, using default");
    QString name = "Labels";
    QString qstr = "LABEL data font=BITMAPFONT \n";
    qstr += "LABEL text=\"$day $date $auto UTC\" tcolour=red bcolour=black fcolour=white:200 polystyle=both halign=left valign=center ";
    qstr += "font=BITMAPFONT fontsize=12 \n";
    qstr += "LABEL anno=<table,fcolour=white:150> halign=right valign=top fcolour=white:0 margin=0 \n";
    setup_annoStrings[name]=qstr;
    current_annoStrings[name]=qstr;
    defaultAnno = qstr;
    return;
  }

}


/*******************************************************/
vector<string> AnnotationDialog::getOKString()
{
  METLIBS_LOG_SCOPE();

  current_annoStrings[annoBox->currentText()] = textedit->toPlainText();
  std::string text = textedit->toPlainText().toStdString();
  METLIBS_LOG_DEBUG(LOGVAL(text));
  vector<string> str = miutil::split(text, 0, "\n");
  for ( size_t i=0; i<str.size(); i++ ) {
    miutil::trim( str[i] );
  }
  return str;
}


vector<string> AnnotationDialog::writeLog()
{
  METLIBS_LOG_SCOPE();
  return vector<string>(1, "================");
}


void AnnotationDialog::readLog(const vector<string>&, const string&, const string&)
{
  METLIBS_LOG_SCOPE();
}

void AnnotationDialog::putOKString(const vector<string>& vstr)
{
  METLIBS_LOG_SCOPE(vstr.size());
  if ( vstr.size() == 0 ) {
    textedit->setText(defaultAnno);
    return;
  }

  std::string str;
  for (size_t i=0; i<vstr.size(); i++){
    str += vstr[i];
    str+= std::string("\n");
  }
  textedit->setText(QString(str.c_str()));
}

//called when the dialog is closed by the window manager
void AnnotationDialog::closeEvent( QCloseEvent* e){
  emit AnnotationHide();
}
