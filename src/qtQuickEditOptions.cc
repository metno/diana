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

#include "qtQuickEditOptions.h"

#include "qtUtility.h"

#include <puTools/miStringFunctions.h>

#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QInputDialog>
#include <QPixmap>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QShortcut>

#include "up12x12.xpm"
#include "down12x12.xpm"
#include "filenew.xpm"
#include "editcut.xpm"

#define MILOGGER_CATEGORY "diana.QuickEditOptions"
#include <miLogger/miLogging.h>


QuickEditOptions::QuickEditOptions(QWidget* parent, std::vector<quickMenuOption>& opt)
  : QDialog(parent)
  , options(opt)
  , keynum(-1)
{
  QLabel* mainlabel= new QLabel("<em><b>"+tr("Change Dynamic Options")+"</b></em>", this);
  mainlabel->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

  // up
  upButton = PixmapButton(QPixmap(up12x12_xpm), this, 14, 12);
  upButton->setEnabled( false );
  upButton->setShortcut(tr("Ctrl+Up"));
  connect(upButton, &QPushButton::clicked, this, &QuickEditOptions::upClicked);

  // down
  downButton = PixmapButton(QPixmap(down12x12_xpm), this, 14, 12);
  downButton->setEnabled( false );
  downButton->setShortcut(tr("Ctrl+Down"));
  connect(downButton, &QPushButton::clicked, this, &QuickEditOptions::downClicked);

  list= new QListWidget(this);
  connect(list, SIGNAL(itemClicked ( QListWidgetItem * )),
      SLOT(listClicked ( QListWidgetItem * )));
  list->setMinimumWidth(150);

  // Command buttons for key-list
  // new
  newButton = new QPushButton(QPixmap(filenew_xpm), tr("&New Key"), this );
  newButton->setEnabled( true );
  connect(newButton, &QPushButton::clicked, this, &QuickEditOptions::newClicked);

  // rename
  renameButton = new QPushButton( tr("&Change name.."), this );
  renameButton->setEnabled( false );
  connect(renameButton, &QPushButton::clicked, this, &QuickEditOptions::renameClicked);

  // erase
  eraseButton = new QPushButton(QPixmap(editcut_xpm), tr("Remove"), this );
  eraseButton->setEnabled( false );
  eraseButton->setShortcut(tr("Ctrl+X"));
  connect(eraseButton, &QPushButton::clicked, this, &QuickEditOptions::eraseClicked);

  QLabel* clabel= new QLabel(tr("Options (comma separated)"), this);
  // the choices
  choices= new QLineEdit(this);
  choices->setEnabled(false);
  connect(choices, &QLineEdit::textChanged, this, &QuickEditOptions::chChanged);

  // a horizontal frame line
  QFrame* line = new QFrame( this );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  // last row of buttons
  QPushButton* ok = new QPushButton(tr("&OK"), this);
  QPushButton* cancel = new QPushButton(tr("&Cancel"), this);
  connect(ok, &QPushButton::clicked, this, &QuickEditOptions::accept);
  connect(cancel, &QPushButton::clicked, this, &QuickEditOptions::reject);

  QVBoxLayout* vl1= new QVBoxLayout();
  vl1->addWidget(upButton);
  vl1->addWidget(downButton);

  QVBoxLayout* vl2= new QVBoxLayout();
  vl2->addWidget(newButton);
  vl2->addWidget(renameButton);
  vl2->addWidget(eraseButton);

  QHBoxLayout* hl1= new QHBoxLayout();
  hl1->addLayout(vl1);
  hl1->addWidget(list);
  hl1->addLayout(vl2);

  QHBoxLayout* hl4= new QHBoxLayout();
  hl4->addStretch();
  hl4->addWidget(ok);
  hl4->addStretch();
  hl4->addWidget(cancel);
  hl4->addStretch();
  //hl4->addWidget(help);

  // top layout
  QVBoxLayout* vlayout=new QVBoxLayout(this);

  vlayout->addWidget(mainlabel);
  vlayout->addLayout(hl1);
  vlayout->addWidget(clabel);
  vlayout->addWidget(choices);
  vlayout->addWidget(line);
  vlayout->addLayout(hl4);

  vlayout->activate();

  updateList();
}

std::vector<quickMenuOption> QuickEditOptions::getOptions()
{
  std::vector<quickMenuOption> opt;
  for (quickMenuOption& o : options) {
    if (!o.options.empty())
      opt.push_back(o);
  }
  return opt;
}

void QuickEditOptions::helpClicked()
{
  Q_EMIT help("quickeditoptions");
}


void QuickEditOptions::updateList()
{
  int origkeynum= keynum;

  list->clear();
  for (quickMenuOption& o : options) {
    list->addItem(QString::fromStdString(o.key));
  }
  keynum= origkeynum;

  const int n = options.size();
  if (n==0){
    keynum= -1;
    choices->setEnabled(false);
    upButton->setEnabled(false);
    downButton->setEnabled(false);
    newButton->setEnabled(true);
    renameButton->setEnabled(false);
    eraseButton->setEnabled(false);
    choices->setEnabled(false);

  } else {
    if (keynum<0)
      keynum= 0;
    else if (keynum>n-1){
      keynum= n-1;
    }
    list->item(keynum)->setSelected(true);
    listClicked(list->item(keynum));
  }
}

void QuickEditOptions::listClicked( QListWidgetItem * item) // new select in list
{
  int idx = list->row(item);
  if (idx < 0 || idx >= (int)options.size())
    return;
  keynum= idx;

  choices->setEnabled(false);
  upButton->setEnabled(false);
  downButton->setEnabled(false);
  newButton->setEnabled(true);
  renameButton->setEnabled(true);
  eraseButton->setEnabled(true);
  choices->setEnabled(true);

  if (keynum>0) upButton->setEnabled(true);
  if (keynum<int(options.size())-1) downButton->setEnabled(true);

  std::string s;
  int n= options[keynum].options.size();
  for (int i=0; i<n; i++){
    s+= options[keynum].options[i];
    if (i!=n-1) s+= ",";
  }
  choices->setText(QString::fromStdString(s));
}

void QuickEditOptions::chChanged(const QString& s)
{
  if (keynum<0 || keynum>=int(options.size()))
    return;

  options[keynum].options = miutil::split_protected(s.toStdString(), '"','"',",", false); //Do not skip blank enteries
}

void QuickEditOptions::upClicked()    // move item up
{
  quickMenuOption tmp= options[keynum-1];
  options[keynum-1]= options[keynum];
  options[keynum]= tmp;
  keynum--;
  updateList();
}

void QuickEditOptions::downClicked()  // move item down
{
  quickMenuOption tmp= options[keynum+1];
  options[keynum+1]= options[keynum];
  options[keynum]= tmp;
  keynum++;
  updateList();
}

void QuickEditOptions::newClicked()   // new key
{
  bool ok = false;
  QString text = QInputDialog::getText(this,
      tr("New key"),
      tr("Make new key with name:"),
      QLineEdit::Normal,
      QString::null, &ok);
  if (ok && !text.isEmpty()) {
    quickMenuOption tmp;
    tmp.key= text.toStdString();
    options.insert(options.end(), tmp);
    keynum= options.size()-1;
  }
  updateList();
}

void QuickEditOptions::renameClicked()// rename key
{
  if (keynum<0 || keynum>=int(options.size()))
    return;
  bool ok = false;
  QString text = QInputDialog::getText(this,
      tr("New name"),
      tr("Change key name:"),
      QLineEdit::Normal,
      options[keynum].key.c_str(),
      &ok);
  if (ok && !text.isEmpty()) {
    options[keynum].key= text.toStdString();
    updateList();
  }
}

void QuickEditOptions::eraseClicked() // erase key
{
  if (keynum<0 || keynum>=int(options.size()))
    return;
  options.erase(options.begin()+keynum);
  updateList();
}
