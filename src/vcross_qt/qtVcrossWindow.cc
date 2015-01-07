/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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

#include "qtVcrossWindow.h"

#include "qtVcrossLayerButton.h"
#include "qtVcrossStyleDialog.h"
#include "qtVcrossWizard.h"

#include "diLocationData.h"
#include "diLogFile.h"
#include "diUtilities.h"

#include "qtActionButton.h"
#include "qtUtility.h"
#include "qtVcrossSetupDialog.h"
#include "qtPrintManager.h"

#include <puTools/mi_boost_compatibility.hh>
#include <puTools/miSetupParser.h>
#include <puTools/miStringFunctions.h>

#include <QAbstractListModel>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QLayout>
#include <QMessageBox>
#include <QPixmap>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

#include <vector>

#define MILOGGER_CATEGORY "diana.VcrossWindow"
#include <miLogger/miLogging.h>

#include "vcross_window.ui.h"

#include "addempty.xpm"
#include "bakover.xpm"
#include "clock.xpm"
#include "edit.xpm"
#include "exit.xpm"
#include "fileprint.xpm"
#include "filesave.xpm"
#include "forover.xpm"
#include "info.xpm"
#include "icon_settings.xpm"
#include "kill.xpm"

using namespace vcross;

namespace /* anonymous */ {

namespace VectorModelDetail {

template<class ValueType>
struct BasicExtract {
  typedef ValueType value_t;
};

} // namespace VectorModelDetail

// ========================================================================

template<class Extract>
class VectorModel : public QAbstractListModel {
public:
  typedef typename Extract::value_t value_t;
  typedef std::vector<value_t> vector_t;

  VectorModel(const vector_t& values, const Extract& e = Extract())
    : mValues(values), mExtract(e) { }

  int rowCount(const QModelIndex&) const
    { return mValues.size(); }

  QVariant data(const QModelIndex& index, int role) const;
  
  const vector_t& values() const
    { return mValues; }

private:
  vector_t mValues;
  Extract mExtract;
};

template<class Extract>
QVariant VectorModel<Extract>::data(const QModelIndex& index, int role) const
{
  if (role == Qt::DisplayRole)
    return mExtract.text(mValues.at(index.row()));
  else if (role == Qt::ToolTipRole or role == Qt::StatusTipRole)
    return mExtract.tip(mValues.at(index.row()));
  else
    return QVariant();
}

struct MiTimeExtract : public VectorModelDetail::BasicExtract<miutil::miTime>
{
  QVariant text(const miutil::miTime& t) const
    { return QString::fromStdString(t.isoTime()); }
  QVariant tip(const miutil::miTime& t) const
    { return QVariant(); }
};
typedef VectorModel<MiTimeExtract> MiTimeModel;

static const char LOG_WINDOW[] = "VCROSS.WINDOW.LOG";
static const char LOG_SETUP[]  = "VCROSS.SETUP.LOG";
static const char LOG_FIELD[]  = "VCROSS.FIELD.LOG";

} // namespace anonymous

VcrossWindow::VcrossWindow()
  : QWidget(0)
  , ui(new Ui_VcrossWindow)
  , vcrossDialogX(16)
  , vcrossDialogY(16)
  , firstTime(true)
  , active(false)
{
  METLIBS_LOG_SCOPE();
  setupUi();
  
  vcrossm =  miutil::make_shared<QtManager>();
  selectionManager.reset(new VcrossSelectionManager(vcrossm));

  //central widget
  ui->vcross->setVcrossManager(vcrossm);
  connect(ui->vcross, SIGNAL(timeChanged(int)), SLOT(timeChangedSlot(int)));
  connect(ui->vcross, SIGNAL(crossectionChanged(int)), SLOT(crossectionChangedSlot(int)));

  ui->comboTime->setModel(new MiTimeModel(std::vector<miutil::miTime>()));

  connect(ui->toggleCsEdit, SIGNAL(toggled(bool)),
      SIGNAL(requestVcrossEditor(bool)));

  //connected dialogboxes
  vcStyleDialog = new VcrossStyleDialog(this);
  vcStyleDialog->setSelectionManager(selectionManager.get());
  vcStyleDialog->setVisible(false);

  vcSetupDialog = new VcrossSetupDialog(this, vcrossm);
  connect(vcSetupDialog, SIGNAL(SetupApply()), SLOT(changeSetup()));
  connect(vcSetupDialog, SIGNAL(showsource(const std::string&, const std::string&)),
      SIGNAL(requestHelpPage(const std::string&, const std::string&)));

  connect(selectionManager.get(), SIGNAL(fieldAdded(const std::string&, const std::string&, int)),
      this, SLOT(onFieldAdded(const std::string&, const std::string&, int)));
  connect(selectionManager.get(), SIGNAL(fieldUpdated(const std::string&, const std::string&, int)),
      this, SLOT(onFieldUpdated(const std::string&, const std::string&, int)));
  connect(selectionManager.get(), SIGNAL(fieldRemoved(const std::string&, const std::string&, int)),
      this, SLOT(onFieldRemoved(const std::string&, const std::string&, int)));
  connect(selectionManager.get(), SIGNAL(fieldsRemoved()),
      this, SLOT(onFieldsRemoved()));
}

/***************************************************************************/

VcrossWindow::~VcrossWindow()
{
}

void VcrossWindow::setupUi()
{
  ui->setupUi(this);

  ui->actionAddField->setIcon(QPixmap(addempty_xpm));
  new ActionButton(ui->toolAddField, ui->actionAddField, this);

  ui->toggleTimeGraph->setIcon(QPixmap(clock_xpm));
  ui->buttonClose->setIcon(QPixmap(exit_xpm));
  ui->buttonHelp->setIcon(QPixmap(info_xpm));
  ui->buttonPrint->setIcon(QPixmap(fileprint_xpm));
  ui->buttonSave->setIcon(QPixmap(filesave));
  ui->buttonSettings->setIcon(QPixmap(icon_settings));

  ui->toolRemoveAllFields->setIcon(QPixmap(kill_xpm));

  const QPixmap back(bakover_xpm), forward(forward_xpm);

  ui->buttonCsPrevious->setIcon(back);
  ui->buttonCsNext->setIcon(forward);

  ui->buttonTimePrevious->setIcon(back);
  ui->buttonTimeNext->setIcon(forward);

  QBoxLayout* lbl = new QVBoxLayout;
  lbl->setContentsMargins(0, 0, 0, 0);
  lbl->setSpacing(1);
  ui->layerButtons->setLayout(lbl);
}

/***************************************************************************/

void VcrossWindow::onAddField()
{
  METLIBS_LOG_SCOPE();
  VcrossWizard wiz(this, selectionManager.get());
  wiz.move(vcrossDialogX, vcrossDialogY);
  if (wiz.exec()) {
    vcrossDialogX = wiz.pos().x();
    vcrossDialogY = wiz.pos().y();

    const std::string model = wiz.getSelectedModel().toStdString();
    const QStringList fields = wiz.getSelectedFields();
    for (int i=0; i<fields.size(); ++i) {
      const std::string fld = fields.at(i).toStdString();
      const std::string opt = selectionManager->defaultOptions(model, fld, false);
      selectionManager->addField(model, fld, opt, selectionManager->countFields());
    }

    changeFields();
  }
}

/***************************************************************************/

void VcrossWindow::onRemoveAllFields()
{
  selectionManager->removeAllFields();
  changeFields();
}

/***************************************************************************/

void VcrossWindow::onFieldAction(int position, int action)
{
  METLIBS_LOG_SCOPE(LOGVAL(position));

  const std::string model = selectionManager->getModelAt(position);
  const std::string field = selectionManager->getFieldAt(position);
  const std::string opt   = selectionManager->getOptionsAt(position);
  const std::string dflt  = selectionManager->defaultOptions(model, field, true);

  METLIBS_LOG_DEBUG(LOGVAL(model) << LOGVAL(field) << LOGVAL(opt) << LOGVAL(dflt));

  if (action == VcrossLayerButton::EDIT) {
    const QString mdl = QString::fromStdString(model);
    const QString fld = QString::fromStdString(field);
    vcStyleDialog->showModelField(mdl, fld);
    vcStyleDialog->show();
  } else if (action == VcrossLayerButton::REMOVE) {
    selectionManager->removeField(model, field);
    changeFields();
  } else if (action == VcrossLayerButton::SHOW_HIDE) {
    QBoxLayout* lbl = static_cast<QBoxLayout*>(ui->layerButtons->layout());
    QWidgetItem* wi = static_cast<QWidgetItem*>(lbl->itemAt(position));
    VcrossLayerButton *button = static_cast<VcrossLayerButton*>(wi->widget());

    selectionManager->setVisibleAt(position, button->isChecked());
    changeFields();
  }
}

void VcrossWindow::onFieldAdded(const std::string& model, const std::string& field, int position)
{
  METLIBS_LOG_SCOPE();
  QBoxLayout* lbl = static_cast<QBoxLayout*>(ui->layerButtons->layout());
  const int n = lbl->count();
  METLIBS_LOG_DEBUG(LOGVAL(position) << LOGVAL(n));
  for (int i=position; i<n; ++i) {
    QWidgetItem* wi = static_cast<QWidgetItem*>(lbl->itemAt(i));
    VcrossLayerButton *button = static_cast<VcrossLayerButton*>(wi->widget());
    button->setPosition(i+1, i==n-1);
  }

  VcrossLayerButton* button = new VcrossLayerButton(QString::fromStdString(model),
      QString::fromStdString(field), position, this);
  connect(button, SIGNAL(triggered(int, int)), SLOT(onFieldAction(int, int)));
  lbl->insertWidget(position, button);
}

void VcrossWindow::onFieldUpdated(const std::string& model, const std::string& field, int position)
{
  METLIBS_LOG_SCOPE();
  changeFields();
}

void VcrossWindow::onFieldRemoved(const std::string& model, const std::string& field, int position)
{
  METLIBS_LOG_SCOPE();
  QBoxLayout* lbl = static_cast<QBoxLayout*>(ui->layerButtons->layout());
  const int n = lbl->count();
  METLIBS_LOG_DEBUG(LOGVAL(position) << LOGVAL(n));
  for (int i=position+1; i<n; ++i) {
    QWidgetItem* wi = static_cast<QWidgetItem*>(lbl->itemAt(i));
    VcrossLayerButton *button = static_cast<VcrossLayerButton*>(wi->widget());
    button->setPosition(i-1, i==n-1);
  }

  QWidgetItem* wi = static_cast<QWidgetItem*>(lbl->itemAt(position));
  QToolButton *button = static_cast<QToolButton*>(wi->widget());
  button->deleteLater();
}

void VcrossWindow::onFieldsRemoved()
{
  QBoxLayout* lbl = static_cast<QBoxLayout*>(ui->layerButtons->layout());
  while (lbl->count() > 0) {
    QWidgetItem* wi = static_cast<QWidgetItem*>(lbl->takeAt(0));
    QToolButton *button = static_cast<QToolButton*>(wi->widget());
    button->deleteLater();
  }
}

/***************************************************************************/

void VcrossWindow::leftCrossectionClicked()
{
  //called when the left Crossection button is clicked
  const std::string s = vcrossm->setCrossection(-1);
  crossectionChangedSlot(-1);
  ui->vcross->update();
  emitQmenuStrings();
}

/***************************************************************************/

void VcrossWindow::rightCrossectionClicked()
{
  //called when the right Crossection button is clicked
  const std::string s= vcrossm->setCrossection(+1);
  crossectionChangedSlot(+1);
  ui->vcross->update();
  emitQmenuStrings();
}

/***************************************************************************/

void VcrossWindow::stepTime(int direction)
{
  METLIBS_LOG_SCOPE(LOGVAL(direction));
  const int step = std::max(ui->timeSpinBox->value(), 1)
      * (direction < 0 ? -1 : 1);
  vcrossm->setTime(step);
  timeChangedSlot(step);
  ui->vcross->update();
}

/***************************************************************************/

void VcrossWindow::leftTimeClicked()
{
  stepTime(-1);
}

/***************************************************************************/

void VcrossWindow::rightTimeClicked()
{
  stepTime(+1);
}

/***************************************************************************/

bool VcrossWindow::timeChangedSlot(int diff)
{
  // called if signal timeChanged is emitted from graphics window (qtVcrossWidget)
  METLIBS_LOG_SCOPE();

  const int count = ui->comboTime->count();
  METLIBS_LOG_DEBUG(LOGVAL(count) << LOGVAL(diff));
  if (count <= 0)
    return false;

  if (diff != 0) {
    int index = ui->comboTime->currentIndex();
    index += diff;
    while (index < 0)
      index += count;
    index %= count;
    ui->comboTime->setCurrentIndex(index);
  }

  const miutil::miTime& vct = vcrossm->getTime();
  const MiTimeModel* tim = static_cast<MiTimeModel*>(ui->comboTime->model());
  const miutil::miTime& tbt = tim->values().at(ui->comboTime->currentIndex());

  bool ok = (vct == tbt);

  // search timeList
  if (not ok) {
    for (int i = 0; i<count; i++) {
      if (vct == tim->values().at(i)) {
        ui->comboTime->setCurrentIndex(i);
        METLIBS_LOG_DEBUG(LOGVAL(i));
        ok = true;
        break;
      }
    }
  }

  if (not ok) {
    METLIBS_LOG_DEBUG(LOGVAL(vct) << "not found");
    return false;
  }

  /*emit*/ setTime("vcross", vct);
  return true;
}

/***************************************************************************/

bool VcrossWindow::crossectionChangedSlot(int diff)
{
  METLIBS_LOG_SCOPE();

  const int count = ui->comboCs->count();
  METLIBS_LOG_DEBUG(LOGVAL(count) << LOGVAL(diff));
  if (count <= 0)
    return false;

  if (diff != 0) {
    int index = ui->comboCs->currentIndex();
    index += diff;
    while (index < 0)
      index += count;
    index %= count;
    ui->comboCs->setCurrentIndex(index);
  }

  //get current crossection
  std::string s = vcrossm->getCrossection();
  //if no current crossection, use last crossection plotted
  if (s.empty())
    s = ""; // FIXME vcrossm->getLastCrossection();
  std::string sbs = ui->comboCs->currentText().toStdString();
  if (sbs != s){
    for(int i = 0; i<count; i++) {
      if (s==ui->comboCs->itemText(i).toStdString()) {
        ui->comboCs->setCurrentIndex(i);
        sbs = ui->comboCs->currentText().toStdString();
        break;
      }
    }
  }
  QString sq = QString::fromStdString(s);
  if (sbs == s) {
    Q_EMIT crossectionChanged(sq); //name of current crossection (to mainWindow)
    return true;
  } else {
    //    METLIBS_LOG_WARN("WARNING! crossectionChangedSlot  crossection from vcrossm ="
    // 	 << s    <<" not equal to crossectionBox text = " << sbs);
    //current or last crossection plotted is not in the list, insert it...
    ui->comboCs->addItem(sq);
    ui->comboCs->setCurrentIndex(0);
    ui->comboCs->setEnabled(true);
    ui->buttonCsPrevious->setEnabled(true);
    ui->buttonCsNext->setEnabled(true);
    return false;
  }
}


/***************************************************************************/

void VcrossWindow::printClicked()
{
  printerManager pman;
  std::string command = pman.printCommand();

  QPrinter qprt;
  fromPrintOption(qprt, priop);

  QPrintDialog printerDialog(&qprt, this);
  if (printerDialog.exec()) {
    // fill printOption from qprinter-selections
    toPrintOption(qprt, priop);

    diutil::OverrideCursor waitCursor;
    ui->vcross->print(qprt);
  }
}

/***************************************************************************/

void VcrossWindow::saveClicked()
{
  QString filename = QFileDialog::getSaveFileName(this,
      tr("Save plot as image"),
      mRasterFilename,
      tr("Images (*.png *.xpm *.bmp);;All (*.*)"));
  
  if (not filename.isNull()) {// got a filename
    mRasterFilename = filename;
    if (not ui->vcross->saveRasterImage(filename))
      QMessageBox::warning(this, tr("Save image failed"),
          tr("Saveing the vertical cross section plot as '%1' failed. Sorry.").arg(filename));
  }
}


void VcrossWindow::makeEPS(const std::string& filename)
{
  diutil::OverrideCursor waitCursor;
  printOptions priop;
  priop.fname= filename;
  priop.colop= d_print::incolour;
  priop.orientation= d_print::ori_automatic;
  priop.pagesize= d_print::A4;
  priop.numcopies= 1;
  priop.usecustomsize= false;
  priop.fittopage= false;
  priop.drawbackground= true;
  priop.doEPS= true;

//  vcrossw->print(priop);
}

/***************************************************************************/

void VcrossWindow::onShowSetupDialog()
{
  vcSetupDialog->start();
  vcSetupDialog->show();
}

/***************************************************************************/

void VcrossWindow::timeGraphClicked(bool on)
{
  // called when the timeGraph button is clicked
  METLIBS_LOG_SCOPE("on=" << on);

  if (on && vcrossm->timeGraphOK()) {
    ui->vcross->enableTimeGraph(true);
  } else if (on) {
    ui->toggleTimeGraph->setChecked(false);
  } else {
    vcrossm->disableTimeGraph();
    ui->vcross->enableTimeGraph(false);
    ui->vcross->update();
  }
}

/***************************************************************************/

void VcrossWindow::quitClicked()
{
  //called when the quit button is clicked
  METLIBS_LOG_SCOPE();

  // cleanup selections in dialog and data in memory
  vcrossm->cleanup();

  ui->comboCs->clear();
  ui->comboCs->setEnabled(false);
  ui->buttonCsPrevious->setEnabled(false);
  ui->buttonCsNext->setEnabled(false);

  const std::vector<miutil::miTime> NO_TIMES;
  ui->comboTime->setModel(new MiTimeModel(NO_TIMES));
  ui->comboTime->setEnabled(false);
  ui->buttonTimePrevious->setEnabled(false);
  ui->buttonTimeNext->setEnabled(false);

  vcStyleDialog->setVisible(false);

  active = false;
  Q_EMIT VcrossHide();
  Q_EMIT emitTimes("vcross", NO_TIMES);
  hide();
}

/***************************************************************************/

void VcrossWindow::helpClicked()
{
  //called when the help button in Vcrosswindow is clicked
  Q_EMIT requestHelpPage("ug_verticalcrosssections.html");
}

void VcrossWindow::slotCheckEditmode(bool editing)
{
  if (vcrossm->supportsDynamicCrossections())
    ui->toggleCsEdit->setChecked(editing);
}

void VcrossWindow::dynCrossEditManagerChange(const QVariantMap &props)
{
  METLIBS_LOG_SCOPE();

  const char key_points[] = "latLonPoints", key_id[] = "id";
  if (not (props.contains(key_points) and props.contains(key_id)))
    return;

  std::string label = QString("dyn_%1").arg(props.value(key_id).toInt()).toStdString();

  vcross::LonLat_v points;
  foreach (QVariant v, props.value(key_points).toList()) {
    const QPointF p = v.toPointF();
    const float lat = p.x(), lon = p.y(); // FIXME swpa x <-> y
    points.push_back(LonLat::fromDegrees(lon, lat));
  }
  if (points.size() < 2)
    return;

  vcrossm->setDynamicCrossection(label, points);

  updateCrossectionBox();

  vcrossm->setCrossection(label);

  ui->vcross->update();
}

void VcrossWindow::dynCrossEditManagerRemoval(int id)
{
  METLIBS_LOG_SCOPE();

  std::string label = QString("dyn_%1").arg(id).toStdString();
  vcrossm->setDynamicCrossection(label, vcross::LonLat_v()); // empty points => remove
  updateCrossectionBox();

  ui->vcross->update();
}

/***************************************************************************/

void VcrossWindow::emitQmenuStrings()
{
  const std::vector<std::string> qm_string = vcrossm->getQuickMenuStrings();
  if (!qm_string.empty()) {
    const std::string plotname = "<font color=\"#005566\">" + selectionManager->getShortname() + " " + vcrossm->getCrossection() + "</font>";
    Q_EMIT quickMenuStrings(plotname, qm_string);
  }
}

/***************************************************************************/

void VcrossWindow::changeFields()
{
  METLIBS_LOG_SCOPE();

  const std::vector<std::string> vstr = selectionManager->getOKString();
  const bool modelChanged = vcrossm->setSelection(vstr);

  //called when the apply button from model/field dialog is clicked
  //... or field is changed ?

  enableDynamicCsIfSupported();

  if (modelChanged) {
    updateCrossectionBox();
    updateTimeBox();

    //get correct selection in comboboxes
    crossectionChangedSlot(0);
    timeChangedSlot(0);
  }

  ui->vcross->update();
  emitQmenuStrings();
}

void VcrossWindow::enableDynamicCsIfSupported()
{
  if (vcrossm->supportsDynamicCrossections()) {
    ui->toggleCsEdit->setEnabled(true);

    typedef std::set<std::string> string_s;
    const string_s& csPredefined = vcrossm->getCrossectionPredefinitions();
    if (not csPredefined.empty()) {
      QStringList filenames;
      for (string_s::const_iterator it = csPredefined.begin(); it != csPredefined.end(); ++it)
        filenames << QString::fromStdString(*it);
      Q_EMIT requestLoadCrossectionFiles(filenames);
    } else {
      ui->toggleCsEdit->setChecked(true);
    }
  } else {
    ui->toggleCsEdit->setChecked(false);
    ui->toggleCsEdit->setEnabled(false);
  }
}

/***************************************************************************/

void VcrossWindow::changeSetup()
{
  //called when the apply from setup dialog is clicked
  METLIBS_LOG_SCOPE();

  emitCrossectionSet();
  ui->vcross->update();
  emitQmenuStrings();
}

void VcrossWindow::emitCrossectionSet()
{
  // emit to MainWindow (updates crossectionPlot)
  LocationData locations;
  vcrossm->getCrossections(locations);
  Q_EMIT crossectionSetChanged(locations);
}

/***************************************************************************/

// update list of crossections in ui->comboCs
void VcrossWindow::updateCrossectionBox()
{
  METLIBS_LOG_SCOPE();

  emitCrossectionSet();

  ui->comboCs->clear();
  const std::vector<std::string>& crossections = vcrossm->getCrossectionList();
  const bool haveCs = not crossections.empty();
  ui->comboCs->setEnabled(haveCs);
  ui->buttonCsPrevious->setEnabled(haveCs);
  ui->buttonCsNext->setEnabled(haveCs);
  for (size_t i=0; i<crossections.size(); i++)
    ui->comboCs->addItem(QString::fromStdString(crossections[i]));
}

/***************************************************************************/

void VcrossWindow::updateTimeBox()
{
  METLIBS_LOG_SCOPE();

  const std::vector<miutil::miTime>& times = vcrossm->getTimeList();
  const bool haveTimes = not times.empty();
  ui->comboTime->setEnabled(haveTimes);
  ui->buttonTimePrevious->setEnabled(haveTimes);
  ui->buttonTimeNext->setEnabled(haveTimes);
  ui->comboTime->setModel(new MiTimeModel(times));
  Q_EMIT emitTimes("vcross",times);
}

/***************************************************************************/

void VcrossWindow::crossectionBoxActivated(int index)
{
  const QString& sq = ui->comboCs->currentText();
  vcrossm->setCrossection(sq.toStdString());
  ui->vcross->update();
  Q_EMIT crossectionChanged(sq); //name of current crossection (to mainWindow)
  emitQmenuStrings();
}

/***************************************************************************/

void VcrossWindow::timeBoxActivated(int index)
{
  const std::vector<miutil::miTime>& times = vcrossm->getTimeList();

  if (index>=0 && index < int(times.size())) {
    vcrossm->setTime(times[index]);
    ui->vcross->update();
    Q_EMIT setTime("vcross", times[index]);
  }
}

/***************************************************************************/

bool VcrossWindow::changeCrossection(const std::string& crossection)
{
  METLIBS_LOG_SCOPE();

  vcrossm->setCrossection(crossection); //HK ??? should check if crossection exists ?
  ui->vcross->update();
  raise();
  emitQmenuStrings();

  return crossectionChangedSlot(0);
}

/***************************************************************************/

void VcrossWindow::mainWindowTimeChanged(const miutil::miTime& t)
{
  METLIBS_LOG_SCOPE();
  if (!active)
    return;
  METLIBS_LOG_DEBUG("time = " << t);

  vcrossm->mainWindowTimeChanged(t);
  //get correct selection in comboboxes
  crossectionChangedSlot(0);
  timeChangedSlot(0);
  ui->vcross->update();
}


/***************************************************************************/

void VcrossWindow::makeVisible(bool visible)
{
  METLIBS_LOG_SCOPE();

  if (visible and active) {
    raise();
    return;
  }

  if (visible != active) {
    active = visible;
    if (visible) {
      show(); // includes raise() ?
      
      //do something first time we start Vertical crossections
      if (firstTime) {
        firstTime = false;
        ui->vcross->update();
      }
    } else {
      quitClicked();
    }
  }
}

void VcrossWindow::parseQuickMenuStrings(const std::vector<std::string>& qm_string)
{
  vcrossm->parseQuickMenuStrings(qm_string);
  selectionManager->putOKString(qm_string);

  enableDynamicCsIfSupported();
  updateCrossectionBox();
  updateTimeBox();

  crossectionChangedSlot(0);
  ui->vcross->update();
}

/***************************************************************************/
void VcrossWindow::parseSetup()
{
  METLIBS_LOG_SCOPE();
  string_v sources, computations, plots;
  miutil::SetupParser::getSection("VERTICAL_CROSSECTION_FILES", sources);
  miutil::SetupParser::getSection("VERTICAL_CROSSECTION_COMPUTATIONS", computations);
  miutil::SetupParser::getSection("VERTICAL_CROSSECTION_PLOTS", plots);
  vcrossm->parseSetup(sources,computations,plots);
}

void VcrossWindow::writeLog(LogFileIO& logfile)
{
  { LogFileIO::Section& sec_window = logfile.getSection(LOG_WINDOW);
    sec_window.addLine("VcrossWindow.size " + miutil::from_number(width()) + " "
        + miutil::from_number(height()));
    sec_window.addLine("VcrossWindow.pos " + miutil::from_number(x()) + " "
        + miutil::from_number(y()));
    sec_window.addLine("VcrossDialog.pos "  + miutil::from_number(vcrossDialogX) + " "
        + miutil::from_number(vcrossDialogY));
    sec_window.addLine("VcrossSetupDialog.pos " + miutil::from_number(vcSetupDialog->x()) + " "
        + miutil::from_number(vcSetupDialog->y()));
    sec_window.addLine("VcrossStyleDialog.pos " + miutil::from_number(vcStyleDialog->x()) + " "
        + miutil::from_number(vcStyleDialog->y()));

    // printer name & options...
    if (not priop.printer.empty()){
      sec_window.addLine("PRINTER " + priop.printer);
      if (priop.orientation==d_print::ori_portrait)
        sec_window.addLine("PRINTORIENTATION portrait");
      else
        sec_window.addLine("PRINTORIENTATION landscape");
    }
  }
  { LogFileIO::Section& sec_setup = logfile.getSection(LOG_SETUP);
    sec_setup.addLines(vcrossm->writeLog());
  }
  { LogFileIO::Section& sec_field = logfile.getSection(LOG_FIELD);
    sec_field.addLines(selectionManager->writeLog());
  }
}

void VcrossWindow::readLog(const LogFileIO& logfile,
    const std::string& thisVersion, const std::string& logVersion,
    int displayWidth, int displayHeight)
{
  { const LogFileIO::Section& sec_window = logfile.getSection(LOG_WINDOW);
    for (size_t i=0; i<sec_window.size(); i++) {
      const std::vector<std::string> tokens = miutil::split(sec_window[i], 0, " ");
      if (tokens.size()==3) {

        int x= atoi(tokens[1].c_str());
        int y= atoi(tokens[2].c_str());
        if (x>20 && y>20 && x<=displayWidth && y<=displayHeight) {
          if (tokens[0]=="VcrossWindow.size") this->resize(x,y);
        }
        if (x>=0 && y>=0 && x<displayWidth-20 && y<displayHeight-20) {
          if      (tokens[0]=="VcrossWindow.pos")      this->move(x,y);
          else if (tokens[0]=="VcrossDialog.pos")      { vcrossDialogX = x; vcrossDialogY = y; }
          else if (tokens[0]=="VcrossSetupDialog.pos") vcSetupDialog->move(x,y);
          else if (tokens[0]=="VcrossStyleDialog.pos") vcStyleDialog->move(x,y);
        }

      } else if (tokens.size()>=2) {

        if (tokens[0]=="PRINTER") {
          priop.printer=tokens[1];
        } else if (tokens[0]=="PRINTORIENTATION") {
          if (tokens[1]=="portrait")
            priop.orientation=d_print::ori_portrait;
          else
            priop.orientation=d_print::ori_landscape;
        }

      }
    }
  }
  { const LogFileIO::Section& sec_setup = logfile.getSection(LOG_SETUP);
    vcrossm->readLog(sec_setup.lines(), thisVersion, logVersion);
  }
  { const LogFileIO::Section& sec_field = logfile.getSection(LOG_FIELD);
    selectionManager->readLog(sec_field.lines(), thisVersion, logVersion);
  }
}

/***************************************************************************/

void VcrossWindow::closeEvent(QCloseEvent * e)
{
  quitClicked();
}

// ########################################################################

VcrossWindowInterface::VcrossWindowInterface()
  : window(new VcrossWindow())
{
  connect(window, SIGNAL(VcrossHide()),
      this, SIGNAL(VcrossHide()));

  connect(window, SIGNAL(requestHelpPage(const std::string&, const std::string&)),
      this, SIGNAL(requestHelpPage(const std::string&, const std::string&)));

  connect(window, SIGNAL(requestLoadCrossectionFiles(const QStringList&)),
      this, SIGNAL(requestLoadCrossectionFiles(const QStringList&)));

  connect(window, SIGNAL(requestVcrossEditor(bool)),
      this, SIGNAL(requestVcrossEditor(bool)));

  connect(window, SIGNAL(crossectionSetChanged(const LocationData&)),
      this, SIGNAL(crossectionSetChanged(const LocationData&)));

  connect(window, SIGNAL(crossectionChanged(const QString &)),
      this, SIGNAL(crossectionChanged(const QString &)));

  connect(window, SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&)),
      this, SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&)));

  connect(window, SIGNAL(setTime(const std::string&, const miutil::miTime&)),
      this, SIGNAL(setTime(const std::string&, const miutil::miTime&)));

  connect(window, SIGNAL(quickMenuStrings(const std::string&, const std::vector<std::string>&)),
      this, SIGNAL(quickMenuStrings(const std::string&, const std::vector<std::string>&)));

  connect(window, SIGNAL(prevHVcrossPlot()),
      this, SIGNAL(prevHVcrossPlot()));

  connect(window, SIGNAL(nextHVcrossPlot()),
      this, SIGNAL(nextHVcrossPlot()));
}

VcrossWindowInterface::~VcrossWindowInterface()
{
  delete window;
}
