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

#include <qtQuickEditOptions.h>

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

#include <qtUtility.h>
#include <up12x12.xpm>
#include <down12x12.xpm>
#include <filenew.xpm>
#include <editcopy.xpm>
#include <editcut.xpm>

QuickEditOptions::QuickEditOptions(QWidget* parent,
				   vector<quickMenuOption>& opt)
  : QDialog(parent, "QUICKEDITOPTIONS", TRUE),
    options(opt), keynum(-1)
{
  QFont m_font= QFont( "Helvetica", 12, 75 );
 
  QLabel* mainlabel= new QLabel("<em><b>"+tr("Change Dynamic Options")+"</b></em>", this);
  mainlabel->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

  // up
  QPixmap upPicture = QPixmap(up12x12_xpm);
  upButton = PixmapButton( upPicture, this, 14, 12 );
  upButton->setEnabled( false );
  upButton->setAccel(Qt::CTRL+Qt::Key_Up);
  connect( upButton, SIGNAL(clicked()), SLOT(upClicked()));
  // down
  QPixmap downPicture = QPixmap(down12x12_xpm);
  downButton = PixmapButton( downPicture, this, 14, 12 );
  downButton->setEnabled( false );
  downButton->setAccel(Qt::CTRL+Qt::Key_Down);
  connect( downButton, SIGNAL(clicked()), SLOT(downClicked()));

  list= new QListWidget(this);
  connect(list, SIGNAL(itemClicked ( QListWidgetItem * )),
	  SLOT(listClicked ( QListWidgetItem * )));
  list->setMinimumWidth(150);

  // Command buttons for key-list
  // new
  newButton = new QPushButton(QPixmap(filenew_xpm), tr("&New Key"), this );
  newButton->setEnabled( true );
  connect( newButton, SIGNAL(clicked()), SLOT(newClicked()));
  
  // rename
  renameButton = new QPushButton( tr("&Change name.."), this );
  renameButton->setEnabled( false );
  connect( renameButton, SIGNAL(clicked()), SLOT(renameClicked()));
  
  // erase
  eraseButton = new QPushButton(QPixmap(editcut_xpm), tr("Remove"), this );
  eraseButton->setEnabled( false );
  eraseButton->setAccel(Qt::CTRL+Qt::Key_X);
  connect( eraseButton, SIGNAL(clicked()), SLOT(eraseClicked()));
  
  QLabel* clabel= new QLabel(tr("Options (comma separated)"), this);
  // the choices
  choices= new QLineEdit(this);
  choices->setEnabled(false);
  connect(choices, SIGNAL(textChanged(const QString&)),
	  this, SLOT(chChanged(const QString&)));

  // a horizontal frame line
  QFrame* line = new QFrame( this );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );


  // last row of buttons
  QPushButton* ok= new QPushButton( tr("&OK"), this);
  QPushButton* cancel= new QPushButton( tr("&Cancel"), this );
  //QPushButton* help=PushButton( "&Hjelp", this, m_font );
  connect( ok, SIGNAL(clicked()), SLOT(accept()) );
  connect( cancel, SIGNAL(clicked()), SLOT(reject()) );
  //connect( help, SIGNAL(clicked()), SLOT(helpClicked()) );

  QVBoxLayout* vl1= new QVBoxLayout(5);
  vl1->addWidget(upButton);
  vl1->addWidget(downButton);

  QVBoxLayout* vl2= new QVBoxLayout(5);
  vl2->addWidget(newButton);
  vl2->addWidget(renameButton);
  vl2->addWidget(eraseButton);

  QHBoxLayout* hl1= new QHBoxLayout(5);
  hl1->addLayout(vl1);
  hl1->addWidget(list);
  hl1->addLayout(vl2);
  
  QHBoxLayout* hl4= new QHBoxLayout(5);
  hl4->addStretch();
  hl4->addWidget(ok);
  hl4->addStretch();
  hl4->addWidget(cancel);
  hl4->addStretch();
  //hl4->addWidget(help);

  // top layout
  QVBoxLayout* vlayout=new QVBoxLayout(this,5,5);
  
  vlayout->addWidget(mainlabel);
  vlayout->addLayout(hl1);
  vlayout->addWidget(clabel);
  vlayout->addWidget(choices);
  vlayout->addWidget(line);
  vlayout->addLayout(hl4);
  
  vlayout->activate();

  updateList();
}

vector<quickMenuOption> QuickEditOptions::getOptions(){

  vector<quickMenuOption> opt;
  int n=options.size();
  for(int i=0;i<n;i++){
    if( options[i].options.size() > 0 )
      opt.push_back(options[i]);
  }

  return opt;

}

void QuickEditOptions::helpClicked(){
  emit help("quickeditoptions");
}


void QuickEditOptions::updateList()
{

  int origkeynum= keynum;

  list->clear();
  int n= options.size();
  for (int i=0; i<n; i++){
    list->addItem(options[i].key.cStr());
  }
  keynum= origkeynum;

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
  }
}

void QuickEditOptions::listClicked( QListWidgetItem * item) // new select in list
{

  int idx = list->row(item);
  if (idx < 0 || idx >= options.size()) return;
  keynum= idx;

  choices->setEnabled(false);
  upButton->setEnabled(false);
  downButton->setEnabled(false);
  newButton->setEnabled(true);
  renameButton->setEnabled(true);
  eraseButton->setEnabled(true);
  choices->setEnabled(true);

  if (keynum>0) upButton->setEnabled(true);
  if (keynum<options.size()-1) downButton->setEnabled(true);

  miString s;
  int n= options[keynum].options.size();
  for (int i=0; i<n; i++){
    s+= options[keynum].options[i];
    if (i!=n-1) s+= ",";
  }
  choices->setText(s.cStr());
}

void QuickEditOptions::chChanged(const QString& s)
{
  if (keynum<0 || keynum>=options.size()) return;
  miString ms= s.toStdString();
  vector<miString> vs= ms.split(",");
  
  options[keynum].options= vs;
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
  bool ok = FALSE;
  QString text = QInputDialog::getText(tr("New key"),
				       tr("Make new key with name:"),
				       QLineEdit::Normal,
				       QString::null, &ok, this );
  if ( ok && !text.isEmpty() ){
    quickMenuOption tmp;
    tmp.key= text.toStdString();
    options.insert(options.end(), tmp);
    keynum= options.size()-1;
  }
  updateList();
}

void QuickEditOptions::renameClicked()// rename key
{
  if (keynum<0 || keynum>=options.size()) return;
  bool ok = FALSE;
  QString text = QInputDialog::getText(tr("New name"),
				       tr("Change key name:"),
				       QLineEdit::Normal,
				       options[keynum].key.cStr(),
				       &ok, this );
  if ( ok && !text.isEmpty() ){
    options[keynum].key= text.toStdString();
    updateList();
  }
}

void QuickEditOptions::eraseClicked() // erase key
{
  if (keynum<0 || keynum>=options.size()) return;
  options.erase(options.begin()+keynum);
  updateList();
}
