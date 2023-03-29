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
//#define DEBUGPRINT

#include "diana_config.h"

#include "qtTrajectoryDialog.h"
#include "qtUtility.h"
#include "qtGeoPosLineEdit.h"
#include "diLinetype.h"
#include "diUtilities.h"
#include "util/qstring_util.h"

#include <puTools/miStringFunctions.h>

#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QString>
#include <QToolTip>
#include <QFrame>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>

#include <cmath>
#include <iomanip>
#include <sstream>

#define MILOGGER_CATEGORY "diana.TrajectoryDialog"
#include <miLogger/miLogging.h>


TrajectoryDialog::TrajectoryDialog( QWidget* parent, Controller* llctrl )
  : QDialog(parent), contr(llctrl)
{
  METLIBS_LOG_SCOPE();

  setWindowTitle(tr("Trajectories"));
  setFocusPolicy(Qt::StrongFocus);

  //define colours
  colourInfo = Colour::getColourInfo();

  // ********** create the various QT widgets to appear in dialog ***********

  //colours
  QLabel* collabel = new QLabel(tr("Colour"),this);
  colbox= ColourBox( this, colourInfo);
  connect(colbox,SIGNAL(activated ( int )),SLOT(colourSlot(int)));

  // combobox - line width
  QLabel* lineWidthLabel= new QLabel(tr("Line width"), this);
  lineWidthBox= LinewidthBox( this );
  connect( lineWidthBox, SIGNAL( activated(int) ),
      SLOT( lineWidthSlot(int) ) );

  // line types
  linetypes = Linetype::getLinetypeNames();
  QLabel *lineTypeLabel= new QLabel( tr("Line type"), this );
  lineTypeBox=  LinetypeBox( this);
  connect( lineTypeBox, SIGNAL( activated(int) ),
      SLOT( lineTypeSlot(int) ) );

  //Number of positions, radius and kind of marker
  QLabel* numposLabel = new QLabel(tr("No. of positions"), this);
  numposSpin = new QSpinBox(this);
  numposSpin->setMinimum(1);
  numposSpin->setMaximum(60);
  numposSpin->setSingleStep(1);
  connect(numposSpin, SIGNAL(valueChanged(int)), this, SLOT(numposSlot(int)));

  //radius
  QLabel* radiusLabel = new QLabel( tr("Radius"), this );
  radiusSpin = new QSpinBox(this);
  radiusSpin->setMinimum(10);
  radiusSpin->setMaximum(500);
  radiusSpin->setSingleStep(10);
  radiusSpin->setValue(100);
  radiusSpin->setSuffix(tr("km"));
  connect(radiusSpin,SIGNAL(valueChanged(int)),SLOT(radiusSpinChanged(int)));

  /*****************************************************************/

  // Positions
  posButton = new QCheckBox(tr("Select positions on map"),this);
  posButton->setChecked(true);
  connect( posButton, SIGNAL( toggled(bool)), SLOT( posButtonToggled(bool) ) );

  QLabel* posLabel = new QLabel( tr("Write positions (Lat Lon):"), this );
  edit = new GeoPosLineEdit(this);
  edit->setToolTip(tr("Lat Lon (deg:min:sec or decimal)") );
  connect( edit, SIGNAL( returnPressed()), SLOT( editDone() ));

  posList = new QListWidget(this);
  connect( posList, SIGNAL( itemClicked( QListWidgetItem * ) ),
      SLOT( posListSlot( ) ) );

  //push button to delete last pos
  deleteButton = new QPushButton(tr("Delete"), this );
  connect( deleteButton, SIGNAL(clicked()),SLOT(deleteClicked()));

  //push button to delete all
  deleteAllButton = new QPushButton( tr("Delete all"), this);
  connect( deleteAllButton, SIGNAL(clicked()),SLOT(deleteAllClicked()));

  //push button to start calculation
  startCalcButton = new QPushButton( tr("Start computation"), this );
  connect(startCalcButton, SIGNAL(clicked()), SLOT( startCalcButtonClicked()));

  fieldName = new QLabel(this);
  fieldName->setAlignment(Qt::AlignCenter);

  //push button to show help
  QPushButton* Help = new QPushButton(tr("Help"), this);
  connect(  Help, SIGNAL(clicked()), SLOT( helpClicked()));

  //push button to print pos
  QPushButton* print = new QPushButton(tr("Print"), this);
  print->setToolTip(tr("Print calc. positions to file: trajectory.txt") );
  connect(  print, SIGNAL(clicked()), SLOT( printClicked()));

  //push button to hide dialog
  QPushButton* Hide = new QPushButton(tr("Hide"), this);
  connect( Hide, SIGNAL(clicked()), SIGNAL(TrajHide()));

  //push button to quit, deletes all trajectoryPlot objects
  QPushButton* quit = new QPushButton(tr("Quit"), this);
  connect(quit, SIGNAL(clicked()), SLOT( quitClicked()) );

  QFrame *line1 = new QFrame( this );
  line1->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  //now create a grid layout
  QGridLayout* gridlayout = new QGridLayout(this);
  gridlayout->setColumnStretch(1,1);
  gridlayout->addWidget( collabel,        0, 0 );
  gridlayout->addWidget( colbox,          0, 1 );
  gridlayout->addWidget( lineWidthLabel,  1, 0 );
  gridlayout->addWidget( lineWidthBox,    1, 1 );
  gridlayout->addWidget( lineTypeLabel,   2, 0 );
  gridlayout->addWidget( lineTypeBox,     2, 1 );
  gridlayout->addWidget( line1,           4, 0, 1, 2 );
  gridlayout->addWidget( numposLabel,     5, 0 );
  gridlayout->addWidget( numposSpin,      5, 1 );
  gridlayout->addWidget( radiusLabel,     6, 0 );
  gridlayout->addWidget( radiusSpin,      6, 1 );
  gridlayout->addWidget( posButton,       7, 0, 1, 2);
  gridlayout->addWidget( posLabel,        8, 0, 1, 2);
  gridlayout->addWidget( edit,            9, 0, 1, 2);
  gridlayout->addWidget( posList,        10, 0, 1, 2);
  gridlayout->addWidget( deleteButton,   11, 0);
  gridlayout->addWidget( deleteAllButton,11, 1);
  gridlayout->addWidget( startCalcButton,12, 0, 1, 2);
  gridlayout->addWidget( fieldName,      13, 0, 1, 2);
  gridlayout->addWidget( Help,           14, 0 );
  gridlayout->addWidget( print,          14,1);
  gridlayout->addWidget( Hide,           15,0 );
  gridlayout->addWidget( quit,           15,1 );
}

/********************************************************/

void TrajectoryDialog::posButtonToggled(bool b)
{
  METLIBS_LOG_SCOPE(LOGVAL(b));

  //called when "Select positions om map" is clicked
  Q_EMIT markPos(b);
  clearPlottedTrajectories();
}

/*********************************************/

void TrajectoryDialog::numposSlot(int value)
{
  int current = posList->currentRow();
  if (current < 0)
    return;

  positionVector[current].numPos = value;
  sendAllPositions();
}

/*********************************************/

void TrajectoryDialog::radiusSpinChanged(int i)
{
  //this slot is called when the spin box is changed

  if (i == 110) {
    // qt4 fix: setSteps() is removed from QRangeControl
    // using alternative method to set linestep(now called singlestep)
    // pagestep is NOT set at all!
    // Old line: radiusSpin->setSteps(50,50);
    radiusSpin->setSingleStep(50);

    radiusSpin->setValue(150);
  } else if (i == 100) {
    // Old line: radiusSpin->setSteps(10,10);
    radiusSpin->setSingleStep(10);
  }

  int current = posList->currentRow();
  if (current < 0)
    return;

  positionVector[current].radius=i;
  sendAllPositions();
}

/****************************************************************************/

void TrajectoryDialog::posListSlot()
{
  int current = posList->currentRow();
  if(current<0)
    return; // empty list

  radiusSpin->setValue(positionVector[current].radius);
  if (positionVector[current].radius > 100)
    // Old line: radiusSpin->setSteps(50,50);
    radiusSpin->setSingleStep(50);
  numposSpin->setValue(positionVector[current].numPos);

  clearPlottedTrajectories();
  sendAllPositions();
}

/****************************************************************************/

void TrajectoryDialog::editDone()
{
  //this slot is called when return is pressed in the line edit

  clearPlottedTrajectories();

  float lat=0,lon=0;
  if(!edit->getValues(lat,lon))
    METLIBS_LOG_DEBUG("getValues returned false");

  mapPos(lat,lon);
  edit->clear();
}

/*********************************************/

void TrajectoryDialog::deleteClicked()
{
  //delete selected group of start positions

  if(posList->currentRow()<0)
    return; // empty list

  if (positionVector.size()==1) {
    deleteAllClicked();
  } else {
    std::vector<posStruct>::iterator p;
    p= positionVector.begin()+posList->currentRow();
    positionVector.erase(p);
    posList->takeItem(posList->currentRow());

    clearPlottedTrajectories();
    sendAllPositions();

    posListSlot();
  }
}

/*********************************************/

void TrajectoryDialog::deleteAllClicked()
{
  //this slot is called when delete all button pressed

  posList->clear();
  positionVector.clear();

  clearPlottedTrajectories();
  sendAllPositions();
}

/*********************************************/

void TrajectoryDialog::startCalcButtonClicked()
{
  //this slot is called when start calc. button pressed
  METLIBS_LOG_SCOPE();

  //ask for new fields
  std::vector<std::string> fields = contr->getTrajectoryFields();
  int nr_fields=fields.size();
  std::string fName;
  //using first field if there is any field
  if(nr_fields > 0){
    fieldName->setText(QString(fields[0].c_str()));
    fName = fields[0];
  } else {
    fieldName->setText(tr("No field selected"));
    fName ="No field selected";
  }

  //send field name to TrajectoryPlot
  std::vector<std::string> vstr;
  vstr.push_back("clear");
  vstr.push_back("field=\"" + fName + "\"");
  contr->trajPos(vstr);

  Q_EMIT updateTrajectories();

  diutil::OverrideCursor waitCursor;
  contr->startTrajectoryComputation();
}

/*********************************************/

void TrajectoryDialog::quitClicked()
{
  std::vector<std::string> vstr;
  vstr.push_back("quit");
  contr->trajPos(vstr);
  //   posList->clear();
  Q_EMIT markPos(false);
  Q_EMIT TrajHide();
}

/*********************************************/

void TrajectoryDialog::helpClicked()
{
  emit showsource("ug_trajectories.html");
}

/*********************************************/

void TrajectoryDialog::printClicked()
{
  contr->printTrajectoryPositions("trajectory.txt");
}

/*********************************************/

void TrajectoryDialog::applyhideClicked()
{
  Q_EMIT TrajHide();
}

/*********************************************/

void TrajectoryDialog::colourSlot(int i)
{
  contr->trajPos(std::vector<std::string>(1, "colour=" + colourInfo[i].name));
  Q_EMIT updateTrajectories();
}

/*********************************************/

void TrajectoryDialog::lineWidthSlot(int i)
{
  std::ostringstream ss;
  ss << "linewidth=" << i + 1;  // 1,2,3,...
  contr->trajPos(std::vector<std::string>(1, ss.str()));
  Q_EMIT updateTrajectories();
}

void TrajectoryDialog::lineTypeSlot( int i) {

  std::ostringstream ss;
  ss << "linetype=" << linetypes[i];
  contr->trajPos(std::vector<std::string>(1, ss.str()));
  Q_EMIT updateTrajectories();
}

/*********************************************/

void TrajectoryDialog::clearPlottedTrajectories()
{
  contr->trajPos(std::vector<std::string>(1, "clear")); //delete all trajectories
}

std::string TrajectoryDialog::makePosString(const posStruct& pos) const
{
  std::ostringstream ost;
  ost << "numpos=" << pos.numPos;
  ost << " radius=" << pos.radius;
  ost << std::setw(5) << std::setprecision(2) << std::setiosflags(std::ios::fixed);
  ost << " latitudelongitude=" << pos.lat << "," << pos.lon;
  return ost.str();
}

void TrajectoryDialog::mapPos(float lat, float lon)
{
  METLIBS_LOG_SCOPE();

  //Put this position in vector of positions
  posStruct pos;
  pos.lat=lat;
  pos.lon=lon;
  pos.radius=radiusSpin->value();
  pos.numPos=numposSpin->value();
  positionVector.push_back(pos);

  //Make string and insert in posList
  update_posList(lat,lon);

  //Make string and send to trajectoryPlot
  std::vector<std::string> vstr;
  vstr.push_back("clear");
  vstr.push_back(makePosString(pos));
  contr->trajPos(vstr);
  Q_EMIT updateTrajectories();
}

/*********************************************/

void TrajectoryDialog::update_posList(float lat, float lon)
{
  METLIBS_LOG_SCOPE();
  const QString pos = diutil::formatLatitudeDegMin(lat)
      + " "
      + diutil::formatLongitudeDegMin(lon);
  posList->addItem(pos);
}

/*********************************************/

void TrajectoryDialog::sendAllPositions()
{
  METLIBS_LOG_SCOPE();

  std::vector<std::string> vstr(1, "delete"); //delete all start positions
  for (std::vector<posStruct>::const_iterator itP = positionVector.begin(); itP != positionVector.end(); ++itP) {
    vstr.push_back(makePosString(*itP));
  }
  contr->trajPos(vstr);
  Q_EMIT updateTrajectories();
}


void TrajectoryDialog::showplus()
{
  METLIBS_LOG_SCOPE();
  this->show();

  if (posButton->isChecked())
    Q_EMIT markPos(true);

  std::ostringstream ss;
  ss <<"colour="<<colourInfo[colbox->currentIndex()].name;
  ss <<" linewidth="<< lineWidthBox->currentIndex() + 1;  // 1,2,3,...
  ss <<" linetype="<< linetypes[lineTypeBox->currentIndex()];  // 1,2,3,...
  ss <<" plot=on";
  contr->trajPos(std::vector<std::string>(1, ss.str()));

  sendAllPositions();
}

/*********************************************/

std::vector<std::string> TrajectoryDialog::writeLog()
{
  std::vector<std::string> vstr;

  for (std::vector<posStruct>::const_iterator itP = positionVector.begin(); itP != positionVector.end(); ++itP) {
    vstr.push_back(makePosString(*itP));
  }

  std::ostringstream ostr;
  if (colbox->currentIndex() > -1)
    ostr <<"colour="<<colourInfo[colbox->currentIndex()].name;
  if (lineWidthBox->currentIndex() > -1)
    ostr <<" linewidth="<< lineWidthBox->currentIndex() + 1;  // 1,2,3,...
  if (lineTypeBox->currentIndex() > -1)
    ostr <<" linetype="<< linetypes[lineTypeBox->currentIndex()];
  vstr.push_back(ostr.str());
  vstr.push_back("================");
  return vstr;

}
/*********************************************/

void TrajectoryDialog::readLog(const std::vector<std::string>& vstr, const std::string& thisVersion, const std::string& logVersion)
{
  int n=0, nvstr= vstr.size();
  int radius = 0, numPos = 1;

  while (n<nvstr && vstr[n].substr(0,4)!="====") {

    posStruct pos;
    bool position=false;
    std::vector<std::string> parts = miutil::split(vstr[n], 0, " ", true);

    int nr=parts.size();
    for (int i=0; i<nr; i++) {
      std::vector<std::string> tokens = miutil::split(parts[i], 0, "=", true);
      if (tokens.size() == 2) {
        std::string key = miutil::to_lower(tokens[0]);
        std::string value = miutil::to_lower(tokens[1]);
        if (key == "colour") {
          int number= getIndex( colourInfo, value);
          if (number>=0) {
            colbox->setCurrentIndex(number);
          }
        } else if (key == "line" || key == "linewidth") {
          lineWidthBox->setCurrentIndex(atoi(value.c_str())-1);
        } else if (key == "linetype") {
          int j=0;
          int nr_linetypes=linetypes.size();
          while (j<nr_linetypes && value!=linetypes[j]) j++;
          if (j==nr_linetypes) j=0;
          lineTypeBox->setCurrentIndex(j);
        } else if (key == "radius") {
          pos.radius = radius = miutil::to_int(value);
        } else if (key == "numpos") {
          numPos = pos.numPos = miutil::to_int(value);
        } else if (key == "latitudelongitude") {
          const std::vector<std::string> latlon = miutil::split(value, 0, ",");
          if(latlon.size() != 2)
            continue;
          pos.lat = miutil::to_float(latlon[0]);
          pos.lon = miutil::to_float(latlon[1]);
          position=true;
          update_posList(pos.lat,pos.lon);
        }
      }
    }
    if (position)
      positionVector.push_back(pos);
    n++;
  }

  radiusSpin->setValue(radius);
  if (radius > 100)
    // qt4 fix: insertStrList() -> insertStringList()
    // (uneffective, have to make QStringList and QString!)
    // Old line: radiusSpin->setSteps(50,50);
    radiusSpin->setSingleStep(50);
  numposSpin->setValue(numPos);

  //positions are not sent to TrajectoryPlot yet
}

void TrajectoryDialog::closeEvent(QCloseEvent* e)
{
  Q_EMIT markPos(false);
  Q_EMIT TrajHide();
}
