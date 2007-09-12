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
#include <fstream>

#include <qtQuickMenu.h>
#include <qtQuickAdmin.h>

#include <qpushbutton.h>
#include <qlistbox.h>
#include <qtextedit.h>
#include <qcursor.h>
#include <qlayout.h>
#include <qframe.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qapplication.h>
#include <miString.h>
#include <iostream>
#include <qmessagebox.h>
#include <qtooltip.h>
#include <qregexp.h>
#include <qtUtility.h>

#include <preferences.xpm>
#include <convert.xpm>
#include <revert.xpm>

using namespace std; 

const miString vprefix= "@";

QuickMenu::QuickMenu( QWidget *parent, Controller* c,
		      const char *name, WFlags f)
  : QDialog( parent, name, FALSE, f), contr(c),
    timerinterval(10), timeron(false), activemenu(0),
    comset(false), browsing(false), instaticmenu(false),
    firstcustom(-1), lastcustom(-1),
    prev_plotindex(0), prev_listindex(0)
{
  
  setCaption(tr("Quickmenu"));
  
  // Create top-level layout manager
  QBoxLayout* tlayout = new QHBoxLayout( this, 5, 20, "tlayout");

  // make a nice frame
  QFrame* frame= new QFrame(this, "frame");
  frame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

  //  frame->setPalette( QPalette( QColor(255, 80, 80) ) );
  tlayout->addWidget(frame,1);

  // Create layout manager
  QBoxLayout* vlayout = new QVBoxLayout(frame,10,10,"vlayout");

  // create quickmenu optionmenu
  QBoxLayout *l2= new QHBoxLayout(vlayout);
  QLabel* menulistlabel= TitleLabel(tr("Menus"),frame);

  menulist= new QComboBox(FALSE, frame, "menulist");
  connect(menulist, SIGNAL(activated(int)),SLOT(menulistActivate(int)));
  QPushButton* adminbut= new QPushButton(tr("&Edit menus.."), frame );
  QToolTip::add( adminbut,
		 tr("Menu editor: Copy, change name and sortorder etc. on your own menus") );
  connect(adminbut, SIGNAL(clicked()),SLOT(adminButton()));

  updatebut= new QPushButton(tr("&Update.."), frame );
  QToolTip::add( updatebut, tr("Update command with current plot") );
  connect(updatebut, SIGNAL(clicked()),SLOT(updateButton()));

  resetbut= new QPushButton(tr("&Reset.."), frame );
  QToolTip::add( resetbut, tr("Reset command to original copy") );
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
  list= new QListBox(frame, "itemlist");
  connect(list, SIGNAL(highlighted(int)),SLOT(listHighlight(int)));
  connect(list, SIGNAL(selected(int)),SLOT(listSelect(int)));
  vlayout->addWidget(list, 10);

  // Create variables/options layout manager
  int half= maxoptions/2;
  QGridLayout* varlayout = new QGridLayout(vlayout,2,maxoptions);
  for (int i=0; i<maxoptions; i++){
    optionlabel[i]= new QLabel("",frame,"optlabel");
    optionmenu[i]=  new QComboBox(FALSE, frame, "optionmenu");
    int row = (i < half ? 0 : 1);
    int col = 2*(i - row*half);
    varlayout->addWidget(optionlabel[i],row,col,Qt::AlignRight);
    varlayout->addWidget(optionmenu[i],row,col+1);//Qt::AlignLeft);
  }

  QFrame *line;
  // Create a horizontal frame line
  line = new QFrame( frame );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  vlayout->addWidget( line );

  // create commands-area
  comedit= new QTextEdit(frame, "comedit");
  comedit->setWordWrap(QTextEdit::NoWrap);
  comedit->setFont(QFont("Courier",12,QFont::Normal));
  comedit->setReadOnly(false);
  comedit->setMinimumHeight(150);
  connect(comedit, SIGNAL(textChanged()), SLOT(comChanged()));
  comlabel= new QLabel(comedit,tr("Command field"),frame,"comlabel");
  comlabel->setMinimumSize(comlabel->sizeHint());
  comlabel->setAlignment(AlignBottom | AlignLeft);

  comlabel->hide();
  comedit->hide();

  QHBoxLayout* l3= new QHBoxLayout(5);
  l3->addWidget(comlabel);
  l3->addStretch();

  vlayout->addLayout(l3,0);
  vlayout->addWidget(comedit, 10);

  // buttons and stuff at the bottom
  QBoxLayout *l= new QHBoxLayout(vlayout);

  QButton* quickhide = new QPushButton( tr("&Hide"), frame );
  connect( quickhide, SIGNAL(clicked()),SIGNAL( QuickHide()) );
  l->addWidget(quickhide);

  QPushButton* combut= new QPushButton(tr("&Command"), frame );
  combut->setToggleButton(true);
  connect(combut, SIGNAL(toggled(bool)),SLOT(comButton(bool)));
  l->addWidget(combut);

  QPushButton* demobut= new QPushButton(tr("&Demo"), frame );
  demobut->setToggleButton(true);
  connect(demobut, SIGNAL(toggled(bool)),SLOT(demoButton(bool)));
  l->addWidget(demobut);

  QSpinBox* interval= new QSpinBox(2,360,2,frame,"interval");
  interval->setValue(timerinterval);
  interval->setSuffix(" sek");
  connect(interval, SIGNAL(valueChanged(int)),SLOT(intervalChanged(int)));
  l->addWidget(interval);
  
  QButton* qhelp= new QPushButton(tr("&Help"), frame );
  connect( qhelp, SIGNAL(clicked()), SLOT( helpClicked() ));
  l->addWidget(qhelp);
  
  QButton* plothidebut= new QPushButton(tr("Apply+Hide"), frame );
  connect(plothidebut, SIGNAL(clicked()),SLOT(plotButton()));
  connect(plothidebut, SIGNAL(clicked()),SIGNAL( QuickHide()) );
  l->addWidget(plothidebut);

  QButton* plotbut= new QPushButton(tr("&Apply"), frame );
  connect(plotbut, SIGNAL(clicked()),SLOT(plotButton()));
  l->addWidget(plotbut);
  //  l->addStretch();

  // Start the geometry management
  vlayout->activate();
  tlayout->activate();

  fillStaticMenues();
}

// set active menu
void QuickMenu::setCurrentMenu(int i)
{
  menulist->setCurrentItem(i);
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
vector<miString> QuickMenu::getCustomMenues()
{
  vector<miString> vs;
  if (firstcustom >= 0 && lastcustom>=0){
    for (int i=firstcustom; i<=lastcustom; i++)
      vs.push_back(qm[i].name);
  }
  return vs;
}

bool QuickMenu::addMenu(const miString& name)
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
  if (lastcustom >-1 && lastcustom<qm.size()){
    writeQuickMenu(qm[lastcustom]);
  }

  return true;
}

bool QuickMenu::addToMenu(const int idx)
{
  if (firstcustom >=0 && lastcustom>=0 &&
      firstcustom+idx < qm.size() &&
      prev_listindex>=0 && prev_plotindex>=0 &&
      prev_listindex<qm.size()){
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


// Push a new command on the history-stack
void QuickMenu::pushPlot(const miString& name,
			 const vector<miString>& pstr)
{
  if (qm.size()==0) return;
  bool goon= true;
  int m= qm[0].menuitems.size();
  if (m > 0){ // check for duplicate
    if (qm[0].menuitems[0].command.size()==pstr.size()){
      goon= false;
      for (int i=0; i<pstr.size(); i++){
	if (pstr[i] != qm[0].menuitems[0].command[i]){
	  goon= true;
	  break;
	}
      }
    }
  }
  if (goon) {
    // keep stack within bounds
    int ms= qm[0].menuitems.size();
    if (ms >= maxplotsinstack)
      qm[0].menuitems.pop_back();
    // update pointer
    qm[0].plotindex=0;
    // push plot on stack
    miString plotname= name;
    plotname.trim();
    quickMenuItem dummy; 
    qm[0].menuitems.push_front(dummy);
    qm[0].menuitems[0].command= pstr;
    qm[0].menuitems[0].name= plotname;
    // switch to history-menu
    //     if (!isVisible() || activemenu==0) setCurrentMenu(0);
    if (activemenu==0) setCurrentMenu(0);

    prev_plotindex= 0;
    prev_listindex= 0;
  }
}

// called from quick-quick menu (Browsing)
bool QuickMenu::prevQPlot(){
  int menu= activemenu;
  if (qm.size()==0 || qm.size()<menu+1)
    return false;
  int n= qm[menu].menuitems.size();
  if (n==0 || qm[menu].plotindex >= n-1 || qm[menu].plotindex<0)
    return false;
  
  qm[menu].plotindex++;
  list->setCurrentItem(qm[menu].plotindex);
  return true;
}

// Go to previous History-plot
bool QuickMenu::prevHPlot(){
  int menu= 0;
  if (qm.size()==0 || qm.size()<menu+1)
    return false;
  int n= qm[menu].menuitems.size();
  
  // if in History-menu or last plot from History:
  // Change plotindex and plot
  if (activemenu==menu || prev_listindex==menu){
    if (n==0 || qm[menu].plotindex >= n-1 || qm[menu].plotindex<0)
      return false;
    qm[menu].plotindex++;
    if (activemenu==menu) list->setCurrentItem(qm[menu].plotindex);
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
  if (qm.size()==0 || qm.size()<menu+1)
    return false;
  int n= qm[menu].menuitems.size();
  if (n==0 || qm[menu].plotindex > n-1 || qm[menu].plotindex<=0)
    return false;
  
  qm[menu].plotindex--;
  list->setCurrentItem(qm[menu].plotindex);
  return true;
}

// Go to next History-plot
bool QuickMenu::nextHPlot(){
  int menu= 0;
  if (qm.size()==0 || qm.size()<menu+1)
    return false;
  int n= qm[menu].menuitems.size();
  
  // if in History-menu or last plot from History:
  // Change plotindex and plot
  if (activemenu==menu || prev_listindex==menu){
    if (n==0 || qm[menu].plotindex > n-1 || qm[menu].plotindex<=0)
      return false;
    qm[menu].plotindex--;
    if (activemenu==menu) list->setCurrentItem(qm[menu].plotindex);
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
  if (qm.size()==0 || activemenu<1 || activemenu>qm.size()-1)
    return false;
  setCurrentMenu(activemenu-1);
  return true;
}


// For browsing: go to next quick-menu
bool QuickMenu::nextList(){
  if (qm.size()==0 || activemenu<0 || activemenu>=qm.size()-1)
    return false;
  setCurrentMenu(activemenu+1);
  return true;
}


// for Browsing: get menu-details
void QuickMenu::getDetails(int& plotidx,
			   miString& listname,
			   miString& plotname)
{
  plotidx= 0;
  if (qm.size()>0 && activemenu < qm.size()){
    plotidx= qm[activemenu].plotindex;
    listname= qm[activemenu].name;
    plotname= (plotidx < qm[activemenu].menuitems.size() ?
	       qm[activemenu].menuitems[plotidx].name:"");
  }
}

bool QuickMenu::applyItem(const miString& mlist, const miString& item)
{
  //find list index
  int n=qm.size();
  int listIndex=0;
  while(listIndex<n && qm[listIndex].name != mlist){
    listIndex++;
  }    
  if( listIndex==n ) return false;

  //find item index
  int m=qm[listIndex].menuitems.size();
  int itemIndex=0;
  while(itemIndex<m && qm[listIndex].menuitems[itemIndex].name != item){
    itemIndex++;
  }
  if( itemIndex==m  ) return false;

  //set menu
  menulist->setCurrentItem(listIndex);
  menulistActivate(listIndex);
  qm[listIndex].plotindex=itemIndex;
  list->setCurrentItem(itemIndex);
  return true;
}

void QuickMenu::applyPlot()
{
  plotButton();
  //emit Apply(qm[activemenu].menuitems[qm[activemenu].plotindex].command);
}

void QuickMenu::adminButton()
{
  QuickAdmin* admin= new QuickAdmin(this, qm, firstcustom, lastcustom);
  //connect(admin, SIGNAL(help(const char*)), this, SIGNAL(help(const char*)));
  if (admin->exec() ){
    // get updated list of menues
    qm= admin->getMenues();
    firstcustom= admin->FirstCustom();
    lastcustom= admin->LastCustom();

    if (prev_listindex >= qm.size()){ // if previous plot now bad
      prev_listindex= -1;
      prev_plotindex= -1;
    }
    // reset widgets
    fillMenuList();
    
    // save custom quickmenues to file
    if (firstcustom != -1){
      for (int m=firstcustom; m<=lastcustom; m++){
	writeQuickMenu(qm[m]);
      }
    }
  }
}

void QuickMenu::fillStaticMenues()
{
  quickMenu qtmp;
  qm.push_back(qtmp);
  qm[0].name= tr("History").latin1();
  qm[0].filename= "Historie.quick";
  qm[0].plotindex= 0;
  
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
    if (idx >=0 && idx < qm[activemenu].menuitems.size()){
      
      bool changename= false;

      if (instaticmenu){
	QString mess=
	  "<b>"+tr("Do you want to replace the content of this menuitem with current plot?")+
	  "</b><br>"+
	  tr("Be aware that this is a static/official menuitem, and you are not guaranteed to be able to keep all changes.");

	
	QMessageBox mb("Diana",mess,
		       QMessageBox::Information,
		       QMessageBox::Yes | QMessageBox::Default,
		       QMessageBox::Cancel | QMessageBox::Escape,
		       QMessageBox::NoButton );
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
	  tr("The name can also be automatically created from the underlying data in the plot");

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

      vector<miString> vs=
	qm[prev_listindex].menuitems[prev_plotindex].command;
      
      if (instaticmenu) {
	// get legal version of current commands
	emit(requestUpdate(chng_qm[activemenu-i].menuitems[idx].command,
			   vs));
	// set it..
	if (vs.size()>0){
	  qm[activemenu].menuitems[idx].command= vs;
	  chng_qm[activemenu-i].menuitems[idx].command= vs;
	}
      } else {
	// set it..
	if (vs.size()>0){
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
      listHighlight(idx);
    }
  }
}

void QuickMenu::resetButton()
{
  int i= (lastcustom>0 ? lastcustom+1 : 1); // index to first static menu
  
  if (activemenu >= i && activemenu < qm.size()){ // static menu
    int idx= qm[activemenu].plotindex;
    if (idx >=0 && idx < qm[activemenu].menuitems.size()){

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
      listHighlight(idx);
    }
  }
}

bool QuickMenu::itemChanged(int menu, int item)
{
  int i= (lastcustom>0 ? lastcustom+1 : 1); // index to first static menu

  if (menu < i) return false; // not static menu

  int oidx= menu-i; // in original list
  int msize= orig_qm[oidx].menuitems[item].command.size();
  if (msize != qm[menu].menuitems[item].command.size())
    return true;

  // check each command-line
  for (int j=0; j<msize; j++){
    if (orig_qm[oidx].menuitems[item].command[j] !=
	qm[menu].menuitems[item].command[j])
      return true;
  }
  
  return false;
}


void QuickMenu::readLog(const vector<miString>& vstr,
			const miString& thisVersion,
			miString& logVersion) {
  // check version
  //   if (logVersion

  miString line, str;
  vector<miString> vs,vvs;
  int n= vstr.size();
  bool skipmenu=true;
  int idx= -1, actidx, oidx;
  miString key,value;

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
      miString name, filename, update;
      if (line.length()>1){
	str= line.substr(1,line.length()-1);
      }
      vs= str.split(",");
      for (int j=0; j<vs.size(); j++){
	vvs= vs[j].split("=");
	if (vvs.size()>1){
	  key= vvs[0].upcase();
	  value= vvs[1];
	  if (key=="NAME")
	    name= value;
	  else if (key=="FILENAME")
	    filename= value;
	  else if (key=="UPDATE")
	    update= value;
	}
      }
      if (idx<0){ // History menu
	actidx= 0;
	if (filename.exists())
	  qm[0].filename= filename;
	idx++;
	// read menu from file
	readQuickMenu(qm[actidx]);

      } else if (update.exists()){ // update of static menu
	actidx= -1;
	int itmp= (lastcustom >= 0 ? lastcustom+1 : 1);
	for (int l=0; l<qm.size(); l++){
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
	
      } else if (name.exists() && filename.exists()){ // custom menues
	quickMenu qtmp;
	qtmp.name= name;
	qtmp.filename= filename;
	qtmp.plotindex= 0;
	idx++;
	if (firstcustom<0){
	  firstcustom= idx;
	  lastcustom= idx-1;
	}
	lastcustom++;
	qm.insert(qm.begin()+lastcustom,qtmp);
	actidx= lastcustom;
	// read menu from file
	readQuickMenu(qm[actidx]);
      } else
	skipmenu= true;
      
    } else if (line[0]=='%' && !skipmenu){ // dynamic options
      if (line.length()>1 && actidx>=0){
	miString opt= line.substr(1,line.length()-1);
	vs= opt.split("=");
	if (vs.size()>1){
	  miString key= vs[0];
	  opt= vs[1];
	  
	  for (int l=0; l<qm[actidx].opt.size(); l++){
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
	  // update static menues with logged items
	  int m= logitems.size();
	  for (int l=0; l<m; l++){
	    // find item in static list: actidx
	    int r= qm[actidx].menuitems.size();
	    int ridx=-1;
	    for (int k=0; k<r; k++){
	      if (qm[actidx].menuitems[k].name==logitems[l].name){
		ridx= k;
		break;
	      }
	    }
	    if (ridx==-1) continue; // not found
	    // check that logged items are legal changes
	    emit(requestUpdate(orig_qm[oidx].menuitems[ridx].command,
			       logitems[l].command));
	    // Ok, change it
	    qm[actidx].menuitems[ridx].command= logitems[l].command;
	    chng_qm[oidx].menuitems[ridx].command= logitems[l].command;
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
vector<miString> QuickMenu::writeLog()
{
  // save any changes to the command
  saveChanges(-1,-1);

  vector<miString> vstr;
  miString str;
  
  int n= qm.size();
  int m= orig_qm.size();
  int i2= (lastcustom >= 0 ? lastcustom+1 : 1);

  if (n > 0){
    for (int j=0; j<n; j++){
      // menuname
      if (j<i2)
	str= ">name="+qm[j].name+", filename="+qm[j].filename;
      else
	str= ">update="+qm[j].name;
      vstr.push_back(str);
      // write defaults for dynamic options
      for (int k=0; k<qm[j].opt.size(); k++){
	miString optline="%"+qm[j].opt[k].key+"="+qm[j].opt[k].def;
	vstr.push_back(optline);
      }
      if (j<i2)
	writeQuickMenu(qm[j]); // save custom menues to file
      else {
	// log changes in static menues
	int oidx= -1;
	for (int i=0; i<m; i++)
	  if (orig_qm[i].name == qm[j].name){
	    oidx= i;
	    break;
	  }
	if (oidx==-1) continue;// not found in original list
	int msize= chng_qm[oidx].menuitems.size();
	if (orig_qm[oidx].menuitems.size() != msize)
	  continue; // illegal change
	for (int i=0; i<msize; i++){
	  bool isdiff= false;
	  int csize= chng_qm[oidx].menuitems[i].command.size();
	  if (orig_qm[oidx].menuitems[i].command.size() != csize)
	    isdiff= true;
	  if (!isdiff){
	    for (int k=0; k<csize; k++){ // check if any changes
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
	    for (int k=0; k<csize; k++)
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
    qnames.push_back(qm[i].name.cStr());
  }

  for (int i=0; i<n; i++) menulist->insertItem(qnames[i]);

  // set active menu
  if (activemenu >= qm.size()) activemenu= qm.size()-1;
  setCurrentMenu(activemenu);
}


void QuickMenu::menulistActivate(int idx)
{
  //   cerr << "Menulistactivate called:" << idx << endl;
  if (qm.size() == 0) return;
  if (idx >= qm.size()) idx= qm.size()-1;

  activemenu= idx;
  list->clear();
  int numitems= qm[idx].menuitems.size();
  if (numitems>0){
    QStringList itemlist;
    for(int i=0; i<numitems; i++){
      // remove richtext tags
      QString qstr= qm[idx].menuitems[i].name.cStr();
      qstr.replace(QRegExp("</*font[^>]*>"), "" );
      itemlist+= qstr;
    }
    list->insertStringList(itemlist);
    if (qm[idx].plotindex >= qm[idx].menuitems.size())
      qm[idx].plotindex= 0;
    list->setSelected(qm[idx].plotindex,true);
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
      optionlabel[i]->setText(qm[idx].opt[i].key.cStr());
      
      int nopts= qm[idx].opt[i].options.size();
      int defidx= -1;
      if (nopts > 0){
	const char** itemlist= new const char*[nopts];
	for(int j=0; j<nopts; j++){
	  itemlist[j]=  qm[idx].opt[i].options[j].cStr();
	  if (qm[idx].opt[i].options[j] == qm[idx].opt[i].def)
	    defidx= j;
	}
	optionmenu[i]->insertStrList(itemlist, nopts);
	if (defidx>=0) optionmenu[i]->setCurrentItem(defidx);
	delete[] itemlist;
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
      oldmenu<qm.size() && oldindex<qm[oldmenu].menuitems.size() &&
      comset){
    vector<miString> s;
    getCommand(s);
    qm[oldmenu].menuitems[oldindex].command= s;
  }
  oldindex= lidx;
  oldmenu=  midx;
}

void QuickMenu::listHighlight(int idx)
{
  //   cerr << "list highlight:" << idx << endl;
  saveChanges(activemenu, idx);
  miString ts;
  int n= qm[activemenu].menuitems[idx].command.size();
  for (int i=0; i<n; i++){
    ts += qm[activemenu].menuitems[idx].command[i];
    //     if (i<n-1) ts+= miString("\n");
    ts+= miString("\n");
  }
  // set command into command-edit
  comedit->setText(ts.cStr());
  comset= true;
  qm[activemenu].plotindex= idx;
  // enable/disable resetButton
  resetbut->setEnabled(instaticmenu && itemChanged(activemenu, idx));
}

void QuickMenu::comChanged(){
  //   cerr << "Command text changed" << endl;
  miString ts= comedit->text().latin1();
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


void QuickMenu::listSelect(int idx)
{
  plotButton();
}

void QuickMenu::getCommand(vector<miString>& s){
  int ni= comedit->paragraphs();
  s.clear();
  for (int i=0; i<ni; i++){
    miString str= comedit->text(i).latin1();
    str.trim();
    if (str.contains("\n"))
      str.erase(str.end()-1);
    if (str.exists()) s.push_back(str);
  }
}


void QuickMenu::varExpand(vector<miString>& com)
{
  int n= com.size();
  int m= qm[activemenu].opt.size();
  if (m>maxoptions) m= maxoptions;

  // sort keys by length - make index-list
  vector<int> keys;
  for (int i=0; i<m; i++){
    miString key= qm[activemenu].opt[i].key;
    vector<int>::iterator it= keys.begin();
    for (; it!=keys.end() &&
	   key.length()<qm[activemenu].opt[*it].key.length();
	 it++)
      ;
    keys.insert(it, i);
  }

  for (int i=0; i<n; i++){
    for (int j=0; j<m; j++){
      miString key= vprefix+qm[activemenu].opt[keys[j]].key;
      miString val= optionmenu[keys[j]]->currentText().latin1();
      com[i].replace(key,val);
      // keep for later default
      qm[activemenu].opt[keys[j]].def= val;
    }
  }
}

void QuickMenu::plotButton()
{
  vector<miString> com;
  getCommand(com);

  if (com.size()>0){
    if (optionsexist) varExpand(com);
    if (!browsing ||  // plot if no browsing..
	(activemenu!=prev_listindex || // or if selected plot different
	 qm[activemenu].plotindex!=prev_plotindex))// from previous
      emit Apply(com,true);
    prev_plotindex= qm[activemenu].plotindex;
    prev_listindex= activemenu;
    browsing= false;
  }
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
    list->setCurrentItem(0);
    listSelect(0);
  } else {
    killTimer(demoTimer);
    timeron = false;
  }
}

void QuickMenu::timerEvent(QTimerEvent *e)
{
  if (e->timerId()==demoTimer){
    qm[activemenu].plotindex++;//activeplot++;
    if (qm[activemenu].plotindex>=qm[activemenu].menuitems.size())
      qm[activemenu].plotindex=0;
    list->setCurrentItem(qm[activemenu].plotindex);
    listSelect(qm[activemenu].plotindex);
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
  emit showdoc("ug_quickmenu.html"); 
}


bool QuickMenu::close(bool alsoDelete){
  emit QuickHide();
  return true;
}


