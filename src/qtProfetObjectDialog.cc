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

#include "qtProfetObjectDialog.h"
#include <q3hbox.h>
#include <q3vbox.h>
#include <qlayout.h>
#include <q3buttongroup.h>
//Added by qt3to4:
#include <QCloseEvent>
#include <QLabel>
#include <Q3VBoxLayout>
#include <qUtilities/miLogFile.h>


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
  Q3VBoxLayout * mainLayout = new Q3VBoxLayout(this);
  Q3HBox * titleBox = new Q3HBox(this);
  parameterLabel = new QLabel(titleBox);
  sessionLabel = new QLabel(titleBox);

  mainLayout->addWidget(titleBox);

  algGroupBox = new Q3GroupBox(2, Qt::Vertical, "Algorithm", this);
  baseComboBox = new QComboBox(algGroupBox);
  algDescriptionLabel = new QLabel(algGroupBox);

  mainLayout->addWidget(algGroupBox);

  areaGroupBox = new Q3GroupBox(4, Qt::Vertical, "Area", this);
  databaseAreaButton = new QPushButton("From Database", areaGroupBox);
  fileAreaButton = new QPushButton("From File", areaGroupBox);
  areaInfoLabel = new QLabel("", areaGroupBox);

  mainLayout->addWidget(areaGroupBox);

  if(mode == VIEW_OBJECT_MODE)
    areaGroupBox->setVisible(false);

  stackGroupBox = new Q3GroupBox(1, Qt::Vertical, "Parameters", this);
  stackGroupBox->setInsideMargin(2);
  stackGroupBox->setInsideSpacing(2);

  widgetStack = new Q3WidgetStack(stackGroupBox);

  mainLayout->addWidget(stackGroupBox);

  reasonGroupBox = new Q3GroupBox(1, Qt::Vertical, "Reason", this);
  reasonText = new Q3TextEdit(reasonGroupBox);

  mainLayout->addWidget(reasonGroupBox);

  statGroupBox = new Q3GroupBox(1, Qt::Vertical, "Statistics", this);
  statisticLabel = new QLabel("", statGroupBox);

  mainLayout->addWidget(statGroupBox);
  if(mode == VIEW_OBJECT_MODE)
    statGroupBox->setVisible(false);

  Q3HBox * buttonBox = new Q3HBox(this);

  if(mode == VIEW_OBJECT_MODE){
    saveObjectButton = new QPushButton(tr("Save"));
    cancelObjectButton = new QPushButton(tr("Close"), buttonBox);
  } else {
    saveObjectButton = new QPushButton(tr("Save"), buttonBox);
    cancelObjectButton = new QPushButton(tr("Cancel"), buttonBox);
  }

  mainLayout->addWidget(buttonBox);
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
  baseComboBox->insertItem(obj.name().cStr(),0);
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
  int index = baseComboBox->currentItem();
  if(!dynamicGuiMap[index]){
    dynamicGuiMap[index] = new fetDynamicGui(this,components);
    int id = widgetStack->addWidget(dynamicGuiMap[index]);
     connect( dynamicGuiMap[index], SIGNAL( valueChanged() ),
     this, SIGNAL( dynamicGuiChanged() ) );
  }
  widgetStack->raiseWidget(dynamicGuiMap[index]);
  widgetStack->show();
}

vector<fetDynamicGui::GuiComponent> ProfetObjectDialog::getCurrentGuiComponents(){
  vector<fetDynamicGui::GuiComponent> comp;
  int index = baseComboBox->currentItem();
  if(dynamicGuiMap[index]){
    dynamicGuiMap[index]->getValues(comp);
  }
  return comp;
}


void ProfetObjectDialog::setSession(const miTime & time){
  ostringstream ost;
  ost << time.hour() << ":00 " << miString(time.date().format("%a %e.%b"));
 // qt4 fix: setText takes QString as argument
  sessionLabel->setText(QString(ost.str().c_str()));
}

void ProfetObjectDialog::setParameter(const miString & p){
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
  for(int i=0;i<o.size();i++){
    baseComboBox->insertItem(o[i].name().cStr(),i);
    descriptionMap[QString(o[i].name().cStr())] = QString(o[i].description().cStr());
  }
}

void ProfetObjectDialog::setAreaStatus(AreaStatus status){
  areaInfoLabel->setText(getAreaStatusString(status));
  if(status == AREA_OK) {
    areaInfoLabel->setBackgroundColor(QColor("Green"));
    saveObjectButton->setEnabled(true);
    widgetStack->setEnabled(true);
  }
  else {
    areaInfoLabel->setBackgroundColor(QColor("Red"));
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
  baseComboBox->setCurrentItem(0);
  baseObjectChanged(baseComboBox->text(0));
}

miString ProfetObjectDialog::getSelectedBaseObject(){
//  miString r = baseComboBox->currentText().toStdString();
  return selectedBaseObject;
}

miString ProfetObjectDialog::getReason(){
  miString r = reasonText->text().toStdString();
  return r;
}


void ProfetObjectDialog::setStatistics(map<miString,float>& stat)
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

  map<miString,float>::iterator itr=stat.begin();
  ostringstream ost1;
  ostringstream ost2;
  miString nb="&nbsp;&nbsp;";
  ost1 << "<table border=0 align=center><tr>";
  ost2 << "<tr>";
  int i=1;
  for(;itr!=stat.end();itr++,i++) {
    float value = itr->second;
    int   prec  = ((int(value*100) == int(value)*100) ? 0 : 2);

    miString bgcolor= (i%2?"lightGray":"white");

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


void ProfetObjectDialog::startBookmarkDialog(vector<miString>& boom)
{
  if(boom.empty())
    return;

  PolygonBookmarkDialog * bookmarks = new PolygonBookmarkDialog(this,boom,lastSavedPolygonName);
  connect(bookmarks,SIGNAL(polygonQuit()),
        this,SLOT(quitBookmarks()));
  connect(bookmarks,SIGNAL(polygonCopied(miString,miString,bool)),
      this,SIGNAL(copyPolygon(miString,miString,bool)));
  connect(bookmarks,SIGNAL(polygonSelected(miString)),
      this,SIGNAL(selectPolygon(miString)));
  bookmarks->show();

  databaseAreaButton->setEnabled(false);

}

void ProfetObjectDialog::logSizeAndPos()
{
  miLogFile logfile;
  logfile.setSection("PROFET.LOG");
  logfile.logSizeAndPos(this,"ProfetEditObjectDialog");
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


