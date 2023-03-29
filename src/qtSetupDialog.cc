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

#include "diana_config.h"

#include "qtSetupDialog.h"
#include "diLocalSetupParser.h"
#include "miSetupParser.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QGridLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

#include <puTools/miStringFunctions.h>


SetupDialog::SetupDialog(QWidget* parent)
  : QDialog(parent)
{
  setWindowTitle(tr("Setup"));
  setModal(true);

  QVBoxLayout* v= new QVBoxLayout(this);

  //setupfile
  QLabel* setupLabel = new QLabel( tr("Setupfile"), this);
  const std::string& setupfile = LocalSetupParser::getSetupFileName();
  setupLineEdit = new QLineEdit(QString::fromStdString(setupfile), this);
  QToolButton* setupFileSelect = new QToolButton(this);
  setupFileSelect->setText("...");
  connect(setupFileSelect, &QToolButton::clicked, this, &SetupDialog::onFilechooserClicked);
  QHBoxLayout* setuplayout= new QHBoxLayout();
  setuplayout->addWidget(setupLabel);
  setuplayout->addWidget(setupLineEdit);
  setuplayout->addWidget(setupFileSelect);
  v->addLayout(setuplayout);

  QScrollArea* scroll = new QScrollArea(this);
  v->addWidget(scroll);

  QWidget* config = new QWidget(scroll);
  QGridLayout* grid = new QGridLayout;
  grid->setColumnStretch(1, 1);
  config->setLayout(grid);
  for (auto& kv : miutil::SetupParser::getUserVariables()) {
    const int row = options.size();
    options.push_back(new QLabel(QString::fromStdString(kv.first), config));
    values.push_back(new QLineEdit(QString::fromStdString(kv.second), config));
    grid->addWidget(options.back(), row, 0);
    grid->addWidget(values.back(), row, 1);
  }
  scroll->setWidget(config);
  scroll->setWidgetResizable(true);

  // last row of buttons
  QPushButton* cancel= new QPushButton( tr("&Cancel"), this );
  QPushButton* okButton = new QPushButton( tr("&OK"), this );
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

void SetupDialog::onFilechooserClicked()
{
  const QString fn = QFileDialog::getOpenFileName(this, tr("Choose setup file"), setupLineEdit->text());
  if (!fn.isEmpty())
    setupLineEdit->setText(fn);
}
