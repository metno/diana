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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>
#include <puCtools/puCglob.h>
#include <puCtools/glob_cache.h>

#include "qtQuickMenu.h"
#include "qtQuickAdmin.h"

#include <QPushButton>
#include <QListWidget>
#include <QTextEdit>
#include <QFrame>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QTimerEvent>
#include <QGridLayout>
#include <QVBoxLayout>
#include <qmessagebox.h>
#include <qtooltip.h>
#include <qregexp.h>

#include <puTools/miString.h>
#include <iostream>
#include "qtUtility.h"
#include "diLocalSetupParser.h"
#include "diCommonTypes.h"

#include <QString>
#include <QStringList>

using namespace std;

const miutil::miString vprefix= "@";

QuickMenu::QuickMenu( QWidget *parent, Controller* c,
    Qt::WFlags f)
: QDialog( parent, f), contr(c), comset(false),
activemenu(0), timerinterval(10), timeron(false),
browsing(false),
prev_plotindex(0), prev_listindex(0),
firstcustom(-1), lastcustom(-1), instaticmenu(false)
{

  setWindowTitle(tr("Quickmenu"));

  // Create top-level layout manager
  QBoxLayout* tlayout = new QHBoxLayout( this);

  // make a nice frame
  QFrame* frame= new QFrame(this);
  frame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

  //  frame->setPalette( QPalette( QColor(255, 80, 80) ) );
  tlayout->addWidget(frame,1);

  // Create layout manager
  QBoxLayout* vlayout = new QVBoxLayout(frame);

  // create quickmenu optionmenu
  QBoxLayout *l2= new QHBoxLayout();
  QLabel* menulistlabel= TitleLabel(tr("Menus"),frame);

  menulist= new QComboBox(frame);
  connect(menulist, SIGNAL(activated(int)),SLOT(menulistActivate(int)));
  QPushButton* adminbut= new QPushButton(tr("&Edit menus.."), frame );
  adminbut->setToolTip(tr("Menu editor: Copy, change name and sortorder etc. on your own menus") );
  connect(adminbut, SIGNAL(clicked()),SLOT(adminButton()));

  updatebut= new QPushButton(tr("&Update.."), frame );
  updatebut->setToolTip(tr("Update command with current plot") );
  connect(updatebut, SIGNAL(clicked()),SLOT(updateButton()));

  resetbut= new QPushButton(tr("&Reset.."), frame );
  resetbut->setToolTip(tr("Reset command to original copy") );
  connect(resetbut, SIGNAL(clicked()),SLOT(resetButton()));

  l2->addWidget(menulistlabel);
  l2->addWidget(menulist);
  l2->addStretch();
  l2->addWidget(updatebut);
  l2->addStretch();
  l2->addWidget(resetbut);
  l2->addStretch();
  l2->addWidget(adminbut);

  // create menuitem list
  list= new QListWidget(frame);
  connect(list, SIGNAL(itemClicked( QListWidgetItem * )),
      SLOT(listClicked( QListWidgetItem * )));
  connect(list, SIGNAL(itemDoubleClicked( QListWidgetItem * )),
      SLOT(listDoubleClicked( QListWidgetItem * )));
  vlayout->addLayout(l2);
  vlayout->addWidget(list, 10);

  // Create variables/options layout manager
  int quarter= maxoptions/4;
  int breakpoint = quarter;
  int row = 0;
  int col = 0;
  QGridLayout* varlayout = new QGridLayout();
  for (int i=0; i<maxoptions; i++){
    optionlabel[i]= new QLabel("",frame);
    optionmenu[i]=  new QComboBox(frame);
    optionmenu[i]->setSizeAdjustPolicy ( QComboBox::AdjustToContents);
    if( i >= breakpoint ) {
      row ++;
      col = 0;
      breakpoint += quarter;
    }
    varlayout->addWidget(optionlabel[i],row,col,Qt::AlignRight);
    varlayout->addWidget(optionmenu[i],row,col+1);
    col += 2;
  }
  vlayout->addLayout(varlayout);

  QFrame *line;
  // Create a horizontal frame line
  line = new QFrame( frame );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  vlayout->addWidget( line );

  // create commands-area
  comedit= new QTextEdit(frame);
  comedit->setLineWrapMode(QTextEdit::NoWrap);
  comedit->setFont(QFont("Courier",12,QFont::Normal));
  comedit->setReadOnly(false);
  comedit->setMinimumHeight(150);
  connect(comedit, SIGNAL(textChanged()), SLOT(comChanged()));
  comlabel= new QLabel(tr("Command field"),frame);
  comlabel->setMinimumSize(comlabel->sizeHint());
  comlabel->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

  comlabel->hide();
  comedit->hide();

  QHBoxLayout* l3= new QHBoxLayout();
  l3->addWidget(comlabel);
  l3->addStretch();

  vlayout->addLayout(l3,0);
  vlayout->addWidget(comedit, 10);

  // buttons and stuff at the bottom
  QBoxLayout *l= new QHBoxLayout();

  QPushButton* quickhide = new QPushButton( tr("&Hide"), frame );
  connect( quickhide, SIGNAL(clicked()),SIGNAL( QuickHide()) );
  l->addWidget(quickhide);

  QPushButton* combut= new QPushButton(tr("&Command"), frame );
  combut->setCheckable(true);
  connect(combut, SIGNAL(toggled(bool)),SLOT(comButton(bool)));
  l->addWidget(combut);

  QPushButton* demobut= new QPushButton(tr("&Demo"), frame );
  demobut->setCheckable(true);
  connect(demobut, SIGNAL(toggled(bool)),SLOT(demoButton(bool)));
  l->addWidget(demobut);

  QSpinBox* interval= new QSpinBox(frame);
  interval->setMinimum(2);
  interval->setMaximum(360);
  interval->setSingleStep(2);
  interval->setValue(timerinterval);
  interval->setSuffix(" sec");
  connect(interval, SIGNAL(valueChanged(int)),SLOT(intervalChanged(int)));
  l->addWidget(interval);

  QPushButton* qhelp= new QPushButton(tr("&Help"), frame );
  connect( qhelp, SIGNAL(clicked()), SLOT( helpClicked() ));
  l->addWidget(qhelp);

  QPushButton* plothidebut= new QPushButton(tr("Apply+Hide"), frame );
  connect(plothidebut, SIGNAL(clicked()),SLOT(plotButton()));
  connect(plothidebut, SIGNAL(clicked()),SIGNAL( QuickHide()) );
  l->addWidget(plothidebut);

  QPushButton* plotbut= new QPushButton(tr("&Apply"), frame );
  plotbut->setDefault( true );
  connect(plotbut, SIGNAL(clicked()),SLOT(plotButton()));
  l->addWidget(plotbut);

  vlayout->addLayout(l);

  fillPrivateMenus();
  fillStaticMenus();
}

// set active menu
void QuickMenu::setCurrentMenu(int i)
{
  menulist->setCurrentIndex(i);
  menulistActivate(i);
}


void QuickMenu::start()
{
  fillMenuList();
  setCurrentMenu(0);
}


void QuickMenu::startBrowsing()
{
  browsing= true;
}


// quick-quick menu methods
vector<miutil::miString> QuickMenu::getCustomMenus()
{
  vector<miutil::miString> vs;
  if (firstcustom >= 0 && lastcustom>=0){
    for (int i=firstcustom; i<=lastcustom; i++)
      vs.push_back(qm[i].name);
  }
  return vs;
}

bool QuickMenu::addMenu(const miutil::miString& name)
{
  if (firstcustom<0){
    firstcustom= 1;
    lastcustom=0;
  }
  lastcustom++;
  quickMenu qtmp;
  qtmp.name= name;
  qtmp.name.trim();
  qtmp.name.replace(","," ");
  qtmp.filename= qtmp.name + ".quick";
  qtmp.filename.replace(" ","_");
  qtmp.plotindex= 0;

  qm.insert(qm.begin()+lastcustom, qtmp);

  // update index to previous list
  if (prev_listindex >= lastcustom)
    prev_listindex++;

  fillMenuList();

  // save quickmenu to file
  if (lastcustom >-1 && lastcustom<int(qm.size())){
    writeQuickMenu(qm[lastcustom]);
  }

  return true;
}

bool QuickMenu::addToMenu(const int idx)
{
  if (firstcustom >=0 && lastcustom>=0 &&
      firstcustom+idx < int(qm.size()) &&
      prev_listindex>=0 && prev_plotindex>=0 &&
      prev_listindex<int(qm.size())){
    int qidx= idx+firstcustom;
    qm[qidx].menuitems.
    push_back(qm[prev_listindex].menuitems[prev_plotindex]);

    fillMenuList();

    // save quickmenu to file
    writeQuickMenu(qm[qidx]);

  } else
    return false;
  return true;
}

miutil::miString QuickMenu::getCurrentName()
{
  if (prev_listindex>=0 && prev_plotindex>=0 &&
      prev_listindex<int(qm.size())){
    return qm[prev_listindex].menuitems[prev_plotindex].name;
  }
  return miutil::miString();
}

// Push a new command on the history-stack
void QuickMenu::pushPlot(const miutil::miString& name,
    vector<miutil::miString> pstr, int index)
{

  //replace reftime by refhour, refoffset must be set manually
  for (size_t i=0; i<pstr.size(); i++){
    if ( pstr[i].find("reftime=") != std::string::npos ) {
      miutil::miString hourstr = pstr[i].substr(pstr[i].find("reftime=")+19,2);
      miutil::miString str1 = pstr[i].substr(pstr[i].find("reftime="),27);
      miutil::miString str2 = "refhour=" + hourstr;
      pstr[i].replace(str1,str2);
    }
  }

  if (qm.size()==0) return;
  bool goon= true;
  int m= qm[index].menuitems.size();
  if (m > 0){ // check for duplicate
    if (qm[index].menuitems[0].command.size()==pstr.size()){
      goon= false;
      for (unsigned int i=0; i<pstr.size(); i++){
        if (pstr[i] != qm[index].menuitems[0].command[i]){
          goon= true;
          break;
        }
      }
    }
  }
  if (goon) {
    // keep stack within bounds
    int ms= qm[index].menuitems.size();
    if (ms >= maxplotsinstack)
      qm[index].menuitems.pop_back();
    // update pointer
    qm[index].plotindex=0;
    // push plot on stack
    miutil::miString plotname= name;
    plotname.trim();
    quickMenuItem dummy;
    qm[index].menuitems.push_front(dummy);
    qm[index].menuitems[0].command= pstr;
    qm[index].menuitems[0].name= plotname;
    // switch to history-menu
    if (activemenu==index) setCurrentMenu(index);

    prev_plotindex= 0;
    prev_listindex= index;
  }
}

//bool QuickMenu::replacereferencetime( vector<miutil::miString>& pstr ){
//
//  for (size_t i=0; i<pstr.size(); i++){
//  std::string str = pstr[i];
//  if ( str.find_first_of("referencetime=") != str.npos ) {
//    std::string timestr = str.substr(str.find_first_of("referencetime=")+14,17);
//    DEBUG_ <<timestr;
//  }
////  vector<miutil::miString> tokens = pstr[i].split(('"', '"');
////  for ( size_t j=0; j<tokens.size(); j++ ) {
////    vector<miutil::miString> stokens = tokens[j].split("=");
////    if ( stokens.size() && stokens[0]="referencetime" ) {
////
////    }
////  }
//
//}

// called from quick-quick menu (Browsing)
bool QuickMenu::prevQPlot(){
  int menu= activemenu;
  if (qm.size()==0 || int(qm.size())<menu+1)
    return false;
  int n= qm[menu].menuitems.size();
  if (n==0 || qm[menu].plotindex >= n-1 || qm[menu].plotindex<0)
    return false;

  qm[menu].plotindex++;
  list->setCurrentRow(qm[menu].plotindex);
  listClicked(list->currentItem());
  return true;
}

// Go to previous History-plot
bool QuickMenu::prevHPlot(int index){
  int menu= index;
  if (qm.size()==0 || int(qm.size())<menu+1)
    return false;
  int n= qm[menu].menuitems.size();

  // if in History-menu or last plot from History:
  // Change plotindex and plot
  if (activemenu==menu || prev_listindex==menu){
    if (n==0 || qm[menu].plotindex >= n-1 || qm[menu].plotindex<0)
      return false;
    qm[menu].plotindex++;
    if (activemenu==menu) list->setCurrentRow(qm[menu].plotindex);
  } else {
    // Jumping back to History (plotindex intact)
    if (n==0 || qm[menu].plotindex > n-1 || qm[menu].plotindex<0)
      return false;
  }
  // apply plot
  emit Apply(qm[menu].menuitems[qm[menu].plotindex].command,true);
  prev_plotindex= qm[menu].plotindex;
  prev_listindex= menu;

  return true;
}

// called from quick-quick menu (Browsing)
bool QuickMenu::nextQPlot(){
  int menu= activemenu;
  if (qm.size()==0 || int(qm.size())<menu+1)
    return false;
  int n= qm[menu].menuitems.size();
  if (n==0 || qm[menu].plotindex > n-1 || qm[menu].plotindex<=0)
    return false;

  qm[menu].plotindex--;
  list->setCurrentRow(qm[menu].plotindex);
  listClicked(list->currentItem());
  return true;
}

// Go to next History-plot
bool QuickMenu::nextHPlot(int index){
  int menu= index;
  if (qm.size()==0 || int(qm.size())<menu+1)
    return false;
  int n= qm[menu].menuitems.size();

  // if in History-menu or last plot from History:
  // Change plotindex and plot
  if (activemenu==menu || prev_listindex==menu){
    if (n==0 || qm[menu].plotindex > n-1 || qm[menu].plotindex<=0)
      return false;
    qm[menu].plotindex--;
    if (activemenu==menu) list->setCurrentRow(qm[menu].plotindex);
  } else {
    // Jumping back to History (plotindex intact)
    if (n==0 || qm[menu].plotindex > n-1 || qm[menu].plotindex<0)
      return false;
  }
  // apply plot
  emit Apply(qm[menu].menuitems[qm[menu].plotindex].command,true);
  prev_plotindex= qm[menu].plotindex;
  prev_listindex= menu;

  return true;
}


// For browsing: go to previous quick-menu
bool QuickMenu::prevList(){
  if (qm.size()==0 || activemenu<1 || activemenu>int(qm.size())-1)
    return false;
  setCurrentMenu(activemenu-1);
  return true;
}


// For browsing: go to next quick-menu
bool QuickMenu::nextList(){
  if (qm.size()==0 || activemenu<0 || activemenu>=int(qm.size())-1)
    return false;
  setCurrentMenu(activemenu+1);
  return true;
}


// for Browsing: get menu-details
void QuickMenu::getDetails(int& plotidx,
    miutil::miString& listname,
    miutil::miString& plotname)
{
  plotidx= 0;
  if (qm.size()>0 && activemenu < int(qm.size())){
    plotidx= qm[activemenu].plotindex;
    listname= qm[activemenu].name;
    plotname= (plotidx < int(qm[activemenu].menuitems.size()) ?
        qm[activemenu].menuitems[plotidx].name:"");
  }
}

bool QuickMenu::applyItem(const miutil::miString& mlist, const miutil::miString& item)
{
  //find list index
  int n=qm.size();
  int listIndex=0;
  while(listIndex<n && qm[listIndex].name != mlist){
    listIndex++;
  }
  if( listIndex==n ) {
    ERROR_ <<"list not found";
    return false;
  }

  //find item index
  int m=qm[listIndex].menuitems.size();
  int itemIndex=0;
  INFO_ <<item;
  while(itemIndex<m && qm[listIndex].menuitems[itemIndex].name != item){
    INFO_ <<qm[listIndex].menuitems[itemIndex].name;
    itemIndex++;
  }
  if( itemIndex==m  ) {
    ERROR_ <<"item not found";
    return false;
  }

  //set menu
  menulist->setCurrentIndex(listIndex);
  menulistActivate(listIndex);
  qm[listIndex].plotindex=itemIndex;
  list->setCurrentRow(itemIndex);
  return true;
}

void QuickMenu::applyPlot()
{
  plotButton();
}

void QuickMenu::adminButton()
{
  QuickAdmin* admin= new QuickAdmin(this, qm, firstcustom, lastcustom);
  //connect(admin, SIGNAL(help(const char*)), this, SIGNAL(help(const char*)));
  if (admin->exec() ){
    // get updated list of menus
    qm= admin->getMenus();
    firstcustom= admin->FirstCustom();
    lastcustom= admin->LastCustom();

    if (prev_listindex >= int(qm.size())){ // if previous plot now bad
      prev_listindex= -1;
      prev_plotindex= -1;
    }
    // reset widgets
    fillMenuList();

    // save custom quickmenus to file
    if (firstcustom != -1){
      for (int m=firstcustom; m<=lastcustom; m++){
        writeQuickMenu(qm[m]);
      }
    }
  }
}

void QuickMenu::fillPrivateMenus()
{

  quickMenu qtmp;

  //History
  qm.push_back(qtmp);
  qm[0].name= tr("History").toStdString();
  qm[0].filename= LocalSetupParser::basicValue("homedir") + "/History.quick";
  qm[0].plotindex= 0;
  readQuickMenu(qm[0]);

  qm.push_back(qtmp);
  qm[1].name= tr("History-vcross").toStdString();
  qm[1].filename= LocalSetupParser::basicValue("homedir") + "/History-vcross.quick";
  qm[1].plotindex= 0;
  readQuickMenu(qm[1]);

  //Private menus
  miutil::miString quickfile= LocalSetupParser::basicValue("homedir") + "/*.quick";
  glob_t globBuf;
  glob_cache(quickfile.c_str(),0,0,&globBuf);
  for(int k=0; k<globBuf.gl_pathc; k++) {
    qtmp.name= "";
    qtmp.filename= globBuf.gl_pathv[k];
    int i = 0;
    while ( i<QMENU &&  qtmp.filename != qm[i].filename ) i++;
    if ( i<QMENU ) continue; //History
    qtmp.plotindex = 0;
    qtmp.menuitems.clear();
    if (readQuickMenu(qtmp)){
      i = 0;
      while ( i<QMENU &&  qtmp.name != qm[i].name ) i++;
      if ( i<QMENU ) continue; //Avoid mix with History
      qm.push_back(qtmp);
    }
    if (firstcustom<0){
      firstcustom= qm.size()-1;
      lastcustom = qm.size()-2;
    }
    lastcustom++;
  }

}

void QuickMenu::fillStaticMenus()
{

  quickMenu qtmp;
  orig_qm.clear();
  vector<QuickMenuDefs> qdefs;
  contr->getQuickMenus(qdefs);

  int n= qdefs.size();
  for (int i=0; i<n; i++){
    qtmp.name= "";
    qtmp.filename= qdefs[i].filename;
    qtmp.plotindex= 0;
    qtmp.menuitems.clear();
    if (readQuickMenu(qtmp)){
      qm.push_back(qtmp);
      orig_qm.push_back(qtmp);
      chng_qm.push_back(qtmp);
    }
  }
}

void QuickMenu::updateButton()
{

  int i= (lastcustom>0 ? lastcustom+1 : 1); // index to first static menu

  if (prev_listindex >= 0 && prev_plotindex >= 0){
    int idx= qm[activemenu].plotindex;
    if (idx >=0 && idx < int(qm[activemenu].menuitems.size())){

      bool changename= false;

      if (instaticmenu){
        QString mess=
          "<b>"+tr("Do you want to replace the content of this menuitem with current plot?")+
          "</b><br>"+
          tr("This is a static/official menuitem, which can be reset to default value.");


        QMessageBox mb("Diana",mess,
            QMessageBox::Information,
            QMessageBox::Yes | QMessageBox::Default,
            QMessageBox::Cancel | QMessageBox::Escape,
            Qt::NoButton );
        mb.setButtonText( QMessageBox::Yes, tr("Yes") );
        mb.setButtonText( QMessageBox::Cancel, tr("Cancel") );
        switch( mb.exec() ) {
        case QMessageBox::Yes:
          // Yes
          break;
        case QMessageBox::Cancel:
          // cancel operation
          return;
          break;
        }
      } else {
        QString mess=
          "<b>"+tr("Do you want to replace the content of this menuitem with current plot?")+
          "</b><br>"+
          tr("The menu name can be automatically created from the underlying data in the plot");

        QMessageBox mb("Diana",mess,
            QMessageBox::Information,
            QMessageBox::Yes | QMessageBox::Default,
            QMessageBox::No,
            QMessageBox::Cancel | QMessageBox::Escape );
        mb.setButtonText( QMessageBox::Yes, tr("Yes, make new menu name") );
        mb.setButtonText( QMessageBox::No, tr("Yes, keep menu name") );
        mb.setButtonText( QMessageBox::Cancel, tr("Cancel") );
        switch( mb.exec() ) {
        case QMessageBox::Yes:
          // Yes
          changename= true;
          break;
        case QMessageBox::No:
          // Yes, but keep the name
          changename= false;
          break;
        case QMessageBox::Cancel:
          // cancel operation
          return;
          break;
        }
      }

      vector<miutil::miString> vs=
        qm[prev_listindex].menuitems[prev_plotindex].command;

      if (instaticmenu) {
        // set it..
        if (vs.size()>0){
          qm[activemenu].menuitems[idx].command= vs;
          chng_qm[activemenu-i].menuitems[idx].command= vs;
        }
      } else {
        // set it..
        if (vs.size()>0){
          replaceDynamicOptions(qm[activemenu].menuitems[idx].command,vs);
          qm[activemenu].menuitems[idx].command= vs;
          if (changename){
            qm[activemenu].menuitems[idx].name=
              qm[prev_listindex].menuitems[prev_plotindex].name;
          }

          // save quickmenu to file if custom
          if (activemenu>= firstcustom && activemenu<=lastcustom){
            writeQuickMenu(qm[activemenu]);
          }
        }
      }

      setCurrentMenu(activemenu);
      listClicked(list->item(idx));
    }
  }
}

void QuickMenu::replaceDynamicOptions(vector<miutil::miString>& oldCommand,
    vector<miutil::miString>& newCommand)
{

  int nold=oldCommand.size();
  int nnew=newCommand.size();

  for(int i=0;i<nold && i<nnew;i++){
    if(!oldCommand[i].contains("@")) continue;
    vector<miutil::miString> token =oldCommand[i].split(" ");
    int ntoken = token.size();
    for(int j=0;j<ntoken;j++){
      if(!token[j].contains("@")) continue;
      vector<miutil::miString> stoken = token[j].split("=");
      if(stoken.size()!=2 || !stoken[1].contains("@")) continue;
      //found item to replace
      vector<miutil::miString> newtoken =newCommand[i].split(" ");
      int nnewtoken = newtoken.size();
      if(nnewtoken<2 || token[0]!=newtoken[0]) continue;
      for(int k=1;k<nnewtoken;k++){
        vector<miutil::miString> snewtoken = newtoken[k].split("=");
        if(snewtoken.size()==2 && snewtoken[0]==stoken[0]){
          newCommand[i].replace(newtoken[k],token[j]);
        }
      }
    }
  }

}

void QuickMenu::resetButton()
{
  int i= (lastcustom>0 ? lastcustom+1 : 1); // index to first static menu

  if (activemenu >= i && activemenu < int(qm.size())){ // static menu
    int idx= qm[activemenu].plotindex;
    if (idx >=0 && idx < int(qm[activemenu].menuitems.size())){

      switch( QMessageBox::warning( this, "Diana",
          tr("Replace command with original copy?"),
          tr("OK"), tr("Cancel"), 0,
          0, 1 )){
      case 0: // Ja
        break;
      case 1: // Quit or Escape
        return;
        break;
      }

      // set menu-item
      qm[activemenu].menuitems[idx]=
        orig_qm[activemenu-i].menuitems[idx];
      // also update change-list
      chng_qm[activemenu-i].menuitems[idx]=
        orig_qm[activemenu-i].menuitems[idx];

      setCurrentMenu(activemenu);
      listClicked(list->item(idx));
    }
  }
}

bool QuickMenu::itemChanged(int menu, int item)
{
  int i= (lastcustom>0 ? lastcustom+1 : 2); // index to first static menu
  if (menu < i) return false; // not static menu

  int oidx= menu - i; // in original list
  int msize= orig_qm[oidx].menuitems[item].command.size();
  if (msize != int(qm[menu].menuitems[item].command.size()))
    return true;

  // check each command-line
  for (int j=0; j<msize; j++){
    if (orig_qm[oidx].menuitems[item].command[j] !=
      qm[menu].menuitems[item].command[j])
      return true;
  }

  return false;
}


void QuickMenu::readLog(const vector<miutil::miString>& vstr,
    const miutil::miString& thisVersion,
    miutil::miString& logVersion) {
  // check version
  //   if (logVersion

  miutil::miString line, str;
  vector<miutil::miString> vs,vvs;
  int n= vstr.size();
  bool skipmenu=true;
  int actidx = -1, oidx = -1;
  unsigned int priIndex = QMENU;
  miutil::miString key,value;

  quickMenuItem tmpitem;
  vector<quickMenuItem> logitems;
  bool itemlog= false, firstitemline= false;

  for (int i=0; i<n; i++){
    line = vstr[i];
    line.trim();
    if (!line.exists()) continue;
    if (line[0]=='#') continue;

    if (line[0]=='>'){ // new menu
      skipmenu= false;
      miutil::miString name, update;
      if (line.length()>1){
        str= line.substr(1,line.length()-1);
      }
      vs= str.split(",");
      for (unsigned int j=0; j<vs.size(); j++){
        vvs= vs[j].split("=");
        if (vvs.size()>1){
          key= vvs[0].upcase();
          value= vvs[1];
          if (key=="NAME")
            name= value;
          else if (key=="UPDATE")
            update= value;
        }
      }

      if (update.exists()){ // update of static menu
        actidx= -1;
        int itmp= (lastcustom >= 0 ? lastcustom+1 : 1);
        for (unsigned int l=0; l<qm.size(); l++){
          if (qm[l].name==update){
            actidx= l;
            break;
          }
        }
        if (actidx<0 || actidx < itmp){ // not found or not static
          skipmenu= true;
          continue;
        }
        // find index to original list
        oidx= actidx-itmp; // in original list

      } else if (name.exists()){ // custom menus, sort according to log
        for (unsigned int l=QMENU; l<qm.size(); l++){ //skip History
          if (qm[l].name==name){
            actidx = priIndex;
            if (l!=priIndex) {
              qm.insert(qm.begin()+priIndex,qm[l]);
              qm.erase(qm.begin()+l+1);
            }
            priIndex++;
            break;
          }
        }
      } else {
        skipmenu= true;
      }

    } else if (line[0]=='%' && !skipmenu){ // dynamic options
      if (line.length()>1 && actidx >= 0 && actidx < int(qm.size())){
        miutil::miString opt= line.substr(1,line.length()-1);
        vs= opt.split("=");
        if (vs.size()>1){
          miutil::miString key= vs[0];
          opt= vs[1];

          for (unsigned int l=0; l<qm[actidx].opt.size(); l++){
            if (key==qm[actidx].opt[l].key){
              qm[actidx].opt[l].def= opt;
              break;
            }
          }
        }
      }
    } else if ((line[0]=='-' || line[0]=='=') && !skipmenu){
      if (itemlog) {
        // end of one item - save it
        logitems.push_back(tmpitem);

        // maybe end of all items
        if (line[0]=='='){
          if (actidx >= 0 && actidx < int(qm.size())) {
            // update static menus with logged items
            int m = logitems.size();
            for (int l = 0; l < m; l++) {
              // find item in static list: actidx
              int r = qm[actidx].menuitems.size();
              int ridx = -1;
              for (int k = 0; k < r; k++) {
                if (qm[actidx].menuitems[k].name == logitems[l].name) {
                  ridx = k;
                  break;
                }
              }
              if (ridx < 0){
                continue; // not found
              }
              // Ok, change it
              qm[actidx].menuitems[ridx].command = logitems[l].command;
              chng_qm[oidx].menuitems[ridx].command = logitems[l].command;
            }
          }
          // update finished - remove it
          itemlog= false;
          logitems.clear();
        }
      } else if (line[0]=='-'){
        itemlog= true;
      }
      firstitemline= true;
    } else if (itemlog){ // reading items
      if (firstitemline){
        tmpitem.name= line;
        firstitemline= false;
        tmpitem.command.clear();
      } else {
        tmpitem.command.push_back(line);
      }
    }
  }
}

// write log of plot-commands
vector<miutil::miString> QuickMenu::writeLog()
{

  // save any changes to the command
  saveChanges(-1,-1);

  vector<miutil::miString> vstr;
  miutil::miString str;

  int n= qm.size();
  int m= orig_qm.size();
  int i2= (lastcustom >= 0 ? lastcustom+1 : 1);


  if (n > 0){

    for (int j=0; j<QMENU; j++){
      writeQuickMenu(qm[j]); // save History to file
    }

    for (int j=QMENU; j<n; j++){

      // menuname
      if (j<i2 ){ //custom menus
        writeQuickMenu(qm[j]); // save custom menus to file
        str= ">name="+qm[j].name;
      } else {
        str= ">update="+qm[j].name;
      }
      vstr.push_back(str);
      // write defaults for dynamic options
      for (unsigned int k=0; k<qm[j].opt.size(); k++){
        miutil::miString optline="%"+qm[j].opt[k].key+"="+qm[j].opt[k].def;
        vstr.push_back(optline);
      }
      if (j>=i2){
        // log changes in static menus
        int oidx= -1;
        for (int i=0; i<m; i++)
          if (orig_qm[i].name == qm[j].name){
            oidx= i;
            break;
          }
        if (oidx==-1) continue;// not found in original list
        unsigned int msize= chng_qm[oidx].menuitems.size();
        if (orig_qm[oidx].menuitems.size() != msize)
          continue; // illegal change
        for (unsigned int i=0; i<msize; i++){
          bool isdiff= false;
          unsigned int csize= chng_qm[oidx].menuitems[i].command.size();
          if (orig_qm[oidx].menuitems[i].command.size() != csize)
            isdiff= true;
          if (!isdiff){
            for (unsigned int k=0; k<csize; k++){ // check if any changes
              if (orig_qm[oidx].menuitems[i].command[k] !=
                chng_qm[oidx].menuitems[i].command[k]){
                // 		  qm[j].menuitems[i].command[k]){
                isdiff= true;
                break;
              }
            }
          }
          if (isdiff){
            vstr.push_back("-----------------------");
            vstr.push_back(chng_qm[oidx].menuitems[i].name);
            for (unsigned int k=0; k<csize; k++)
              vstr.push_back(chng_qm[oidx].menuitems[i].command[k]);
          }
        }
      }
      vstr.push_back("=======================");
    }
  }
  return vstr;
}

void QuickMenu::fillMenuList()
{
  menulist->clear();
  int n= qm.size();
  if (n == 0) return;

  vector<QString> qnames;
  for (int i=0; i<n; i++){
    qnames.push_back(qm[i].name.c_str());
  }

  for (int i=0; i<n; i++) menulist->addItem(qnames[i]);

  // set active menu
  if (activemenu >= int(qm.size())) activemenu= qm.size()-1;
  setCurrentMenu(activemenu);
}


void QuickMenu::menulistActivate(int idx)
{
  //  DEBUG_ << "Menulistactivate called:" << idx;
  if (qm.size() == 0) return;
  if (idx >= int(qm.size())) idx= qm.size()-1;

  activemenu= idx;
  list->clear();
  int numitems= qm[idx].menuitems.size();
  if (numitems>0){
    QStringList itemlist;
    for(int i=0; i<numitems; i++){
      // remove richtext tags
      QString qstr= qm[idx].menuitems[i].name.c_str();
      qstr.replace(QRegExp("</*font[^>]*>"), "" );
      itemlist+= qstr;
    }
    list->addItems(itemlist);
    if (qm[idx].plotindex >= int(qm[idx].menuitems.size()))
      qm[idx].plotindex= 0;
    list->item(qm[idx].plotindex)->setSelected(true);
    listClicked(list->item(qm[idx].plotindex));
  }

  // hide old options
  for (int i=0; i<maxoptions; i++){
    optionmenu[i]->clear();
    optionmenu[i]->adjustSize();
    optionlabel[i]->hide();
    optionmenu[i]->hide();
  }
  // add options
  optionsexist= false;
  int n= qm[idx].opt.size();
  if (n > maxoptions) n= maxoptions;
  if (n>0){
    for (int i=0; i<n; i++){
      optionlabel[i]->setText(qm[idx].opt[i].key.c_str());

      int nopts= qm[idx].opt[i].options.size();
      int defidx= -1;
      if (nopts > 0){
        for(int j=0; j<nopts; j++){
          optionmenu[i]->addItem(QString(qm[idx].opt[i].options[j].c_str()));
          if (qm[idx].opt[i].options[j] == qm[idx].opt[i].def)
            defidx= j;
        }
        if (defidx>=0) optionmenu[i]->setCurrentIndex(defidx);
      }
      optionmenu[i]->adjustSize();
      optionlabel[i]->show();
      optionmenu[i]->show();
      //optionlabel[i]->setEnabled(true);
    }
    optionsexist= true;
  }
  // check if in static menu
  instaticmenu= ((lastcustom>0 && activemenu>lastcustom) ||
      (lastcustom==-1 && activemenu > 0));
  // enable updateButton
  updatebut->setEnabled(true);//instaticmenu);
}


void QuickMenu::saveChanges(int midx, int lidx){
  static int oldindex= -1;
  static int oldmenu=  -1;
  if (oldindex!=-1 && (oldindex!=lidx || oldmenu!= midx) &&
      oldmenu<int(qm.size()) && oldindex<int(qm[oldmenu].menuitems.size()) &&
      comset){
    vector<miutil::miString> s;
    getCommand(s);
    qm[oldmenu].menuitems[oldindex].command= s;
  }
  oldindex= lidx;
  oldmenu=  midx;
}

void QuickMenu::listClicked( QListWidgetItem * item)
{
  int idx = list->row(item);

  saveChanges(activemenu, idx);
  miutil::miString ts;
  int n= qm[activemenu].menuitems[idx].command.size();
  for (int i=0; i<n; i++){
    ts += qm[activemenu].menuitems[idx].command[i];
    //     if (i<n-1) ts+= miutil::miString("\n");
    ts+= miutil::miString("\n");
  }
  // set command into command-edit
  comedit->setText(QString(ts.c_str()));
  comset= true;
  qm[activemenu].plotindex= idx;
  // enable/disable resetButton
  resetbut->setEnabled(instaticmenu && itemChanged(activemenu, idx));
}

void QuickMenu::comChanged(){
  //   DEBUG_ << "Command text changed";
  miutil::miString ts= comedit->toPlainText().toStdString();
  // check if any variables to set here
  int m= qm[activemenu].opt.size();
  if (m > maxoptions) m= maxoptions;
  for (int i=0; i<m; i++){
    bool enable= ts.contains(vprefix+qm[activemenu].opt[i].key);
    optionmenu[i]->setEnabled(enable);
    optionlabel[i]->setEnabled(enable);
  }
  // enable/disable resetButton
  resetbut->setEnabled(instaticmenu);
}


void QuickMenu::listDoubleClicked( QListWidgetItem * item)
{
  plotButton();
}

void QuickMenu::getCommand(vector<miutil::miString>& s){

  miutil::miString text = comedit->toPlainText().toStdString();
  s.clear();
  s = text.split("\n");
  int ni = s.size();
  for (int i=0; i<ni; i++){
    s[i].trim();
  }
}


void QuickMenu::varExpand(vector<miutil::miString>& com)
{
  int n= com.size();
  int m= qm[activemenu].opt.size();
  if (m>maxoptions) m= maxoptions;

  // sort keys by length - make index-list
  vector<int> keys;
  for (int i=0; i<m; i++){
    miutil::miString key= qm[activemenu].opt[i].key;
    vector<int>::iterator it= keys.begin();
    for (; it!=keys.end() &&
    key.length()<qm[activemenu].opt[*it].key.length();
    it++)
      ;
    keys.insert(it, i);
  }

  for (int i=0; i<n; i++){
    for (int j=0; j<m; j++){
      miutil::miString key= vprefix+qm[activemenu].opt[keys[j]].key;
      miutil::miString val= optionmenu[keys[j]]->currentText().toStdString();
      com[i].replace(key,val);
      // keep for later default
      qm[activemenu].opt[keys[j]].def= val;
    }
  }
}

void QuickMenu::plotButton()
{
  vector<miutil::miString> com;
  getCommand(com);

  if (com.size()>0){
    if (optionsexist) varExpand(com);
    emit Apply(com,true);
    prev_plotindex= qm[activemenu].plotindex;
    prev_listindex= activemenu;
    browsing= false;
  }
  saveChanges(-1,-1);
}

void QuickMenu::comButton(bool on)
{
  if (on) {
    comlabel->show();
    comedit->show();

    int w= this->width();
    int h= comlabel->height() + comedit->height();
    this->resize(w, this->height()+h);


  } else {
    int w= this->width();
    int h= comlabel->height() + comedit->height();

    comlabel->hide();
    comedit->hide();

    this->resize(w, this->height()-h);
  }
}

void QuickMenu::demoButton(bool on)
{
  if (on) {
    qm[activemenu].plotindex=0;//activeplot = 0;
    demoTimer= startTimer(timerinterval*1000);
    timeron = true;
    list->setCurrentRow(0);
    listDoubleClicked(list->item(0));
  } else {
    killTimer(demoTimer);
    timeron = false;
  }
}

void QuickMenu::timerEvent(QTimerEvent *e)
{
  if (e->timerId()==demoTimer){
    qm[activemenu].plotindex++;//activeplot++;
    if (qm[activemenu].plotindex>=int(qm[activemenu].menuitems.size()))
      qm[activemenu].plotindex=0;
    list->setCurrentRow(qm[activemenu].plotindex);
    listClicked(list->item(qm[activemenu].plotindex));
    listDoubleClicked(list->item(qm[activemenu].plotindex));
  }
}

void QuickMenu::intervalChanged(int value)
{
  timerinterval= value;
  if (timeron){
    killTimer(demoTimer);
    demoTimer= startTimer(timerinterval*1000);
  }
}


void QuickMenu::helpClicked(){
  emit showsource("ug_quickmenu.html");
}


void QuickMenu::closeEvent( QCloseEvent* e) {
  emit QuickHide();
}


