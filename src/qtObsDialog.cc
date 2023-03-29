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

#include "diController.h"
#include "qtObsDialog.h"
#include "qtObsWidget.h"
#include "qtToggleButton.h"
#include "qtUtility.h"

#include "util/misc_util.h"
#include "util/string_util.h"

#include <puTools/miStringFunctions.h>

#include <QAction>
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

#include "synop.xpm"

#define MILOGGER_CATEGORY "diana.ObsDialog"
#include <miLogger/miLogging.h>


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
  return "Error"; // not reached
}

static void numberList(QComboBox* cBox, float number)
{
  const float enormal[] = {0.001, 0.01, 0.1, 1.0, 10., 100., 1000., 10000., -1};
  diutil::numberList(cBox, number, enormal, false);
  cBox->setEnabled(true);
}

ObsDialog::ObsDialog(QWidget* parent, Controller* llctrl)
    : DataDialog(parent, llctrl)
{
  setWindowTitle(tr("Observations"));
  m_action = new QAction(QIcon(QPixmap(synop_xpm)), windowTitle(), this);
  m_action->setShortcut(Qt::ALT + Qt::Key_O);
  m_action->setCheckable(true);
  m_action->setIconVisibleInMenu(true);
  helpFileName = "ug_obsdialogue.html";

  plotbox = new QComboBox(this);
  plotbox->setToolTip(tr("select plot type"));
  stackedWidget = new QStackedWidget;

  loadDialogInfo();

  if (nr_plot() > 0)
    plotbox->setCurrentIndex(0);
  savelog.resize(nr_plot());

  multiplot = false;

  multiplotButton = new ToggleButton(this, tr("Show all"));
  multiplotButton->setToolTip(tr("Show all plot types"));

  connect(multiplotButton, &QPushButton::toggled, this, &ObsDialog::multiplotClicked);

  QHBoxLayout* helplayout = new QHBoxLayout();
  helplayout->addWidget(multiplotButton);

  QLayout* applylayout = createStandardButtons(true);

  QVBoxLayout* vlayout = new QVBoxLayout(this);
  vlayout->setSpacing(1);
  vlayout->addWidget(plotbox);
  vlayout->addWidget(stackedWidget, 1);
  vlayout->addLayout(helplayout);
  vlayout->addLayout(applylayout);

  connect(plotbox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &ObsDialog::plotSelected);

  plotbox->setFocus();

  this->hide();
  setOrientation(Qt::Horizontal);
  makeExtension();
  setExtension(extension);
  showExtension(false);
}

std::string ObsDialog::name() const
{
  static const std::string OBS_DATATYPE = "obs";
  return OBS_DATATYPE;
}

void ObsDialog::loadDialogInfo()
{
  dialog = m_ctrl->initObsDialog();

  plotbox->clear();
  for (const ObsDialogInfo::PlotType& plt : dialog.plottype) {
    if (plt.plottype == OPT_OTHER)
      plotbox->addItem(QString::fromStdString(plt.name));
    else
      plotbox->addItem(labelForObsPlotType(plt.plottype), (int)plt.plottype);
  }

  m_selected = 0;

  // remove old widgets
  for (ObsWidget* ow : obsWidget) {
    stackedWidget->removeWidget(ow);
    ow->close();
    delete ow;
  }
  obsWidget.clear();

  // create new widgets
  for (const ObsDialogInfo::PlotType& pt : dialog.plottype) {
    ObsWidget* ow = new ObsWidget(this);
    if (!pt.button.empty()) {
      ow->setDialogInfo(pt);
      connect(ow, &ObsWidget::getTimes, this, &ObsDialog::getTimes);
      connect(ow, &ObsWidget::rightClicked, this, &ObsDialog::rightButtonClicked);
      connect(ow, &ObsWidget::extensionToggled, this, &ObsDialog::extensionToggled);
      connect(ow, &ObsWidget::criteriaOn, this, &ObsDialog::criteriaOn);
    }
    stackedWidget->addWidget(ow);
    if (!obsWidget.empty()) // do not hide 1st
      ow->hide();
    obsWidget.push_back(ow);
  }
}

void ObsDialog::updateDialog()
{
  PlotCommand_cpv vstr = getOKString();
  loadDialogInfo();
  putOKString(vstr);
}

void ObsDialog::plotSelected(int index)
{
  METLIBS_LOG_SCOPE();
  /* This function is called when a new plottype is selected and builds
   the screen up with a new obsWidget widget */

  if (m_selected == index && obsWidget[index]->initialized())
    return;

  ObsWidget* ow = obsWidget[index];
  if (ow && !ow->initialized()) {
    m_ctrl->updateObsDialog(dialog.plottype[index], plotbox->itemText(index).toStdString());

    ow->setDialogInfo(dialog.plottype[index]);
    connect(ow, &ObsWidget::getTimes, this, &ObsDialog::getTimes);
    connect(ow, &ObsWidget::rightClicked, this, &ObsDialog::rightButtonClicked);
    connect(ow, &ObsWidget::extensionToggled, this, &ObsDialog::extensionToggled);
    connect(ow, &ObsWidget::criteriaOn, this, &ObsDialog::criteriaOn);

    if (index < (int)savelog.size() && !savelog[index].empty()) {
      ow->readLog(savelog[index]);
      savelog[index].clear();
    }
  }

  //Emit empty time list
  emitTimes(plottimes_t());

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

  getTimes(false);
}

void ObsDialog::updateTimes()
{
  getTimes(true);
}

void ObsDialog::getTimes(bool update)
{
  // Names of datatypes selected are sent to controller,
  // and times are returned

  diutil::OverrideCursor waitCursor;

  std::vector<std::string> dataName;
  if (multiplot) {

    std::set<std::string> nameset;
    for (ObsWidget* ow : obsWidget) {
      if (ow->initialized()) {
        diutil::insert_all(nameset, ow->getDataTypes());
      }
    }
    dataName = std::vector<std::string>(nameset.begin(), nameset.end());

  } else {

    dataName = obsWidget[m_selected]->getDataTypes();
  }

  plottimes_t times = m_ctrl->getObsTimes(dataName, update);
  emitTimes(times);
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

void ObsDialog::doShowMore(bool show)
{
  extensionToggled(show);
}

void ObsDialog::criteriaOn()
{
  updateExtension();
}

void ObsDialog::archiveMode(bool)
{
  getTimes(true);
}

/*******************************************************/
PlotCommand_cpv ObsDialog::getOKString()
{
  PlotCommand_cpv str;

  if (nr_plot() == 0)
    return str;

  if (multiplot) {
    for (int i=nr_plot()-1; i>-1; i--) {
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


std::vector<std::string> ObsDialog::writeLog()
{
  std::vector<std::string> vstr;

  if (nr_plot() == 0)
    return vstr;

  // first write the plot type selected now
  if (KVListPlotCommand_cp cmd = obsWidget[m_selected]->getOKString(true))
    vstr.push_back(miutil::mergeKeyValue(cmd->all()));

  //then the others
  for (int i=0; i<nr_plot(); i++) {
    if (i != m_selected) {
      if (obsWidget[i]->initialized()) {
        if (KVListPlotCommand_cp cmd = obsWidget[i]->getOKString(true))
          vstr.push_back(miutil::mergeKeyValue(cmd->all()));
      } else if (i < (int)savelog.size() && !savelog[i].empty()) {
        // ascii obs dialog not activated
        vstr.push_back(miutil::mergeKeyValue(savelog[i]));
      }
    }
  }

  vstr.push_back("================");

  return vstr;
}

void ObsDialog::readLog(const std::vector<std::string>& vstr, const std::string& /*thisVersion*/, const std::string& /*logVersion*/)
{
  for (const std::string& l : vstr) {
    if (diutil::startswith(l, "===="))
      break;

    const miutil::KeyValue_v kvs = miutil::splitKeyValue(l);
    const int index = findPlotnr(kvs);
    if (index < nr_plot()) {
      if (obsWidget[index]->initialized())
        obsWidget[index]->readLog(kvs);
      // save until ascii dialog activated, or until writeLog
      savelog[index] = kvs;
    }
  }
}

std::string ObsDialog::getShortname()
{
  std::string name;

  if (nr_plot() == 0)
    return name;

  name = obsWidget[m_selected]->getShortname();
  if (not name.empty())
    name= "<font color=\"#999900\">" + name + "</font>";

  return name;
}


void ObsDialog::putOKString(const PlotCommand_cpv& vstr)
{
  //unselect everything
  for (int i=0; i<nr_plot(); i++) {
    if ( obsWidget[i]->initialized() ) {
      obsWidget[i]->setFalse();
    }
  }
  multiplotButton->setChecked(false);
  multiplot=false;

  //Emit empty time list
  emitTimes(plottimes_t());

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
    if (l < nr_plot()) {
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
    const int l = plotbox->findText(QString::fromStdString(str.at(i_plot).value()), Qt::MatchFixedString); // case insensitive
    if (l >= 0)
      return l;
  }
  return nr_plot(); // not found
}

void ObsDialog::makeExtension()
{
  freeze = false;

  extension = new QWidget(this);

  QLabel* listLabel = TitleLabel(tr("List of Criteria"),extension);
  std::vector<std::string> critName;
  if (m_selected >= 0 && m_selected < (int)obsWidget.size() && m_selected < nr_plot())
    critName = obsWidget[m_selected]->getCriteriaNames();
  criteriaBox = ComboBox( extension,critName,true);

  QLabel* criteriaLabel = TitleLabel(tr("Criteria"),extension);
  criteriaListbox = new QListWidget(extension);

  QPushButton* delButton = new QPushButton(tr("Delete"), extension);
  QPushButton* delallButton = new QPushButton(tr("Delete all"), extension);
  delButton->setToolTip(tr("Delete selected criteria") );
  delallButton->setToolTip( tr("Delete all criteria") );

  radiogroup  = new QButtonGroup(extension);
  plotButton = new QRadioButton(tr("Plot"), this);
  colourButton = new QRadioButton(tr("Colour - parameter"), this);
  totalColourButton = new QRadioButton(tr("Colour - observation"), this);
  markerButton = new QRadioButton(tr("Marker"), this);
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

  QPushButton* saveButton = new QPushButton(tr("Save"), extension);
  QLabel* editLabel = TitleLabel(tr("Save criteria list"),extension);
  lineedit = new QLineEdit(extension);
  lineedit->setToolTip(tr("Name of list to save") );

  connect(criteriaBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &ObsDialog::criteriaListSelected);
  connect(criteriaListbox, &QListWidget::itemClicked, this, &ObsDialog::criteriaSelected);
  connect(signBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &ObsDialog::signSlot);
  connect(colourButton, &QPushButton::toggled, colourBox, &QComboBox::setEnabled);
  connect(totalColourButton, &QPushButton::toggled, colourBox, &QComboBox::setEnabled);
  connect(markerButton, &QPushButton::toggled, markerBox, &QComboBox::setEnabled);
  connect(colourButton, &QPushButton::toggled, this, &ObsDialog::changeCriteriaString);
  connect(colourBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &ObsDialog::changeCriteriaString);
  connect(markerBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &ObsDialog::changeCriteriaString);
  connect(totalColourButton, &QPushButton::toggled, this, &ObsDialog::changeCriteriaString);
  connect(markerButton, &QPushButton::toggled, this, &ObsDialog::changeCriteriaString);
  connect(limitSlider, &QSlider::valueChanged, this, &ObsDialog::sliderSlot);
  connect(stepComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &ObsDialog::stepSlot);
  connect(delButton, &QPushButton::clicked, this, &ObsDialog::deleteSlot);
  connect(delallButton, &QPushButton::clicked, this, &ObsDialog::deleteAllSlot);
  connect(saveButton, &QPushButton::clicked, this, &ObsDialog::saveSlot);

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
}

void ObsDialog::criteriaListSelected(int index)
{
  obsWidget[m_selected]->setCurrentCriteria(index);

  ObsDialogInfo::CriteriaList critList = obsWidget[m_selected]->getCriteriaList();

  criteriaListbox->clear();

  lineedit->clear();

  if (critList.criteria.empty())
    return;

  lineedit->setText(QString::fromStdString(critList.name));
  for (const auto& c : critList.criteria)
    criteriaListbox->addItem(QString::fromStdString(c));

  criteriaListbox->item(0)->setSelected(true);
  criteriaSelected(criteriaListbox->item(0));
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

void ObsDialog::stepSlot(int)
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
    std::vector<std::string> vstr;
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
    std::vector<std::string> vstr;
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
  std::vector<std::string> vstr;
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

  std::vector<std::string> sub = miutil::split(str, " ");

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
    std::vector<std::string> sstr = miutil::split(sub[0], sign);
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
  std::vector<std::string> vstr;
  for (int i = 0; i < n; i++) {
    vstr.push_back(criteriaListbox->item(i)->text().toStdString());
  }
  //save criterias and reread in order to get buttons marked right
  obsWidget[m_selected]->saveCriteria(vstr);

}

void ObsDialog::deleteAllSlot()
{
  criteriaListbox->clear();
  std::vector<std::string> vstr;
  obsWidget[m_selected]->saveCriteria(vstr);
}

void ObsDialog::saveSlot()
{
  std::string name = lineedit->text().toStdString();
  if (name.empty())
    return;

  int n = criteriaListbox->count();
  std::vector<std::string> vstr;
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

void ObsDialog::rightButtonClicked(const std::string& name)
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
  std::vector<std::string> critName = obsWidget[m_selected]->getCriteriaNames();
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

  std::vector<std::string> criteriaList = cList.criteria;
  for (unsigned int j = 0; j < criteriaList.size(); j++) {
    criteriaListbox->addItem(QString(criteriaList[j].c_str()));
  }
  if (criteriaListbox->count()) {
    criteriaListbox->item(0)->setSelected(true);
    criteriaListbox->setCurrentRow(0);
    criteriaSelected(criteriaListbox->item(0));
  }
}
