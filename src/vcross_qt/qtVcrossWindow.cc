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
#include "qtVcrossAddPlotDialog.h"

#include "diLocationData.h"
#include "diLogFile.h"
#include "diUtilities.h"

#include "qtActionButton.h"
#include "qtUtility.h"
#include "qtVcrossSetupDialog.h"
#include "qtPrintManager.h"

#include <diField/VcrossUtil.h>

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

  VectorModel(const vector_t& values, QObject* parent=0)
    : QAbstractListModel(parent), mValues(values) { }

  VectorModel(const vector_t& values, const Extract& e, QObject* parent=0)
    : QAbstractListModel(parent), mValues(values), mExtract(e) { }

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
  , mInFieldChangeGroup(false)
  , mGroupChangedFields(false)
{
  METLIBS_LOG_SCOPE();
  setupUi();
  
  vcrossm =  miutil::make_shared<QtManager>();
  quickmenues.reset(new VcrossQuickmenues(vcrossm));
  connect(quickmenues.get(), SIGNAL(quickmenuUpdate(const std::string&, const std::vector<std::string>&)),
      this, SIGNAL(quickMenuStrings(const std::string&, const std::vector<std::string>&)));

  //central widget
  ui->vcross->setVcrossManager(vcrossm);
  connect(ui->vcross, SIGNAL(stepTime(int)), SLOT(stepTime(int)));
  connect(ui->vcross, SIGNAL(stepCrossection(int)), SLOT(stepCrossection(int)));

  ui->comboTime->setModel(new MiTimeModel(std::vector<miutil::miTime>()));

  connect(ui->toggleCsEdit, SIGNAL(toggled(bool)),
      SIGNAL(requestVcrossEditor(bool)));

  //connected dialogboxes
  vcStyleDialog = new VcrossStyleDialog(this);
  vcStyleDialog->setManager(vcrossm);
  vcStyleDialog->setVisible(false);

  vcSetupDialog = new VcrossSetupDialog(this, vcrossm);
  connect(vcSetupDialog, SIGNAL(SetupApply()), SLOT(changeSetup()));
  connect(vcSetupDialog, SIGNAL(showsource(const std::string&, const std::string&)),
      SIGNAL(requestHelpPage(const std::string&, const std::string&)));

  { vcross::QtManager* m = vcrossm.get();
    connect(m, SIGNAL(fieldChangeBegin(bool)),
        this, SLOT(onFieldChangeBegin(bool)));
    connect(m, SIGNAL(fieldAdded(const std::string&, const std::string&, int)),
        this, SLOT(onFieldAdded(const std::string&, const std::string&, int)));
    connect(m, SIGNAL(fieldRemoved(const std::string&, const std::string&, int)),
        this, SLOT(onFieldRemoved(const std::string&, const std::string&, int)));
    connect(m, SIGNAL(fieldOptionsChanged(const std::string&, const std::string&, int)),
        this, SLOT(onFieldOptionsChanged(const std::string&, const std::string&, int)));
    connect(m, SIGNAL(fieldVisibilityChanged(const std::string&, const std::string&, int)),
        this, SLOT(onFieldVisibilityChanged(const std::string&, const std::string&, int)));
    connect(m, SIGNAL(fieldChangeEnd()),
        this, SLOT(onFieldChangeEnd()));

    connect(m, SIGNAL(crossectionListChanged()),
        this, SLOT(crossectionListChangedSlot()));
    connect(m, SIGNAL(crossectionIndexChanged(int)),
        this, SLOT(crossectionChangedSlot(int)));
    connect(m, SIGNAL(timeListChanged()),
        this, SLOT(timeListChangedSlot()));
    connect(m, SIGNAL(timeIndexChanged(int)),
        this, SLOT(timeChangedSlot(int)));
  }
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
  VcrossAddPlotDialog d(this, vcrossm);
  d.move(vcrossDialogX, vcrossDialogY);
  d.restart();
  d.exec();
  vcrossDialogX = d.pos().x();
  vcrossDialogY = d.pos().y();
}

/***************************************************************************/

void VcrossWindow::onRemoveAllFields()
{
  METLIBS_LOG_SCOPE();
  vcrossm->removeAllFields();
}

/***************************************************************************/

void VcrossWindow::onFieldAction(int position, int action)
{
  METLIBS_LOG_SCOPE(LOGVAL(position));

  const std::string model = vcrossm->getModelAt(position);
  const std::string field = vcrossm->getFieldAt(position);
  METLIBS_LOG_DEBUG(LOGVAL(model) << LOGVAL(field));

  if (action == VcrossLayerButton::EDIT) {
    const QString mdl = QString::fromStdString(model);
    const QString fld = QString::fromStdString(field);
    vcStyleDialog->showModelField(mdl, fld);
    vcStyleDialog->show();
  } else if (action == VcrossLayerButton::REMOVE) {
    vcrossm->removeField(model, field);
  } else if (action == VcrossLayerButton::SHOW_HIDE) {
    QBoxLayout* lbl = static_cast<QBoxLayout*>(ui->layerButtons->layout());
    QWidgetItem* wi = static_cast<QWidgetItem*>(lbl->itemAt(position));
    VcrossLayerButton *button = static_cast<VcrossLayerButton*>(wi->widget());

    vcrossm->setFieldVisible(position, button->isChecked());
  }
}

void VcrossWindow::onFieldChangeBegin(bool fromScript)
{
  METLIBS_LOG_SCOPE();
  mInFieldChangeGroup = true;
  mGroupChangedFields = false;
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

  repaintPlotIfNotInGroup();
}

void VcrossWindow::onFieldOptionsChanged(const std::string& model, const std::string& field, int position)
{
  repaintPlotIfNotInGroup();
}

void VcrossWindow::onFieldVisibilityChanged(const std::string& model, const std::string& field, int position)
{
  repaintPlotIfNotInGroup();
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

  QWidgetItem* wi = static_cast<QWidgetItem*>(lbl->takeAt(position));
  QToolButton *button = static_cast<QToolButton*>(wi->widget());
  button->deleteLater();

  repaintPlotIfNotInGroup();
}

void VcrossWindow::repaintPlotIfNotInGroup()
{
  if (mInFieldChangeGroup)
    mGroupChangedFields = true;
  else
    repaintPlot();
}

void VcrossWindow::onFieldChangeEnd()
{
  if (mInFieldChangeGroup && mGroupChangedFields)
    repaintPlot();
  mInFieldChangeGroup = mGroupChangedFields = false;
}

/***************************************************************************/

void VcrossWindow::stepCrossection(int direction)
{
  METLIBS_LOG_SCOPE();
  int index = vcrossm->getCrossectionIndex();
  if (vcross::util::step_index(index, direction > 0 ? +1 : -1, vcrossm->getCrossectionCount()))
    vcrossm->setCrossectionIndex(index);
}


void VcrossWindow::rightCrossectionClicked()
{
  stepCrossection(+1);
}


void VcrossWindow::leftCrossectionClicked()
{
  stepCrossection(-1);
}


void VcrossWindow::crossectionChangedSlot(int current)
{
  METLIBS_LOG_SCOPE();
  ui->comboCs->setCurrentIndex(current);
  ui->vcross->update();
  Q_EMIT crossectionChanged(vcrossm->getCrossectionLabel()); //name of current crossection (to mainWindow)
}


void VcrossWindow::crossectionListChangedSlot()
{
  METLIBS_LOG_SCOPE();
  ui->comboCs->clear();
  const int count = vcrossm->getCrossectionCount();
  for (int i=0; i<count; ++i)
    ui->comboCs->addItem(vcrossm->getCrossectionLabel(i));

  ui->comboCs->setEnabled(count > 0);
  ui->buttonCsPrevious->setEnabled(count > 0);
  ui->buttonCsNext->setEnabled(count > 0);

  enableDynamicCsIfSupported();

  emitCrossectionSet();
}


void VcrossWindow::emitCrossectionSet()
{
  METLIBS_LOG_SCOPE();
  // emit to MainWindow (updates crossectionPlot)
  LocationData locations;
  vcrossm->getCrossections(locations);
  Q_EMIT crossectionSetChanged(locations);
  Q_EMIT crossectionChanged(vcrossm->getCrossectionLabel());
}


void VcrossWindow::enableDynamicCsIfSupported()
{
  METLIBS_LOG_SCOPE();
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


void VcrossWindow::crossectionBoxActivated(int index)
{
  vcrossm->setCrossectionIndex(index);
}

/***************************************************************************/

void VcrossWindow::stepTime(int direction)
{
  METLIBS_LOG_SCOPE(LOGVAL(direction));
  const int step = std::max(ui->timeSpinBox->value(), 1)
      * (direction < 0 ? -1 : 1);

  int index = vcrossm->getTimeIndex();
  if (vcross::util::step_index(index, step, vcrossm->getTimeCount()))
    vcrossm->setTimeIndex(index);
}


void VcrossWindow::leftTimeClicked()
{
  stepTime(-1);
}


void VcrossWindow::rightTimeClicked()
{
  stepTime(+1);
}


void VcrossWindow::timeChangedSlot(int current)
{
  METLIBS_LOG_SCOPE(LOGVAL(current));
  ui->comboTime->setCurrentIndex(current);
  ui->vcross->update();
  Q_EMIT setTime("vcross", vcrossm->getTimeValue(current));
}


void VcrossWindow::timeListChangedSlot()
{
  METLIBS_LOG_SCOPE();

  std::vector<miutil::miTime> times;
  const int count = vcrossm->getTimeCount();
  METLIBS_LOG_DEBUG(LOGVAL(count));
  for (int i=0; i<count; ++i) {
    METLIBS_LOG_DEBUG(LOGVAL(i));
    times.push_back(vcrossm->getTimeValue(i));
  }
  ui->comboTime->setModel(new MiTimeModel(times));

  ui->comboTime->setEnabled(count > 0);
  ui->buttonTimePrevious->setEnabled(count > 0);
  ui->buttonTimeNext->setEnabled(count > 0);

  if (count > 0)
    Q_EMIT emitTimes("vcross", times);
}


void VcrossWindow::timeBoxActivated(int index)
{
  METLIBS_LOG_SCOPE(LOGVAL(index));
  vcrossm->setTimeIndex(index);
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

  // these have to match the keys used by EditItemManager::emitItemChanged
  const char KEY_POINTS[] = "latLonPoints", KEY_ID[] = "id", KEY_NAME[] = "Placemark:name";

  if (not (props.contains(KEY_POINTS) and props.contains(KEY_ID)))
    return;

  QString label;
  if (props.contains(KEY_NAME))
    label = props.value(KEY_NAME).toString();
  if (label.isEmpty())
    label = QString("dyn_%1").arg(props.value(KEY_ID).toInt());
  
  vcross::LonLat_v points;
  foreach (QVariant v, props.value(KEY_POINTS).toList()) {
    const QPointF p = v.toPointF();
    const float lat = p.x(), lon = p.y(); // FIXME swpa x <-> y
    points.push_back(LonLat::fromDegrees(lon, lat));
  }
  if (points.size() < 2)
    return;

  vcrossm->addDynamicCrossection(label, points);
}

void VcrossWindow::dynCrossEditManagerRemoval(int id)
{
  METLIBS_LOG_SCOPE(LOGVAL(id));
  vcrossm->removeDynamicCrossection(QString("dyn_%1").arg(id));
}

/***************************************************************************/

void VcrossWindow::repaintPlot()
{
  ui->vcross->update();
}

/***************************************************************************/

void VcrossWindow::changeSetup()
{
  emitCrossectionSet();
  repaintPlot();
}

/***************************************************************************/

bool VcrossWindow::changeCrossection(const std::string& crossection)
{
  vcrossm->setCrossectionIndex(vcrossm->findCrossectionIndex(QString::fromStdString(crossection)));
  return true;
}

/***************************************************************************/

void VcrossWindow::mainWindowTimeChanged(const miutil::miTime& t)
{
  if (!active)
    return;

  vcrossm->setTimeToBestMatch(t);
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

void VcrossWindow::parseQuickMenuStrings(const string_v& qm_strings)
{
  quickmenues->parse(qm_strings);
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
    sec_setup.addLines(vcrossm->writeVcrossOptions());
  }
  { LogFileIO::Section& sec_field = logfile.getSection(LOG_FIELD);
    sec_field.addLines(vcrossm->writePlotOptions());
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
    vcrossm->readVcrossOptions(sec_setup.lines(), thisVersion, logVersion);
  }
  { const LogFileIO::Section& sec_field = logfile.getSection(LOG_FIELD);
    vcrossm->readPlotOptions(sec_field.lines(), thisVersion, logVersion);
  }
}

/***************************************************************************/

void VcrossWindow::closeEvent(QCloseEvent * e)
{
  quitClicked();
}
