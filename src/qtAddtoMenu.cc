/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "qtAddtoMenu.h"
#include "qtQuickMenu.h"

#include <QFrame>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QInputDialog>
#include <QPixmap>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <puTools/miStringFunctions.h>
#include "filenew.xpm"


AddtoMenu::AddtoMenu(QWidget* parent, QuickMenu* qm)
: QDialog(parent), quick(qm)
{
  QHBoxLayout* b= new QHBoxLayout(this);
  setModal(true);

  QFrame* frame= new QFrame(this);
  frame->setFrameStyle(QFrame::Panel | QFrame::Sunken);

  QString t= "<em><b>"+tr("Add current plot to a private quickmenu")+"</b></em>";
  QLabel* label= new QLabel(t, frame);
  label->setFrameStyle(QFrame::Panel | QFrame::Raised);

  QString plotname;
  if( quick ) {
    plotname = quick->getCurrentName().c_str();
  }
  QLabel* plotnameLabel = new QLabel(plotname,this);

  list= new QListWidget(frame);
  connect(list, SIGNAL( itemClicked( QListWidgetItem * )),
      SLOT(menuSelected( QListWidgetItem * )));
  connect(list, SIGNAL(itemDoubleClicked( QListWidgetItem *  )),
      SLOT(okClicked()));
  list->setMinimumWidth(100);

  newButton = new QPushButton(QPixmap(filenew_xpm), tr("&Make new"), frame );
  newButton->setEnabled( true );
  connect( newButton, SIGNAL(clicked()), SLOT(newClicked()));

  QHBoxLayout* hl= new QHBoxLayout();
  hl->addWidget(list);
  hl->addWidget(newButton);

  // a horizontal frame line
  QFrame* line = new QFrame( frame );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  // last row of buttons
  okButton= new QPushButton( tr("&OK"), frame );
  okButton->setEnabled(false);
  QPushButton* cancel= new QPushButton( tr("&Cancel"), frame );
  connect( okButton, SIGNAL(clicked()), SLOT(okClicked()) );
  connect( cancel, SIGNAL(clicked()), SLOT(reject()) );

  QHBoxLayout* hl2= new QHBoxLayout();
  hl2->addWidget(okButton);
  hl2->addWidget(cancel);

  QVBoxLayout* vl= new QVBoxLayout(frame);
  vl->addWidget(label);
  vl->addWidget(plotnameLabel);
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
  if (!quick)
    return;
  const std::vector<std::string> vs = quick->getUserMenus();
  for (const std::string& qn : vs)
    list->addItem(QString::fromStdString(qn));
  if (!vs.empty()) {
    list->setCurrentRow(0);
    list->item(0)->setSelected(true);
    okButton->setEnabled(true);
  }
}

void AddtoMenu::okClicked( )
{
  std::string name = list->currentItem()->text().toStdString();
  if (quick->addToMenu(name))
    accept();
}

void AddtoMenu::newClicked()
{
  bool ok = false;
  QString text = QInputDialog::getText(this, tr("New Menu"), tr("Make new menu with name:"), QLineEdit::Normal, QString::null, &ok);
  if (ok && !text.isEmpty()) {
    quick->addMenu(text.toStdString());
    fillMenu();
  }
}

void AddtoMenu::menuSelected(QListWidgetItem*)
{
  okButton->setEnabled(true);
}
