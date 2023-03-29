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

#include "qtEditNewDialog.h"
#include "qtUtility.h"
#include "qtEditDefineField.h"
#include "qtTimeSpinbox.h"

#include "diController.h"
#include "diEditManager.h"

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

#define MILOGGER_CATEGORY "diana.EditNewDialog"
#include <miLogger/miLogging.h>


EditNewDialog::EditNewDialog(QWidget* parent, Controller* llctrl)
    : QDialog(parent)
    , m_ctrl(llctrl)
    , m_editm(m_ctrl->getEditManager())
{
  TABNAME_NORMAL = tr("Normal");

  normal= true;
  currprod= -1;
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

  QGridLayout* gridlayout = new QGridLayout(pframe);

  gridlayout->addWidget( prodlabel,0,0 );
  gridlayout->addWidget( prodbox,0,1 );
  gridlayout->addWidget( idlabel,1,0 );
  gridlayout->addWidget( idbox,1,1 );

  // TAB-widget
  twd = new QTabWidget( this );

  // NORMAL-TAB -------------------------------------------------
  QWidget* normaltab = new QWidget(twd);

  QVBoxLayout* normallayout = new QVBoxLayout(normaltab);

  QLabel* normallabel= new QLabel( tr("Make product from:"), normaltab );
  QGridLayout* fieldlayout = new QGridLayout();
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
  QHBoxLayout* h2layout = new QHBoxLayout();
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

  QLabel* ctimelabel= TitleLabel(tr("Product validity time"), combinetab);
  QHBoxLayout* h5layout = new QHBoxLayout();
  h5layout->addWidget(ctimelabel,0,Qt::AlignHCenter);

  cselectlabel= new QLabel(tr("Time undefined"), combinetab );
  QHBoxLayout* h3layout = new QHBoxLayout();
  h3layout->addWidget(cselectlabel,0,Qt::AlignHCenter);

  QLabel* cpid1label= TitleLabel( tr("Product Ids to combine:"),combinetab );
  QHBoxLayout* h6layout = new QHBoxLayout();
  h6layout->addWidget(cpid1label,0,Qt::AlignHCenter);

  cpid2label= new QLabel(tr("None"), combinetab );
  QHBoxLayout* h4layout = new QHBoxLayout();
  h4layout->addWidget(cpid2label,0,Qt::AlignHCenter);


  combinelayout->addWidget(combinelabel);
  combinelayout->addWidget(cBox);
  combinelayout->addLayout(h5layout);
  combinelayout->addLayout(h3layout);
  combinelayout->addLayout(h6layout);
  combinelayout->addLayout(h4layout);
  combinelayout->activate();

  twd->addTab( combinetab, tr("Combine") );

  connect( twd, SIGNAL(currentChanged( int )),
      SLOT( tabSelected( const int ) ));

  // TAB-setup ended

  // lower buttons
  QHBoxLayout* hlayout = new QHBoxLayout();
  ok = new QPushButton(tr("OK"), this);
  ok->setDefault( true );

  help = new QPushButton(tr("Help"), this);

  QPushButton* cancel = new QPushButton(tr("Cancel"), this);

  hlayout->addWidget( help );
  hlayout->addWidget( cancel );
  connect( ok, SIGNAL(clicked()),  SLOT(ok_clicked()) );
  connect( help, SIGNAL(clicked()),  SLOT(help_clicked()) );
  connect( cancel, SIGNAL(clicked()), SLOT(cancel_clicked()) );

  // major layout
  QVBoxLayout* vlayout = new QVBoxLayout( this );

  vlayout->addWidget( pframe );
  vlayout->addSpacing(10);
  vlayout->addWidget(twd);
  vlayout->addSpacing(15);
  vlayout->addWidget(ok);
  vlayout->addSpacing(5);
  vlayout->addLayout( hlayout );

  vlayout->activate();
  // vlayout->freeze();

  first= false;
  newActive = false;
}//end constructor EditNewDialog


void EditNewDialog::tabSelected(int tabindex)
{
  METLIBS_LOG_SCOPE();
  normal= (tabindex == 0);

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
  std::string s= item->text().toStdString();
  METLIBS_LOG_DEBUG("EditNewDialog::Combineselect:" << s);
  if (miutil::miTime::isValid(s)){
    combinetime= miutil::miTime(s);
  }
  cselectlabel->setText(s.c_str());
  std::string tmp;
  std::vector <std::string> pids =
      m_editm->getCombineIds(combinetime,products[currprod],pid);
  int n = pids.size();
  for (int i=0;i<n-1;i++) tmp+=pids[i]+", ";
  if ( n > 0 ) {
    tmp+=pids[n-1];
  }
  cpid2label->setText(tmp.c_str());
}


void EditNewDialog::combineClear(){
  cselectlabel->setText(tr("Time undefined"));
  cpid2label->setText(tr("None"));
}


void EditNewDialog::prodtimechanged(int v)
{
  prodtime= timespin->Time();
  METLIBS_LOG_DEBUG("EditNewDialog::Prodtime changed:" << prodtime);
}


bool EditNewDialog::checkStatus()
{
  ok->setEnabled(false);
  if (currprod<0) return false;

  if (!normal){
    // combine products
    if (!isdata)
      return false;
  } else {
    // normal production
    for (unsigned int i=0; i<products[currprod].fields.size(); i++){
      if ((products[currprod].fields[i].fromfield &&
          products[currprod].fields[i].fromfname.empty()) ||
          (!products[currprod].fields[i].fromfield &&
              products[currprod].fields[i].fromprod.filename.empty()))
        return false;
    }
  }
  ok->setEnabled(true);
  return true;
}

void EditNewDialog::prodBox(int idx)
{
  METLIBS_LOG_SCOPE();
  int n= products.size();
  if (n==0 || idx<0 || idx>=n) return;

  if (normal)
    ok->setText(tr("OK Start") + " " + prodbox->currentText());
  else
    ok->setText(tr("OK Combine") + " " + prodbox->currentText());

  currprod= idx;
  METLIBS_LOG_DEBUG("....Selected Product:" << products[idx].name);

  idbox->clear();
  int m= products[currprod].pids.size();
  int sid= 0;
  for (int i=0; i<m; i++){
    idbox->addItem(products[currprod].pids[i].name.c_str());
    if (products[currprod].pids[i].name==pid.name)
      sid= i;
  }

  if (m>0) {
    idbox->setEnabled(true);
    idbox->setCurrentIndex(sid);
    idBox(sid);
  } else {
    idbox->setEnabled(false);
    productfree= false;
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
  METLIBS_LOG_SCOPE();
  int n= products.size();
  if (n==0 || currprod<0) return;
  int m= products[currprod].pids.size();
  if (idx<0 || idx>=m) return;

  pid= products[currprod].pids[idx];

  METLIBS_LOG_DEBUG("....Selected Pid:" << products[currprod].pids[idx].name);

  if(!pid.combinable){
    if (twd->currentIndex()!=0) twd->setCurrentIndex(0);
    twd->setTabEnabled(1, false);
  } else
    twd->setTabEnabled(1, true);

  if (!normal) load_combine();

  checkStatus();
}


bool EditNewDialog::load(){
  METLIBS_LOG_SCOPE();
  isdata= false;
  productfree= false;
  newActive=true;

  if (m_editm) {
    products= m_editm->getEditProducts();
    prodbox->clear();
    if (products.empty())
      return false;

    for (auto& p : products) {
      prodbox->addItem(QString::fromStdString(p.name));
      const int m = p.fields.size();
      p.objectprods.clear();
      for (int j=0; j<m; j++){
        p.fields[j].fromfield = true;

        std::vector<std::string> fstr = m_editm->getValidEditFields(p, j);
        if (fstr.size())
          p.fields[j].fromfname = fstr[0];
        else
          p.fields[j].fromfname.clear();
      }
    }

    prodbox->setEnabled(true);
    prodbox->setCurrentIndex(0);
    prodBox(0);

    //set to closest hour
    const miutil::miTime& t = m_ctrl->getPlotTime();
    int nhour = t.hour();
    if (t.min() > 29)
      nhour++;
    prodtime = miutil::miTime(t.year(), t.month(), t.day(), nhour);
    timespin->setTime(prodtime);

  } else {
    METLIBS_LOG_DEBUG("EditNewDialog::load - No controller!");
    checkStatus();
    return false;
  }

  checkStatus();
  return true;
}


std::string EditNewDialog::savedProd2Str(const savedProduct& sp, const std::string undef)
{
  if (sp.ptime.undef())
    return sp.pid;
  else
    return sp.pid + std::string(" - ") + sp.ptime.isoTime();
}


bool EditNewDialog::setNormal()
{
  METLIBS_LOG_SCOPE();

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
  std::string tmp =
      std::string("<font color=\"blue\"> ") + tr("No startobjects").toStdString() + std::string(" </font> ");
  if (products[currprod].objectprods.size()){
    if (not products[currprod].objectprods[0].filename.empty()) {
      tmp= std::string("<font color=\"red\"> ") +
          savedProd2Str(products[currprod].objectprods[0]) +
          std::string(" </font> ");
    }
  }
  elab[0]->setText(tmp.c_str());
}


void EditNewDialog::setFieldLabel(){
  METLIBS_LOG_SCOPE();
  // set field labels
  int n= products[currprod].fields.size();
  for (int i=0; i<n && i<maxelements-1; i++){

    std::string s;
    if (products[currprod].fields[i].fromfield){
      if (not products[currprod].fields[i].fromfname.empty())
        s= std::string("<font color=\"blue\"> ") +
        products[currprod].fields[i].fromfname + std::string(" </font> ");
      else
        s= std::string("<font color=\"blue\"> ") + tr("Field undefined").toStdString() +
        std::string(" </font> ");
    }
    else
      s= std::string("<font color=\"red\"> ") +
      savedProd2Str(products[currprod].fields[i].fromprod) +
      std::string(" </font> ");

    elab[i+1]->setText(s.c_str());
  }

}

void EditNewDialog::handleObjectButton(int /*num*/)
{
  if (m_editm) {
    EditDefineFieldDialog edf(this, m_editm, -1, products[currprod]);
    if (!edf.exec())
      return;

    if (edf.productSelected()){
      products[currprod].objectprods=edf.vselectedProd();
    } else {
      products[currprod].objectprods.clear();
    }
  } else {
    METLIBS_LOG_WARN("EditNewDialog::handleelementButton - No controller!");
  }
  setObjectLabel();
  checkStatus();
}

void EditNewDialog::handleFieldButton(int num)
{
  METLIBS_LOG_SCOPE();
  if (m_editm) {
    EditDefineFieldDialog edf(this, m_editm, num, products[currprod]);
    if (!edf.exec())
      return;

    std::string s;
    if (edf.fieldSelected()){
      products[currprod].fields[num].fromfname= edf.selectedField();
      products[currprod].fields[num].fromfield= true;
    } else if (edf.productSelected()){
      std::vector <savedProduct> vsap= edf.vselectedProd();
      if (vsap.size()){
        products[currprod].fields[num].fromprod= vsap[0];
        if (num==0){
          //product time=time of field 0 !
          if ( !vsap[0].ptime.undef() ) {
            miutil::miTime t = vsap[0].ptime;
            prodtime= miutil::miTime(t);
            timespin->setTime(prodtime);
          } else {
            prodtime= miutil::miTime();
            timespin->setTime(prodtime);
          }
        }
      }
      products[currprod].fields[num].fromfield= false;
    } else {
      products[currprod].fields[num].fromfname= "";
      products[currprod].fields[num].fromfield= true;
    }

  } else {
    METLIBS_LOG_DEBUG("EditNewDialog::handleelementButton - No controller!");
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
  METLIBS_LOG_SCOPE();
  isdata= false;
  if( m_editm ){
    std::vector<miutil::miTime> vt= m_editm->getCombineProducts(products[currprod],pid);
    int n= vt.size();
    int index=0;
    cBox->clear();
    if (n>0) {
      for (int i=0; i<n; i++){
        std::string tstr= vt[i].isoTime();
        cBox->addItem(QString(tstr.c_str()));
        if (combinetime ==vt[i]) index=i; //selected time
      }
      cBox->setCurrentRow(index);
      combineSelect(cBox->currentItem());
    } else {
      METLIBS_LOG_WARN("EditNewDialog::load - no analyses found");
      checkStatus();
      combineClear();
      return false;
    }

  } else {
    METLIBS_LOG_DEBUG("EditNewDialog::load - No controller!");
    checkStatus();
    return false;
  }

  isdata= (cBox->count() > 0);
  checkStatus();
  return true;
}


bool EditNewDialog::checkProductFree()
{
  METLIBS_LOG_SCOPE(LOGVAL(prodtime));
  QString message;

  bool ok = m_editm->fileExists(products[currprod],pid,prodtime,message);
  if (!ok) {
    bool cancel =  QMessageBox::warning( this, tr("Product time"),message,
        tr("Continue"),tr("Cancel"));
    return !cancel;
  }
  return ok;

}

void EditNewDialog::ok_clicked(){
  METLIBS_LOG_SCOPE(LOGVAL(prodtime));
  miutil::miTime ptime;
  if (normal) ptime= prodtime;
  else        ptime= combinetime;
  if ( !ptime.undef() ) {
    int minutes= miutil::miTime::minDiff(miutil::miTime::nowTime(),ptime);

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

  }

  productfree= checkProductFree();
  if (!productfree){
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
