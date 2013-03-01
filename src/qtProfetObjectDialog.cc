/*
  Diana - A Free Meteorological Visualisation Tool

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

#include "qtProfetObjectDialog.h"
#include <QGroupBox>
#include <QLayout>
#include <QCloseEvent>
#include <QLabel>
#include <QTextEdit>
#include <QComboBox>
#include <QStackedWidget>
#include <qUtilities/miLogFile.h>

using namespace std;

ProfetObjectDialog::ProfetObjectDialog(QWidget * parent, OperationMode om)
  : QDialog(parent), mode(om)
{
  if(mode == NEW_OBJECT_MODE) setWindowTitle(tr("New Object"));
  else if(mode == VIEW_OBJECT_MODE) setWindowTitle(tr("View Object"));
  else setWindowTitle(tr("Current Object"));

  initGui();

  if(mode == VIEW_OBJECT_MODE)
    setAllEnabled(false);

  // Disable non-implemented functions
  fileAreaButton->setEnabled(false);

  connectSignals();
  setAreaStatus(AREA_NOT_SELECTED);
}

void ProfetObjectDialog::initGui()
{

  QVBoxLayout * mainLayout = new QVBoxLayout(this);

  parameterLabel = new QLabel(this);
  sessionLabel = new QLabel(this);
  QHBoxLayout * titleLayout = new QHBoxLayout();
  titleLayout->addWidget(parameterLabel);
  titleLayout->addWidget(sessionLabel);

  mainLayout->addLayout(titleLayout);

//  alg_frame= new QFrame(this);
 // alg_frame->setFrameStyle(QFrame::Box | QFrame::Sunken);

  algGroupBox = new QGroupBox(tr("Algorithm"),this);
  baseComboBox = new QComboBox(algGroupBox);
  algDescriptionLabel = new QLabel(algGroupBox);
  QVBoxLayout * algoLayout = new QVBoxLayout(algGroupBox);
  algoLayout->addWidget(baseComboBox);
  algoLayout->addWidget(algDescriptionLabel);

  mainLayout->addWidget(algGroupBox);

//  area_frame= new QFrame(this);
//  area_frame->setFrameStyle(QFrame::Box | QFrame::Sunken);
  areaGroupBox = new QGroupBox(tr("Area"), this);
  databaseAreaButton = new QPushButton("From Database", areaGroupBox);
  fileAreaButton = new QPushButton("From File", areaGroupBox);
  areaInfoLabel = new QLabel("", areaGroupBox);

  QVBoxLayout * areaLayout = new QVBoxLayout(areaGroupBox);
  areaLayout->addWidget(databaseAreaButton);
  areaLayout->addWidget(fileAreaButton);
  areaLayout->addWidget(areaInfoLabel);
  mainLayout->addWidget(areaGroupBox);

  if(mode == VIEW_OBJECT_MODE)
    areaGroupBox->setVisible(false);

  stackGroupBox = new QGroupBox(tr("Parameters"), this);
  widgetStack = new QStackedWidget(stackGroupBox);
  QVBoxLayout * stackLayout = new QVBoxLayout(stackGroupBox);
  stackLayout->addWidget(widgetStack);

  mainLayout->addWidget(stackGroupBox);

  reasonGroupBox = new QGroupBox(tr("Reason"), this);
  reasonText = new QTextEdit(reasonGroupBox);
  QVBoxLayout * reasonLayout = new QVBoxLayout(reasonGroupBox);
  reasonLayout->addWidget(reasonText);

  mainLayout->addWidget(reasonGroupBox);

  statGroupBox = new QGroupBox(tr("Statistics"), this);
  statisticLabel = new QLabel("", statGroupBox);
  QVBoxLayout * statisticLayout = new QVBoxLayout(statGroupBox);
  statisticLayout->addWidget(statisticLabel);
  mainLayout->addWidget(statGroupBox);

  if(mode == VIEW_OBJECT_MODE)
    statGroupBox->setVisible(false);

 // QHBox * buttonBox = new QHBox(this);

  if(mode == VIEW_OBJECT_MODE){
    saveObjectButton = new QPushButton(tr("Save"));
    cancelObjectButton = new QPushButton(tr("Close"), this);
  } else {
    saveObjectButton = new QPushButton(tr("Save"), this);
    cancelObjectButton = new QPushButton(tr("Cancel"), this);
  }
  QHBoxLayout * buttonLayout = new QHBoxLayout();
  buttonLayout->addWidget(saveObjectButton);
  buttonLayout->addWidget(cancelObjectButton);

  mainLayout->addLayout(buttonLayout);
}

void ProfetObjectDialog::connectSignals(){
  connect(saveObjectButton, SIGNAL( clicked() ),
        this, SLOT( saveDialog() ));


  connect(cancelObjectButton, SIGNAL( clicked() ), this, SLOT(quitDialog()));
  connect(this, SIGNAL(rejected()),  this, SLOT(quitDialog()));


  connect(baseComboBox, SIGNAL( activated(const QString&) ),
      this, SLOT( baseObjectChanged(const QString&) ));


  connect(databaseAreaButton,SIGNAL(clicked()),
      this, SIGNAL (requestPolygonList()));
}

void ProfetObjectDialog::setAllEnabled(bool enable) {
  algGroupBox->setEnabled(enable);
  areaGroupBox->setEnabled(enable);
  stackGroupBox->setEnabled(enable);
  reasonGroupBox->setEnabled(enable);
  saveObjectButton->setEnabled(enable);
}


void ProfetObjectDialog::baseObjectChanged(const QString & qs){
  selectedBaseObject = qs.toStdString();
  algDescriptionLabel->setText(descriptionMap[qs]);
  emit baseObjectSelected(selectedBaseObject);
}

void ProfetObjectDialog::closeEvent(QCloseEvent * e){
  databaseAreaButton->setEnabled(true);
  quitDialog();
}

void ProfetObjectDialog::showObject(const fetObject & obj,
    vector<fetDynamicGui::GuiComponent> components)
{
  vector<fetBaseObject> fbo;
  setBaseObjects(fbo);//remove base objects/gui
  baseComboBox->insertItem(0,obj.name().cStr());
  baseComboBox->setEnabled(false);
  addDymanicGui(components);
  cerr << "ProfetObjectDialog::showObject reason: " << obj.reason() << endl;
  reasonText->setText(obj.reason().cStr());
}

void ProfetObjectDialog::newObjectMode(){
  mode = NEW_OBJECT_MODE;
  baseComboBox->setEnabled(true);
  reasonText->setText("");
}

void ProfetObjectDialog::editObjectMode(const fetObject & obj,
    vector<fetDynamicGui::GuiComponent> components){
  mode = EDIT_OBJECT_MODE;
  showObject(obj,components);
}

void ProfetObjectDialog::addDymanicGui(
    vector<fetDynamicGui::GuiComponent> components){
  int index = baseComboBox->currentIndex();
  if(!dynamicGuiMap[index]){
    dynamicGuiMap[index] = new fetDynamicGui(this,components);
    widgetStack->addWidget(dynamicGuiMap[index]);
     connect( dynamicGuiMap[index], SIGNAL( valueChanged() ),
     this, SIGNAL( dynamicGuiChanged() ) );
  }
  widgetStack->setCurrentWidget(dynamicGuiMap[index]);
  widgetStack->show();
}

vector<fetDynamicGui::GuiComponent> ProfetObjectDialog::getCurrentGuiComponents(){
  vector<fetDynamicGui::GuiComponent> comp;
  int index = baseComboBox->currentIndex();
  if(dynamicGuiMap[index]){
    dynamicGuiMap[index]->getValues(comp);
  }
  return comp;
}


void ProfetObjectDialog::setSession(const miutil::miTime & time){
  ostringstream ost;
  ost << time.hour() << ":00 " << miutil::miString(time.date().format("%a %e.%b"));
 // qt4 fix: setText takes QString as argument
  sessionLabel->setText(QString(ost.str().c_str()));
}

void ProfetObjectDialog::setParameter(const miutil::miString & p){
  parameterLabel->setText(p.cStr());
}

void ProfetObjectDialog::setBaseObjects(const vector<fetBaseObject> & o){
  for(DynamicGuiMap::const_iterator iter = dynamicGuiMap.begin();
      iter!=dynamicGuiMap.end(); iter ++){
    widgetStack->removeWidget(iter->second);
  }
  dynamicGuiMap.clear();
  baseComboBox->clear();
  descriptionMap.clear();
  for(unsigned int i=0;i<o.size();i++){
    baseComboBox->insertItem(i,o[i].name().cStr());
    descriptionMap[QString(o[i].name().cStr())] = QString(o[i].description().cStr());
  }
}

void ProfetObjectDialog::setAreaStatus(AreaStatus status){
  areaInfoLabel->setText(getAreaStatusString(status));
  if(status == AREA_OK) {
 //   areaInfoLabel->setBackgroundColor(QColor("Green"));
    saveObjectButton->setEnabled(true);
    widgetStack->setEnabled(true);
  }
  else {
 //   areaInfoLabel->setBackgroundColor(QColor("Red"));
    saveObjectButton->setEnabled(false);
    widgetStack->setEnabled(true);
  }
}

QString ProfetObjectDialog::getAreaStatusString(AreaStatus status){
  static const QString areaStatusString[3] = {"No Area Selected","Area Selected",
      "Invalid Area Selected"};
  return areaStatusString[status];
}

void ProfetObjectDialog::selectDefault(){
  baseComboBox->setCurrentIndex(0);
  baseObjectChanged(baseComboBox->itemText(0));
}

miutil::miString ProfetObjectDialog::getSelectedBaseObject(){
//  miutil::miString r = baseComboBox->currentText().toStdString();
  return selectedBaseObject;
}

miutil::miString ProfetObjectDialog::getReason(){
  miutil::miString r = reasonText->toPlainText().toStdString();
  return r;
}


void ProfetObjectDialog::setStatistics(map<miutil::miString,float>& stat)
{

  if(!stat.size()) {
    statisticLabel->setText("");
    return;
  }

  if(!stat.count("size")) {
    statisticLabel->setText("");
    return;
  }

  if(stat["size"] < 1 ) {
    statisticLabel->setText(tr("<font color=red><b>EMPTY OBJECT - NOTHING TO EDIT!</b></font>"));
    return;
  }

  map<miutil::miString,float>::iterator itr=stat.begin();
  ostringstream ost1;
  ostringstream ost2;
  miutil::miString nb="&nbsp;&nbsp;";
  ost1 << "<table border=0 align=center><tr>";
  ost2 << "<tr>";
  int i=1;
  for(;itr!=stat.end();itr++,i++) {
    float value = itr->second;
    int   prec  = ((int(value*100) == int(value)*100) ? 0 : 2);

    miutil::miString bgcolor= (i%2?"lightGray":"white");

    ost1	<< "<td halign=center bgcolor="<<bgcolor <<"><b>" << nb << itr->first.upcase() << nb <<" </b> </td> ";
    ost2  << "<td halign=center bgcolor="<<bgcolor <<"> "   << nb
      << std::fixed << std::setprecision(prec) << itr->second<< nb <<" </td>";
  }
  ost1 << ost2.str() <<  "</table>";

 // qt4 fix: setText takes QString as argument
  statisticLabel->setText(QString(ost1.str().c_str()));

}

void ProfetObjectDialog::quitBookmarks()
{
  databaseAreaButton->setEnabled(true);
}


void ProfetObjectDialog::startBookmarkDialog(vector<miutil::miString>& boom)
{
  if(boom.empty())
    return;

  PolygonBookmarkDialog * bookmarks = new PolygonBookmarkDialog(this,boom,lastSavedPolygonName);
  connect(bookmarks,SIGNAL(polygonQuit()),
        this,SLOT(quitBookmarks()));
  connect(bookmarks,SIGNAL(polygonCopied(miutil::miString,miutil::miString,bool)),
      this,SIGNAL(copyPolygon(miutil::miString,miutil::miString,bool)));
  connect(bookmarks,SIGNAL(polygonSelected(miutil::miString)),
      this,SIGNAL(selectPolygon(miutil::miString)));
  bookmarks->show();

  databaseAreaButton->setEnabled(false);

}

void ProfetObjectDialog::logSizeAndPos()
{
  miLogFile logfile;
  logfile.setSection("PROFET.LOG");
  miutil::miString logname="ProfetEditObjectDialog";
  if(mode == VIEW_OBJECT_MODE)
    logname="ProfetViewObjectDialog";

  logfile.logSizeAndPos(this,logname);
}

void ProfetObjectDialog::saveDialog()
{
  logSizeAndPos();
  emit saveObjectClicked();
}

void ProfetObjectDialog::quitDialog()
{
  logSizeAndPos();
  emit cancelObjectDialog();
}


