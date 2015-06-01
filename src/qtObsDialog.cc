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

#include "qtObsDialog.h"
#include "qtObsWidget.h"
#include "qtUtility.h"
#include "qtToggleButton.h"

#include <puTools/miStringFunctions.h>

#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLCDNumber>
#include <QSlider>
#include <QCheckBox>
#include <QStackedWidget>
#include <QFrame>
#include <QRadioButton>
#include <QLineEdit>
#include <QToolTip>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QButtonGroup>

#include <qpushbutton.h>
#include <math.h>

using namespace std;

ObsDialog::ObsDialog( QWidget* parent, Controller* llctrl )
  : QDialog(parent)
{
  setWindowTitle(tr("Observations"));

  m_ctrl=llctrl;
  ObsDialogInfo dialog = m_ctrl->initObsDialog() ;

  vector<ObsDialogInfo::PlotType> &plt = dialog.plottype;

  vector<std::string> dialog_name;
  nr_plot = plt.size();

  if( nr_plot==0 ) return;

  for (int i=0; i<nr_plot; i++){
    m_name.push_back(plt[i].name);
    //Possible to translate plot types for use in dialogues,
    //but plot strings etc  will use english names
    if(plt[i].name=="Pressure")
      dialog_name.push_back(tr("Pressure").toStdString());
    else
      dialog_name.push_back(plt[i].name);
  }
  savelog.clear();
  savelog.resize(nr_plot);

  m_selected = 0;

  plotbox = ComboBox( this, dialog_name,true,0);
  plotbox->setToolTip(tr("select plot type"));

  stackedWidget = new QStackedWidget;

  for( int i=0; i < nr_plot; i++){
    ObsWidget* ow = new ObsWidget( this );
    obsWidget.push_back(ow);
    if (dialog.plottype[i].button.size()>0) {
      obsWidget[i]->setDialogInfo( m_ctrl, dialog, i );
      connect(obsWidget[i],SIGNAL(getTimes()),
          SLOT(getTimes()));
      connect(obsWidget[i],SIGNAL(rightClicked(std::string)),
          SLOT(rightButtonClicked(std::string)));
      connect(obsWidget[i],SIGNAL(extensionToggled(bool)),
          SLOT(extensionToggled(bool)));
      connect(obsWidget[i],SIGNAL(criteriaOn()),
          SLOT(criteriaOn()));
    }
    stackedWidget->addWidget(obsWidget[i]);
  }

  for( int i=1; i < nr_plot; i++)
    if (obsWidget[i]) obsWidget[i]->hide();

  multiplot = false;

  multiplotButton = new ToggleButton(this, tr("Show all"));
  multiplotButton->setToolTip(tr("Show all plot types") );
  obshelp =  NormalPushButton( tr("Help"), this );
  obsrefresh = NormalPushButton( tr("Refresh"), this );
  obshide = NormalPushButton( tr("Hide"), this);
  obsapplyhide = NormalPushButton( tr("Apply + Hide"), this );
  obsapply = NormalPushButton( tr("Apply"), this);
  obsapply->setDefault( true );

  connect( multiplotButton,
      SIGNAL( toggled(bool)), SLOT( multiplotClicked( bool) ));
  connect( obshide, SIGNAL(clicked()), SIGNAL( ObsHide() ));
  connect( obsapply, SIGNAL(clicked()), SIGNAL( ObsApply() ));
  connect( obsrefresh, SIGNAL(clicked()), SLOT(getTimes()));
  connect( obsapplyhide, SIGNAL(clicked()), SLOT(applyhideClicked()));
  connect( obshelp, SIGNAL(clicked()), SLOT( helpClicked() ));


  QHBoxLayout* helplayout = new QHBoxLayout();
  helplayout->addWidget( obshelp );
  helplayout->addWidget( obsrefresh );
  helplayout->addWidget( multiplotButton );

  QHBoxLayout* applylayout = new QHBoxLayout();
  applylayout->addWidget( obshide );
  applylayout->addWidget(obsapplyhide );
  applylayout->addWidget( obsapply );

  QVBoxLayout* vlayout= new QVBoxLayout( this);
  vlayout->setSpacing(1);
  vlayout->addWidget( plotbox );
  vlayout->addWidget( stackedWidget );
  vlayout->addLayout( helplayout );
  vlayout->addLayout( applylayout );

  connect( plotbox, SIGNAL( activated(int) ), SLOT( plotSelected(int) ) );

  plotbox->setFocus();

  this->hide();
  //  setOrientation(Horizontal);
  setOrientation(Qt::Horizontal);
  makeExtension();
  setExtension(extension);
  showExtension(false);

}

void ObsDialog::updateDialog()
{
  //save selections
  vector<string> vstr = getOKString();

  //remove old widgets
  for( int i=0; i < nr_plot; i++){
    stackedWidget->removeWidget(obsWidget[i]);
    obsWidget[i]->close();
    obsWidget[i] = NULL;
  }
  obsWidget.clear();
  m_name.clear();

  //Make new widgets
  ObsDialogInfo dialog = m_ctrl->initObsDialog() ;

  QStringList  dialog_name;
  nr_plot = dialog.plottype.size();

  for (int i=0; i<nr_plot; i++){
    m_name.push_back(dialog.plottype[i].name);
    //Possible to translate plot types for use in dialogues,
    //but plot strings etc  will use english names
    if(dialog.plottype[i].name=="Pressure")
      dialog_name <<tr("Pressure");
    else
      dialog_name <<dialog.plottype[i].name.c_str();
  }

  m_selected = 0;

  plotbox->clear();
  plotbox->addItems(dialog_name);

  for( int i=0; i < nr_plot; i++){
    ObsWidget* ow = new ObsWidget( this );
    obsWidget.push_back(ow);
    if (dialog.plottype[i].button.size()>0) {
      obsWidget[i]->setDialogInfo( m_ctrl, dialog, i );
      connect(obsWidget[i],SIGNAL(getTimes()),
          SLOT(getTimes()));
      connect(obsWidget[i],SIGNAL(rightClicked(std::string)),
          SLOT(rightButtonClicked(std::string)));
      connect(obsWidget[i],SIGNAL(extensionToggled(bool)),
          SLOT(extensionToggled(bool)));
      connect(obsWidget[i],SIGNAL(criteriaOn()),
          SLOT(criteriaOn()));
    }
    stackedWidget->addWidget(obsWidget[i]);
  }

  for( int i=1; i < nr_plot; i++)
    if (obsWidget[i]) obsWidget[i]->hide();


  //reset selections
  putOKString( vstr );

}

void ObsDialog::plotSelected( int index, bool sendTimes )
{
  /* This function is called when a new plottype is selected and builds
   the screen up with a new obsWidget widget */

  if( m_selected == index && obsWidget[index]->initialized()) return;

  if (!obsWidget[index]->initialized()) {
    ObsDialogInfo dialog= m_ctrl->updateObsDialog(m_name[index]);

    obsWidget[index]->setDialogInfo( m_ctrl, dialog, index);
    connect(obsWidget[index],SIGNAL(getTimes()),
        SLOT(getTimes()));
    connect(obsWidget[index],SIGNAL(rightClicked(std::string)),
        SLOT(rightButtonClicked(std::string)));
    connect(obsWidget[index],SIGNAL(extensionToggled(bool)),
        SLOT(extensionToggled(bool)));
    connect(obsWidget[index],SIGNAL(criteriaOn()),
        SLOT(criteriaOn()));

    if (savelog[index].size()) {
      obsWidget[index]->readLog(savelog[index]);
      savelog[index].clear();
    }
  }

  //Emit empty time list
  vector<miutil::miTime> noTimes;
  emit emitTimes( "obs",noTimes );

  //criteria
  if(obsWidget[m_selected]->moreToggled()){
    showExtension(false);
  }

  m_selected = index;

  stackedWidget->setCurrentIndex(m_selected);

  //criteria
  if(obsWidget[m_selected]->moreToggled()){
    showExtension(true);
  }

  updateExtension();

  if(sendTimes)
    getTimes();
}

void ObsDialog::getTimes()
{
  // Names of datatypes selected are sent to controller,
  // and times are returned

  diutil::OverrideCursor waitCursor;

  vector<std::string> dataName;
  if(multiplot){

    set<std::string> nameset;
    for (int i=0; i<nr_plot; i++) {
      if (obsWidget[i]->initialized()) {
        vector<std::string> name=obsWidget[i]->getDataTypes();
        vector<std::string>::iterator p = name.begin();
        for(;p!=name.end();p++)
          nameset.insert(*p);
      }
    }
    if(nameset.size()>0){
      set<std::string>::iterator p = nameset.begin();
      for(;p!=nameset.end();p++)
        dataName.push_back(*p);
    }

  } else {

    dataName = obsWidget[m_selected]->getDataTypes();

  }

  vector<miutil::miTime> times = m_ctrl->getObsTimes(dataName);
  Q_EMIT emitTimes("obs",times);
}


void ObsDialog::applyhideClicked(){
  // cerr<<"applyhideClicked()"<<endl;
  emit ObsHide();
  emit ObsApply();
}


void ObsDialog::helpClicked(){
  //  emit setSource("ug_obsdialogue.html");
  emit showsource("ug_obsdialogue.html");


}

void ObsDialog::multiplotClicked(bool b){
  multiplot = b;
}

void ObsDialog::extensionToggled(bool b){
  if(b) updateExtension();
  showExtension(b);
}

void ObsDialog::criteriaOn(){
  updateExtension();
}

void ObsDialog::archiveMode(bool on){
  getTimes();
}


/*******************************************************/
vector<string> ObsDialog::getOKString()
{
  /* This function is called by the external program */
  //  cerr <<"QT - getOKString  "<<m_selected<<endl;

  vector<string> str;

  if (nr_plot == 0)
    return str;

  if(multiplot) {
    for (int i=nr_plot-1; i>-1; i--) {
      if (obsWidget[i]->initialized()) {
        std::string  tmpstr = obsWidget[i]->getOKString();
        if (not tmpstr.empty()) {
          //          tmpstr += getCriteriaOKString();
          str.push_back(tmpstr);
        }
      }
    }
  } else {
    std::string  tmpstr = obsWidget[m_selected]->getOKString();
    //    if(tmpstr.exists()) tmpstr += getCriteriaOKString();
    str.push_back(tmpstr);
  }

  return str;
}


vector<string> ObsDialog::writeLog()
{
  vector<string> vstr;

  if(nr_plot==0) return vstr;

  std::string str;

  //first write the plot type selected now
  str = obsWidget[m_selected]->getOKString(true);
  vstr.push_back(str);

  //then the others
  for (int i=0; i<nr_plot; i++) {
    if (i != m_selected) {
      if (obsWidget[i]->initialized()) {
        str= obsWidget[i]->getOKString(true);
        vstr.push_back(str);
      } else if (savelog[i].size()>0) {
        // ascii obs dialog not activated
        vstr.push_back(savelog[i]);
      }
    }
  }

  vstr.push_back("================");

  return vstr;
}


void ObsDialog::readLog(const vector<string>& vstr,
    const string& thisVersion, const string& logVersion){

  int n=0, nvstr= vstr.size();
  bool first=true;

  while (n<nvstr && vstr[n].substr(0,4)!="====") {

    int index=findPlotnr(vstr[n]);
    if (index<nr_plot) {
      if (obsWidget[index]->initialized() || first) {
        if (first) {  //will be selected
          first = false;
          plotbox->setCurrentIndex(index);
          plotSelected(index);
        }
        obsWidget[index]->readLog(vstr[n]);
      }
      // save until (ascii/hqc obs) dialog activated, or until writeLog
      savelog[index]= vstr[n];

    }

    n++;
  }
}


std::string ObsDialog::getShortname()
{
  std::string name;

  if (nr_plot == 0)
    return name;

  name = obsWidget[m_selected]->getShortname();
  if (not name.empty())
    name= "<font color=\"#999900\">" + name + "</font>";

  return name;
}


void ObsDialog::putOKString(const vector<string>& vstr)
{
  //unselect everything
  for (int i=0; i<nr_plot; i++) {
    if ( obsWidget[i]->initialized() ) {
      obsWidget[i]->setFalse();
    }
  }
  multiplotButton->setChecked(false);
  multiplot=false;

  //Emit empty time list
  vector<miutil::miTime> noTimes;
  emit emitTimes( "obs",noTimes );

  std::string key,value;
  int n=vstr.size();
  if(n>1) {
    multiplot=true;
    multiplotButton->setChecked(true);
  }
  for(int i=0; i<n; i++){
    int l = findPlotnr(vstr[i]);
    if (l<nr_plot) {
      plotbox->setCurrentIndex(l);
      plotSelected(l);
      obsWidget[l]->putOKString(vstr[i]);
    }
  }
}


int ObsDialog::findPlotnr(const std::string& str)
{

  vector<std::string> tokens = miutil::split(str, " ");
  int m=tokens.size();
  for(int j=0; j<m; j++){
    vector<std::string> stokens = miutil::split(tokens[j], "=");
    if( stokens.size()==2 && stokens[0]=="plot"){
      std::string value = miutil::to_lower(stokens[1]);
      if(value=="enkel") value="list"; //obsolete
      if(value=="trykk") value="pressure"; //obsolete
      int l=0;
      while (l<nr_plot && miutil::to_lower(m_name[l])!=value) l++;
      return l;
    }
  }

  return nr_plot; //not found

}

bool  ObsDialog::setPlottype(const std::string& str, bool on)
{
  //  cerr <<"setplottype:"<<str<<endl;
  int l=0;
  while (l<nr_plot && m_name[l]!=str) l++;

  if( l == nr_plot) return false;

  if( on ){
    plotbox->setCurrentIndex(l);
    ObsWidget* ow = new ObsWidget( this );
    std::string str = savelog[l];
    if (obsWidget[l]->initialized() ) {
      str = obsWidget[l]->getOKString();
    }
    stackedWidget->removeWidget(obsWidget[l]);
    obsWidget[l]->close();
    obsWidget[l]=ow;
    stackedWidget->insertWidget(l,obsWidget[l]);

    plotSelected(l,false);
    if (not str.empty()) {
      obsWidget[l]->putOKString(str);
    }

  } else if( obsWidget[l]->initialized() ){
    obsWidget[l]->setFalse();
    getTimes();
  }


  return true;

}

//called when the dialog is closed by the window manager
void ObsDialog::closeEvent( QCloseEvent* e){
  emit ObsHide();
}

void ObsDialog::makeExtension()
{

  freeze = false;

  extension = new QWidget(this);

  QLabel* listLabel = TitleLabel(tr("List of Criteria"),extension);
  vector<std::string> critName = obsWidget[m_selected]->getCriteriaNames();
  criteriaBox = ComboBox( extension,critName,true);

  QLabel* criteriaLabel = TitleLabel(tr("Criteria"),extension);
  criteriaListbox = new QListWidget(extension);

  QPushButton* delButton = NormalPushButton(tr("Delete"),extension);
  QPushButton* delallButton = NormalPushButton(tr("Delete all"),extension);
  delButton->setToolTip(tr("Delete selected criteria") );
  delallButton->setToolTip( tr("Delete all criteria") );

  radiogroup  = new QButtonGroup(extension);
  plotButton =
    new QRadioButton(tr("Plot"),this);
  colourButton =
    new QRadioButton(tr("Colour - parameter"),this);
  totalColourButton =
    new QRadioButton(tr("Colour - observation"),this);
  markerButton =
    new QRadioButton(tr("Marker"),this);
  radiogroup->addButton(plotButton);
  radiogroup->addButton(colourButton);
  radiogroup->addButton(totalColourButton);
  radiogroup->addButton(markerButton);
  QVBoxLayout *radioLayout = new QVBoxLayout();
  radioLayout->addWidget(plotButton);
  radioLayout->addWidget(colourButton);
  radioLayout->addWidget(totalColourButton);
  radioLayout->addWidget(markerButton);
  radiogroup->setExclusive(true);
  plotButton->setChecked(true);
  plotButton->setToolTip(tr("Plot observations which meet all criteria of at least one parameter") );
  colourButton->setToolTip(tr("Plot a parameter in the colour specified if it meets any criteria of that parameter") );
  totalColourButton->setToolTip(tr("Plot observations in the colour specified if one parameter meet any criteria of that parameter ") );
  markerButton->setToolTip(tr("Plot marker specified if one parameter meets any criteria of that parameter ") );

  QLabel* colourLabel = TitleLabel(tr("Colour"),extension);
  QLabel* markerLabel = TitleLabel(tr("Marker"),extension);
  QLabel* limitLabel = TitleLabel(tr("Limit"),extension);
  QLabel* precLabel = TitleLabel(tr("Precision"),extension);
  signBox = new QComboBox( extension );
  signBox->addItem(">");
  signBox->addItem(">=");
  signBox->addItem("<");
  signBox->addItem("<=");
  signBox->addItem("=");
  signBox->addItem("==");
  signBox->addItem("");
  stepComboBox = new QComboBox(extension);
  numberList(stepComboBox,1.0);
  stepComboBox->setToolTip(tr("Precision of limit") );
  limitLcd = LCDNumber(7,extension);
  limitSlider = new QSlider(Qt::Horizontal, extension);
  limitSlider->setMinimum(-100);
  limitSlider->setMaximum(100);
  limitSlider->setPageStep(1);
  limitSlider->setValue(0);

  cInfo = Colour::getColourInfo();
  colourBox = ColourBox( extension, cInfo);
  markerBox = PixmapBox( extension, markerName);

  // Layout for colour
  QGridLayout* colourlayout = new QGridLayout();
  colourlayout->addWidget( colourLabel, 0,0);
  colourlayout->addWidget( colourBox,   0,1);
  colourlayout->addWidget( markerLabel, 1,0);
  colourlayout->addWidget( markerBox,   1,1);
  colourlayout->addWidget( limitLabel,   2,0);
  colourlayout->addWidget( signBox,      2,1);
  colourlayout->addWidget( limitLcd,     3,0 );
  colourlayout->addWidget( limitSlider,  3,1 );
  colourlayout->addWidget( precLabel,   4,0 );
  colourlayout->addWidget( stepComboBox, 4,1 );

  QPushButton* saveButton = NormalPushButton(tr("Save"),extension);
  QLabel* editLabel = TitleLabel(tr("Save criteria list"),extension);
  lineedit = new QLineEdit(extension);
  lineedit->setToolTip(tr("Name of list to save") );


  connect(criteriaBox,SIGNAL(activated(int)),
      SLOT(criteriaListSelected(int)));
  connect( criteriaListbox, SIGNAL(itemClicked(QListWidgetItem*)),
      SLOT(criteriaSelected(QListWidgetItem*)));
  connect(signBox, SIGNAL(activated(int)),SLOT(signSlot(int)));
  connect(colourButton,SIGNAL(toggled(bool)),colourBox,SLOT(setEnabled(bool)));
  connect(totalColourButton,SIGNAL(toggled(bool)),
      colourBox,SLOT(setEnabled(bool)));
  connect(markerButton,SIGNAL(toggled(bool)),markerBox,SLOT(setEnabled(bool)));
  connect(colourButton,SIGNAL(toggled(bool)),SLOT(changeCriteriaString()));
  connect(colourBox, SIGNAL(activated(int)),SLOT(changeCriteriaString()));
  connect(markerBox, SIGNAL(activated(int)),SLOT(changeCriteriaString()));
  connect(totalColourButton, SIGNAL(toggled(bool)),
      SLOT(changeCriteriaString()));
  connect(markerButton, SIGNAL(toggled(bool)),SLOT(changeCriteriaString()));
  connect( limitSlider, SIGNAL(valueChanged(int)),SLOT(sliderSlot(int)));
  connect( stepComboBox, SIGNAL(activated(int)),SLOT(stepSlot(int)));
  connect( delButton, SIGNAL(clicked()),SLOT(deleteSlot()));
  connect( delallButton, SIGNAL(clicked()),SLOT(deleteAllSlot()));
  connect( saveButton, SIGNAL(clicked()),SLOT(saveSlot()));

  QFrame *line0 = new QFrame( extension );
  line0->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  QVBoxLayout *exLayout = new QVBoxLayout();
  exLayout->addWidget( listLabel );
  exLayout->addWidget( criteriaBox );
  exLayout->addWidget( criteriaLabel );
  exLayout->addWidget( criteriaListbox );
  exLayout->addWidget( delButton );
  exLayout->addWidget( delallButton );
  exLayout->addLayout( radioLayout );
  exLayout->addLayout( colourlayout );
  exLayout->addWidget( line0 );
  exLayout->addWidget( editLabel );
  exLayout->addWidget( lineedit );
  exLayout->addWidget( saveButton );

  //separator
  QFrame* verticalsep= new QFrame( extension );
  verticalsep->setFrameStyle( QFrame::VLine | QFrame::Raised );
  verticalsep->setLineWidth( 5 );

  QHBoxLayout *hLayout = new QHBoxLayout( extension);

  hLayout->addWidget(verticalsep);
  hLayout->addLayout(exLayout);


  return;
}

void ObsDialog::criteriaListSelected(int index)
{

  obsWidget[m_selected]->setCurrentCriteria(index);

  ObsDialogInfo::CriteriaList critList =
    obsWidget[m_selected]->getCriteriaList();

  criteriaListbox->clear();

  obsWidget[m_selected]->markButton("ALL_PARAMS",false);
  lineedit->clear();

  int n = critList.criteria.size();
  if( n==0 ) return;

  lineedit->setText(critList.name.c_str());
  for(int j=0; j<n; j++){
    criteriaListbox->addItem(QString(critList.criteria[j].c_str()));
    markButton(critList.criteria[j]);
  }
  if(criteriaListbox->count()){
    criteriaListbox->item(0)->setSelected(true);
    criteriaSelected(criteriaListbox->item(0));
  }

}

void ObsDialog::markButton(std::string& str)
{
  vector<std::string> vstr;
  if(miutil::contains(str, "<"))
    vstr=miutil::split(str, "<");
  else if(miutil::contains(str, ">"))
    vstr=miutil::split(str, ">");
  else if(miutil::contains(str, "="))
    vstr=miutil::split(str, "=");
  else
    vstr=miutil::split(str, " ");

  if(vstr.size()==0) return;

  obsWidget[m_selected]->markButton(vstr[0],true);

  //Wind
  if(vstr[0]=="dd" || vstr[0]=="ff")
    obsWidget[m_selected]->markButton("Wind",true);
  //Pos
  if(vstr[0]=="lat" || vstr[0]=="lon")
    obsWidget[m_selected]->markButton("Pos",true);
}


void ObsDialog::signSlot( int number ){
  bool on = (number!=3);
  limitLcd->setEnabled(on);
  limitSlider->setEnabled(on);
  changeCriteriaString();
}

void ObsDialog::sliderSlot( int number ){

  double scalednumber= number * stepComboBox->currentText().toFloat();
  limitLcd->display( scalednumber );
  changeCriteriaString();

}

void ObsDialog::stepSlot( int number )
{

//todo: smarter slider limits
  float scalesize = stepComboBox->currentText().toFloat();
  numberList(stepComboBox,scalesize);
  limitSlider->setMaximum(1000);
  limitSlider->setMinimum(-1000);
  limitSlider->setValue(int(limitLcd->value()/scalesize));
  double scalednumber= limitSlider->value()*scalesize;
  limitLcd->display( scalednumber );
  changeCriteriaString();

}

void  ObsDialog::changeCriteriaString( )
{

  if(freeze) return;

  if( criteriaListbox->count()==0 ) return;

  if( criteriaListbox->currentRow()<0 ) {
    criteriaListbox->setCurrentRow(0);
  }


  freeze = true; //don't change the string while updating the widgets

  std::string str=makeCriteriaString();

  if (not str.empty()) {
    criteriaListbox->currentItem()->setText(QString::fromStdString(str));
    // save changes
    int n=criteriaListbox->count();
    vector<std::string> vstr;
    for( int i=0; i<n; i++){
      vstr.push_back(criteriaListbox->item(i)->text().toStdString());
    }
    obsWidget[m_selected]->saveCriteria(vstr);

  }else{
    deleteSlot();
  }

  freeze=false;

}

bool  ObsDialog::newCriteriaString( )
{

  if(freeze)   return false;

  std::string str=makeCriteriaString();
  if(str.empty())
    return false;

  freeze=true;
  int n=criteriaListbox->count();
  bool found=false;
  int i=0;
  for( ; i<n; i++){
    std::string sstr = criteriaListbox->item(i)->text().toStdString();
    vector<std::string> vstr;
    if(miutil::contains(sstr, "<"))
      vstr=miutil::split(sstr, "<");
    else if(miutil::contains(sstr, ">"))
      vstr=miutil::split(sstr, ">");
    else if(miutil::contains(sstr, "="))
      vstr=miutil::split(sstr, "=");
    else
      vstr=miutil::split(sstr, " ");
    //    if(vstr.size() && vstr[0]==parameterLabel->text().toStdString()){
    if(vstr.size() && vstr[0]==parameter){
      found=true;
    }else if(found){
      break;
    }
  }

  criteriaListbox->insertItem(i,QString(str.c_str()));
  criteriaListbox->setCurrentRow(i);
  // save changes
  n=criteriaListbox->count();
  vector<std::string> vstr;
  for( int i=0; i<n; i++){
    vstr.push_back(criteriaListbox->item(i)->text().toStdString());
  }
  obsWidget[m_selected]->saveCriteria(vstr);


  freeze=false;
  return true;
}

std::string ObsDialog::makeCriteriaString( )
{

  //make string
  std::string str=parameter;

  std::string sign = signBox->currentText().toStdString();
  if (not sign.empty()) {
    str += sign;
    std::string lcdstr = miutil::from_number(limitLcd->value());
    str += lcdstr;
  }

  if(colourButton->isChecked()){
    str += "  ";
    str += cInfo[colourBox->currentIndex()].name;
  }
  else if(totalColourButton->isChecked()){
    str += "  ";
    str += cInfo[colourBox->currentIndex()].name;
    str += " total";
  }
  else if(markerButton->isChecked()){
    str += "  ";
    str += markerName[markerBox->currentIndex()];
    str += " marker";
  }
  else {
    str += "  plot";
  }


  return str;
}


void ObsDialog::criteriaSelected(QListWidgetItem* item)
{
  if(freeze) return;

  if(!item ) {
    parameter.clear();
    return;
  }

  freeze=true;

  std::string str = item->text().toStdString();

  vector<std::string> sub = miutil::split(str, " ");

  //  std::string sign,parameter;
  std::string sign;
  if( miutil::contains(sub[0], ">") ){
    sign = ">";
    signBox->setCurrentIndex(0);
  } else if( miutil::contains(sub[0], "<") ){
    sign = "<";
    signBox->setCurrentIndex(1);
  } else if( miutil::contains(sub[0], "=") ){
    sign = "=";
    signBox->setCurrentIndex(2);
  } else {
    signBox->setCurrentIndex(3);
  }

  float value = 0.0;
  if (not sign.empty()) {
    vector<std::string> sstr = miutil::split(sub[0], sign);
    if(sstr.size()!=2) return;
    parameter = sstr[0];
    value = atof(sstr[1].c_str());
  } else {
    parameter = sub[0];
  }

  int low,high;
  if(  obsWidget[m_selected]->getCriteriaLimits(parameter,low,high)){
    plotButton->setEnabled(true);
    signBox->setEnabled(true);
    limitSlider->setMinimum(low);
    limitSlider->setMaximum(high);
  } else {
    limitSlider->setEnabled(false);
    limitLcd->setEnabled(false);
    signBox->setCurrentIndex(3);
    signBox->setEnabled(false);
    colourButton->setChecked(true);
    plotButton->setEnabled(false);
  }


  if (not sign.empty()) {
    limitSlider->setEnabled(true);
    limitLcd->setEnabled(true);
    limitLcd->display(value);
    float scalesize;
    if(value == 0){
      scalesize = 1;
    } else {
      float absvalue = fabsf(value);
      int ii = int(log10(absvalue));
      scalesize = powf(10.0,ii-1);
    }

    numberList(stepComboBox,scalesize);
    float r = value>0 ? 0.5:-0.5;
    int ivalue = int(value/scalesize+r);
    limitSlider->setValue(ivalue);
  } else{
    limitSlider->setEnabled(false);
    limitLcd->setEnabled(false);
  }

  if(sub.size()>1) {
    if( sub[1]=="plot" ){
      plotButton->setChecked(true);
      colourBox->setEnabled(false);
      markerBox->setEnabled(false);
    } else if( sub.size()>2 && sub[2]=="marker" ){
      int number= getIndex( markerName, sub[1]);
      if (number>=0) markerBox->setCurrentIndex(number);
      markerButton->setChecked(true);
    } else {
      int number= getIndex( cInfo, sub[1]);
      if (number>=0) colourBox->setCurrentIndex(number);
      if(sub.size()==3 && miutil::to_lower(sub[2])=="total"){
        totalColourButton->setChecked(true);
      } else{
        colourButton->setChecked(true);
      }
    }
  }

  freeze=false;
}

void ObsDialog::deleteSlot( )
{

  if(criteriaListbox->currentRow() == -1) return;

  criteriaListbox->takeItem(criteriaListbox->currentRow());
  criteriaSelected(criteriaListbox->currentItem());
  int n=criteriaListbox->count();
  vector<std::string> vstr;
  for( int i=0; i<n; i++){
    vstr.push_back(criteriaListbox->item(i)->text().toStdString());
  }
  //save criterias and reread in order to get buttons marked right
  obsWidget[m_selected]->saveCriteria(vstr);
  obsWidget[m_selected]->markButton("ALL_PARAMS",false);

  for(int i=0; i<n; i++){
    markButton(vstr[i]);
  }

}

void ObsDialog::deleteAllSlot( )
{

  criteriaListbox->clear();
  vector<std::string> vstr;
  obsWidget[m_selected]->saveCriteria(vstr);
  obsWidget[m_selected]->markButton("ALL_PARAMS",false);
}

void ObsDialog::saveSlot( )
{

  std::string name = lineedit->text().toStdString();
  if (name.empty())
    return;

  int n=criteriaListbox->count();
  vector<std::string> vstr;
  for( int i=0; i<n; i++){
    vstr.push_back(criteriaListbox->item(i)->text().toStdString());
  }

  //save criterias and reread in order to get buttons marked right

  bool newItem = obsWidget[m_selected]->saveCriteria(vstr,name);
  if( newItem ){
    criteriaBox->addItem(name.c_str());
    int index = criteriaBox->count()-1;
    criteriaBox->setCurrentIndex(index);
    criteriaListSelected(index);
    return;
  }

  if(vstr.size()==0){
    int index = criteriaBox->currentIndex();
    criteriaBox->removeItem(index);
    if(index==criteriaBox->count()) index--;
    if(index<0){
      lineedit->clear();
    } else {
      criteriaBox->setCurrentIndex(index);
      criteriaListSelected(index);
    }
  }

}

void ObsDialog::rightButtonClicked(std::string name)
{
  //Wind
  if(name=="Wind") {
    rightButtonClicked("dd");
    rightButtonClicked("ff");
    obsWidget[m_selected]->markButton(name,true);
    return;
  }

  //Pos
  if(name=="Pos") {
    rightButtonClicked("lat");
    rightButtonClicked("lon");
    obsWidget[m_selected]->markButton(name,true);
    return;
  }

  freeze =true;
  bool sameParameter = false;
  if( parameter == name) sameParameter = true;


  parameter = name;

  int low,high;
  if(  obsWidget[m_selected]->getCriteriaLimits(name,low,high)){
    limitSlider->setMinimum(low);
    limitSlider->setMaximum(high);
    signBox->setEnabled(true);
    plotButton->setEnabled(true);
    colourBox->setEnabled(colourButton->isChecked() || totalColourButton->isChecked());
    markerBox->setEnabled(markerButton->isChecked());
    bool sign = (signBox->currentIndex()!=3);
    limitSlider->setEnabled(sign);
    limitLcd->setEnabled(sign);
    if(!sameParameter){
      numberList(stepComboBox,1.0);
      double scalednumber= limitSlider->value();
      limitLcd->display( scalednumber );
      changeCriteriaString();
    }

  } else {
    limitSlider->setEnabled(false);
    limitLcd->setEnabled(false);
    signBox->setCurrentIndex(3);
    signBox->setEnabled(false);
    totalColourButton->setChecked(true);
    plotButton->setEnabled(false);
  }

  freeze = false;
  newCriteriaString();
  obsWidget[m_selected]->markButton(name,true);

}

void ObsDialog::updateExtension()
{

  ObsDialogInfo::CriteriaList cList;
  criteriaBox->clear();
  vector<std::string> critName = obsWidget[m_selected]->getCriteriaNames();
  int n = critName.size();
  if(n==0){ // no lists, read saved criterias
    cList = obsWidget[m_selected]->getSavedCriteria();
  } else {
    for(unsigned int i=0;i<critName.size();i++) {
      criteriaBox->addItem(critName[i].c_str());
    }
    cList = obsWidget[m_selected]->getSavedCriteria();
  }

  criteriaListbox->clear();
  obsWidget[m_selected]->markButton("ALL_PARAMS",false);
  lineedit->setText(criteriaBox->currentText());

  vector<std::string> criteriaList = cList.criteria;
  for(unsigned int j=0; j<criteriaList.size(); j++){
    criteriaListbox->addItem(QString(criteriaList[j].c_str()));
    markButton(criteriaList[j]);
  }
  if(criteriaListbox->count()){
    criteriaListbox->item(0)->setSelected(true);
    criteriaListbox->setCurrentRow(0);
    criteriaSelected(criteriaListbox->item(0));
  }
}

void ObsDialog::numberList(QComboBox* cBox, float number)
{
  const float enormal[] = { 0.001, 0.01, 0.1, 1.0, 10., 100., 1000., 10000., -1 };
  diutil::numberList(cBox, number, enormal, false);
  cBox->setEnabled(true);
}
