/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013-2015 met.no

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

#include <diDrawingManager.h>
#include <diController.h>
#include <EditItems/drawingdialog.h>
#include <EditItems/toolbar.h>
#include <EditItems/layergroupspane.h>
#include <EditItems/drawinglayerspane.h>

#include <EditItems/editpolyline.h>
#include <EditItems/editsymbol.h>
#include <EditItems/edittext.h>
#include <EditItems/editcomposite.h>

#include <drawing.xpm> // ### for now
#include <QAction>
#include <QApplication>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QSplitter>
#include <QVBoxLayout>

#include <EditItems/layer.h>
#include <EditItems/layergroup.h>
#include <EditItems/layermanager.h>
#include <EditItems/kml.h>
#include <QMessageBox>
#include <QDir>

#ifdef ENABLE_DRAWINGDIALOG_TESTING
#include <QPushButton>
#include <QCheckBox>
#include <QDebug>
#endif // ENABLE_DRAWINGDIALOG_TESTING

namespace EditItems {

DrawingDialog::DrawingDialog(QWidget *parent, Controller *ctrl)
  : DataDialog(parent, ctrl)
{
  drawm_ = DrawingManager::instance();

  // create an action that can be used to open the dialog from within a menu or toolbar
  m_action = new QAction(QIcon(QPixmap(drawing_xpm)), tr("Drawing Dialog"), this);
  m_action->setShortcut(Qt::ALT + Qt::Key_B);
  m_action->setCheckable(true);
  m_action->setIconVisibleInMenu(true);

  // create the GUI
  setWindowTitle(tr("Drawing Dialog"));
  setFocusPolicy(Qt::StrongFocus);
  QSplitter *splitter = new QSplitter(Qt::Vertical);
  layerGroupsPane_ = new LayerGroupsPane();
  splitter->addWidget(layerGroupsPane_);
  layersPane_ = new DrawingLayersPane(tr("Active Layers"));
  layersPane_->init();
  splitter->addWidget(layersPane_);
  splitter->setSizes(QList<int>() << 500 << 500);
  //
  //
  QVBoxLayout *mainLayout = new QVBoxLayout;
  setLayout(mainLayout);
  mainLayout->addWidget(splitter);

  mainLayout->addLayout(createStandardButtons());

  // load available layer groups
  QMap<QString, QString> drawings = drawm_->getDrawings();
  foreach (const QString &name, drawings.keys()) {
    QSharedPointer<LayerGroup> layerGroup(new LayerGroup(name.isEmpty() ? "new layer group" : name));
    layerGroup->setFileName(drawings[name]);
    fileMap_[drawings[name]] = name;
    layerGroupsPane_->addWidgetForLG(layerGroup);
  }
  layerGroupsPane_->updateWidgetStructure();

  // add connections
  connect(layerGroupsPane_, SIGNAL(updated()), layersPane_, SLOT(updateWidgetStructure()));
  connect(layersPane_, SIGNAL(updated()), layerGroupsPane_, SLOT(updateWidgetContents()));

  connect(layerGroupsPane_, SIGNAL(updated()), SLOT(handleDialogUpdated()));
  connect(layersPane_, SIGNAL(updated()), SLOT(handleDialogUpdated()));

  connect(layersPane_, SIGNAL(newEditLayerRequested(const QSharedPointer<Layer> &)), SIGNAL(newEditLayerRequested(const QSharedPointer<Layer> &)));

  connect(this, SIGNAL(applyData()), SLOT(makeProduct()));
}

void DrawingDialog::showInfo(bool checked)
{
  layerGroupsPane_->showInfo(checked);
  layersPane_->showInfo(checked);
}

std::string DrawingDialog::name() const
{
  return "DRAWING";
}

void DrawingDialog::updateTimes()
{
  std::vector<miutil::miTime> times;
  times = drawm_->getTimes();
  emit emitTimes("DRAWING", times);
}

void DrawingDialog::updateDialog()
{
}

std::vector<std::string> DrawingDialog::getOKString()
{
  std::vector<std::string> lines;

  if (!drawm_->isEnabled())
    return lines;

  QMap<QString, QString> loaded = drawm_->getLoaded();
  foreach (QString name, loaded.keys()) {
    QString line;
    QString fileName = loaded[name];
    if (fileName == name)
      line = "DRAWING file=\"" + fileName + "\"";
    else
      line = "DRAWING name=\"" + name + "\"";
    lines.push_back(line.toStdString());
  }

  return lines;
}

void DrawingDialog::putOKString(const std::vector<std::string>& vstr)
{
  // Submit the lines as new input.
  std::vector<std::string> inp;
  inp.insert(inp.begin(), vstr.begin(), vstr.end());
  drawm_->processInput(inp);
}

void DrawingDialog::makeProduct()
{
  // Obtain a set of the files in use.
  QSet<QString> sources;
  foreach (DrawingItemBase *item, drawm_->allItems())
    sources.insert(item->property("srcFile").toString());

  // Map the files back to names for the drawings if possible.
  std::vector<std::string> inp;
  foreach (QString &source, sources) {
    if (!fileMap_.contains(source))
      inp.push_back("DRAWING file=\"" + source.toStdString() + "\"");
    else
      inp.push_back("DRAWING name=\"" + fileMap_.value(source).toStdString() + "\"");
  }

  putOKString(inp);

  // Update the available times.
  updateTimes();

  indicateUnappliedChanges(false); // indicate that the DrawingManager is now synchronized with the dialog
}

void DrawingDialog::handleDialogUpdated()
{
  indicateUnappliedChanges(true);
  layersPane_->updateButtons();
}


} // namespace
