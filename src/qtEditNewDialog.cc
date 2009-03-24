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
#include <QComboBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QMessageBox>
#include <QTabWidget>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPixmap>
#include <QFrame>
#include <QVBoxLayout>

#include <qtEditNewDialog.h>
#include <qtUtility.h>
#include <qtEditDefineField.h>
#include <qtTimeSpinbox.h>


#include <miString.h>
#include <iostream>

#include <diController.h>
#include <diEditManager.h>

#include <kill.xpm>

#ifdef METNOPRODDB
#include <qtLoginDialog.h>
#endif

EditNewDialog::EditNewDialog( QWidget* parent, Controller* llctrl )
  : QDialog(parent,"editnewdialog",false), m_ctrl(llctrl), m_editm(0)
{
  ConstructorCernel();
}

/*********************************************/
void EditNewDialog::ConstructorCernel(){
#ifdef dEditDlg
  cout<<"EditNewDialog::ConstructorCernel called"<<endl;
#endif

  m_editm= m_ctrl->getEditManager();

  TABNAME_NORMAL = tr("Normal");

  normal= true;
  currprod= -1;
  dbi.loggedin= false;
  productfree= false;

  setWindowTitle(tr("New product"));

  first= true;

  QFrame* pframe= new QFrame(this);
  pframe->setFrameStyle( QFrame::Panel | QFrame::Sunken );

  QLabel* prodlabel= new QLabel( tr("Product type:"), pframe );
  prodbox= new QComboBox(pframe);
  prodbox->setEnabled(false);
  connect(prodbox, SIGNAL(activated(int)), SLOT(prodBox(int)));

  QLabel* idlabel= new QLabel( tr("Product Id:"), pframe );
  idbox= new QComboBox(pframe);
  idbox->setEnabled(false);
  connect(idbox, SIGNAL(activated(int)), SLOT(idBox(int)));

  loginb= new QPushButton(tr("Log in"), pframe);
  connect(loginb, SIGNAL(clicked()),  SLOT(login_clicked()) );
  loginlabel= new QLabel("--------------", pframe );
  loginb->setEnabled(false);

  availlabel= new QLabel(pframe);
  availlabel->setText("<font color=\"#009900\"> ---------- </font>");

  killb= new QPushButton(QPixmap(kill_xpm),"", pframe);
  killb->setMaximumSize(30,30);
  killb->setEnabled(false);
  connect(killb, SIGNAL(clicked()),  SLOT(kill_clicked()) );

  QGridLayout* gridlayout = new QGridLayout( pframe, 4,3,5,5 );

  gridlayout->addWidget( prodlabel,0,0 );
  gridlayout->addWidget( prodbox,0,1 );
  gridlayout->addWidget( idlabel,1,0 );
  gridlayout->addWidget( idbox,1,1 );
  gridlayout->addWidget( loginb,2,0 );
  gridlayout->addWidget( loginlabel,2,1 );
  gridlayout->addMultiCellWidget( availlabel,3,3,0,1);
  gridlayout->addWidget( killb,3,2);

  // TAB-widget
  twd = new QTabWidget( this );

  // NORMAL-TAB -------------------------------------------------
  QWidget* normaltab = new QWidget(twd);

  QVBoxLayout* normallayout = new QVBoxLayout(normaltab);

  QLabel* normallabel= new QLabel( tr("Make product from:"), normaltab );
  QGridLayout* fieldlayout = new QGridLayout(3,2);
  for (int i=0; i<maxelements; i++){
    ebut[i]= new QPushButton(tr("Objects"),normaltab);
    elab[i]= new QLabel((i==0 ? tr("No startobjects"):tr("Field undefined")),normaltab);
    elab[i]->setMinimumWidth(170);
    fieldlayout->addWidget( ebut[i],i,0 );
    fieldlayout->addWidget( elab[i],i,1 );
  }

  connect(ebut[0], SIGNAL(clicked()), SLOT(ebutton0()));
  connect(ebut[1], SIGNAL(clicked()), SLOT(ebutton1()));
  connect(ebut[2], SIGNAL(clicked()), SLOT(ebutton2()));
  connect(ebut[3], SIGNAL(clicked()), SLOT(ebutton3()));

  // time widgets
  QLabel* timelabel= TitleLabel(tr("Product validity time:"), normaltab );
  QHBoxLayout* h7layout = new QHBoxLayout();
  h7layout->addWidget(timelabel,0,Qt::AlignHCenter);

  timespin= new TimeSpinbox(false, normaltab);
  connect(timespin, SIGNAL(valueChanged(int)), SLOT(prodtimechanged(int)));
  QHBoxLayout* h2layout = new QHBoxLayout( 5 );
  h2layout->addWidget(timespin,0,Qt::AlignHCenter);

  normallayout->addWidget(normallabel);
  normallayout->addLayout(fieldlayout);
  normallayout->addLayout( h7layout);
  normallayout->addLayout( h2layout);
  normallayout->activate();

  twd->addTab( normaltab, TABNAME_NORMAL );

  // COMBINE-TAB -------------------------------------------------

  combinetab = new QWidget(twd);

  QVBoxLayout* combinelayout = new QVBoxLayout(combinetab);

  QLabel* combinelabel= TitleLabel(tr("Combine products valid at:"), combinetab );
  combinelabel->setEnabled(false);
  cBox=new QListWidget(combinetab);
  cBox->setMinimumHeight(100);
  connect(cBox, SIGNAL(itemClicked ( QListWidgetItem * ) ),
	  SLOT(combineSelect(QListWidgetItem * ) ));

  QLabel* ctimelabel= TitleLabel(tr("Product validity time"), combinetab );
  QHBoxLayout* h5layout = new QHBoxLayout( 5 );
  h5layout->addWidget(ctimelabel,0,Qt::AlignHCenter);

  cselectlabel= new QLabel(tr("Time undefined"), combinetab );
  QHBoxLayout* h3layout = new QHBoxLayout( 5 );
  h3layout->addWidget(cselectlabel,0,Qt::AlignHCenter);

  QLabel* cpid1label= TitleLabel( tr("Product Ids to combine:"),combinetab );
  QHBoxLayout* h6layout = new QHBoxLayout( 5 );
  h6layout->addWidget(cpid1label,0,Qt::AlignHCenter);

  cpid2label= new QLabel(tr("None"), combinetab );
  QHBoxLayout* h4layout = new QHBoxLayout( 5 );
  h4layout->addWidget(cpid2label,0,Qt::AlignHCenter);


  combinelayout->addWidget(combinelabel);
  combinelayout->addWidget(cBox);
  combinelayout->addLayout(h5layout);
  combinelayout->addLayout(h3layout);
  combinelayout->addLayout(h6layout);
  combinelayout->addLayout(h4layout);
  combinelayout->activate();

  twd->addTab( combinetab, tr("Combine") );

  connect( twd, SIGNAL(selected( const QString& )),
	   SLOT( tabSelected( const QString& ) ));

  // TAB-setup ended

  // lower buttons
  QHBoxLayout* hlayout = new QHBoxLayout( 5 );
  ok= NormalPushButton( tr("OK"), this);

  help = NormalPushButton(tr("Help"), this );

  QPushButton* cancel= NormalPushButton( tr("Cancel"), this);

  hlayout->addWidget( help );
  hlayout->addWidget( cancel );
  connect( ok, SIGNAL(clicked()),  SLOT(ok_clicked()) );
  connect( help, SIGNAL(clicked()),  SLOT(help_clicked()) );
  connect( cancel, SIGNAL(clicked()), SLOT(cancel_clicked()) );

  // major layout
  QVBoxLayout* vlayout = new QVBoxLayout( this, 5, 5 );

  vlayout->addWidget( pframe );
  vlayout->addSpacing(10);
  vlayout->addWidget(twd);
  vlayout->addSpacing(15);
  vlayout->addWidget(ok);
  vlayout->addSpacing(5);
  vlayout->addLayout( hlayout );

  vlayout->activate();
  vlayout->freeze();

  first= false;
  newActive = false;
}//end constructor EditNewDialog


void EditNewDialog::tabSelected(const QString& name)
{
//  cerr << "EditNewDialog::Selected tab:" << name.toStdString() << endl;
  normal= (name==TABNAME_NORMAL);

  if (normal)
    ok->setText(tr("OK Start") + " " + prodbox->currentText());
  else
    ok->setText(tr("OK Combine") + " " + prodbox->currentText());

  if (!normal){
    load_combine();
    //in combine, should not be possible to change time...
    timespin->setEnabled(false);
    if(cBox->currentItem()){
      combineSelect(cBox->currentItem());
    }
  }
  else{
    setNormal();
    timespin->setEnabled(true);
  }
  checkStatus();
}

void EditNewDialog::combineSelect(QListWidgetItem * item)
{
  miString s= item->text().toStdString();
//  cerr << "EditNewDialog::Combineselect:" << s << endl;
  if (miTime::isValid(s)){
    combinetime= miTime(s);
   }
  cselectlabel->setText(s.cStr());
  miString tmp;
  vector <miString> pids =
    m_editm->getCombineIds(combinetime,products[currprod],pid);
  int n = pids.size();
  for (int i=0;i<n-1;i++) tmp+=pids[i]+", ";
  if ( n > 0 ) {
      tmp+=pids[n-1];
  }
  cpid2label->setText(tmp.cStr());
}


void EditNewDialog::combineClear(){
  cselectlabel->setText(tr("Time undefined"));
  cpid2label->setText(tr("None"));
}


void EditNewDialog::prodtimechanged(int v)
{
  prodtime= timespin->Time();
  //  cerr << "EditNewDialog::Prodtime changed:" << prodtime << endl;
  productfree= checkProductFree();
}


bool EditNewDialog::checkStatus()
{
  ok->setEnabled(false);
  if (currprod<0) return false;

  // first normal-area
  bool enable= normal
    && (currprod>=0)
    && (!pid.sendable || dbi.loggedin)
    && (!pid.sendable || productfree);

  for (int i=0; i<maxelements; i++){
    ebut[i]->setEnabled(enable);
  }

  // then check if ok to start
  if (pid.sendable && !dbi.loggedin)
    return false;
  if (pid.sendable && !productfree)
    return false;
  if (!normal){
    // combine products
    if (!isdata)
      return false;
  } else {
    // normal production
    for (int i=0; i<products[currprod].fields.size(); i++){
      if ((products[currprod].fields[i].fromfield &&
	   products[currprod].fields[i].fromfname.empty()) ||
	  (!products[currprod].fields[i].fromfield &&
	   products[currprod].fields[i].fromprod.ptime.undef()))
	return false;
    }
  }
  ok->setEnabled(true);
  return true;
}

void EditNewDialog::prodBox(int idx)
{
  //cerr << "EditNewDialog::prodBox called" << endl;
  int n= products.size();
  if (n==0 || idx<0 || idx>=n) return;

  if (normal)
    ok->setText(tr("OK Start") + " " + prodbox->currentText());
  else
    ok->setText(tr("OK Combine") + " " + prodbox->currentText());

  currprod= idx;
  //cerr << "....Selected Product:" << products[idx].name << endl;

  idbox->clear();
  int m= products[currprod].pids.size();
  int sid= 0;
  for (int i=0; i<m; i++){
    idbox->insertItem(products[currprod].pids[i].name.c_str());
    if (products[currprod].pids[i].name==pid.name)
      sid= i;
  }

  if (m>0) {
    idbox->setEnabled(true);
    idbox->setCurrentItem(sid);
    idBox(sid);
  } else {
    idbox->setEnabled(false);
    productfree= false;
  }

  if (dbi.host!=products[currprod].dbi.host){
    dbi= products[currprod].dbi;
    dbi.loggedin= false;
    // set login-label
    setLoginLabel();
  }

  // make list of fields etc.
  setNormal();

  if (!normal){
    load_combine();
  }

  checkStatus();
}

void EditNewDialog::idBox(int idx)
{
  //cerr << "EditNewDialog::idBox" << endl;
  int n= products.size();
  if (n==0 || currprod<0) return;
  int m= products[currprod].pids.size();
  if (idx<0 || idx>=m) return;

  pid= products[currprod].pids[idx];
#ifdef METNOPRODDB
  loginb->setEnabled(pid.sendable);
#endif

  productfree= checkProductFree();

  //cerr << "....Selected Pid:" << products[currprod].pids[idx].name << endl;

  if(!pid.combinable){
    if (twd->currentPageIndex()!=0) twd->setCurrentPage(0);
    twd->setTabEnabled(combinetab, false);
  } else
    twd->setTabEnabled(combinetab, true);

  if (!normal) load_combine();

  checkStatus();
}


bool EditNewDialog::load(editDBinfo& edbi){
  //cerr << "EditNewDialog::load" << endl;
  isdata= false;
  productfree= false;
  newActive=true;
  dbi= edbi;
  setLoginLabel();

  if( m_ctrl && m_editm ){
    products= m_editm->getEditProducts();
    prodbox->clear();
    int n= products.size();
    for (int i=0; i<n; i++){
      prodbox->insertItem(products[i].name.c_str());
      int m= products[i].fields.size();
      products[i].objectprods.clear();
      for (int j=0; j<m; j++){
	products[i].fields[j].fromfield= true;

	vector<miString> fstr=
	  m_editm->getValidEditFields(products[i],j);
	if (fstr.size())
	  products[i].fields[j].fromfname= fstr[0];
	else
	  products[i].fields[j].fromfname.clear();
     }
    }
    if (n>0) {
      prodbox->setEnabled(true);
      prodbox->setCurrentItem(0);
      prodBox(0);
    }
    m_ctrl->getPlotTime(prodtime);
    //set to closest hour
    miTime t= prodtime;
    int nhour=t.hour(); int nmin=t.min();
    if (nmin>29) nhour++;
    t.setTime(t.year(),t.month(),t.day(),nhour);
    prodtime=t;
    timespin->setTime(prodtime);

  } else {
    //cerr << "EditNewDialog::load - No controller!"<<endl;
    checkStatus();
    return false;
  }

  checkStatus();
  return true;
}


miString EditNewDialog::savedProd2Str(const savedProduct& sp,
				      const miString undef)
{
  if (sp.ptime.undef())
    return undef;
  else
    return sp.pid + miString(" - ") + sp.ptime.isoTime();
}


bool EditNewDialog::setNormal()
{
  //cerr << "EditNewDialog::setNormal called" << endl;
  isdata= false;
  for (int i=1; i<maxelements; i++){
    ebut[i]->hide();
    elab[i]->hide();
  }

  if (currprod<0) {
    ebut[0]->setEnabled(false);
    checkStatus();
    return false;
  }
  ebut[0]->setEnabled(true);

  int n= products[currprod].fields.size();
  for (int i=0; i<n && i<maxelements-1; i++){
    ebut[i+1]->show();
    elab[i+1]->show();
    ebut[i+1]->setText(products[currprod].fields[i].name.c_str());
  }

  setObjectLabel();
  setFieldLabel();

  checkStatus();
  return true;
}


void EditNewDialog::setObjectLabel(){
  // set object label
  miString tmp =
    miString("<font color=\"blue\"> ") + tr("No startobjects").toStdString() + miString(" </font> ");
  if (products[currprod].objectprods.size()){
    if (products[currprod].objectprods[0].filename.exists()){
      tmp= miString("<font color=\"red\"> ") +
	savedProd2Str(products[currprod].objectprods[0]) +
	miString(" </font> ");
    }
  }
  elab[0]->setText(tmp.cStr());
}


void EditNewDialog::setFieldLabel(){
  // set field labels
  int n= products[currprod].fields.size();
  for (int i=0; i<n && i<maxelements-1; i++){

    miString s;
    if (products[currprod].fields[i].fromfield){
      if (products[currprod].fields[i].fromfname.exists())
	s= miString("<font color=\"blue\"> ") +
	  products[currprod].fields[i].fromfname + miString(" </font> ");
      else
	s= miString("<font color=\"blue\"> ") + tr("Field undefined").toStdString() +
	  miString(" </font> ");
    }
    else
      s= miString("<font color=\"red\"> ") +
	savedProd2Str(products[currprod].fields[i].fromprod) +
	miString(" </font> ");

    elab[i+1]->setText(s.cStr());
  }

}


void EditNewDialog::handleObjectButton(int num)
{
  if( m_editm ){

    EditDefineFieldDialog edf(this,m_ctrl,-1,products[currprod]);
    if (!edf.exec()) return;

    if (edf.productSelected()){
      products[currprod].objectprods=edf.vselectedProd();
    } else {
      products[currprod].objectprods.clear();
    }
  } else {
    cerr << "EditNewDialog::handleelementButton - No controller!"<<endl;
  }
  setObjectLabel();
  checkStatus();
}

void EditNewDialog::handleFieldButton(int num)
{
  if( m_editm ){
    EditDefineFieldDialog edf(this,m_ctrl, num, products[currprod]);
    if (!edf.exec()) return;

    miString s;
    if (edf.fieldSelected()){
      products[currprod].fields[num].fromfname= edf.selectedField();
      products[currprod].fields[num].fromfield= true;
    } else if (edf.productSelected()){
      vector <savedProduct> vsap= edf.vselectedProd();
      if (vsap.size()){
	products[currprod].fields[num].fromprod= vsap[0];
	if (num==0){
	  //product time=time of field 0 !
	  miTime t = vsap[0].ptime;
	  prodtime= miTime(t);
	  timespin->setTime(prodtime);
	}
      }
      products[currprod].fields[num].fromfield= false;
    } else {
      products[currprod].fields[num].fromfname= "";
      products[currprod].fields[num].fromfield= true;
    }

  } else {
    //cerr << "EditNewDialog::handleelementButton - No controller!"<<endl;
  }
  setFieldLabel();
  checkStatus();
}

void EditNewDialog::ebutton0()
{
  handleObjectButton(0);
}

void EditNewDialog::ebutton1()
{
  handleFieldButton(0);
}

void EditNewDialog::ebutton2()
{
  handleFieldButton(1);
}

void EditNewDialog::ebutton3()
{
  handleFieldButton(2);
}

bool EditNewDialog::load_combine(){
  //cerr << "EditNewDialog::Load-combine" << endl;
  isdata= false;
  if (pid.sendable && !dbi.loggedin) {
    checkStatus();
    return false;
  }
  if( m_editm ){
    vector<miTime> vt= m_editm->getCombineProducts(products[currprod],pid);
    int n= vt.size();
    int index=0;
    cBox->clear();
    if (n>0) {
      for (int i=0; i<n; i++){
	miString tstr= vt[i].isoTime();
	cBox->addItem(QString(tstr.cStr()));
	if (combinetime ==vt[i]) index=i; //selected time
      }
      cBox->setCurrentRow(index);
    } else {
      cerr << "EditNewDialog::load - no analyses found"<<endl;
      checkStatus();
      combineClear();
      return false;
    }

  } else {
    //cerr << "EditNewDialog::load - No controller!"<<endl;
    checkStatus();
    return false;
  }

  isdata= (cBox->count() > 0);
  checkStatus();
  return true;
}


bool EditNewDialog::checkProductFree()
{
  killb->setEnabled(false);

  if (!pid.sendable){
    availlabel->setText("<font color=\"#009900\"> ---------- </font>");
    return true;
  }

  if (!dbi.loggedin || (currprod<0)){
    availlabel->setText("<font color=\"red\"> ---------- </font>");
    return false;
  }

  // send request to controller
  miString message;
  bool res= m_editm->checkProductAvailability(products[currprod].db_name,
					      pid.name,
					      prodtime,
					      message);
  if (res){
    availlabel->setText("<font color=\"#009900\">" + tr("product free") +"</font>");
    return true;
  } else {
    killb->setEnabled(true);
    availlabel->setText(message.c_str());
    return false;
  }
}

void EditNewDialog::kill_clicked()
{
  miString message=tr("Are you sure you want to take over this product?\n").toStdString();

  QMessageBox *mb= new QMessageBox(tr("Warning!"),
				   message.cStr(),
				   QMessageBox::Warning,
				   QMessageBox::Yes,
				   QMessageBox::Cancel | QMessageBox::Default,
				   Qt::NoButton,
				   this);
  mb->setButtonText( QMessageBox::Yes, tr("Yes") );
  mb->setButtonText( QMessageBox::Cancel, tr("Cancel"));

  switch( mb->exec() ) {
  case QMessageBox::Cancel:
    return;
    break;
  }

  bool res= m_editm->killProduction(products[currprod].db_name,
				    pid.name,
				    prodtime,
				    message);


  if (res) productfree= checkProductFree();
  checkStatus();
}


void EditNewDialog::setLoginLabel()
{
  if (dbi.loggedin){
    miString ltext;
    int i= dbi.host.find_first_of(".");
    if (i>0) ltext= dbi.host.substr(0, i);
    else ltext= dbi.host;
    ltext+= miString(" - ") + dbi.user;
    loginlabel->setText(QString(ltext.c_str()));
  } else {
    loginlabel->setText("--------------");
  }
}


void EditNewDialog::login_clicked()
{
#ifdef METNOPRODDB
  LoginDialog db(dbi,this);
  while (1) {
    if (!db.exec()) {
      emit newLogin(dbi);
      checkStatus();
      return;
    } else {
      dbi= db.getDbInfo();
    }

    miString message;
    if (!dbi.user.exists() || !dbi.host.exists()){
      message= tr("Username and server required").toStdString();
      dbi.loggedin= false;
    } else {
//       cerr << "Trying host:" << dbi.host << " User:" << dbi.user << " Pass:"
// 	   << dbi.pass << endl;
      dbi.loggedin= m_editm->loginDatabase(dbi,message);
    }
    if (dbi.loggedin) {
//       cerr << "Success...logged in" << endl;
      emit newLogin(dbi);

      setLoginLabel();

      // check if product free
      productfree= checkProductFree();

      if (!normal) load_combine();
      // check if enable any widgets
      checkStatus();

      return;
    } else {
      message= miString(tr("Can not log in. Message from server:\n").toStdString()) +message;
      QMessageBox::warning( this, "Diana database message",
			    message.c_str());

      setLoginLabel();
    }
  }
#endif
}

void EditNewDialog::ok_clicked(){
  productfree= checkProductFree();

  miTime ptime;
  if (normal) ptime= prodtime;
  else        ptime= combinetime;
  int minutes= miTime::minDiff(miTime::nowTime(),ptime);
  //   miString msg;
//   if ((products[currprod].startEarly &&
//        products[currprod].minutesStartEarly>minutes))
//     msg= "Produktet lages nå tidligere enn normalt!";
//   if ((products[currprod].startLate &&
//        products[currprod].minutesStartLate<minutes))
//     msg= "Produktet lages nå seinere enn normalt!";
//   if (msg.exists()) {
//     miString pname= prodbox->currentText().toStdString();
//     miString message= pname + "\n" + msg;
//     if (QMessageBox::warning( this, "Tid for produkt",message.c_str(),
//     			     "Fortsett","Avbryt") != 0) return;
//   }


  QString msg;
  if ((products[currprod].startEarly &&
       products[currprod].minutesStartEarly>minutes))
    msg= tr("Product made earlier than normal!");
  if ((products[currprod].startLate &&
       products[currprod].minutesStartLate<minutes))
    msg= tr("Product made later than normal!");
  if (!msg.isEmpty()) {
    QString pname= prodbox->currentText();
    QString message= pname + "\n" + msg;
    if (QMessageBox::warning( this, tr("Product time"),message,
			      tr("Continue"),tr("Cancel")) != 0) return;
  }


  if (!productfree){
    // give error
    //miString message= "Produktet er ikke tilgjengelig.\n Kan ikke starte produksjon";
      QMessageBox::warning( this, tr("Diana database message"),
			    tr("Product not available.\n Can not start production"));
    return;
  }
  this->hide();
  if (normal){
    if (currprod>=0)
      emit EditNewOk(products[currprod],pid,prodtime);

  } else {
    if (currprod>=0)
      emit EditNewCombineOk(products[currprod],pid,combinetime);
  }
  newActive=false;
}



void EditNewDialog::help_clicked(){
  emit EditNewHelp();
}


void EditNewDialog::cancel_clicked(){
  this->hide();
  emit EditNewCancel();
  newActive=false;
}


void EditNewDialog::closeEvent( QCloseEvent* e) {
  cancel_clicked();
}
