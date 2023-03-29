/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2020 met.no

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

#include "qtAnnotationDialog.h"
#include "diLabelPlotCommand.h"
#include "miSetupParser.h"
#include "util/string_util.h"

#include <puTools/miStringFunctions.h>

#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>

#define MILOGGER_CATEGORY "diana.AnnotationDialog"
#include <miLogger/miLogging.h>


AnnotationDialog::AnnotationDialog( QWidget* parent, Controller* llctrl )
: QDialog(parent)
{
  setWindowTitle(tr("Annotations"));

  m_ctrl=llctrl;

  parseSetup();

  annoBox = new QComboBox(this);
  annoBox->addItems(annoNames);
  connect( annoBox, SIGNAL(activated(int)), SLOT(annoBoxActivated(int)));

  defaultButton = new QPushButton(tr("Default"), this);
  connect( defaultButton, SIGNAL(clicked()), SLOT( defaultButtonClicked() ));

  textedit = new QTextEdit(this);
  textedit->setLineWrapMode(QTextEdit::NoWrap);
  textedit->setFont(QFont("Courier",12,QFont::Normal));
  textedit->setReadOnly(false);
  textedit->setMinimumHeight(150);

  annotationhide = new QPushButton(tr("Hide"), this);
  annotationapplyhide = new QPushButton(tr("Apply + Hide"), this);
  annotationapply = new QPushButton(tr("Apply"), this);
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

  const QString defaultName="default";
  const std::string anno_section="ANNOTATIONS";
  const std::string label_section="LABELS";
  std::vector<std::string> vstr;

  if (miutil::SetupParser::getSection(anno_section,vstr)){
    int nv=0, nvstr=vstr.size();
    std::string key;
    std::vector<std::string> values;
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
      if (name == defaultName)
        annoNames.prepend(name);
      else
        annoNames.append(name);
      qstr.clear();
    }

  } else if ( miutil::SetupParser::getSection(label_section,vstr) ) { //obsolete syntax, use annotations section
    METLIBS_LOG_INFO(LOGVAL(label_section));
    QString qstr;
    for(size_t i=0; i<vstr.size(); i++) {
      qstr += vstr[i].c_str();
      qstr += " \n";
    }
    setup_annoStrings[defaultName]=qstr;
    current_annoStrings[defaultName]=qstr;
    annoNames.append(defaultName);

  } else {

    METLIBS_LOG_WARN(anno_section << " section not found, using default");
    QString qstr = "LABEL data font=BITMAPFONT fontsize=8 \n";
    qstr += "LABEL text=\"$day $date $auto UTC\" tcolour=red bcolour=black fcolour=white:200 polystyle=both halign=left valign=top ";
    qstr += "font=BITMAPFONT fontsize=8 \n";
    qstr += "LABEL anno=<table,fcolour=white:150> halign=right valign=top polystyle=none margin=0 fontsize=10 \n";
    setup_annoStrings[defaultName]=qstr;
    current_annoStrings[defaultName]=qstr;
    annoNames.append(defaultName);
    return;
  }

}


/*******************************************************/
PlotCommand_cpv AnnotationDialog::getOKString()
{
  METLIBS_LOG_SCOPE();

  current_annoStrings[annoBox->currentText()] = textedit->toPlainText();
  std::string text = textedit->toPlainText().toStdString();
  METLIBS_LOG_DEBUG(LOGVAL(text));
  const std::vector<std::string> str = miutil::split(text, 0, "\n");
  PlotCommand_cpv cmd;
  cmd.reserve(str.size());
  for (std::string s : str) {
    miutil::trim(s);
    // we only want to create LABEL commands
    if (diutil::startswith(s, "LABEL "))
      cmd.push_back(LabelPlotCommand::fromString(s.substr(6))); // split after "LABEL "
  }
  return cmd;
}

std::vector<std::string> AnnotationDialog::writeLog()
{
  METLIBS_LOG_SCOPE();
  return std::vector<std::string>(1, "================");
}

void AnnotationDialog::readLog(const std::vector<std::string>&, const std::string&, const std::string&)
{
  METLIBS_LOG_SCOPE();
}

void AnnotationDialog::putOKString(const PlotCommand_cpv& vstr)
{
  METLIBS_LOG_SCOPE(vstr.size());
  if ( vstr.size() == 0 ) {
    annoBox->setCurrentIndex(0);
    annoBoxActivated(0);
    return;
  }

  std::string str;
  for (PlotCommand_cp cmd : vstr) {
    if (LabelPlotCommand_cp s = std::dynamic_pointer_cast<const LabelPlotCommand>(cmd))
      str += s->toString() + "\n";
  }
  textedit->setText(QString(str.c_str()));
}

//called when the dialog is closed by the window manager
void AnnotationDialog::closeEvent( QCloseEvent* e){
  emit AnnotationHide();
}
