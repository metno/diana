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

#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "qtUtility.h"
#include "diSpectrumManager.h"
#include "qtSpectrumModelDialog.h"

#define MILOGGER_CATEGORY "diana.SpectrumModelDialog"
#include <miLogger/miLogging.h>


//#define HEIGHTLISTBOX 100

/***************************************************************************/

SpectrumModelDialog::SpectrumModelDialog( QWidget* parent,SpectrumManager * vm )
: QDialog(parent),spectrumm(vm)
{

  METLIBS_LOG_SCOPE();


  //caption to appear on top of dialog
  setWindowTitle( tr("Diana Wavespectrum") );

  modelfileList = new QListWidget(this);
  connect(modelfileList, SIGNAL(itemClicked(QListWidgetItem*)), SLOT(modelfilelistClicked(QListWidgetItem*)));

  QLabel* refLabel = TitleLabel(tr("Referencetime"),this);
  reftimeWidget = new QListWidget(this);
  connect(reftimeWidget, SIGNAL(itemClicked(QListWidgetItem*)), SLOT(reftimeWidgetClicked(QListWidgetItem*)));

  QLabel* selectedLabel = TitleLabel(tr("Selected models"),this);
  selectedModelsWidget = new QListWidget(this);

  //push button to show help
  QPushButton* modelhelp = new QPushButton(tr("Help"), this);
  connect(modelhelp, SIGNAL(clicked()), SLOT(helpClicked()));

  //push button to delete
  QPushButton* deleteButton = new QPushButton(tr("Delete"), this);
  connect(deleteButton, SIGNAL(clicked()), SLOT(deleteClicked()));
  QPushButton* deleteAllButton = new QPushButton(tr("Delete all"), this);
  connect(deleteAllButton, SIGNAL(clicked()), SLOT(deleteAllClicked()));

  //push button to refresh
  QPushButton* refresh = new QPushButton(tr("Refresh"), this);
  connect(refresh, SIGNAL(clicked()), SLOT(refreshClicked()));

  //push button to hide dialog
  QPushButton* modelhide = new QPushButton(tr("Hide"), this);
  connect(modelhide, SIGNAL(clicked()), SIGNAL(ModelHide()));

  //push button to apply the selected command and then hide dialog
  QPushButton* modelapplyhide = new QPushButton(tr("Apply+Hide"), this);
  connect(modelapplyhide, SIGNAL(clicked()), SLOT(applyhideClicked()));

  //push button to apply the selected command
  QPushButton* modelapply = new QPushButton(tr("Apply"), this);
  connect(modelapply, SIGNAL(clicked()), SLOT(applyClicked()));

  // ************ place all the widgets in layouts ****************

  QHBoxLayout* hlayout1 = new QHBoxLayout();
  hlayout1->addWidget(deleteButton);
  hlayout1->addWidget(deleteAllButton);

  QHBoxLayout* hlayout2 = new QHBoxLayout();
  hlayout2->addWidget(modelhelp);
  hlayout2->addWidget(refresh);

  QHBoxLayout* hlayout3 = new QHBoxLayout();
  hlayout3->addWidget(modelhide);
  hlayout3->addWidget(modelapplyhide);
  hlayout3->addWidget(modelapply);

  //create a vertical layout to put all widgets and layouts in
  QVBoxLayout * vlayout = new QVBoxLayout(this);
  vlayout->addWidget(modelfileList);
  vlayout->addWidget(refLabel);
  vlayout->addWidget(reftimeWidget);
  vlayout->addWidget(selectedLabel);
  vlayout->addWidget(selectedModelsWidget);
  vlayout->addLayout(hlayout1);
  vlayout->addLayout(hlayout2);
  vlayout->addLayout(hlayout3);
}

/*********************************************/
void SpectrumModelDialog::modelfilelistClicked(QListWidgetItem* item)
{
  reftimeWidget->clear();
  std::vector<std::string> rfv = spectrumm->getReferencetimes(item->text().toStdString());
  for ( size_t i=0; i<rfv.size(); ++i){
    reftimeWidget->addItem(rfv[i].c_str());
  }

  if ( rfv.empty() ) {
    selectedModelsWidget->addItem(modelfileList->currentItem()->text());
    selectedModelsWidget->setCurrentRow(selectedModelsWidget->count()-1);
  } else {
    reftimeWidget->setCurrentRow(reftimeWidget->count()-1);
    QString qstr = getSelectedModelString();
    selectedModelsWidget->addItem(qstr);
    selectedModelsWidget->setCurrentRow(selectedModelsWidget->count()-1);
  }
}

void SpectrumModelDialog::reftimeWidgetClicked(QListWidgetItem* item)
{
  if ( selectedModelsWidget->count() && selectedModelsWidget->currentItem()->text().contains(modelfileList->currentItem()->text())) {
    QString qstr = getSelectedModelString();
    QListWidgetItem* ii = selectedModelsWidget->item(selectedModelsWidget->currentRow());
    ii->setText(qstr);
  }
}

void SpectrumModelDialog::refreshClicked()
{
  METLIBS_LOG_SCOPE();
  updateModelfileList();

}

/*********************************************/

void SpectrumModelDialog::deleteClicked()
{
  METLIBS_LOG_SCOPE();
  if ( selectedModelsWidget->count() ) {
    int row = selectedModelsWidget->currentRow();
    selectedModelsWidget->takeItem(row);
  }
}

/*********************************************/

void SpectrumModelDialog::deleteAllClicked()
{
  METLIBS_LOG_SCOPE();
  selectedModelsWidget->clear();
  reftimeWidget->clear();
}

/*********************************************/

void SpectrumModelDialog::helpClicked()
{
  METLIBS_LOG_SCOPE();
  emit showsource("ug_verticalprofiles.html");
}

/*********************************************/

void SpectrumModelDialog::applyClicked()
{
  METLIBS_LOG_SCOPE();
  setModel();
  emit ModelApply();

}

/*********************************************/

void SpectrumModelDialog::applyhideClicked()
{
  METLIBS_LOG_SCOPE();
  setModel();
  emit ModelHide();
  emit ModelApply();

}

/*********************************************/
void SpectrumModelDialog::setModel()
{
  METLIBS_LOG_SCOPE();

  std::vector<std::string> selectedModels;
  int n = selectedModelsWidget->count();
  for (int i = 0; i < n; i++) {
    selectedModels.push_back(selectedModelsWidget->item(i)->text().toStdString());
  }
  spectrumm->setSelectedModels(selectedModels);

}

QString SpectrumModelDialog::getSelectedModelString()
{
  QString qstr;
  if ( modelfileList->currentItem() && reftimeWidget->currentItem()) {
    qstr = modelfileList->currentItem()->text() + " " + reftimeWidget->currentItem()->text();
  }
  return qstr;
}

/*********************************************/

void SpectrumModelDialog::updateModelfileList()
{
  METLIBS_LOG_SCOPE();

  //want to keep the selected models
  int n = modelfileList->count();
  std::set<std::string> current;
  for (int i = 0; i < n; i++)
    if (modelfileList->item(i)->isSelected())
      current.insert(std::string(modelfileList->item(i)->text().toStdString()));

  //clear box with list of files
  modelfileList->clear();

  std::vector<std::string> modelnames = spectrumm->getModelNames();
  std::vector<std::string> modelfiles = spectrumm->getModelFiles();
  size_t nr_models = modelnames.size();
  bool tooltip = nr_models == modelfiles.size();
  for ( size_t i = 0; i < nr_models; i++) {
    QListWidgetItem* item = new QListWidgetItem(modelnames[i].c_str(),modelfileList);
    if ( tooltip )
      item->setToolTip(modelfiles[i].c_str());
  }

  std::set<std::string>::iterator pend = current.end();
  n = modelfileList->count();
  for (int i = 0; i < n; i++)
    if (current.find(std::string(modelfileList->item(i)->text().toStdString()))
        != pend)
      modelfileList->item(i)->setSelected(true);

}

void SpectrumModelDialog::closeEvent(QCloseEvent* e)
{
  METLIBS_LOG_SCOPE();
  emit ModelHide();
}
