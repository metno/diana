/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "qtObsDialog.h"
#include "qtObsWidget.h"
#include "qtUtility.h"
#include "qtToggleButton.h"

#include "util/misc_util.h"

#include <puTools/miStringFunctions.h>

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLCDNumber>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QRadioButton>
#include <QSlider>
#include <QStackedWidget>
#include <QToolTip>
#include <QVBoxLayout>

#include <qpushbutton.h>
#include <math.h>

#define MILOGGER_CATEGORY "diana.ObsDialog"
#include <miLogger/miLogging.h>

using namespace std;

static QString labelForObsPlotType(ObsPlotType opt)
{
  switch (opt) {
  case OPT_SYNOP:
    return "Synop";
  case OPT_METAR:
    return "Metar";
  case OPT_LIST:
    return "List";
  case OPT_PRESSURE:
    return QApplication::translate("ObsDialog", "Pressure");
  case OPT_TIDE:
    return QApplication::translate("ObsDialog", "Tide");
  case OPT_OCEAN:
    return QApplication::translate("ObsDialog", "Ocean");
  case OPT_OTHER:
    return QApplication::translate("ObsDialog", "Other");
  }
}

ObsDialog::ObsDialog(QWidget* parent, Controller* llctrl)
    : QDialog(parent)
    , m_ctrl(llctrl)
{
  setWindowTitle(tr("Observations"));

  dialog = m_ctrl->initObsDialog();

  vector<std::string> dialog_name;
  nr_plot = dialog.plottype.size();

  if (nr_plot == 0)
    return;

  plotbox = new QComboBox(this);
  plotbox->setToolTip(tr("select plot type"));
  for (const ObsDialogInfo::PlotType& plt : dialog.plottype) {
    if (plt.plottype == OPT_OTHER)
      plotbox->addItem(QString::fromStdString(plt.name));
    else
      plotbox->addItem(labelForObsPlotType(plt.plottype), (int)plt.plottype);
  }
  plotbox->setCurrentIndex(0);
  savelog.clear();
  savelog.resize(nr_plot);

  m_selected = 0;

  stackedWidget = new QStackedWidget;

  for (int i = 0; i < nr_plot; i++) {
    ObsWidget* ow = new ObsWidget(this);
    if (!dialog.plottype[i].button.empty()) {
      ow->setDialogInfo(dialog.plottype[i]);
      connect(ow, SIGNAL(getTimes(bool)), SLOT(getTimes(bool)));
      connect(ow, SIGNAL(rightClicked(std::string)), SLOT(rightButtonClicked(std::string)));
      connect(ow, SIGNAL(extensionToggled(bool)), SLOT(extensionToggled(bool)));
      connect(ow, SIGNAL(criteriaOn()), SLOT(criteriaOn()));
    }
    stackedWidget->addWidget(ow);
    obsWidget.push_back(ow);
  }

  for (int i = 1; i < nr_plot; i++)
    if (obsWidget[i])
      obsWidget[i]->hide();

  multiplot = false;

  multiplotButton = new ToggleButton(this, tr("Show all"));
  multiplotButton->setToolTip(tr("Show all plot types"));
  obshelp = NormalPushButton(tr("Help"), this);
  obsrefresh = NormalPushButton(tr("Refresh"), this);
  obshide = NormalPushButton(tr("Hide"), this);
  obsapplyhide = NormalPushButton(tr("Apply + Hide"), this);
  obsapply = NormalPushButton(tr("Apply"), this);
  obsapply->setDefault(true);

  connect(multiplotButton, SIGNAL(toggled(bool)), SLOT(multiplotClicked(bool)));
  connect(obshide, SIGNAL(clicked()), SIGNAL(ObsHide()));
  connect(obsapply, SIGNAL(clicked()), SIGNAL(ObsApply()));
  connect(obsrefresh, SIGNAL(clicked()), SLOT(getTimes()));
  connect(obsapplyhide, SIGNAL(clicked()), SLOT(applyhideClicked()));
  connect(obshelp, SIGNAL(clicked()), SLOT(helpClicked()));

  QHBoxLayout* helplayout = new QHBoxLayout();
  helplayout->addWidget(obshelp);
  helplayout->addWidget(obsrefresh);
  helplayout->addWidget(multiplotButton);

  QHBoxLayout* applylayout = new QHBoxLayout();
  applylayout->addWidget(obshide);
  applylayout->addWidget(obsapplyhide);
  applylayout->addWidget(obsapply);

  QVBoxLayout* vlayout = new QVBoxLayout(this);
  vlayout->setSpacing(1);
  vlayout->addWidget(plotbox);
  vlayout->addWidget(stackedWidget);
  vlayout->addLayout(helplayout);
  vlayout->addLayout(applylayout);

  connect(plotbox, SIGNAL(activated(int)), SLOT(plotSelected(int)));

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
  PlotCommand_cpv vstr = getOKString();

  //remove old widgets
  for (int i = 0; i < nr_plot; i++) {
    stackedWidget->removeWidget(obsWidget[i]);
    obsWidget[i]->close();
    delete obsWidget[i];
    obsWidget[i] = NULL;
  }
  obsWidget.clear();

  //Make new widgets
  dialog = m_ctrl->initObsDialog();

  nr_plot = dialog.plottype.size();

  plotbox->clear();
  for (const ObsDialogInfo::PlotType& plt : dialog.plottype)
    plotbox->addItem(labelForObsPlotType(plt.plottype), (int)plt.plottype);

  m_selected = 0;

  for (int i = 0; i < nr_plot; i++) {
    ObsWidget* ow = new ObsWidget(this);
    if (!dialog.plottype[i].button.empty()) {
      ow->setDialogInfo(dialog.plottype[i]);
      connect(ow, SIGNAL(getTimes(bool)), SLOT(getTimes(bool)));
      connect(ow, SIGNAL(rightClicked(std::string)), SLOT(rightButtonClicked(std::string)));
      connect(ow, SIGNAL(extensionToggled(bool)), SLOT(extensionToggled(bool)));
      connect(ow, SIGNAL(criteriaOn()), SLOT(criteriaOn()));
    }
    stackedWidget->addWidget(ow);
    obsWidget.push_back(ow);
  }

  for (int i = 1; i < nr_plot; i++)
    if (obsWidget[i])
      obsWidget[i]->hide();

  //reset selections
  putOKString(vstr);
}

void ObsDialog::plotSelected(int index, bool sendTimes)
{
  METLIBS_LOG_SCOPE();
  /* This function is called when a new plottype is selected and builds
   the screen up with a new obsWidget widget */

  if (m_selected == index && obsWidget[index]->initialized())
    return;

  ObsWidget* ow = obsWidget[index];
  if (!ow->initialized()) {
    m_ctrl->updateObsDialog(dialog.plottype[index], plotbox->itemText(index).toStdString());

    ow->setDialogInfo(dialog.plottype[index]);
    connect(ow, SIGNAL(getTimes(bool)), SLOT(getTimes(bool)));
    connect(ow, SIGNAL(rightClicked(std::string)), SLOT(rightButtonClicked(std::string)));
    connect(ow, SIGNAL(extensionToggled(bool)), SLOT(extensionToggled(bool)));
    connect(ow, SIGNAL(criteriaOn()), SLOT(criteriaOn()));

    if (index < (int)savelog.size() && !savelog[index].empty()) {
      ow->readLog(savelog[index]);
      savelog[index].clear();
    }
  }

  //Emit empty time list
  Q_EMIT emitTimes("obs", plottimes_t());

  //criteria
  if (obsWidget[m_selected]->moreToggled()) {
    showExtension(false);
  }

  m_selected = index;

  stackedWidget->setCurrentIndex(m_selected);

  //criteria
  if (obsWidget[m_selected]->moreToggled()) {
    showExtension(true);
  }

  updateExtension();

  if (sendTimes)
    getTimes(false);
}

void ObsDialog::getTimes(bool update)
{
  // Names of datatypes selected are sent to controller,
  // and times are returned

  diutil::OverrideCursor waitCursor;

  vector<std::string> dataName;
  if (multiplot) {

    set<std::string> nameset;
    for (int i = 0; i < nr_plot; i++) {
      if (obsWidget[i]->initialized()) {
        diutil::insert_all(nameset, obsWidget[i]->getDataTypes());
      }
    }
    dataName = std::vector<std::string>(nameset.begin(), nameset.end());

  } else {

    dataName = obsWidget[m_selected]->getDataTypes();
  }

  plottimes_t times = m_ctrl->getObsTimes(dataName, update);
  Q_EMIT emitTimes("obs", times);
}

void ObsDialog::applyhideClicked()
{
  emit ObsHide();
  emit ObsApply();
}

void ObsDialog::helpClicked()
{
  //  emit setSource("ug_obsdialogue.html");
  emit showsource("ug_obsdialogue.html");
}

void ObsDialog::multiplotClicked(bool b)
{
  multiplot = b;
}

void ObsDialog::extensionToggled(bool b)
{
  if (b)
    updateExtension();
  showExtension(b);
}

void ObsDialog::criteriaOn()
{
  updateExtension();
}

void ObsDialog::archiveMode(bool on)
{
  getTimes(true);
}

/*******************************************************/
PlotCommand_cpv ObsDialog::getOKString()
{
  PlotCommand_cpv str;

  if (nr_plot == 0)
    return str;

  if (multiplot) {
    for (int i=nr_plot-1; i>-1; i--) {
      if (obsWidget[i]->initialized()) {
        if (PlotCommand_cp cmd = obsWidget[i]->getOKString())
          str.push_back(cmd);
      }
    }
  } else {
    if (PlotCommand_cp cmd = obsWidget[m_selected]->getOKString())
      str.push_back(cmd);
  }

  return str;
}


vector<string> ObsDialog::writeLog()
{
  vector<string> vstr;

  if (nr_plot == 0)
    return vstr;

  std::string str;

  //first write the plot type selected now
  if (KVListPlotCommand_cp cmd = obsWidget[m_selected]->getOKString(true))
    vstr.push_back(miutil::mergeKeyValue(cmd->all()));

  //then the others
  for (int i=0; i<nr_plot; i++) {
    if (i != m_selected) {
      if (obsWidget[i]->initialized()) {
        if (KVListPlotCommand_cp cmd = obsWidget[i]->getOKString(true))
          vstr.push_back(miutil::mergeKeyValue(cmd->all()));
      } else if (i < savelog.size() && savelog[i].size() > 0) {
        // ascii obs dialog not activated
        vstr.push_back(miutil::mergeKeyValue(savelog[i]));
      }
    }
  }

  vstr.push_back("================");

  return vstr;
}

void ObsDialog::readLog(const vector<string>& vstr, const string& thisVersion, const string& logVersion)
{

  int n=0, nvstr= vstr.size();
  bool first=true;

  while (n < nvstr && vstr[n].substr(0, 4) != "====") {

    const miutil::KeyValue_v kvs = miutil::splitKeyValue(vstr[n]);
    const int index = findPlotnr(kvs);
    if (index < nr_plot) {
      if (obsWidget[index]->initialized() || first) {
        if (first) {  //will be selected
          first = false;
          plotbox->setCurrentIndex(index);
          plotSelected(index);
        }
        obsWidget[index]->readLog(kvs);
      }
      // save until (ascii/hqc obs) dialog activated, or until writeLog
      savelog[index] = kvs;

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


void ObsDialog::putOKString(const PlotCommand_cpv& vstr)
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
  Q_EMIT emitTimes("obs", plottimes_t());

  if (vstr.size() > 1) {
    multiplot=true;
    multiplotButton->setChecked(true);
  }
  for (PlotCommand_cp cmd : vstr) {
    KVListPlotCommand_cp c = std::dynamic_pointer_cast<const KVListPlotCommand>(cmd);
    METLIBS_LOG_DEBUG(LOGVAL(cmd->toString()));
    if (!c)
      continue;

    int l = findPlotnr(c->all());
    if (l < nr_plot) {
      plotbox->setCurrentIndex(l);
      plotSelected(l);
      obsWidget[l]->putOKString(cmd);
    }
  }
}


int ObsDialog::findPlotnr(const miutil::KeyValue_v& str)
{
  const size_t i_plot = miutil::rfind(str, "plot");
  if (i_plot != size_t(-1)) {
    std::string value = (miutil::to_lower(str.at(i_plot).value()));
    int l = 0;
    while (l < nr_plot && miutil::to_lower(plotbox->itemText(l).toStdString()) != value)
      l++;
    return l;
  }
  return nr_plot; // not found
}

bool ObsDialog::setPlottype(const std::string& name, bool on)
{
  METLIBS_LOG_SCOPE();

  int l=0;
  while (l < nr_plot && miutil::to_lower(plotbox->itemText(l).toStdString()) != name)
    l++;
  if (l == nr_plot)
    return false;

  if (on) {
    plotbox->setCurrentIndex(l);
    ObsWidget* ow = new ObsWidget( this );
    PlotCommand_cp str;
    if (obsWidget[l]->initialized()) {
      str = obsWidget[l]->getOKString();
    } else if (l < savelog.size() && !savelog[l].empty()) {
      KVListPlotCommand_p cmd = std::make_shared<KVListPlotCommand>("OBS");
      cmd->add(savelog[l]);
      str = cmd;
    }
    stackedWidget->removeWidget(obsWidget[l]);
    obsWidget[l]->close();
    delete obsWidget[l];
    obsWidget[l]=ow;
    stackedWidget->insertWidget(l,obsWidget[l]);

    plotSelected(l,false);
    if (str)
      obsWidget[l]->putOKString(str);

  } else if (obsWidget[l]->initialized()) {
    obsWidget[l]->setFalse();
    getTimes(false);
  }

  return true;
}

void ObsDialog::closeEvent(QCloseEvent*)
{
  Q_EMIT ObsHide();
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

  ObsDialogInfo::CriteriaList critList = obsWidget[m_selected]->getCriteriaList();

  criteriaListbox->clear();

  lineedit->clear();

  int n = critList.criteria.size();
  if (n == 0)
    return;

  lineedit->setText(critList.name.c_str());
  for (int j = 0; j < n; j++) {
    criteriaListbox->addItem(QString(critList.criteria[j].c_str()));
  }
  if (criteriaListbox->count()) {
    criteriaListbox->item(0)->setSelected(true);
    criteriaSelected(criteriaListbox->item(0));
  }
}

void ObsDialog::signSlot(int number)
{
  bool on = (number != 3);
  limitLcd->setEnabled(on);
  limitSlider->setEnabled(on);
  changeCriteriaString();
}

void ObsDialog::sliderSlot(int number)
{
  double scalednumber = number * stepComboBox->currentText().toFloat();
  limitLcd->display(scalednumber);
  changeCriteriaString();
}

void ObsDialog::stepSlot(int number)
{
//todo: smarter slider limits
  float scalesize = stepComboBox->currentText().toFloat();
  numberList(stepComboBox, scalesize);
  limitSlider->setMaximum(1000);
  limitSlider->setMinimum(-1000);
  limitSlider->setValue(int(limitLcd->value()/scalesize));
  double scalednumber = limitSlider->value() * scalesize;
  limitLcd->display(scalednumber);
  changeCriteriaString();
}

void ObsDialog::changeCriteriaString()
{
  if (freeze)
    return;
  if (criteriaListbox->count() == 0)
    return;
  if (criteriaListbox->currentRow() < 0) {
    criteriaListbox->setCurrentRow(0);
  }

  freeze = true; //don't change the string while updating the widgets

  std::string str = makeCriteriaString();

  if (!str.empty()) {
    criteriaListbox->currentItem()->setText(QString::fromStdString(str));
    // save changes
    int n = criteriaListbox->count();
    vector<std::string> vstr;
    for (int i = 0; i < n; i++) {
      vstr.push_back(criteriaListbox->item(i)->text().toStdString());
    }
    obsWidget[m_selected]->saveCriteria(vstr);

  } else {
    deleteSlot();
  }

  freeze=false;
}

bool ObsDialog::newCriteriaString()
{
  if (freeze)
    return false;

  std::string str = makeCriteriaString();
  if (str.empty())
    return false;

  freeze=true;
  int n = criteriaListbox->count();
  bool found = false;
  int i = 0;
  for (; i < n; i++) {
    std::string sstr = criteriaListbox->item(i)->text().toStdString();
    vector<std::string> vstr;
    if (miutil::contains(sstr, "<"))
      vstr = miutil::split(sstr, "<");
    else if (miutil::contains(sstr, ">"))
      vstr = miutil::split(sstr, ">");
    else if (miutil::contains(sstr, "="))
      vstr = miutil::split(sstr, "=");
    else
      vstr = miutil::split(sstr, " ");
    //    if(vstr.size() && vstr[0]==parameterLabel->text().toStdString()){
    if (vstr.size() && vstr[0] == parameter) {
      found = true;
    } else if (found) {
      break;
    }
  }

  criteriaListbox->insertItem(i, QString(str.c_str()));
  criteriaListbox->setCurrentRow(i);
  // save changes
  n = criteriaListbox->count();
  vector<std::string> vstr;
  for (int i = 0; i < n; i++) {
    vstr.push_back(criteriaListbox->item(i)->text().toStdString());
  }
  obsWidget[m_selected]->saveCriteria(vstr);

  freeze=false;
  return true;
}

std::string ObsDialog::makeCriteriaString()
{
  std::string str=parameter;

  std::string sign = signBox->currentText().toStdString();
  if (not sign.empty()) {
    str += sign;
    std::string lcdstr = miutil::from_number(limitLcd->value());
    str += lcdstr;
  }

  if (colourButton->isChecked()) {
    str += "  ";
    str += cInfo[colourBox->currentIndex()].name;
  } else if (totalColourButton->isChecked()) {
    str += "  ";
    str += cInfo[colourBox->currentIndex()].name;
    str += " total";
  } else if (markerButton->isChecked()) {
    str += "  ";
    str += markerName[markerBox->currentIndex()];
    str += " marker";
  } else {
    str += "  plot";
  }

  return str;
}


void ObsDialog::criteriaSelected(QListWidgetItem* item)
{
  if (freeze)
    return;

  if (!item) {
    parameter.clear();
    return;
  }

  freeze=true;

  std::string str = item->text().toStdString();

  vector<std::string> sub = miutil::split(str, " ");

  //  std::string sign,parameter;
  std::string sign;
  if (miutil::contains(sub[0], ">")) {
    sign = ">";
    signBox->setCurrentIndex(0);
  } else if (miutil::contains(sub[0], "<")) {
    sign = "<";
    signBox->setCurrentIndex(1);
  } else if (miutil::contains(sub[0], "=")) {
    sign = "=";
    signBox->setCurrentIndex(2);
  } else {
    signBox->setCurrentIndex(3);
  }

  float value = 0.0;
  if (!sign.empty()) {
    vector<std::string> sstr = miutil::split(sub[0], sign);
    if (sstr.size() != 2)
      return;
    parameter = sstr[0];
    value = atof(sstr[1].c_str());
  } else {
    parameter = sub[0];
  }

  int low,high;
  if (obsWidget[m_selected]->getCriteriaLimits(parameter, low, high)) {
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
    if (value == 0) {
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

  if (sub.size() > 1) {
    if (sub[1] == "plot") {
      plotButton->setChecked(true);
      colourBox->setEnabled(false);
      markerBox->setEnabled(false);
    } else if (sub.size() > 2 && sub[2] == "marker") {
      int number = getIndex(markerName, sub[1]);
      if (number >= 0)
        markerBox->setCurrentIndex(number);
      markerButton->setChecked(true);
    } else {
      int number = getIndex(cInfo, sub[1]);
      if (number >= 0)
        colourBox->setCurrentIndex(number);
      if (sub.size() == 3 && miutil::to_lower(sub[2]) == "total") {
        totalColourButton->setChecked(true);
      } else{
        colourButton->setChecked(true);
      }
    }
  }

  freeze=false;
}

void ObsDialog::deleteSlot()
{
  if (criteriaListbox->currentRow() == -1)
    return;

  criteriaListbox->takeItem(criteriaListbox->currentRow());
  criteriaSelected(criteriaListbox->currentItem());
  int n = criteriaListbox->count();
  vector<std::string> vstr;
  for (int i = 0; i < n; i++) {
    vstr.push_back(criteriaListbox->item(i)->text().toStdString());
  }
  //save criterias and reread in order to get buttons marked right
  obsWidget[m_selected]->saveCriteria(vstr);

}

void ObsDialog::deleteAllSlot()
{
  criteriaListbox->clear();
  vector<std::string> vstr;
  obsWidget[m_selected]->saveCriteria(vstr);
}

void ObsDialog::saveSlot()
{
  std::string name = lineedit->text().toStdString();
  if (name.empty())
    return;

  int n = criteriaListbox->count();
  vector<std::string> vstr;
  for (int i = 0; i < n; i++) {
    vstr.push_back(criteriaListbox->item(i)->text().toStdString());
  }

  //save criterias and reread in order to get buttons marked right

  bool newItem = obsWidget[m_selected]->saveCriteria(vstr, name);
  if (newItem) {
    criteriaBox->addItem(name.c_str());
    int index = criteriaBox->count() - 1;
    criteriaBox->setCurrentIndex(index);
    criteriaListSelected(index);
    return;
  }

  if (vstr.size() == 0) {
    int index = criteriaBox->currentIndex();
    criteriaBox->removeItem(index);
    if (index == criteriaBox->count())
      index--;
    if (index < 0) {
      lineedit->clear();
    } else {
      criteriaBox->setCurrentIndex(index);
      criteriaListSelected(index);
    }
  }
}

void ObsDialog::rightButtonClicked(std::string name)
{
  // Wind
  if (name == "Wind") {
    rightButtonClicked("dd");
    rightButtonClicked("ff");
    return;
  }

  //Pos
  if (name == "Pos") {
    rightButtonClicked("lat");
    rightButtonClicked("lon");
    return;
  }

  freeze =true;
  bool sameParameter = false;
  if (parameter == name)
    sameParameter = true;

  parameter = name;

  int low, high;
  if (obsWidget[m_selected]->getCriteriaLimits(name, low, high)) {
    limitSlider->setMinimum(low);
    limitSlider->setMaximum(high);
    signBox->setEnabled(true);
    plotButton->setEnabled(true);
    colourBox->setEnabled(colourButton->isChecked() || totalColourButton->isChecked());
    markerBox->setEnabled(markerButton->isChecked());
    bool sign = (signBox->currentIndex() != 3);
    limitSlider->setEnabled(sign);
    limitLcd->setEnabled(sign);
    if (!sameParameter) {
      numberList(stepComboBox, 1.0);
      double scalednumber = limitSlider->value();
      limitLcd->display(scalednumber);
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
}

void ObsDialog::updateExtension()
{
  ObsDialogInfo::CriteriaList cList;
  criteriaBox->clear();
  vector<std::string> critName = obsWidget[m_selected]->getCriteriaNames();
  int n = critName.size();
  if (n == 0) { // no lists, read saved criterias
    cList = obsWidget[m_selected]->getSavedCriteria();
  } else {
    for (unsigned int i = 0; i < critName.size(); i++) {
      criteriaBox->addItem(critName[i].c_str());
    }
    cList = obsWidget[m_selected]->getSavedCriteria();
  }

  criteriaListbox->clear();
  lineedit->setText(criteriaBox->currentText());

  vector<std::string> criteriaList = cList.criteria;
  for (unsigned int j = 0; j < criteriaList.size(); j++) {
    criteriaListbox->addItem(QString(criteriaList[j].c_str()));
  }
  if (criteriaListbox->count()) {
    criteriaListbox->item(0)->setSelected(true);
    criteriaListbox->setCurrentRow(0);
    criteriaSelected(criteriaListbox->item(0));
  }
}

void ObsDialog::numberList(QComboBox* cBox, float number)
{
  const float enormal[] = {0.001, 0.01, 0.1, 1.0, 10., 100., 1000., 10000., -1};
  diutil::numberList(cBox, number, enormal, false);
  cBox->setEnabled(true);
}
