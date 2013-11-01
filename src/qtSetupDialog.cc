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

#include "qtSetupDialog.h"
#include "diLocalSetupParser.h"
#include <puTools/miSetupParser.h>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <puTools/miStringFunctions.h>

using namespace std;

SetupDialog::SetupDialog(QWidget* parent)
  : QDialog(parent)
{
  setWindowTitle(tr("Setup"));
  setModal(true);

  QVBoxLayout* v= new QVBoxLayout(this);

  //setupfile
  QLabel* setupLabel = new QLabel( tr("Setupfile"), this);
  string setupfile = LocalSetupParser::getSetupFileName();
  setupLineEdit = new QLineEdit(setupfile.c_str(), this);
  QHBoxLayout* setuplayout= new QHBoxLayout();
  setuplayout->addWidget(setupLabel);
  setuplayout->addWidget(setupLineEdit);
  v->addLayout(setuplayout);

  //more options
  const map<std::string, std::string> key_value = miutil::SetupParser::getUserVariables();
  map<std::string, std::string>::const_iterator it = key_value.begin();
  for (int i=0 ; it != key_value.end(); ++it,++i  ) {
    values.push_back( new QLineEdit(it->second.c_str(),this));
    options.push_back(new QLabel(it->first.c_str(), this));
    QHBoxLayout* b= new QHBoxLayout();
    b->addWidget(options[i]);
    b->addWidget(values[i]);
    v->addLayout(b);
  }


  // last row of buttons
  QPushButton* cancel= new QPushButton( tr("&Cancel"), this );
  okButton= new QPushButton( tr("&OK"), this );
  connect( cancel, SIGNAL(clicked()), SLOT(reject()) );
  connect( okButton, SIGNAL(clicked()), SLOT(okClicked()) );

  QHBoxLayout* hl2= new QHBoxLayout();
  hl2->addWidget(cancel);
  hl2->addWidget(okButton);

  v->addLayout(hl2);
  v->activate();

}


void SetupDialog::okClicked( )
  {

  //update SetupParser
  LocalSetupParser::setSetupFileName(setupLineEdit->text().toStdString());
  for (size_t i = 0; i<values.size(); ++i ) {
    miutil::SetupParser::replaceUserVariables(options[i]->text().toStdString(),values[i]->text().toStdString());
  }
    accept();
}

