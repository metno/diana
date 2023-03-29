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

#include <QComboBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QFont>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QString>
#include <QToolTip>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "qtUtility.h"
#include "qtEditDefineField.h"
#include "diEditManager.h"

#define MILOGGER_CATEGORY "diana.EditDefineFieldDialog"
#include <miLogger/miLogging.h>


/*********************************************/
EditDefineFieldDialog::EditDefineFieldDialog(QWidget* parent, EditManager* editm, int n, EditProduct ep)
    : QDialog(parent)
    , m_editm(editm)
    , EdProd(ep)
    , num(n)
{
  METLIBS_LOG_SCOPE();
  setModal(true);

  if (num==-1){
    fieldname= tr("Objects").toStdString();
    setWindowTitle(tr("Pick objects for editing"));
  }
  else if (num < int(EdProd.fields.size())){
    fieldname=EdProd.fields[num].name;
    setWindowTitle(tr("Pick fields for editing"));
  }

  MODELFIELDS = tr("Model fields").toStdString();

  std::string txt= fieldname + " " + std::string(tr("from:").toStdString());
  QLabel* mainlabel= TitleLabel( txt.c_str(), this );

  productNames=getProductNames();
  prodnamebox = ComboBox( this, productNames, true, 0);
  connect( prodnamebox, SIGNAL( activated(int) ),
      SLOT( prodnameActivated(int) )  );


  fBox= new QListWidget(this);

  connect(fBox, SIGNAL(itemClicked(QListWidgetItem *)),
      SLOT(fieldselect(QListWidgetItem *)));


  QString xps= tr("Official product") + " -- <i>" +
      tr("Locally stored") ;
  QLabel* xplabel= new QLabel(xps, this);

  QVBoxLayout* vlayout = new QVBoxLayout( this);

  vlayout->addWidget( mainlabel );
  vlayout->addWidget( prodnamebox );
  vlayout->addWidget( fBox );
  vlayout->addSpacing(10);


  vlayout->addSpacing(10);
  vlayout->addWidget( xplabel );
  vlayout->addSpacing(10);

  QFrame *line;
  // Create a horizontal frame line
  line = new QFrame( this );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  vlayout->addWidget( line );


  //*** the box (with label) showing which files/fields have been choosen ****
  if (num==-1)
    filesLabel = TitleLabel(tr("Selected objects"), this);
  else
    filesLabel = TitleLabel(tr("Selected fields"), this);
  filenames = new QListWidget( this );
  connect(filenames, SIGNAL(itemClicked( QListWidgetItem *)),
      SLOT(filenameSlot(QListWidgetItem *)));

  vlayout->addWidget( filesLabel );
  vlayout->addWidget( filenames );


  if (num==-1){
    //*****  Checkboxes for selecting fronts/symbols/areas  **********

    cbs0= new QCheckBox(tr("Fronts"), this);
    cbs1= new QCheckBox(tr("Symbols"),this);
    cbs2= new QCheckBox(tr("Areas"),  this);
    cbs3= new QCheckBox(tr("Form"),   this);

    connect(cbs0, SIGNAL(clicked()), SLOT(cbsClicked()));
    connect(cbs1, SIGNAL(clicked()), SLOT(cbsClicked()));
    connect(cbs2, SIGNAL(clicked()), SLOT(cbsClicked()));
    connect(cbs3, SIGNAL(clicked()), SLOT(cbsClicked()));
    initCbs();

    vlayout->addWidget(cbs0);
    vlayout->addWidget(cbs1);
    vlayout->addWidget(cbs2);
    vlayout->addWidget(cbs3);
  }

  //push buttons to delete all selections
  Delete = new QPushButton(tr("Delete"), this);
  connect( Delete, SIGNAL(clicked()), SLOT(DeleteClicked()));

  //push button to refresh filelists
  refresh = new QPushButton(tr("Refresh"), this);
  connect( refresh, SIGNAL( clicked() ), SLOT( Refresh() ));

  //place  "delete" and "refresh" buttons in hor.layout
  QHBoxLayout* h0layout = new QHBoxLayout();
  h0layout->addWidget( Delete );
  h0layout->addWidget( refresh );

  QHBoxLayout* hlayout = new QHBoxLayout();

  ok = new QPushButton(tr("OK"), this);
  QPushButton* cancel = new QPushButton(tr("Cancel"), this);
  hlayout->addWidget( ok );
  hlayout->addWidget( cancel );
  connect( ok, SIGNAL(clicked()),  SLOT(accept()) );
  connect( cancel, SIGNAL(clicked()), SLOT(reject()) );
  ok->setEnabled(false);

  vlayout->addLayout( h0layout );
  vlayout->addLayout( hlayout );

  //check existing selections for product
  if (num==-1 && EdProd.objectprods.size())
    vselectedprod=EdProd.objectprods;
  else if (num>-1 && num < int(EdProd.fields.size())){
    if (EdProd.fields[num].fromfield)
      selectedfield=EdProd.fields[num].fromfname;
    else
      vselectedprod.push_back(EdProd.fields[num].fromprod);
  }

  selectedProdIndex=-1;

  //update filenames with existing selections
  updateFilenames();

  if (num>-1 && fields.size()==0){
    if (prodnamebox->count() > 1)
      prodnamebox->setCurrentIndex(1);
    prodnameActivated(1);
  }
  else
    prodnameActivated(0);
}

/*********************************************/

std::vector<std::string> EditDefineFieldDialog::getProductNames()
{
  METLIBS_LOG_SCOPE();

  std::vector <std::string> name;
  if (!m_editm)
    return name;
  //get fields
  if (num>-1){
    name.push_back(MODELFIELDS);
    fields= m_editm->getValidEditFields(EdProd,num);
  }
  name.push_back(EdProd.name);
  std::vector<savedProduct> sp = m_editm->getSavedProducts(EdProd, num);
  pmap[EdProd.name]=sp;
  std::vector<std::string> products = m_editm->getEditProductNames();
  int n = products.size();
  for (int i =0;i<n;i++){
    if (products[i]==EdProd.name) continue;
    EditProduct epin;
    if (m_editm->findProduct(epin,products[i])){
      std::vector<savedProduct> spin;
      if (num>-1)
        spin = m_editm->getSavedProducts(epin,EdProd.fields[num].name);
      else
        spin = m_editm->getSavedProducts(epin,num);
      if (spin.size()){
        name.push_back(epin.name);
        pmap[epin.name]=spin;
      }
    }
  }
  return name;
}

/*********************************************/

void EditDefineFieldDialog::prodnameActivated(int iprod){
  METLIBS_LOG_SCOPE();
  if (int(productNames.size())>iprod){
    currentProductName= productNames[iprod];
    fillList();
  }
}

/*********************************************/

void EditDefineFieldDialog::fillList()
{
  METLIBS_LOG_SCOPE();
  if (currentProductName.empty()) return;
  fBox->clear();
  fBox->clearFocus();
  if (currentProductName==MODELFIELDS){
    for (unsigned int i=0; i<fields.size(); i++){
      fBox->addItem(QString(fields[i].c_str()));
    }
  } else {
    std::vector <savedProduct> splist = pmap[currentProductName];
    for (unsigned int i=0; i<splist.size(); i++){
      std::string str;
      if ( splist[i].ptime.undef() ) {
        str = splist[i].pid + std::string(" - ") + splist[i].filename;
      } else {
        str = splist[i].pid + std::string(" - ") +splist[i].ptime.isoTime()+ " - " +splist[i].filename;
      }
      QListWidgetItem* item = new QListWidgetItem(QString(str.c_str()));
      if(splist[i].localSource) {
        QFont font = item->font();
        font.setItalic(true);
        item->setFont(font);
      }
      fBox->addItem(item);
    }
  }
}

/*********************************************/

void EditDefineFieldDialog::fieldselect(QListWidgetItem* item)
{
  METLIBS_LOG_SCOPE();
  int i =fBox->row(item);
  if (currentProductName==MODELFIELDS){
    selectedfield= fields[i];
    //clear selected products, not possible to have both fields and products
    vselectedprod.clear();
    updateFilenames();
    if (filenames->count()) filenames->item(0)->setSelected(true);
  } else {
    std::vector <savedProduct> splist = pmap[currentProductName];
    savedProduct selectedprod = splist[i];
    //check this savedproduct not already selected
    std::vector<savedProduct>::iterator p=vselectedprod.begin();
    for (; p!=vselectedprod.end() && selectedprod.filename!=p->filename;p++);
    if (p==vselectedprod.end()){
      initCbs();
      selectedprod.selectObjectTypes=selectedObjectTypes();
      if (num>-1) vselectedprod.clear(); //only one field !
      vselectedprod.push_back(selectedprod);
      selectedProdIndex=vselectedprod.size()-1;
    }
    //clear selected field, not possible to have both fields and products
    selectedfield.clear();
    updateFilenames();
    if (selectedProdIndex>-1 && selectedProdIndex < filenames->count())
      filenames->item(selectedProdIndex)->setSelected(true);
  }
  ok->setEnabled(true);
}

/*********************************************/

void EditDefineFieldDialog::updateFilenames()
{
  METLIBS_LOG_SCOPE();
  filenames->clear();
  if (fieldSelected()){
    std::string namestr=selectedfield;
    if (!namestr.empty()) {
      filenames->addItem(QString(namestr.c_str()));
    }
  }
  if (productSelected()){
    for (unsigned int i = 0;i<vselectedprod.size();i++){
      std::string namestr=m_editm->savedProductString(vselectedprod[i]);
      if (!namestr.empty()) {
        filenames->addItem(QString(namestr.c_str()));
      }
    }
  }

  if ( filenames->count() ) {
    filenames->item(0)->setSelected(true);
    selectedProdIndex = 0;
  }
}

/*********************************************/

void EditDefineFieldDialog::filenameSlot(QListWidgetItem* item)
{
  METLIBS_LOG_SCOPE();
  selectedProdIndex=filenames->row(item);
  if (num == -1) {
    std::map<std::string, bool> useEditobject = WeatherObjects::decodeTypeString(vselectedprod[selectedProdIndex].selectObjectTypes);
    setCheckedCbs(useEditobject);
  }
}

/*********************************************/
void EditDefineFieldDialog::DeleteClicked(){
  METLIBS_LOG_SCOPE();
  if (fieldSelected()){
    selectedfield.clear();
    updateFilenames();
  } else if (productSelected() && selectedProdIndex > -1 && selectedProdIndex < int(vselectedprod.size())) {
    vselectedprod.erase(vselectedprod.begin()+selectedProdIndex);
    selectedProdIndex--;
    if (selectedProdIndex<0 && vselectedprod.size()) selectedProdIndex=0;
    updateFilenames();
    if (selectedProdIndex>-1 && selectedProdIndex < filenames->count())
      filenames->item(selectedProdIndex)->setSelected(true);
    else
      initCbs();
  }
  ok->setEnabled(true);
  //refresh List, clear selection
  fillList();
}

/*********************************************/

void EditDefineFieldDialog::Refresh()
{
  METLIBS_LOG_SCOPE();
  getProductNames();
  fillList();
}

/***********************************************************/

void EditDefineFieldDialog::cbsClicked()
{
  METLIBS_LOG_SCOPE();
  if (num > -1)
    return;
  if (productSelected()){
    if (selectedProdIndex > -1 && selectedProdIndex < int(vselectedprod.size())){
      vselectedprod[selectedProdIndex].selectObjectTypes=selectedObjectTypes();
      updateFilenames();
      if (selectedProdIndex>-1 && selectedProdIndex < filenames->count())
        filenames->item(selectedProdIndex)->setSelected(true);
    }
  }
  if (vselectedprod.size() && selectedProdIndex > -1)
    ok->setEnabled(true);
}

void EditDefineFieldDialog::setCheckedCbs(const std::map<std::string, bool>& useEditobject)
{
  if (num > -1)
    return;
  cbs0->setChecked(useEditobject.find("front") != useEditobject.end());
  cbs1->setChecked(useEditobject.find("symbol") != useEditobject.end());
  cbs2->setChecked(useEditobject.find("area") != useEditobject.end());
  cbs3->setChecked(useEditobject.find("anno") != useEditobject.end());
}

void EditDefineFieldDialog::initCbs()
{
  if (num > -1)
    return;
  cbs0->setChecked(true);
  cbs1->setChecked(true);
  cbs2->setChecked(true);
  cbs3->setChecked(true);
}


std::string EditDefineFieldDialog::selectedObjectTypes()
{
  //fronts /symbols/areas ?
  if (num>-1)
    return " ";

  std::string str;
  if (cbs0->isChecked()) str+="front,";
  if (cbs1->isChecked()) str+="symbol,";
  if (cbs2->isChecked()) str+="area,";
  if (cbs3->isChecked()) str+="anno";

  return str;
}
