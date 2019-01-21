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

#include "qtSatDialogAdvanced.h"

#include "diSatTypes.h"
#include "qtToggleButton.h"
#include "qtUtility.h"
#include "vcross_v2/VcrossQtUtil.h"

#include <puTools/miStringFunctions.h>

#include <QSlider>
#include <QLabel>
#include <qcheckbox.h>
#include <qlcdnumber.h>
#include <QPixmap>
#include <QVBoxLayout>

SatDialogAdvanced::SatDialogAdvanced(QWidget* parent, const SatDialogInfo& info)
    : QWidget(parent)
    , m_cut(info.cut)
    , m_alphacut(info.alphacut)
    , m_alpha(info.alpha)
    , palette(false)
{
  // Cut
  cutCheckBox = new QCheckBox(tr("Use stretch from first picture"),this);

  cut = new ToggleButton(this, tr("Cut"));

  cutlcd = LCDNumber(4, this);

  scut = Slider(m_cut.minValue, m_cut.maxValue, 1, m_cut.value, Qt::Horizontal, this);
  connect(cutCheckBox, &QCheckBox::toggled, this, &SatDialogAdvanced::cutCheckBoxSlot);
  connect(cutCheckBox, &QCheckBox::toggled, this, &SatDialogAdvanced::SatChanged);

  connect(scut, SIGNAL(valueChanged(int)), SLOT(cutDisplay(int)));
  connect(scut, SIGNAL(valueChanged(int)), SIGNAL(SatChanged()));

  connect(cut, &ToggleButton::toggled, this, &SatDialogAdvanced::greyCut);
  connect(cut, &ToggleButton::clicked, this, &SatDialogAdvanced::SatChanged);

  // AlphaCut
  alphacut = new ToggleButton(this, tr("Alpha cut"));
  connect(alphacut, &ToggleButton::toggled, this, &SatDialogAdvanced::greyAlphaCut);
  connect(alphacut, &ToggleButton::clicked, this, &SatDialogAdvanced::SatChanged);

  alphacutlcd = LCDNumber(4, this);

  salphacut = Slider(m_alphacut.minValue, m_alphacut.maxValue, 1, m_alphacut.value, Qt::Horizontal, this);
  connect(salphacut, SIGNAL(valueChanged(int)), SLOT(alphacutDisplay(int)));
  connect(salphacut, SIGNAL(valueChanged(int)), SIGNAL(SatChanged()));

  // Alpha
  alpha = new ToggleButton(this, tr("Alpha"));
  connect(alpha, &ToggleButton::toggled, this, &SatDialogAdvanced::greyAlpha);
  connect(alpha, &ToggleButton::clicked, this, &SatDialogAdvanced::SatChanged);

  m_alphanr = 1.0;

  alphalcd = LCDNumber(4, this);

  salpha = Slider(m_alpha.minValue, m_alpha.maxValue, 1, m_alpha.value, Qt::Horizontal, this);

  connect(salpha, SIGNAL(valueChanged(int)), SLOT(alphaDisplay(int)));
  connect(salpha, SIGNAL(valueChanged(int)), SIGNAL(SatChanged()));

  legendButton = new ToggleButton(this, tr("Table"));
  connect(legendButton, &ToggleButton::clicked, this, &SatDialogAdvanced::SatChanged);

  colourcut = new ToggleButton(this, tr("Colour cut"));
  connect(colourcut, &ToggleButton::clicked, this, &SatDialogAdvanced::SatChanged);
  connect(colourcut, &ToggleButton::toggled, this, &SatDialogAdvanced::colourcutClicked);
  colourcut->setChecked(false);

  standard=NormalPushButton( tr("Standard"), this);
  connect(standard, &QPushButton::clicked, this, &SatDialogAdvanced::setStandard);
  connect(standard, &QPushButton::clicked, this, &SatDialogAdvanced::SatChanged);

  colourList = new QListWidget( this );
  colourList->setSelectionMode(QAbstractItemView::MultiSelection);
  connect(colourList, SIGNAL(itemClicked(QListWidgetItem*)), SIGNAL(SatChanged()));
  connect(colourList, SIGNAL(itemSelectionChanged()), SLOT(colourcutOn()));

  QGridLayout*sliderlayout = new QGridLayout( this );
  sliderlayout->addWidget(cutCheckBox, 0, 0, 1, 3);
  sliderlayout->addWidget(cut, 1, 0);
  sliderlayout->addWidget(scut, 1, 1);
  sliderlayout->addWidget(cutlcd, 1, 2);
  sliderlayout->addWidget(alphacut, 2, 0);
  sliderlayout->addWidget(salphacut, 2, 1);
  sliderlayout->addWidget(alphacutlcd, 2, 2);
  sliderlayout->addWidget(alpha, 3, 0);
  sliderlayout->addWidget(salpha, 3, 1);
  sliderlayout->addWidget(alphalcd, 3, 2);
  sliderlayout->addWidget(legendButton, 4, 0);
  sliderlayout->addWidget(colourcut, 4, 1);
  sliderlayout->addWidget(standard, 4, 2);
  sliderlayout->addWidget(colourList, 6, 0, 1, 3);

  setOff();
  greyOptions();
}

void SatDialogAdvanced::cutCheckBoxSlot(bool on)
{
  greyCut(!on);
  cut->setEnabled(!on);
}

void SatDialogAdvanced::greyCut(bool on)
{
  scut->setEnabled(on);
  cutlcd->setEnabled(on);
  if (on) {
    cutlcd->display(m_cutnr);
    cutCheckBox->setChecked(false);
  } else {
    cutlcd->display("OFF");
  }
}

void SatDialogAdvanced::greyAlphaCut(bool on)
{
  salphacut->setEnabled(on);
  alphacutlcd->setEnabled(on);
  if (on)
    alphacutlcd->display(m_alphacutnr);
  else
    alphacutlcd->display("OFF");
}

void SatDialogAdvanced::greyAlpha(bool on)
{
  salpha->setEnabled(on);
  alphalcd->setEnabled(on);
  if (on)
    alphalcd->display(m_alphanr);
  else
    alphalcd->display("OFF");
}

void SatDialogAdvanced::cutDisplay(int number)
{
  m_cutnr = number * m_cut.scale;
  cutlcd->display(m_cutnr);
}

void SatDialogAdvanced::alphacutDisplay(int number)
{
  m_alphacutnr = number * m_alphacut.scale;
  alphacutlcd->display(m_alphacutnr);
}

void SatDialogAdvanced::alphaDisplay(int number)
{
  m_alphanr = number * m_alpha.scale;
  alphalcd->display(m_alphanr);
}

void SatDialogAdvanced::setStandard()
{
  blockSignals(true);
  //set standard dialog options for palette or rgb files
  if (palette){
    cut->setChecked(false);
    greyCut(false);
    legendButton->setChecked(true);
    scut->setValue(m_cut.minValue);
  } else {
    cutCheckBox->setChecked(false);
    cut->setChecked(true);
    greyCut(true);
    legendButton->setChecked(false);
    scut->setValue(m_cut.value);
  }
  salphacut->setValue(m_alphacut.value);
  alphacut->setChecked(false);
  greyAlphaCut(false);
  salpha->setValue(m_alpha.value);
  alpha->setChecked(false);
  greyAlpha(false);

  colourcut->setChecked(false);
  colourList->clearSelection();
  blockSignals(false);
}

void SatDialogAdvanced::setOff()
{
  //turn off all options
  blockSignals(true);
  palette = false;
  cutCheckBox->setChecked(false);
  scut->setValue(m_cut.minValue);
  cut->setChecked(false);
  greyCut(false);
  salphacut->setValue(m_alphacut.value);
  alphacut->setChecked(false);
  greyAlphaCut(false);
  salpha->setValue(m_alpha.value);
  alpha->setChecked(false);
  greyAlpha(false);
  legendButton->setChecked(false);
  colourcut->setChecked(false);
  colourList->clear();
  blockSignals(false);
}

void SatDialogAdvanced::colourcutOn()
{
  colourcut->setChecked(true);
}

void SatDialogAdvanced::colourcutClicked(bool on)
{
  if (on && !colourList->count())
    Q_EMIT getSatColours();
}

void SatDialogAdvanced::applyToCommand(SatPlotCommand_p cmd)
{
  if (!palette) {
    if (cutCheckBox->isChecked())
      cmd->cut = -0.5;
    else if (cut->isChecked())
      cmd->cut = m_cutnr;
    else
      cmd->cut = -1;

    cmd->alphacut = (alphacut->isChecked() ? m_alphacutnr : 0);
  }

  cmd->alpha = (alpha->isChecked() ? m_alphanr : 1);

  if (palette){
    cmd->classtable = legendButton->isChecked();

    cmd->coloursToHideInLegend.clear();
    if (colourcut->isChecked()) {
      const int n = colourList->count();
      for (int i = 0; i < n; i++) {
        const QListWidgetItem* item = colourList->item(i);
        if (item && item->isSelected()) {
          const int c = i;
          bool v_ok = false;
          int v = item->text().toInt(&v_ok);
          if (!v_ok)
            v = 0;
          cmd->coloursToHideInLegend[c] = v;
        }
      }
    }
  } else {
    cmd->classtable = false;
    cmd->coloursToHideInLegend.clear();
  }
}

void SatDialogAdvanced::setPictures(const std::string& str)
{
  picturestring = str;
}

void SatDialogAdvanced::greyOptions()
{
  if (picturestring.empty()) {
    cutCheckBox->setEnabled(false);
    cut->setEnabled(false);
    greyCut(false);
    alphacut->setEnabled(false);
    alpha->setEnabled(false);
    legendButton->setEnabled(false);
    standard->setEnabled(false);
    colourList->clear();
  } else if (palette) {
    cutCheckBox->setEnabled(false);
    cut->setEnabled(false);
    alphacut->setEnabled(false);
    alpha->setEnabled(true);
    legendButton->setEnabled(true);
    standard->setEnabled(true);
  } else {
    cutCheckBox->setEnabled(true);
    cut->setEnabled(!cutCheckBox->isChecked());
    alphacut->setEnabled(true);
    alpha->setEnabled(true);
    legendButton->setEnabled(false);
    standard->setEnabled(true);
  }
}

void SatDialogAdvanced::setColours(const std::vector<Colour>& colours)
{
  colourList->clear();
  palette = !colours.empty();
  if (palette) {
    QPixmap pmap(20, 20);
    for (const Colour& c : colours) {
      pmap.fill(vcross::util::QC(c));
      colourList->addItem(new QListWidgetItem(QIcon(pmap), QString()));
    }
  } else {
    colourcut->setChecked(false);
  }
}

void SatDialogAdvanced::setFromCommand(SatPlotCommand_cp cmd)
{
  setStandard();
  blockSignals(true);

  m_cutnr = cmd->cut;
  if (m_cutnr < 0) {
    if (m_cutnr == -0.5) {
      cutCheckBox->setChecked(true);
      cutCheckBoxSlot(true);
    } else {
      cut->setChecked(false);
      greyCut(false);
    }
  } else {
    int cutvalue = int(m_cutnr / m_cut.scale + m_cut.scale / 2);
    scut->setValue(cutvalue);
    cut->setChecked(true);
    greyCut(true);
  }

  if (!palette) {
    m_alphacutnr = cmd->alphacut;
    if (m_alphacutnr != 0) {
      int m_alphacutvalue = int(m_alphacutnr / m_alphacut.scale + m_alphacut.scale / 2);
      salphacut->setValue(m_alphacutvalue);
      alphacut->setChecked(true);
      greyAlphaCut(true);
    } else {
      alphacut->setChecked(false);
      greyAlphaCut(false);
    }
  }

  m_alphanr = cmd->alpha;
  if (m_alphanr != 1) {
    int m_alphavalue = int(m_alphanr / m_alpha.scale + m_alpha.scale / 2);
    salpha->setValue(m_alphavalue);
    alpha->setChecked(true);
    greyAlpha(true);
  } else {
    alpha->setChecked(false);
    greyAlpha(false);
  }

  legendButton->setChecked(cmd->classtable);

  if (palette) {
    colourcut->setChecked(true);
    // set selected colours
    for (const auto& cv : cmd->coloursToHideInLegend) {
      int icol = cv.first;
      if (icol < colourList->count())
        colourList->item(icol)->setSelected(true);
      if (cv.second != 0)
        colourList->item(icol)->setText(QString::number(cv.second));
    }
  }

  blockSignals(false);
}

void SatDialogAdvanced::blockSignals(bool b)
{
  cutCheckBox->blockSignals(b);
  cut->blockSignals(b);
  alphacut->blockSignals(b);
  alpha->blockSignals(b);
  scut->blockSignals(b);
  salphacut->blockSignals(b);
  salpha->blockSignals(b);
  legendButton->blockSignals(b);
  colourcut->blockSignals(b);
  standard->blockSignals(b);
  colourList->blockSignals(b);
  if (!b) {
    //after signals have been turned off, make sure buttons and LCD displays
    // are correct
    cutCheckBox->setChecked(cutCheckBox->isChecked());
    cut->Toggled(cut->isChecked());
    alphacut->Toggled(alphacut->isChecked());
    alpha->Toggled(alpha->isChecked());
    legendButton->Toggled(legendButton->isChecked());
    colourcut->Toggled(colourcut->isChecked());
    cutDisplay(scut->value());
    alphacutDisplay(salphacut->value());
    alphaDisplay(salpha->value());
  }
}
