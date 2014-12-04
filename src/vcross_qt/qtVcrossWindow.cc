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

#include "qtVcrossWindow.h"

#include "qtVcrossLayerButton.h"
#include "qtVcrossStyleWidget.h"
#include "qtVcrossWizard.h"

#include "diController.h"
#include "diLocationData.h"
#include "diUtilities.h"

#include "qtActionButton.h"
#include "qtUtility.h"
#include "qtToggleButton.h"
#include "qtVcrossSetupDialog.h"
#include "diEditItemManager.h"
#include "EditItems/toolbar.h"
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

} // namespace anonymous

VcrossWindow::VcrossWindow(Controller *co)
  : QWidget(0)
  , ui(new Ui_VcrossWindow)
  , dynEditManagerConnected(false)
  , vcrossDialogX(16)
  , vcrossDialogY(16)
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

  //connected dialogboxes
  vcSetupDialog = new VcrossSetupDialog(this, vcrossm);
  connect(vcSetupDialog, SIGNAL(SetupApply()), SLOT(changeSetup()));
  connect(vcSetupDialog, SIGNAL(showsource(const std::string&, const std::string&)),
      SIGNAL(showsource(const std::string&, const std::string&)));

  connect(selectionManager.get(), SIGNAL(fieldAdded(const std::string&, const std::string&, int)),
      this, SLOT(onFieldAdded(const std::string&, const std::string&, int)));
  connect(selectionManager.get(), SIGNAL(fieldRemoved(const std::string&, const std::string&, int)),
      this, SLOT(onFieldRemoved(const std::string&, const std::string&, int)));
  connect(selectionManager.get(), SIGNAL(fieldsRemoved()),
      this, SLOT(onFieldsRemoved()));

  //inialize everything in startUp
  firstTime = true;
  active = false;
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

    QDialog d(this);
    d.setWindowTitle(tr("Style for %1 -- %2").arg(mdl).arg(fld));
    VcrossStyleWidget* sw = new VcrossStyleWidget(&d);
    QPushButton* accept = new QPushButton(tr("OK"), &d);
    connect(accept, SIGNAL(clicked()), &d, SLOT(accept()));
    QVBoxLayout* l = new QVBoxLayout;
    l->addWidget(sw);
    l->addWidget(accept);
    d.setLayout(l);
    
    sw->setModelFieldName(mdl, fld);
    sw->setOptions(opt, dflt);
    if (d.exec() && sw->valid()) {
      selectionManager->updateField(model, field, sw->options());
      changeFields();
    }
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

  active = false;
  Q_EMIT updateCrossSectionPos(false);
  Q_EMIT VcrossHide();
  Q_EMIT emitTimes("vcross", NO_TIMES);
}

/***************************************************************************/

void VcrossWindow::helpClicked()
{
  //called when the help button in Vcrosswindow is clicked
  Q_EMIT showsource("ug_verticalcrosssections.html");
}

void VcrossWindow::dynCrossEditManagerEnabled(bool on)
{
  if (on) {
    EditItems::ToolBar::instance()->setCreatePolyLineAction("Cross section");
    EditItemManager::instance()->setEditing(true);
    EditItems::ToolBar::instance()->show();
    dynCrossEditManagerEnableSignals();
  } else {
    EditItemManager::instance()->setEditing(false);
  }
}

void VcrossWindow::dynCrossEditManagerEnableSignals()
{
  if (not dynEditManagerConnected) {
    dynEditManagerConnected = true;

    EditItemManager::instance()->enableItemChangeNotification();
    EditItemManager::instance()->setItemChangeFilter("Cross section");
    connect(EditItemManager::instance(), SIGNAL(itemChanged(const QVariantMap &)),
        this, SLOT(dynCrossEditManagerChange(const QVariantMap &)), Qt::UniqueConnection);
    connect(EditItemManager::instance(), SIGNAL(itemRemoved(int)),
        this, SLOT(dynCrossEditManagerRemoval(int)), Qt::UniqueConnection);
    connect(EditItemManager::instance(), SIGNAL(editing(bool)),
        this, SLOT(slotCheckEditmode(bool)), Qt::UniqueConnection);
  }
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
    if (EditItemManager::instance()) {
      typedef std::set<std::string> string_s;
      const string_s& csPredefined = vcrossm->getCrossectionPredefinitions();
      if (not csPredefined.empty()) {
        for (string_s::const_iterator it = csPredefined.begin(); it != csPredefined.end(); ++it)
          EditItemManager::instance()->emitLoadFile(QString::fromStdString(*it));

#if 0
        const QSet<QSharedPointer<DrawingItemBase> > items
            = EditItemManager::instance()->getLayerManager()->itemsInSelectedLayers(false);
        foreach (const QSharedPointer<DrawingItemBase> item, items) {
          if (item->property("style:type").toString() == "Cross section") {
            // TODO add name to vcross manager without actually creating a cross-section
          }
        }
#endif
        dynCrossEditManagerEnableSignals();
      } else {
        ui->toggleCsEdit->setChecked(true);
      }
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

  //###if (mapOptionsChanged) {
  // emit to MainWindow
  // (updates crossectionPlot colour etc., not data, name etc.)
  /*emit*/ crossectionSetUpdate();
  //###}

  ui->vcross->update();
  emitQmenuStrings();
}

/***************************************************************************/

void VcrossWindow::getCrossections(LocationData& locationdata)
{
  METLIBS_LOG_SCOPE();

  vcrossm->getCrossections(locationdata);
}

/***************************************************************************/

// update list of crossections in ui->comboCs
void VcrossWindow::updateCrossectionBox()
{
  METLIBS_LOG_SCOPE();

  //emit to MainWindow (updates crossectionPlot)
  Q_EMIT crossectionSetChanged();

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

void VcrossWindow::startUp(const miutil::miTime& t)
{
  METLIBS_LOG_SCOPE("t= " << t);

  active = true;
  //do something first time we start Vertical crossections
  if (firstTime){
    //vector<miutil::miString> models;
    //define models for dialogs, comboboxes and crossectionplot
    //vcrossm->setSelectedModels(models, true,false);
    //vcDialog->setSelection();
    firstTime=false;
    // show default diagram without any data
    ui->vcross->update();
  }
  //changeModel();
  mainWindowTimeChanged(t);
}

/*
 * Set the position clicked on the map in the current VcrossPlot.
 */
void VcrossWindow::mapPos(float lat, float lon)
{
  METLIBS_LOG_SCOPE(LOGVAL(lat) << LOGVAL(lon));
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

std::vector<std::string> VcrossWindow::writeLog(const std::string& logpart)
{
  std::vector<std::string> vstr;

  if (logpart=="window") {
    std::string str;
    str= "VcrossWindow.size " + miutil::from_number(width()) + " "
        + miutil::from_number(height());
    vstr.push_back(str);
    str= "VcrossWindow.pos " + miutil::from_number(x()) + " "
        + miutil::from_number(y());
    vstr.push_back(str);
    str= "VcrossDialog.pos "  + miutil::from_number(vcrossDialogX) + " "
        + miutil::from_number(vcrossDialogY);
    vstr.push_back(str);
    str= "VcrossSetupDialog.pos " + miutil::from_number(vcSetupDialog->x()) + " "
        + miutil::from_number(vcSetupDialog->y());
    vstr.push_back(str);

    // printer name & options...
    if (not priop.printer.empty()){
      str= "PRINTER " + priop.printer;
      vstr.push_back(str);
      if (priop.orientation==d_print::ori_portrait)
        str= "PRINTORIENTATION portrait";
      else
        str= "PRINTORIENTATION landscape";
      vstr.push_back(str);
    }

  } else if (logpart=="setup") {
    vstr = vcrossm->writeLog();
  } else if (logpart=="field") {
    vstr = selectionManager->writeLog();
  }

  return vstr;
}

void VcrossWindow::readLog(const std::string& logpart, const std::vector<std::string>& vstr,
    const std::string& thisVersion, const std::string& logVersion,
    int displayWidth, int displayHeight)
{
  if (logpart=="window") {

    const int n = vstr.size();

    for (int i=0; i<n; i++) {
      const std::vector<std::string> tokens = miutil::split(vstr[i], 0, " ");
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

  } else if (logpart=="setup") {
    vcrossm->readLog(vstr, thisVersion, logVersion);
  } else if (logpart=="field") {
    selectionManager->readLog(vstr,thisVersion,logVersion);
  }
}

/***************************************************************************/

void VcrossWindow::closeEvent(QCloseEvent * e)
{
  quitClicked();
}
