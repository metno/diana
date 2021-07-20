/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2021 met.no

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

#include "WebMapDialog.h"

#include "WebMapPlotListModel.h"
#include "WebMapManager.h"
#include "WebMapPlot.h"
#include "WebMapService.h"

#include "util/qtComboBoxItemDelegate.h"

#include <puTools/miStringFunctions.h>

#include <QAction>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QStringListModel>

#include <sstream>

#include "ui_webmap_dialog.h"
#include "icon_wms_32.xpm"

#define MILOGGER_CATEGORY "diana.WebMapDialog"
#include <miLogger/miLogging.h>

#include "kill.xpm"

namespace {
const std::string WEBMAP = "WEBMAP";
const size_t IDX_INVALID = size_t(-1);

const std::string STYLE_ALPHA_SCALE = "style.alpha_scale";
const std::string STYLE_ALPHA_OFFSET = "style.alpha_offset";
const std::string STYLE_GREY = "style.grey";
const std::string STYLE_NAME = "style.name";

// this must match ui->comboPlotOrder
const std::string plotorders[] = {
  "background",
  "shade_background",
  "shade",
  "lines_background",
  "lines"
};
const int plotorder_default_idx = 1;
const size_t N_PLOTORDERS = sizeof(plotorders)/sizeof(plotorders[0]);

class DimensionValuesItemDelegate : public ComboBoxItemDelegate
{
public:
  DimensionValuesItemDelegate(WebMapLayer_cx layer, QAbstractItemModel* model, QWidget* parent);
  void configureEditor(QComboBox* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

private:
  WebMapLayer_cx layer_;
};

DimensionValuesItemDelegate::DimensionValuesItemDelegate(WebMapLayer_cx layer, QAbstractItemModel* model, QWidget* parent)
    : ComboBoxItemDelegate(model, parent)
    , layer_(layer)
{
}

void DimensionValuesItemDelegate::configureEditor(QComboBox* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
  if (!layer_)
    return;
  const int dimIdx = model->data(index, Qt::UserRole).toInt();
  if (dimIdx >= (int)layer_->countDimensions())
    return;
  const auto& dim = layer_->dimension(dimIdx);
  for (const auto& v : dim.values()) {
    editor->addItem(QString::fromStdString(v));
  }
}

} // namespace

const std::string WEBMAP_SERVICE = "webmap.service";
const std::string WEBMAP_LAYER = "webmap.layer";
const std::string WEBMAP_DIM = "webmap.dim";
const std::string WEBMAP_ZORDER = "webmap.zorder";
const std::string WEBMAP_TIME_TOLERANCE = "webmap.time_tolerance";
const std::string WEBMAP_TIME_OFFSET = "webmap.time_offset";

WebMapDialog::WebMapDialog(QWidget *parent, Controller *ctrl)
  : DataDialog(parent, ctrl)
  , ui(new Ui_WebMapDialog)
  , mAddSelectedService(0)
{
  METLIBS_LOG_SCOPE();

  m_action = new QAction(QPixmap(icon_wms_32_xpm), tr("Web Maps"), this);
  m_action->setCheckable(true);

  setupUi();
  initializeAddServicePage(true);

  if (WebMapManager* wmm = WebMapManager::instance())
    connect(wmm, &WebMapManager::serviceRefreshFinished, this, &DataDialog::updateTimes);
}

WebMapDialog::~WebMapDialog()
{
}

void WebMapDialog::setupUi()
{
  METLIBS_LOG_SCOPE();
  ui->setupUi(this);
  ui->labelPending->setVisible(false);

  mServicesModel = new QStringListModel(this);
  mServicesFilter = new QSortFilterProxyModel(this);
  mServicesFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
  mServicesFilter->setSourceModel(mServicesModel);
  ui->listAddServices->setModel(mServicesFilter);
  connect(ui->editAddServiceFilter, SIGNAL(textChanged(const QString&)),
      this, SLOT(onAddServicesFilter(const QString&)));

  connect(ui->listAddServices->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection&)),
      this, SLOT(checkAddComplete()));
  connect(ui->listAddServices, SIGNAL(activated(const QModelIndex&)),
      this, SLOT(onAddNext()));

  mLayersModel = new QStringListModel(this);
  mLayersFilter = new QSortFilterProxyModel(this);
  mLayersFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
  mLayersFilter->setSourceModel(mLayersModel);
  ui->listAddLayers->setModel(mLayersFilter);
  connect(ui->editAddLayerFilter, SIGNAL(textChanged(const QString&)),
      this, SLOT(onAddLayersFilter(const QString&)));

  connect(ui->listAddLayers->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection&)),
      this, SLOT(checkAddComplete()));
  connect(ui->listAddLayers, SIGNAL(activated(const QModelIndex&)),
      this, SLOT(onAddNext()));

  connect(ui->buttonNextAdd, SIGNAL(clicked()),
      this, SLOT(onAddNext()));
  connect(ui->buttonBack, SIGNAL(clicked()),
      this, SLOT(onAddBack()));
  connect(ui->buttonRestart, SIGNAL(clicked()),
      this, SLOT(onAddRestart()));

  ui->comboStyleLayer->setModel(new WebMapPlotListModel(this));
  ui->buttonLayerRemove->setIcon(QPixmap(kill_xpm));

  connect(ui->tableOtherDims, &QTableWidget::itemChanged, this, &WebMapDialog::onModifyDimensionValuesChanged);
}

void WebMapDialog::initializeAddServicePage(bool forward)
{
  METLIBS_LOG_SCOPE();
  ui->stackAdd->setCurrentIndex(AddServicePage);

  if (mAddSelectedService) {
    disconnect(mAddSelectedService, SIGNAL(refreshStarting()),
        this, SLOT(onServiceRefreshStarting()));
    disconnect(mAddSelectedService, SIGNAL(refreshFinished()),
        this, SLOT(onServiceRefreshFinished()));
    mAddSelectedService = 0;
  }

  if (forward) {
    ui->editAddServiceFilter->clear();
    QStringList services;
    if (WebMapManager* wmm = WebMapManager::instance()) {
      for (int i=0; i<wmm->getServiceCount(); ++i) {
        const WebMapService* wms = wmm->getService(i);
        services << QString::fromStdString
            ((wms->title().empty()) ? wms->identifier() : wms->title());
      }
    }
    METLIBS_LOG_DEBUG(LOGVAL(services.size()));
    mServicesModel->setStringList(services);
  }

  ui->buttonRestart->setEnabled(false);
  ui->buttonBack->setEnabled(false);
  checkAddComplete();
}

void WebMapDialog::onAddServicesFilter(const QString& text)
{
  mServicesFilter->setFilterFixedString(text);
}

bool WebMapDialog::isAddServiceComplete()
{
  return (not ui->listAddServices->selectionModel()->selectedIndexes().isEmpty());
}

WebMapService* WebMapDialog::selectedAddService() const
{
  METLIBS_LOG_SCOPE();
  WebMapManager* wmm = WebMapManager::instance();
  if (!wmm)
    return 0;

  const QModelIndexList si = ui->listAddServices->selectionModel()->selectedIndexes();
  if (si.size() != 1)
    return 0;

  const int i = mServicesFilter->mapToSource(si.at(0)).row();
  if (i >= wmm->getServiceCount())
    return 0;

  return wmm->getService(i);
}

void WebMapDialog::initializeAddLayerPage(bool forward)
{
  METLIBS_LOG_SCOPE();
  ui->stackAdd->setCurrentIndex(AddLayerPage);

  if (forward) {
    assert(!mAddSelectedService);
    mAddSelectedService = selectedAddService();
    if (mAddSelectedService) {
      connect(mAddSelectedService, SIGNAL(refreshStarting()),
          this, SLOT(onServiceRefreshStarting()));
      connect(mAddSelectedService, SIGNAL(refreshFinished()),
          this, SLOT(onServiceRefreshFinished()));
      mAddSelectedService->refresh();
      ui->labelAddChooseLayer->setText(tr("Choose layer in service %1:")
          .arg(QString::fromStdString(mAddSelectedService->identifier())));
    }
    ui->editAddLayerFilter->clear();

    updateAddLayers();
  }

  ui->buttonRestart->setEnabled(true);
  ui->buttonBack->setEnabled(true);
  checkAddComplete();
}

void WebMapDialog::onServiceRefreshStarting()
{
  mLayersModel->setStringList(QStringList());
  ui->labelPending->setVisible(true);
  ui->listAddLayers->setEnabled(false);
  checkAddComplete();
}

void WebMapDialog::onServiceRefreshFinished()
{
  ui->labelPending->setVisible(false);
  ui->listAddLayers->setEnabled(true);
  updateAddLayers();
  onModifyLayerSelected();
}

void WebMapDialog::updateAddLayers()
{
  QStringList layers;
  if (mAddSelectedService) {
    for (int i=0; i < (int)mAddSelectedService->countLayers(); ++i)
      layers << QString::fromStdString(mAddSelectedService->layer(i)->title());
  }
  mLayersModel->setStringList(layers);
  checkAddComplete();
}

void WebMapDialog::onAddLayersFilter(const QString& text)
{
  mLayersFilter->setFilterFixedString(text);
}

bool WebMapDialog::isAddLayerComplete()
{
  return (not ui->listAddLayers->selectionModel()->selectedIndexes().isEmpty());
}

void WebMapDialog::checkAddComplete()
{
  METLIBS_LOG_SCOPE();
  const int page = ui->stackAdd->currentIndex();
  if (page == AddServicePage) {
    ui->buttonNextAdd->setText(tr("Next"));
    ui->buttonNextAdd->setEnabled(isAddServiceComplete());
  } else if (page == AddLayerPage) {
    ui->buttonNextAdd->setText(tr("Add"));
    ui->buttonNextAdd->setEnabled(isAddLayerComplete());
  }
}

const WebMapLayer* WebMapDialog::selectedAddLayer() const
{
  const WebMapService* wms = selectedAddService();
  const QModelIndexList si = ui->listAddLayers->selectionModel()->selectedIndexes();
  if (!wms || si.size() != 1)
    return 0;

  const int i = mLayersFilter->mapToSource(si.at(0)).row();
  if (i >= (int)wms->countLayers())
    return 0;
  return wms->layer(i);
}

void WebMapDialog::onAddNext()
{
  METLIBS_LOG_SCOPE();
  const int page = ui->stackAdd->currentIndex();
  if (page == AddServicePage) {
    if (isAddServiceComplete())
      initializeAddLayerPage(true);
  } else if (page == AddLayerPage) {
    addSelectedLayer();
    ui->listAddLayers->selectionModel()->clear();
  }
}

void WebMapDialog::onAddBack()
{
  METLIBS_LOG_SCOPE();
  const int page = ui->stackAdd->currentIndex();
  if (page == AddLayerPage)
    initializeAddServicePage(false);
}

void WebMapDialog::onAddRestart()
{
  initializeAddServicePage(true);
}

void WebMapDialog::addSelectedLayer()
{
  const WebMapLayer* layer = selectedAddLayer();
  if (!isAddLayerComplete() || !mAddSelectedService || !layer)
    return;

  KVListPlotCommand_p cmd = std::make_shared<KVListPlotCommand>(WEBMAP);
  cmd->add(WEBMAP_SERVICE, mAddSelectedService->identifier());
  cmd->add(WEBMAP_LAYER, layer->identifier());
  cmd->add(STYLE_ALPHA_SCALE, "0.75");

  WebMapPlotListModel* m = static_cast<WebMapPlotListModel*>(ui->comboStyleLayer->model());
  m->onPlotAddBegin(mOk.size());
  mOk.push_back(cmd);
  m->onPlotAddEnd();

  Q_EMIT applyData();
}

void WebMapDialog::onModifyApply()
{
  Q_EMIT applyData();
}

void WebMapDialog::onModifyReset()
{
}

namespace {
WebMapService* findService(const std::string& id)
{
  if (WebMapManager* wmm = WebMapManager::instance()) {
    for (int i = 0; i < wmm->getServiceCount(); ++i) {
      WebMapService* ws = wmm->getService(i);
      if (ws->identifier() == id)
        return ws;
    }
  }
  return nullptr;
}

WebMapLayer_cx getLayerForCommand(KVListPlotCommand_cp cmd)
{
  const size_t idx_service = cmd->find(WEBMAP_SERVICE);
  const size_t idx_layer = cmd->find(WEBMAP_LAYER);
  if (idx_service == IDX_INVALID || idx_layer == IDX_INVALID)
    return nullptr;

  WebMapService* ws = findService(cmd->get(idx_service).value());
  if (!ws)
    return nullptr;

  return ws->findLayerByIdentifier(cmd->get(idx_layer).value());
}
} // namespace

void WebMapDialog::onModifyLayerSelected()
{
  bool enable = false;
  if (KVListPlotCommand_cp pc = plotCommand(ui->comboStyleLayer->currentIndex())) {
    enable = true;

    WebMapLayer_cx wl = getLayerForCommand(pc);
    {
      ui->comboStyle->clear();
      const int nStyles = wl ? wl->countStyles() : 0;
      ui->comboStyle->setEnabled(nStyles > 1);
      if (nStyles > 0) {
        for (int i = 0; i < nStyles; ++i) {
          ui->comboStyle->addItem(QString::fromStdString(wl->style(i)));
        }
      }
    }
    {
      const size_t idx_name = pc->find(STYLE_NAME);
      int c_idx = 0;
      if (idx_name != IDX_INVALID) {
        const std::string stylename = pc->get(idx_name).value();
        c_idx = std::max(ui->comboStyle->findText(QString::fromStdString(stylename)), 0); // findText returns -1 if not found
      }
      ui->comboStyle->setCurrentIndex(c_idx);
    }
    { const size_t idx_grey = pc->find(STYLE_GREY);
      bool grey = (idx_grey != IDX_INVALID) ? pc->get(idx_grey).toBool() : false;
      ui->checkGrey->setChecked(grey);
    }
    { const size_t idx_as = pc->find(STYLE_ALPHA_SCALE);
      float alpha_scale = (idx_as != IDX_INVALID) ? pc->get(idx_as).toFloat() : 0.75f;
      ui->spinAlphaScale->setValue(alpha_scale);
    }
    { const size_t idx_ao = pc->find(STYLE_ALPHA_OFFSET);
      float alpha_offset = (idx_ao != IDX_INVALID) ? pc->get(idx_ao).toFloat() : 0;
      ui->spinAlphaOffset->setValue(alpha_offset);
    }
    { const size_t idx_tt = pc->find(WEBMAP_TIME_TOLERANCE);
      int time_tolerance_sec = (idx_tt != IDX_INVALID) ? pc->get(idx_tt).toInt() : -1;
      ui->spinTimeTolerance->setValue(time_tolerance_sec);
    }
    { const size_t idx_to = pc->find(WEBMAP_TIME_OFFSET);
      int time_offset_sec = (idx_to != IDX_INVALID) ? pc->get(idx_to).toInt() : 0;
      ui->spinTimeOffset->setValue(time_offset_sec);
    }
    { const size_t idx_po = pc->find(WEBMAP_ZORDER);
      int po = plotorder_default_idx;
      const std::string& pot = (idx_po != IDX_INVALID) ? pc->get(idx_po).value() : plotorders[po];
      for (size_t i=0; i < N_PLOTORDERS; ++i) {
        if (plotorders[i] == pot) {
          po = i;
          break;
        }
      }
      ui->comboPlotOrder->setCurrentIndex(po);
    }
    {
      ui->tableOtherDims->setEditTriggers(QAbstractItemView::AllEditTriggers);
      ui->tableOtherDims->clear();
      const int nDimensions = wl ? wl->countDimensions() : 0;
      ui->tableOtherDims->setEnabled(nDimensions > 0);
      if (wl) {
        std::map<std::string, std::string> dimValues;
        for (const auto& kv : pc->all()) {
          if (kv.key() == WEBMAP_DIM) {
            const std::vector<std::string> dnv = miutil::split(kv.value(), 1, "=");
            if (dnv.size() == 2)
              dimValues[dnv[0]] = dnv[1];
          }
        }

        ui->tableOtherDims->setRowCount(0);
        ui->tableOtherDims->setColumnCount(2);
        for (int i = 0; i < nDimensions; ++i) {
          const auto& wd = wl->dimension(i);
          if (wd.isTime() || wd.values().empty())
            continue;

          const std::string& dim = !wd.title().empty() ? wd.identifier() : wd.identifier();
          const auto itV = dimValues.find(dim);
          const std::string& val = (itV != dimValues.end()) ? itV->second : wd.defaultValue();

          QTableWidgetItem* itemDim = new QTableWidgetItem(QString::fromStdString(dim));
          itemDim->setFlags(Qt::ItemIsEnabled);
          QTableWidgetItem* itemVal = new QTableWidgetItem(QString::fromStdString(val));
          itemVal->setData(Qt::UserRole, i);
          itemVal->setData(Qt::UserRole + 1, QString::fromStdString(wd.identifier()));

          const int r = ui->tableOtherDims->rowCount();
          ui->tableOtherDims->setRowCount(r + 1);
          ui->tableOtherDims->setItem(r, 0, itemDim);
          ui->tableOtherDims->setItem(r, 1, itemVal);
        }
        ui->tableOtherDims->setItemDelegateForColumn(1, new DimensionValuesItemDelegate(wl, ui->tableOtherDims->model(), ui->tableOtherDims));
      }
    }
  }

  ui->buttonModifyApply->setEnabled(enable);
  ui->buttonLayerRemove->setEnabled(enable);

  ui->checkGrey->setEnabled(enable);
  ui->spinAlphaScale->setEnabled(enable);
  ui->spinAlphaOffset->setEnabled(enable);
  ui->comboPlotOrder->setEnabled(enable);
  ui->spinTimeOffset->setEnabled(enable);
  ui->spinTimeTolerance->setEnabled(enable);
}

void WebMapDialog::onModifyLayerRemove()
{
  METLIBS_LOG_SCOPE();
  WebMapPlotListModel* m = static_cast<WebMapPlotListModel*>(ui->comboStyleLayer->model());
  m->onPlotsRemoveBegin();
  mOk.erase(mOk.begin() + ui->comboStyleLayer->currentIndex());
  m->onPlotsRemoveEnd();

  Q_EMIT applyData();
}

void WebMapDialog::onModifyStyleChanged()
{
  METLIBS_LOG_SCOPE();
  replaceCommandKV(miutil::kv(STYLE_NAME, ui->comboStyle->currentText().toStdString()));
}

void WebMapDialog::onModifyGreyToggled()
{
  METLIBS_LOG_SCOPE();
  replaceCommandKV(miutil::kv(STYLE_GREY, ui->checkGrey->isChecked()));
}

void WebMapDialog::onModifyAlphaScaleChanged()
{
  METLIBS_LOG_SCOPE();
  replaceCommandKV(miutil::kv(STYLE_ALPHA_SCALE, (float)ui->spinAlphaScale->value()));
}

void WebMapDialog::onModifyAlphaOffsetChanged()
{
  METLIBS_LOG_SCOPE();
  replaceCommandKV(miutil::kv(STYLE_ALPHA_OFFSET, (float)ui->spinAlphaOffset->value()));
}

void WebMapDialog::onModifyPlotOrderChanged()
{
  METLIBS_LOG_SCOPE();
  replaceCommandKV(miutil::kv(WEBMAP_ZORDER, plotorders[ui->comboPlotOrder->currentIndex()]));
}

void WebMapDialog::onModifyTimeToleranceChanged()
{
  METLIBS_LOG_SCOPE();
  replaceCommandKV(miutil::kv(WEBMAP_TIME_TOLERANCE, ui->spinTimeTolerance->value()));
}

void WebMapDialog::onModifyTimeOffsetChanged()
{
  METLIBS_LOG_SCOPE();
  replaceCommandKV(miutil::kv(WEBMAP_TIME_OFFSET, ui->spinTimeOffset->value()));
}

void WebMapDialog::onModifyDimensionValuesChanged()
{
  METLIBS_LOG_SCOPE();
  const int idx = ui->comboStyleLayer->currentIndex();
  if (KVListPlotCommand_cp pc = plotCommand(idx)) {
    WebMapLayer_cx wl = getLayerForCommand(pc);

    KVListPlotCommand_p pcm = std::make_shared<KVListPlotCommand>(WEBMAP);
    for (const miutil::KeyValue& kv : pc->all()) {
      if (kv.key() != WEBMAP_DIM) {
        pcm->add(kv);
      }
    }
    // time dimension is not in ui->tableOtherDims
    const auto mdl = ui->tableOtherDims->model();
    for (int r = 0; r < ui->tableOtherDims->rowCount(); ++r) {
      const QModelIndex midx = mdl->index(r, 1);
      const int didx = mdl->data(midx, Qt::UserRole).toInt();                             // dimension index
      const std::string dval = mdl->data(midx, Qt::DisplayRole).toString().toStdString(); // dimension value
      const std::string val = wl->dimension(didx).identifier() + "=" + dval;
      pcm->add(miutil::kv(WEBMAP_DIM, val));
    }
    mOk[idx] = pcm;
  }
}

void WebMapDialog::replaceCommandKV(const miutil::KeyValue& kv)
{
  replaceCommandKV(kv, ui->comboStyleLayer->currentIndex());
}

void WebMapDialog::replaceCommandKV(const miutil::KeyValue& kvnew, size_t idx)
{
  METLIBS_LOG_SCOPE(LOGVAL(kvnew));
  if (KVListPlotCommand_cp pc = plotCommand(idx)) {
    KVListPlotCommand_p pcm = std::make_shared<KVListPlotCommand>(WEBMAP);
    bool replaced = false;
    for (const miutil::KeyValue& kv : pc->all()) {
      if (kv.key() == kvnew.key()) {
        if (!replaced)
          pcm->add(kvnew);
        replaced = true;
       } else {
        pcm->add(kv);
      }
    }
    if (!replaced)
      pcm->add(kvnew);

    METLIBS_LOG_DEBUG(LOGVAL(pcm->all()));

    // TODO update model
    mOk[idx] = pcm;
  }
}

std::string WebMapDialog::name() const
{
  return WEBMAP;
}

void WebMapDialog::updateDialog()
{
}

PlotCommand_cpv WebMapDialog::getOKString()
{
  METLIBS_LOG_SCOPE(LOGVAL(mOk.size()));
  return mOk;
}

void WebMapDialog::putOKString(const PlotCommand_cpv& ok)
{
  METLIBS_LOG_SCOPE(LOGVAL(ok.size()));

  const int ci = ui->comboStyleLayer->currentIndex();

  WebMapPlotListModel* m = static_cast<WebMapPlotListModel*>(ui->comboStyleLayer->model());
  m->onPlotsRemoveBegin();
  mOk = ok;
  m->onPlotsRemoveEnd();

  if (!mOk.empty())
    ui->comboStyleLayer->setCurrentIndex(std::max(std::min(ci, (int)mOk.size()-1), 0));
}

KVListPlotCommand_cp WebMapDialog::plotCommand(size_t idx) const
{
  if (idx < mOk.size())
    return std::dynamic_pointer_cast<const KVListPlotCommand>(mOk[idx]);
  else
    return KVListPlotCommand_cp();
}

void WebMapDialog::updateTimes()
{
  if (WebMapManager* wmm = WebMapManager::instance())
    emitTimes(wmm->getTimes(), false);
}
