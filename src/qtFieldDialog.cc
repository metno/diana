/*
 Diana - A Free Meteorological Visualisation Tool

 $Id$

 Copyright (C) 2006 met.no

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
//#define DEBUGREDRAW

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <QApplication>
#include <QComboBox>
#include <QSlider>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <qpainter.h>
#include <QPushButton>
#include <qsplitter.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <QRadioButton>
#include <qtooltip.h>
#include <QPixmap>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>

#include "qtFieldDialog.h"
#include "qtUtility.h"
#include "qtToggleButton.h"
#include "diController.h"
#include <diField/diRectangle.h>
#include <diPlotOptions.h>
#include <diField/FieldSpecTranslation.h>

#include <iostream>
#include <math.h>

#include "up20x20.xpm"
#include "down20x20.xpm"
#include "up12x12.xpm"
#include "down12x12.xpm"
//#include "minus12x12.xpm"

//#define DEBUGPRINT


FieldDialog::FieldDialog(QWidget* parent, Controller* lctrl) :
QDialog(parent)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::FieldDialog called"<<endl;
#endif

  m_ctrl = lctrl;

  m_modelgroup = m_ctrl->initFieldDialog();

  setWindowTitle(tr("Fields"));

  useArchive = false;
  profetEnabled = false;

  numEditFields = 0;
  currentFieldOptsInEdit = false;
  historyPos = -1;

  editName = tr("EDIT").toStdString();

  // translations of fieldGroup names (Qt linguist translations)
  fgTranslations["EPS Probability"] = tr("EPS Probability").toStdString();
  fgTranslations["EPS Clusters"] = tr("EPS Clusters").toStdString();
  fgTranslations["EPS Members"] = tr("EPS Members").toStdString();

  fgTranslations["Analysis"] = tr("Analysis").toStdString();
  fgTranslations["Constant fields"] = tr("Constant fields").toStdString();

  fgTranslations["Surface etc"] = tr("Surface etc.").toStdString();
  fgTranslations["Pressure Levels"] = tr("Pressure Levels").toStdString();
  fgTranslations["FlightLevels"] = tr("FlightLevels").toStdString();
  fgTranslations["Model Levels"] = tr("Model Levels").toStdString();
  fgTranslations["Isentropic Levels"] = tr("Isentropic Levels").toStdString();
  fgTranslations["Temperature Levels"] = tr("Temperature Levels").toStdString();
  fgTranslations["PV Levels"] = tr("PV Levels").toStdString();
  fgTranslations["Ocean Depths"] = tr("Ocean Depths").toStdString();
  fgTranslations["Ocean Model Levels"] = tr("Ocean Model Levels").toStdString();
  //fgTranslations[""]= tr("");

  int i;

  // get all field plot options from setup file
  vector<miutil::miString> fieldNames;
  m_ctrl->getAllFieldNames(fieldNames, fieldPrefixes, fieldSuffixes);
  PlotOptions::getAllFieldOptions(fieldNames, setupFieldOptions);

  //#################################################################
  //  map<miutil::miString,miutil::miString>::iterator pfopt, pfend= setupFieldOptions.end();
  //  for (pfopt=setupFieldOptions.begin(); pfopt!=pfend; pfopt++)
  //    cerr << pfopt->first << "   " << pfopt->second << endl;
  //#################################################################


  // Colours
  csInfo = ColourShading::getColourShadingInfo();
  patternInfo = Pattern::getAllPatternInfo();
  map<miutil::miString, miutil::miString> enabledOptions = PlotOptions::getEnabledOptions();
  plottypes_dim = PlotOptions::getPlotTypes();
  if ( plottypes_dim.size() > 1 ) {
    plottypes = plottypes_dim[1];
  }


  if ( plottypes_dim.size() > 1 ) {
    for ( size_t i = 0; i< plottypes_dim[0].size(); i++ ) {
      if(enabledOptions.count(plottypes_dim[0][i])) {
        enableMap[plottypes_dim[0][i]].contourWidgets =  (enabledOptions[plottypes_dim[0][i]].contains("contour") );
        enableMap[plottypes_dim[0][i]].extremeWidgets =  (enabledOptions[plottypes_dim[0][i]].contains("extreme") );
        enableMap[plottypes_dim[0][i]].shadingWidgets =  (enabledOptions[plottypes_dim[0][i]].contains("shading") );
        enableMap[plottypes_dim[0][i]].lineWidgets =  (enabledOptions[plottypes_dim[0][i]].contains("line") );
        enableMap[plottypes_dim[0][i]].fontWidgets =  (enabledOptions[plottypes_dim[0][i]].contains("font") );
        enableMap[plottypes_dim[0][i]].densityWidgets =  (enabledOptions[plottypes_dim[0][i]].contains("density") );
        enableMap[plottypes_dim[0][i]].unitWidgets =  (enabledOptions[plottypes_dim[0][i]].contains("unit") );
      }
    }
  }

  // linetypes
  linetypes = Linetype::getLinetypeNames();

  // density (of arrows etc, 0=automatic)
  QString qs;
  densityStringList << "Auto";
  for (i = 1; i < 10; i++) {
    densityStringList << qs.setNum(i);
  }
  for (i = 10; i < 60; i += 10) {
    densityStringList << qs.setNum(i);
  }
  densityStringList << qs.setNum(100);
  densityStringList <<  "auto(0.5)";
  densityStringList <<  "auto(0.6)";
  densityStringList <<  "auto(0.7)";
  densityStringList <<  "auto(0.8)";
  densityStringList <<  "auto(0.9)";
  densityStringList <<  "auto(2)";
  densityStringList <<  "auto(3)";
  densityStringList <<  "auto(4)";

  //----------------------------------------------------------------
  cp = new CommandParser();

  // add level options to the cp's keyDataBase
  cp->addKey("model", "", 1, CommandParser::cmdString);
  cp->addKey("plot", "", 1, CommandParser::cmdString);
  cp->addKey("parameter", "", 1, CommandParser::cmdString);
  cp->addKey("vlevel", "", 1, CommandParser::cmdString);
  cp->addKey("elevel", "", 1, CommandParser::cmdString);
  cp->addKey("vcoord", "", 1, CommandParser::cmdString);
  cp->addKey("ecoor", "", 1, CommandParser::cmdString);
  cp->addKey("grid", "", 1, CommandParser::cmdString);
  cp->addKey("unit", "", 1, CommandParser::cmdString);
  cp->addKey("reftime", "", 1, CommandParser::cmdString);
  cp->addKey("refhour", "", 1, CommandParser::cmdInt);
  cp->addKey("refoffset", "", 1, CommandParser::cmdInt);

  // old syntax
  cp->addKey("level", "", 1, CommandParser::cmdString);
  cp->addKey("idnum", "", 1, CommandParser::cmdString);

  cp->addKey("hour.offset", "", 1, CommandParser::cmdInt);
  cp->addKey("hour.diff", "", 1, CommandParser::cmdInt);

  cp->addKey("MINUS", "", 1, CommandParser::cmdNoValue);

  // add more plot options to the cp's keyDataBase
  cp->addKey("plottype", "", 0, CommandParser::cmdString);
  cp->addKey("colour", "", 0, CommandParser::cmdString);
  cp->addKey("colours", "", 0, CommandParser::cmdString);
  cp->addKey("linewidth", "", 0, CommandParser::cmdInt);
  cp->addKey("linetype", "", 0, CommandParser::cmdString);
  cp->addKey("line.interval", "", 0, CommandParser::cmdFloat);
  cp->addKey("line.values", "", 0, CommandParser::cmdString);
  cp->addKey("logline.values", "", 0, CommandParser::cmdString);
  cp->addKey("density", "", 0, CommandParser::cmdInt);
  cp->addKey("vector.unit", "", 0, CommandParser::cmdFloat);
  cp->addKey("extreme.type", "", 0, CommandParser::cmdString);
  cp->addKey("extreme.size", "", 0, CommandParser::cmdFloat);
  cp->addKey("extreme.radius", "", 0, CommandParser::cmdFloat);
  cp->addKey("line.smooth", "", 0, CommandParser::cmdInt);
  cp->addKey("field.smooth", "", 0, CommandParser::cmdInt);
  cp->addKey("frame", "", 0, CommandParser::cmdInt);
  cp->addKey("zero.line", "", 0, CommandParser::cmdInt);
  cp->addKey("value.label", "", 0, CommandParser::cmdInt);
  cp->addKey("label.size", "", 0, CommandParser::cmdFloat);
  cp->addKey("grid.value", "", 0, CommandParser::cmdInt);
  cp->addKey("grid.lines", "", 0, CommandParser::cmdInt);
  cp->addKey("grid.lines.max", "", 0, CommandParser::cmdInt);
  cp->addKey("base", "", 0, CommandParser::cmdFloat);
  cp->addKey("base_2", "", 0, CommandParser::cmdFloat);
  cp->addKey("undef.masking", "", 0, CommandParser::cmdInt);
  cp->addKey("undef.colour", "", 0, CommandParser::cmdString);
  cp->addKey("undef.linewidth", "", 0, CommandParser::cmdInt);
  cp->addKey("undef.linetype", "", 0, CommandParser::cmdString);
  cp->addKey("discontinuous", "", 0, CommandParser::cmdInt);
  cp->addKey("palettecolours", "", 0, CommandParser::cmdString);
  cp->addKey("options.1", "", 0, CommandParser::cmdInt);
  cp->addKey("options.2", "", 0, CommandParser::cmdInt);
  cp->addKey("minvalue", "", 0, CommandParser::cmdFloat);
  cp->addKey("maxvalue", "", 0, CommandParser::cmdFloat);
  cp->addKey("minvalue_2", "", 0, CommandParser::cmdFloat);
  cp->addKey("maxvalue_2", "", 0, CommandParser::cmdFloat);
  cp->addKey("colour_2", "", 0, CommandParser::cmdString);
  cp->addKey("linewidth_2", "", 0, CommandParser::cmdInt);
  cp->addKey("linetype_2", "", 0, CommandParser::cmdString);
  cp->addKey("line.interval_2", "", 0, CommandParser::cmdFloat);
  cp->addKey("table", "", 0, CommandParser::cmdInt);
  cp->addKey("patterns", "", 0, CommandParser::cmdString);
  cp->addKey("patterncolour", "", 0, CommandParser::cmdString);
  cp->addKey("repeat", "", 0, CommandParser::cmdInt);
  cp->addKey("alpha", "", 0, CommandParser::cmdInt);
  cp->addKey("overlay", "", 0, CommandParser::cmdInt);

  // yet only from "external" (QuickMenu) commands
  cp->addKey("forecast.hour", "", 2, CommandParser::cmdInt);
  cp->addKey("forecast.hour.loop", "", 2, CommandParser::cmdInt);

  cp->addKey("allTimeSteps", "", 3, CommandParser::cmdString);
  //----------------------------------------------------------------

  // modelGRbox
  QLabel *modelGRlabel = TitleLabel(tr("Model group"), this);
  modelGRbox = new QComboBox(this);
  connect( modelGRbox, SIGNAL( activated( int ) ),
      SLOT( modelGRboxActivated( int ) ) );

  //modelbox
  QLabel *modellabel = TitleLabel(tr("Models"), this);
  modelbox = new QListWidget(this);
  connect( modelbox, SIGNAL( itemClicked( QListWidgetItem * ) ),
      SLOT( modelboxClicked( QListWidgetItem * ) ) );

  // refTime
  QLabel *refTimelabel = TitleLabel(tr("Reference time"), this);
  refTimeComboBox = new QComboBox(this);
  connect( refTimeComboBox, SIGNAL( activated( int ) ),
      SLOT( updateFieldGroups(  ) ) );

  // fieldGRbox
  QLabel *fieldGRlabel = TitleLabel(tr("Field group"), this);
  fieldGRbox = new QComboBox(this);
  connect( fieldGRbox, SIGNAL( activated( int ) ),
      SLOT( fieldGRboxActivated( int ) ) );

  //fieldGroupCheckBox
  fieldGroupCheckBox = new QCheckBox(tr("Predefined plots"));
  fieldGroupCheckBox->setChecked(true);
  connect( fieldGroupCheckBox, SIGNAL( toggled(bool) ),
      SLOT( updateFieldGroups(  ) ) );

  // fieldbox
  QLabel *fieldlabel = TitleLabel(tr("Fields"), this);
  fieldbox = new QListWidget(this);
  fieldbox->setSelectionMode(QAbstractItemView::MultiSelection);
  connect( fieldbox, SIGNAL( itemClicked(QListWidgetItem*) ),
      SLOT( fieldboxChanged(QListWidgetItem*) ) );

  // selectedFieldbox
  QLabel *selectedFieldlabel = TitleLabel(tr("Selected fields"), this);
  selectedFieldbox = new QListWidget(this);
  selectedFieldbox->setSelectionMode(QAbstractItemView::SingleSelection);

  connect( selectedFieldbox, SIGNAL( itemClicked( QListWidgetItem * ) ),
      SLOT( selectedFieldboxClicked( QListWidgetItem * ) ) );

  // Level: slider & label for the value
  levelLabel = new QLabel("1000hPa", this);
  levelLabel->setMinimumSize(levelLabel->sizeHint().width() + 10,
      levelLabel->sizeHint().height() + 10);
  levelLabel->setMaximumSize(levelLabel->sizeHint().width() + 10,
      levelLabel->sizeHint().height() + 10);
  levelLabel->setText(" ");

  levelLabel->setFrameStyle(QFrame::Box | QFrame::Plain);
  levelLabel->setLineWidth(2);
  levelLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  levelSlider = new QSlider(Qt::Vertical, this);
  levelSlider->setInvertedAppearance(true);
  levelSlider->setMinimum(0);
  levelSlider->setMaximum(1);
  levelSlider->setPageStep(1);
  levelSlider->setValue(0);

  connect( levelSlider, SIGNAL( valueChanged( int )),
      SLOT( levelChanged( int)));
  connect(levelSlider, SIGNAL(sliderPressed()), SLOT(levelPressed()));
  connect(levelSlider, SIGNAL(sliderReleased()), SLOT(updateLevel()));

  levelInMotion = false;

  // sliderlabel
  QLabel *levelsliderlabel = new QLabel(tr("Level"), this);

  // Idnum: slider & label for the value
  idnumLabel = new QLabel("EPS.Total", this);
  idnumLabel->setMinimumSize(idnumLabel->sizeHint().width() + 10,
      idnumLabel->sizeHint().height() + 10);
  idnumLabel->setMaximumSize(idnumLabel->sizeHint().width() + 10,
      idnumLabel->sizeHint().height() + 10);
  idnumLabel->setText(" ");

  idnumLabel->setFrameStyle(QFrame::Box | QFrame::Plain);
  idnumLabel->setLineWidth(2);
  idnumLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  idnumSlider = new QSlider(Qt::Vertical, this);
  idnumSlider->setMinimum(0);
  idnumSlider->setMaximum(1);
  idnumSlider->setPageStep(1);
  idnumSlider->setValue(0);

  connect( idnumSlider, SIGNAL( valueChanged( int )),
      SLOT( idnumChanged( int)));
  connect(idnumSlider, SIGNAL(sliderPressed()), SLOT(idnumPressed()));
  connect(idnumSlider, SIGNAL(sliderReleased()), SLOT(updateIdnum()));

  idnumInMotion = false;

  // sliderlabel
  QLabel *idnumsliderlabel = new QLabel(tr("Type"), this);

  QLabel* unitlabel = new QLabel(tr("Unit"), this);
  unitLineEdit = new QLineEdit(this);
  connect( unitLineEdit, SIGNAL( returnPressed() ),
      SLOT( unitEditingFinished() ) );

  // copyField
  copyField = NormalPushButton(tr("Copy"), this);
  connect(copyField, SIGNAL(clicked()), SLOT(copySelectedField()));

  // deleteSelected
  deleteButton = NormalPushButton(tr("Delete"), this);
  connect(deleteButton, SIGNAL(clicked()), SLOT(deleteSelected()));

  // deleteAll
  deleteAll = NormalPushButton(tr("Delete all"), this);
  connect(deleteAll, SIGNAL(clicked()), SLOT(deleteAllSelected()));

  // changeModelButton
  changeModelButton = NormalPushButton(tr("Model"), this);
  connect(changeModelButton, SIGNAL(clicked()), SLOT(changeModel()));

  int width = changeModelButton->sizeHint().width()/3;
  int height = changeModelButton->sizeHint().height();;

  // historyBack
  historyBackButton = new QPushButton(QPixmap(up20x20_xpm), "", this);
  historyBackButton->setMaximumSize(width,height);
  historyBackButton->hide();
  connect(historyBackButton, SIGNAL(clicked()), SLOT(historyBack()));

  // historyForward
  historyForwardButton = new QPushButton(QPixmap(down20x20_xpm), "", this);
  historyForwardButton->setMaximumSize(width,height);
  historyForwardButton->hide();
  connect(historyForwardButton, SIGNAL(clicked()), SLOT(historyForward()));

  // historyOk
  historyOkButton = NormalPushButton("OK", this);
  historyOkButton->setMaximumSize(width*2,height);
  historyOkButton->setEnabled( false );
  historyOkButton->hide();
  connect(historyOkButton, SIGNAL(clicked()), SLOT(historyOk()));

  // upField
  upFieldButton = new QPushButton(QPixmap(up12x12_xpm), "", this);
  upFieldButton->setMaximumSize(width,height);
  connect(upFieldButton, SIGNAL(clicked()), SLOT(upField()));

  // downField
  downFieldButton = new QPushButton(QPixmap(down12x12_xpm), "", this);
  downFieldButton->setMaximumSize(width,height);
  connect(downFieldButton, SIGNAL(clicked()), SLOT(downField()));

  // resetOptions
  resetOptionsButton = NormalPushButton(tr("Default"), this);
  connect(resetOptionsButton, SIGNAL(clicked()), SLOT(resetOptions()));

  // minus
  minusButton = new ToggleButton(this, "Minus");
  connect( minusButton, SIGNAL(toggled(bool)), SLOT(minusField(bool)));

  // plottype
  QLabel* plottypelabel = new QLabel(tr("Plot type"), this);
  plottypeComboBox = ComboBox(this, plottypes);
  connect( plottypeComboBox, SIGNAL( activated(int) ),
      SLOT( plottypeComboBoxActivated(int) ) );


  // colorCbox
  QLabel* colorlabel = new QLabel(tr("Line colour"), this);
  colorCbox = ColourBox(this, false, 0, tr("off").toStdString(),true);
  colorCbox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
  connect( colorCbox, SIGNAL( activated(int) ),
      SLOT( colorCboxActivated(int) ) );

  // linewidthcbox
  QLabel* linewidthlabel = new QLabel(tr("Line width"), this);
  lineWidthCbox = LinewidthBox(this, false);
  connect( lineWidthCbox, SIGNAL( activated(int) ),
      SLOT( lineWidthCboxActivated(int) ) );

  // linetypecbox
  QLabel* linetypelabel = new QLabel(tr("Line type"), this);
  lineTypeCbox = LinetypeBox(this, false);
  connect( lineTypeCbox, SIGNAL( activated(int) ),
      SLOT( lineTypeCboxActivated(int) ) );

  // lineinterval
  QLabel* lineintervallabel = new QLabel(tr("Line interval"), this);
  lineintervalCbox = new QComboBox(this);
  connect( lineintervalCbox, SIGNAL( activated(int) ),
      SLOT( lineintervalCboxActivated(int) ) );

  // density
  QLabel* densitylabel = new QLabel(tr("Density"), this);
  densityCbox = new QComboBox(this);
  densityCbox->addItems(densityStringList);
  connect( densityCbox, SIGNAL( activated(int) ),
      SLOT( densityCboxActivated(int) ) );

  // vectorunit
  QLabel* vectorunitlabel = new QLabel(tr("Unit"), this);
  vectorunitCbox = new QComboBox(this);
  connect( vectorunitCbox, SIGNAL( activated(int) ),
      SLOT( vectorunitCboxActivated(int) ) );

  // help
  fieldhelp = NormalPushButton(tr("Help"), this);
  connect(fieldhelp, SIGNAL(clicked()), SLOT(helpClicked()));

  // allTimeStep
  allTimeStepButton
  = new ToggleButton(this, tr("All time steps").toStdString());
  allTimeStepButton->setCheckable(true);
  allTimeStepButton->setChecked(false);connect( allTimeStepButton, SIGNAL(toggled(bool)),
      SLOT(allTimeStepToggled(bool)));

  // advanced
  miutil::miString more_str[2] =
  { (tr("<<Less").toStdString()), (tr("More>>").toStdString()) };
  advanced = new ToggleButton(this, more_str);
  advanced->setChecked(false);connect( advanced, SIGNAL(toggled(bool)), SLOT(advancedToggled(bool)));

  // hide
  fieldhide = NormalPushButton(tr("Hide"), this);
  connect(fieldhide, SIGNAL(clicked()), SLOT(hideClicked()));

  // applyhide
  fieldapplyhide = NormalPushButton(tr("Apply+Hide"), this);
  connect(fieldapplyhide, SIGNAL(clicked()), SLOT(applyhideClicked()));

  // apply
  fieldapply = NormalPushButton(tr("Apply"), this);
  fieldapply->setDefault( true );
  connect(fieldapply, SIGNAL(clicked()), SLOT(applyClicked()));

  // layout
  QHBoxLayout* grouplayout = new QHBoxLayout();
  grouplayout->addWidget(fieldGRbox);
  grouplayout->addWidget(fieldGroupCheckBox);

  QVBoxLayout* v1layout = new QVBoxLayout();
  v1layout->setSpacing(1);
  v1layout->addWidget(modelGRlabel);
  v1layout->addWidget(modelGRbox);
  v1layout->addWidget(modellabel);
  v1layout->addWidget(modelbox, 2);
  v1layout->addWidget(refTimelabel);
  v1layout->addWidget(refTimeComboBox);
  v1layout->addWidget(fieldGRlabel);
  v1layout->addLayout(grouplayout);
  v1layout->addWidget(fieldlabel);
  v1layout->addWidget(fieldbox, 4);
  v1layout->addWidget(selectedFieldlabel);
  v1layout->addWidget(selectedFieldbox, 2);

//  QVBoxLayout* h2layout = new QVBoxLayout();
//  h2layout->addStretch(1);

  QHBoxLayout* v1h4layout = new QHBoxLayout();
  v1h4layout->addWidget(upFieldButton);
  v1h4layout->addWidget(deleteButton);
  v1h4layout->addWidget(minusButton);
  v1h4layout->addWidget(copyField);

  QHBoxLayout* vxh4layout = new QHBoxLayout();
  vxh4layout->addWidget(downFieldButton);
  vxh4layout->addWidget(deleteAll);
  vxh4layout->addWidget(resetOptionsButton);
  vxh4layout->addWidget(changeModelButton);

  QVBoxLayout* v3layout = new QVBoxLayout();
  v3layout->addLayout(v1h4layout);
  v3layout->addLayout(vxh4layout);

//  QHBoxLayout* v1h5layout = new QHBoxLayout();
//  v1h5layout->addWidget(historyBackButton);
//  v1h5layout->addWidget(historyForwardButton);
//
//  QVBoxLayout* v4layout = new QVBoxLayout();
//  v4layout->addLayout(v1h5layout);
//  v4layout->addWidget(historyOkButton, 1);

  QHBoxLayout* h3layout = new QHBoxLayout();
  h3layout->addLayout(v3layout);
//  h3layout->addLayout(v4layout);

  QGridLayout* optlayout = new QGridLayout();
  optlayout->addWidget(unitlabel, 0, 0);
  optlayout->addWidget(unitLineEdit, 0, 1);
  optlayout->addWidget(plottypelabel, 1, 0);
  optlayout->addWidget(plottypeComboBox, 1, 1);
  optlayout->addWidget(colorlabel, 2, 0);
  optlayout->addWidget(colorCbox, 2, 1);
  optlayout->addWidget(linewidthlabel, 3, 0);
  optlayout->addWidget(lineWidthCbox, 3, 1);
  optlayout->addWidget(linetypelabel, 4, 0);
  optlayout->addWidget(lineTypeCbox, 4, 1);
  optlayout->addWidget(lineintervallabel, 5, 0);
  optlayout->addWidget(lineintervalCbox, 5, 1);
  optlayout->addWidget(densitylabel, 6, 0);
  optlayout->addWidget(densityCbox, 6, 1);
  optlayout->addWidget(vectorunitlabel, 7, 0);
  optlayout->addWidget(vectorunitCbox, 7, 1);

  QHBoxLayout* levelsliderlayout = new QHBoxLayout();
  levelsliderlayout->setAlignment(Qt::AlignHCenter);
  levelsliderlayout->addWidget(levelSlider);

  QHBoxLayout* levelsliderlabellayout = new QHBoxLayout();
  levelsliderlabellayout->setAlignment(Qt::AlignHCenter);
  levelsliderlabellayout->addWidget(levelsliderlabel);

  QVBoxLayout* levellayout = new QVBoxLayout();
  levellayout->addWidget(levelLabel);
  levellayout->addLayout(levelsliderlayout);
  levellayout->addLayout(levelsliderlabellayout);

  QHBoxLayout* idnumsliderlayout = new QHBoxLayout();
  idnumsliderlayout->setAlignment(Qt::AlignHCenter);
  idnumsliderlayout->addWidget(idnumSlider);

  QHBoxLayout* idnumsliderlabellayout = new QHBoxLayout();
  idnumsliderlabellayout->setAlignment(Qt::AlignHCenter);
  idnumsliderlabellayout->addWidget(idnumsliderlabel);

  QVBoxLayout* idnumlayout = new QVBoxLayout();
  idnumlayout->addWidget(idnumLabel);
  idnumlayout->addLayout(idnumsliderlayout);
  idnumlayout->addLayout(idnumsliderlabellayout);

  QHBoxLayout* h4layout = new QHBoxLayout();
  //h4layout->addLayout(h2layout);
  h4layout->addLayout(optlayout);
  h4layout->addLayout(levellayout);
  h4layout->addLayout(idnumlayout);

  QHBoxLayout* h5layout = new QHBoxLayout();
  h5layout->addWidget(fieldhelp);
  h5layout->addWidget(allTimeStepButton);
  h5layout->addWidget(advanced);

  QHBoxLayout* h6layout = new QHBoxLayout();
  h6layout->addWidget(fieldhide);
  h6layout->addWidget(fieldapplyhide);
  h6layout->addWidget(fieldapply);

  QVBoxLayout* v6layout = new QVBoxLayout();
  v6layout->addLayout(h5layout);
  v6layout->addLayout(h6layout);

  // vlayout
  QVBoxLayout* vlayout = new QVBoxLayout(this);
  vlayout->addLayout(v1layout);
  vlayout->addLayout(h3layout);
  vlayout->addLayout(h4layout);
  vlayout->addLayout(v6layout);

  vlayout->activate();

  CreateAdvanced();

  this->setOrientation(Qt::Horizontal);
  this->setExtension(advFrame);
  advancedToggled(false);

  // tool tips
  toolTips();

  updateModelBoxes();
  setDefaultFieldOptions();

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::ConstructorCernel returned"<<endl;
#endif
}

void FieldDialog::toolTips()
{
  fieldGroupCheckBox->setToolTip(tr("Show predefined plots or all parameters from file"));
  upFieldButton->setToolTip(tr("move selected field up"));
  downFieldButton->setToolTip(tr("move selected field down"));
  deleteButton->setToolTip(tr("delete selected field"));
  deleteAll->setToolTip(tr("delete all selected fields"));
  copyField->setToolTip(tr("copy field"));
  resetOptionsButton->setToolTip(tr("reset plot options"));
  minusButton->setToolTip(tr("selected field minus the field above"));
  changeModelButton->setToolTip(tr("change model/termin"));
  historyBackButton->setToolTip(tr("history back"));
  historyForwardButton->setToolTip(tr("history forward"));
  historyOkButton->setToolTip(tr("use history shown"));
  allTimeStepButton->setToolTip(tr("all time steps / only common time steps"));

  gridValueCheckBox->setToolTip(tr(
      "Grid values->setToolTip( but only when a few grid points are visible"));
  valueLabelCheckBox->setToolTip(tr("numbers on the contour lines"));
  labelSizeSpinBox->setToolTip(tr("Size of numbers on the countour lines and size of values in the plot type \"value\""));
  valuePrecisionBox->setToolTip(tr("Value precision, used in the plot type \"value\""));
  gridLinesSpinBox->setToolTip(tr("Grid lines, 1=all"));
  undefColourCbox->setToolTip(tr("Undef colour"));
  undefLinewidthCbox->setToolTip(tr("Undef linewidth"));
  undefLinetypeCbox->setToolTip(tr("Undef linetype"));
  shadingSpinBox->setToolTip(tr("number of colours in the palette"));
  shadingcoldComboBox->setToolTip(tr("Palette for values below basis"));
  shadingcoldSpinBox->setToolTip(tr("number of colours in the palette"));
  patternColourBox->setToolTip(tr("Colour of pattern"));
}

void FieldDialog::advancedToggled(bool on)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::advancedToggled  on= " << on <<endl;
#endif

  this->showExtension(on);

}

void FieldDialog::CreateAdvanced()
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::CreateAdvanced" <<endl;
#endif

  advFrame = new QWidget(this);

  // Extreme (min,max): type, size and search radius
  QLabel* extremeTypeLabel = TitleLabel(tr("Min,max"), advFrame);
  extremeType.push_back("None");
  extremeType.push_back("L+H");
  extremeType.push_back("C+W");
  extremeType.push_back("Value");
  extremeTypeCbox = ComboBox(advFrame, extremeType);
  connect( extremeTypeCbox, SIGNAL( activated(int) ),
      SLOT( extremeTypeActivated(int) ) );

  QLabel* extremeSizeLabel = new QLabel(tr("Size"), advFrame);
  extremeSizeSpinBox = new QSpinBox(advFrame);
  extremeSizeSpinBox->setMinimum(5);
  extremeSizeSpinBox->setMaximum(300);
  extremeSizeSpinBox->setSingleStep(5);
  extremeSizeSpinBox->setWrapping(true);
  extremeSizeSpinBox->setSuffix("%");
  extremeSizeSpinBox->setValue(100);
  connect( extremeSizeSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( extremeSizeChanged(int) ) );

  QLabel* extremeRadiusLabel = new QLabel(tr("Radius"), advFrame);
  extremeRadiusSpinBox = new QSpinBox(advFrame);
  extremeRadiusSpinBox->setMinimum(5);
  extremeRadiusSpinBox->setMaximum(300);
  extremeRadiusSpinBox->setSingleStep(5);
  extremeRadiusSpinBox->setWrapping(true);
  extremeRadiusSpinBox->setSuffix("%");
  extremeRadiusSpinBox->setValue(100);
  connect( extremeRadiusSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( extremeRadiusChanged(int) ) );

  // line smoothing
  QLabel* lineSmoothLabel = new QLabel(tr("Smooth lines"), advFrame);
  lineSmoothSpinBox = new QSpinBox(advFrame);
  lineSmoothSpinBox->setMinimum(0);
  lineSmoothSpinBox->setMaximum(50);
  lineSmoothSpinBox->setSingleStep(2);
  lineSmoothSpinBox->setSpecialValueText(tr("Off"));
  lineSmoothSpinBox->setValue(0);
  connect( lineSmoothSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( lineSmoothChanged(int) ) );

  // field smoothing
  QLabel* fieldSmoothLabel = new QLabel(tr("Smooth fields"), advFrame);
  fieldSmoothSpinBox = new QSpinBox(advFrame);
  fieldSmoothSpinBox->setMinimum(0);
  fieldSmoothSpinBox->setMaximum(10);
  fieldSmoothSpinBox->setSingleStep(1);
  fieldSmoothSpinBox->setSpecialValueText(tr("Off"));
  fieldSmoothSpinBox->setValue(0);
  connect( fieldSmoothSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( fieldSmoothChanged(int) ) );

  labelSizeSpinBox = new QSpinBox(advFrame);
  labelSizeSpinBox->setMinimum(5);
  labelSizeSpinBox->setMaximum(300);
  labelSizeSpinBox->setSingleStep(5);
  labelSizeSpinBox->setWrapping(true);
  labelSizeSpinBox->setSuffix("%");
  labelSizeSpinBox->setValue(100);
  connect( labelSizeSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( labelSizeChanged(int) ) );

  valuePrecisionBox = new QComboBox(advFrame);
  valuePrecisionBox->addItem("0");
  valuePrecisionBox->addItem("1");
  valuePrecisionBox->addItem("2");
  valuePrecisionBox->addItem("3");
  connect( valuePrecisionBox, SIGNAL( activated(int) ),
      SLOT( valuePrecisionBoxActivated(int) ) );

  // grid values
  gridValueCheckBox = new QCheckBox(QString(tr("Grid value")), advFrame);
  gridValueCheckBox->setChecked(false);
  connect( gridValueCheckBox, SIGNAL( toggled(bool) ),
      SLOT( gridValueCheckBoxToggled(bool) ) );

  // grid lines
  QLabel* gridLinesLabel = new QLabel(tr("Grid lines"), advFrame);
  gridLinesSpinBox = new QSpinBox(advFrame);
  gridLinesSpinBox->setMinimum(0);
  gridLinesSpinBox->setMaximum(50);
  gridLinesSpinBox->setSingleStep(1);
  gridLinesSpinBox->setSpecialValueText(tr("Off"));
  gridLinesSpinBox->setValue(0);
  connect( gridLinesSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( gridLinesChanged(int) ) );

  // grid lines max
  //   QLabel* gridLinesMaxLabel= new QLabel( tr("Max grid l."), advFrame );
  //   gridLinesMaxSpinBox= new QSpinBox( 0,200,5, advFrame );
  //   gridLinesMaxSpinBox->setSpecialValueText(tr("All"));
  //   gridLinesMaxSpinBox->setValue(0);
  //   connect( gridLinesMaxSpinBox, SIGNAL( valueChanged(int) ),
  // 	   SLOT( gridLinesMaxChanged(int) ) );


  QLabel* hourOffsetLabel = new QLabel(tr("Time offset"), advFrame);
  hourOffsetSpinBox = new QSpinBox(advFrame);
  hourOffsetSpinBox->setMinimum(-72);
  hourOffsetSpinBox->setMaximum(72);
  hourOffsetSpinBox->setSingleStep(1);
  hourOffsetSpinBox->setSuffix(tr(" hour(s)"));
  hourOffsetSpinBox->setValue(0);
  connect( hourOffsetSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( hourOffsetChanged(int) ) );

  QLabel* hourDiffLabel = new QLabel(tr("Time diff."), advFrame);
  hourDiffSpinBox = new QSpinBox(advFrame);
  hourDiffSpinBox->setMinimum(0);
  hourDiffSpinBox->setMaximum(24);
  hourDiffSpinBox->setSingleStep(1);
  hourDiffSpinBox->setSuffix(tr(" hour(s)"));
  hourDiffSpinBox->setPrefix(" +/-");
  hourDiffSpinBox->setValue(0);
  connect( hourDiffSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( hourDiffChanged(int) ) );

  // Undefined masking
  QLabel* undefMaskingLabel = TitleLabel(tr("Undefined"), advFrame);
  undefMasking.push_back(tr("Unmarked").toStdString());
  undefMasking.push_back(tr("Coloured").toStdString());
  undefMasking.push_back(tr("Lines").toStdString());
  undefMaskingCbox = ComboBox(advFrame, undefMasking);
  connect( undefMaskingCbox, SIGNAL( activated(int) ),
      SLOT( undefMaskingActivated(int) ) );

  // Undefined masking colour
  undefColourCbox = ColourBox(advFrame, false, 0, "", true);
  connect( undefColourCbox, SIGNAL( activated(int) ),
      SLOT( undefColourActivated(int) ) );

  // Undefined masking linewidth
  undefLinewidthCbox = LinewidthBox(advFrame, false);
  connect( undefLinewidthCbox, SIGNAL( activated(int) ),
      SLOT( undefLinewidthActivated(int) ) );

  // Undefined masking linetype
  undefLinetypeCbox = LinetypeBox(advFrame, false);
  connect( undefLinetypeCbox, SIGNAL( activated(int) ),
      SLOT( undefLinetypeActivated(int) ) );

  // enable/disable numbers on isolines
  valueLabelCheckBox = new QCheckBox(QString(tr("Numbers")), advFrame);
  valueLabelCheckBox->setChecked(true);
  connect( valueLabelCheckBox, SIGNAL( toggled(bool) ),
      SLOT( valueLabelCheckBoxToggled(bool) ) );

  //Options
  QLabel* shadingLabel = new QLabel(tr("Palette"), advFrame);
  QLabel* shadingcoldLabel = new QLabel(tr("Palette (-)"), advFrame);
  QLabel* patternLabel = new QLabel(tr("Pattern"), advFrame);
  QLabel* alphaLabel = new QLabel(tr("Alpha"), advFrame);
  QLabel* headLabel = TitleLabel(tr("Extra contour lines"), advFrame);
  QLabel* colourLabel = new QLabel(tr("Line colour"), advFrame);
  QLabel* intervalLabel = new QLabel(tr("Line interval"), advFrame);
  QLabel* baseLabel = new QLabel(tr("Basis value"), advFrame);
  QLabel* minLabel = new QLabel(tr("Min"), advFrame);
  QLabel* maxLabel = new QLabel(tr("Max"), advFrame);
  QLabel* base2Label = new QLabel(tr("Basis value"), advFrame);
  QLabel* min2Label = new QLabel(tr("Min"), advFrame);
  QLabel* max2Label = new QLabel(tr("Max"), advFrame);
  QLabel* linewidthLabel = new QLabel(tr("Line width"), advFrame);
  QLabel* linetypeLabel = new QLabel(tr("Line type"), advFrame);
  QLabel* threeColourLabel = TitleLabel(tr("Three colours"), advFrame);

  tableCheckBox = new QCheckBox(tr("Table"), advFrame);
  connect( tableCheckBox, SIGNAL( toggled(bool) ),
      SLOT( tableCheckBoxToggled(bool) ) );

  repeatCheckBox = new QCheckBox(tr("Repeat"), advFrame);
  connect( repeatCheckBox, SIGNAL( toggled(bool) ),
      SLOT( repeatCheckBoxToggled(bool) ) );

  //3 colours
  //  threeColoursCheckBox = new QCheckBox(tr("Three colours"), advFrame);

  for (int i = 0; i < 3; i++) {
    threeColourBox.push_back(ColourBox(advFrame, true, 0,
        tr("Off").toStdString(),true));
    connect  ( threeColourBox[i], SIGNAL( activated(int) ),
        SLOT( threeColoursChanged() ) );
  }

  //shading
  shadingComboBox=
      PaletteBox( advFrame,csInfo,false,0,tr("Off").toStdString(),true );
  shadingComboBox->
  setSizeAdjustPolicy ( QComboBox::AdjustToMinimumContentsLength);
  connect( shadingComboBox, SIGNAL( activated(int) ),
      SLOT( shadingChanged() ) );

  shadingSpinBox= new QSpinBox( advFrame );
  shadingSpinBox->setMinimum(0);
  shadingSpinBox->setMaximum(99);
  shadingSpinBox->setSingleStep(1);
  shadingSpinBox->setSpecialValueText(tr("Auto"));
  connect( shadingSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( shadingChanged() ) );

  shadingcoldComboBox=
      PaletteBox( advFrame,csInfo,false,0,tr("Off").toStdString(),true );
  shadingcoldComboBox->
  setSizeAdjustPolicy ( QComboBox::AdjustToMinimumContentsLength);
  connect( shadingcoldComboBox, SIGNAL( activated(int) ),
      SLOT( shadingChanged() ) );

  shadingcoldSpinBox= new QSpinBox( advFrame );
  shadingcoldSpinBox->setMinimum(0);
  shadingcoldSpinBox->setMaximum(99);
  shadingcoldSpinBox->setSingleStep(1);
  shadingcoldSpinBox->setSpecialValueText(tr("Auto"));
  connect( shadingcoldSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( shadingChanged() ) );

  //pattern
  patternComboBox =
      PatternBox( advFrame,patternInfo,false,0,tr("Off").toStdString(),true );
  patternComboBox ->
  setSizeAdjustPolicy ( QComboBox::AdjustToMinimumContentsLength);
  connect( patternComboBox, SIGNAL( activated(int) ),
      SLOT( patternComboBoxToggled(int) ) );

  //pattern colour
  patternColourBox = ColourBox(advFrame,false,0,tr("Auto").toStdString(),true);
  connect( patternColourBox, SIGNAL( activated(int) ),
      SLOT( patternColourBoxToggled(int) ) );

  //alpha blending
  alphaSpinBox= new QSpinBox( advFrame );
  alphaSpinBox->setMinimum(0);
  alphaSpinBox->setMaximum(255);
  alphaSpinBox->setSingleStep(5);
  alphaSpinBox->setValue(255);
  connect( alphaSpinBox, SIGNAL( valueChanged(int) ),
      SLOT( alphaChanged(int) ) );

  //colour
  colour2ComboBox = ColourBox(advFrame, false,0,tr("Off").toStdString(),true);
  connect( colour2ComboBox, SIGNAL( activated(int) ),
      SLOT( colour2ComboBoxToggled(int) ) );

  //line interval
  interval2ComboBox = new QComboBox(advFrame);
  connect( interval2ComboBox, SIGNAL( activated(int) ),
      SLOT( interval2ComboBoxToggled(int) ) );

  //zero value
  zero1ComboBox= new QComboBox( advFrame );
  zero2ComboBox = new QComboBox(advFrame);
  connect( zero1ComboBox, SIGNAL( activated(int) ),
      SLOT( zero1ComboBoxToggled(int) ) );
  connect( zero2ComboBox, SIGNAL( activated(int) ),
      SLOT( zero2ComboBoxToggled(int) ) );

  //min
  min1ComboBox = new QComboBox(advFrame);
  min2ComboBox = new QComboBox(advFrame);

  //max
  max1ComboBox = new QComboBox(advFrame);
  max2ComboBox = new QComboBox(advFrame);

  connect( min1ComboBox, SIGNAL( activated(int) ),
      SLOT( min1ComboBoxToggled(int) ) );
  connect( max1ComboBox, SIGNAL( activated(int) ),
      SLOT( max1ComboBoxToggled(int) ) );
  connect( min2ComboBox, SIGNAL( activated(int) ),
      SLOT( min2ComboBoxToggled(int) ) );
  connect( max2ComboBox, SIGNAL( activated(int) ),
      SLOT( max2ComboBoxToggled(int) ) );

  //linewidth
  linewidth2ComboBox = LinewidthBox( advFrame);
  connect( linewidth2ComboBox, SIGNAL( activated(int) ),
      SLOT( linewidth2ComboBoxToggled(int) ) );
  //linetype
  linetype2ComboBox = LinetypeBox( advFrame,false );
  connect( linetype2ComboBox, SIGNAL( activated(int) ),
      SLOT( linetype2ComboBoxToggled(int) ) );

  // Plot frame
  frameCheckBox= new QCheckBox(QString(tr("Frame")), advFrame);
  frameCheckBox->setChecked( true );
  connect( frameCheckBox, SIGNAL( toggled(bool) ),
      SLOT( frameCheckBoxToggled(bool) ) );

  // enable/disable zero line (isoline with value=0)
  zeroLineCheckBox= new QCheckBox(QString(tr("Zero line")), advFrame);
  //  zeroLineColourCBox= new QComboBox(advFrame);
  zeroLineCheckBox->setChecked( true );

  connect( zeroLineCheckBox, SIGNAL( toggled(bool) ),
      SLOT( zeroLineCheckBoxToggled(bool) ) );

  // Create horizontal frame lines
  QFrame *line0 = new QFrame( advFrame );
  line0->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line1 = new QFrame( advFrame );
  line1->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line2 = new QFrame( advFrame );
  line2->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line3 = new QFrame( advFrame );
  line3->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line4 = new QFrame( advFrame );
  line4->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line5 = new QFrame( advFrame );
  line5->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  QFrame *line6 = new QFrame( advFrame );
  line6->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  // layout......................................................

  QGridLayout* advLayout = new QGridLayout( );
  advLayout->setSpacing(1);
  int line = 0;
  advLayout->addWidget( extremeTypeLabel, line, 0 );
  advLayout->addWidget(extremeSizeLabel, line, 1 );
  advLayout->addWidget(extremeRadiusLabel, line, 2 );
  line++;
  advLayout->addWidget( extremeTypeCbox, line, 0 );
  advLayout->addWidget(extremeSizeSpinBox, line, 1 );
  advLayout->addWidget(extremeRadiusSpinBox, line, 2 );
  line++;
  advLayout->setRowStretch(line,5);
  advLayout->addWidget(line0, line,0,1,3 );

  line++;
  advLayout->addWidget(gridLinesLabel, line, 0 );
  advLayout->addWidget(gridLinesSpinBox, line, 1 );
  advLayout->addWidget(gridValueCheckBox, line, 2 );
  line++;
  advLayout->addWidget(lineSmoothLabel, line, 0 );
  advLayout->addWidget(lineSmoothSpinBox, line, 1 );
  line++;
  advLayout->addWidget(fieldSmoothLabel, line, 0 );
  advLayout->addWidget(fieldSmoothSpinBox, line, 1 );
  line++;
  advLayout->addWidget(hourOffsetLabel, line, 0 );
  advLayout->addWidget(hourOffsetSpinBox, line, 1 );
  line++;
  advLayout->addWidget(hourDiffLabel, line, 0 );
  advLayout->addWidget(hourDiffSpinBox, line, 1 );
  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addWidget(line4, line,0, 1,3 );
  line++;
  advLayout->addWidget(undefMaskingLabel, line, 0 );
  advLayout->addWidget(undefMaskingCbox, line, 1 );
  line++;
  advLayout->addWidget(undefColourCbox, line, 0 );
  advLayout->addWidget(undefLinewidthCbox, line, 1 );
  advLayout->addWidget(undefLinetypeCbox, line, 2 );
  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addWidget(line1, line,0, 1,4 );

  line++;
  advLayout->addWidget(frameCheckBox, line, 0 );
  advLayout->addWidget(zeroLineCheckBox, line, 1 );
  line++;
  advLayout->addWidget(valueLabelCheckBox, line, 0 );
  advLayout->addWidget(labelSizeSpinBox, line, 1 );
  advLayout->addWidget(valuePrecisionBox, line, 2 );
  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addWidget(line2, line,0, 1,3 );

  line++;
  advLayout->addWidget( tableCheckBox, line, 0 );
  advLayout->addWidget( repeatCheckBox, line, 1 );
  line++;
  advLayout->addWidget( shadingLabel, line, 0 );
  advLayout->addWidget( shadingComboBox, line, 1 );
  advLayout->addWidget( shadingSpinBox, line, 2 );
  line++;
  advLayout->addWidget( shadingcoldLabel, line, 0 );
  advLayout->addWidget( shadingcoldComboBox,line, 1 );
  advLayout->addWidget( shadingcoldSpinBox, line, 2 );
  line++;
  advLayout->addWidget( patternLabel, line, 0 );
  advLayout->addWidget( patternComboBox, line, 1 );
  advLayout->addWidget( patternColourBox, line, 2 );
  line++;
  advLayout->addWidget( alphaLabel, line, 0 );
  advLayout->addWidget( alphaSpinBox, line, 1 );
  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addWidget(line6, line,0, 1,3 );

  line++;
  advLayout->addWidget( baseLabel, line, 0 );
  advLayout->addWidget( minLabel, line, 1 );
  advLayout->addWidget( maxLabel, line, 2 );
  line++;
  advLayout->addWidget( zero1ComboBox, line, 0 );
  advLayout->addWidget( min1ComboBox, line, 1 );
  advLayout->addWidget( max1ComboBox, line, 2 );
  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addWidget(line3, line,0,1,3 );

  line++;
  advLayout->addWidget(headLabel,line,0,1,2);
  line++;
  advLayout->addWidget( colourLabel, line, 0 );
  advLayout->addWidget( colour2ComboBox, line, 1 );
  line++;
  advLayout->addWidget( intervalLabel, line, 0 );
  advLayout->addWidget( interval2ComboBox, line, 1 );
  line++;
  advLayout->addWidget( linewidthLabel, line, 0 );
  advLayout->addWidget( linewidth2ComboBox, line, 1 );
  line++;
  advLayout->addWidget( linetypeLabel, line, 0 );
  advLayout->addWidget( linetype2ComboBox, line, 1 );
  line++;
  advLayout->addWidget( base2Label, line, 0 );
  advLayout->addWidget( min2Label, line, 1 );
  advLayout->addWidget( max2Label, line, 2 );
  line++;
  advLayout->addWidget( zero2ComboBox, line, 0 );
  advLayout->addWidget( min2ComboBox, line, 1 );
  advLayout->addWidget( max2ComboBox, line, 2 );

  line++;
  advLayout->setRowStretch(line,5);;
  advLayout->addWidget(line5, line,0, 1,3 );
  line++;
  advLayout->addWidget( threeColourLabel, line, 0 );
  //  advLayout->addWidget( threeColoursCheckBox, 38, 0 );
  line++;
  advLayout->addWidget( threeColourBox[0], line, 0 );
  advLayout->addWidget( threeColourBox[1], line, 1 );
  advLayout->addWidget( threeColourBox[2], line, 2 );

  // a separator
  QFrame* advSep= new QFrame( advFrame );
  advSep->setFrameStyle( QFrame::VLine | QFrame::Raised );
  advSep->setLineWidth(5);

  QHBoxLayout *hLayout = new QHBoxLayout( advFrame);

  hLayout->addWidget(advSep);
  hLayout->addLayout(advLayout);

  return;
}

void FieldDialog::updateModelBoxes()
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::updateModelBoxes called"<<endl;
#endif

  //keep old plots
  vector<miutil::miString> vstr = getOKString();

  modelGRbox->clear();
  indexMGRtable.clear();

  int nr_m = m_modelgroup.size();
  if (nr_m == 0)
    return;

  if (profetEnabled) {
    for (int i = 0; i < nr_m; i++) {
      if (m_modelgroup[i].groupType == "profetfilegroup") {
        indexMGRtable.push_back(i);
        modelGRbox->addItem(QString(m_modelgroup[i].groupName.c_str()));
      }
    }
  }
  if (useArchive) {
    for (int i = 0; i < nr_m; i++) {
      if (m_modelgroup[i].groupType == "archivefilegroup") {
        indexMGRtable.push_back(i);
        modelGRbox->addItem(QString(m_modelgroup[i].groupName.c_str()));
      }
    }
  }
  for (int i = 0; i < nr_m; i++) {
    if (m_modelgroup[i].groupType == "filegroup") {
      indexMGRtable.push_back(i);
      modelGRbox->addItem(QString(m_modelgroup[i].groupName.c_str()));
    }
  }
  modelGRbox->setCurrentIndex(0);

  if (selectedFields.size() > 0)
    deleteAllSelected();

  // show models in the first modelgroup
  modelGRboxActivated(0);

  //replace old plots
  putOKString(vstr);
}

void FieldDialog::updateModels()
{
  m_modelgroup = m_ctrl->initFieldDialog();
  updateModelBoxes();
}

void FieldDialog::archiveMode(bool on)
{
  useArchive = on;
  updateModelBoxes();
}

void FieldDialog::modelGRboxActivated(int index)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::modelGRboxActivated called"<<endl;
#endif

  if (index < 0 || index >= int(indexMGRtable.size()))
    return;

  int indexMGR = indexMGRtable[index];
  modelbox->clear();
  refTimeComboBox->clear();
  fieldGRbox->clear();
  fieldbox->clear();

  int nr_model = m_modelgroup[indexMGR].modelNames.size();
  for (int i = 0; i < nr_model; i++) {
    modelbox->addItem(QString(m_modelgroup[indexMGR].modelNames[i].c_str()));
  }

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::modelGRboxActivated returned"<<endl;
#endif
}

void FieldDialog::modelboxClicked(QListWidgetItem * item)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::modelboxClicked called"<<endl;
#endif

  refTimeComboBox->clear();
  fieldGRbox->clear();
  fieldbox->clear();

  int indexM =modelbox->row(item);
  int indexMGR = indexMGRtable[modelGRbox->currentIndex()];

  miutil::miString model = m_modelgroup[indexMGR].modelNames[indexM];

  set<std::string> refTimes = m_ctrl->getFieldReferenceTimes(model);

  set<std::string>::const_iterator ip=refTimes.begin();
  for (; ip != refTimes.end(); ++ip) {
    refTimeComboBox->addItem(QString((*ip).c_str()));
  }
  if ( refTimeComboBox->count() ) {
    refTimeComboBox->setCurrentIndex(refTimeComboBox->count()-1);
  }
    updateFieldGroups();
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::modelboxClicked returned"<<endl;
#endif
}

void FieldDialog::updateFieldGroups()
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::updateFieldGroups called"<<endl;
#endif

  fieldGRbox->clear();
  fieldbox->clear();

  int indexM =modelbox->currentRow();
  if ( indexM < 0 ) return;

  int indexMGR = indexMGRtable[modelGRbox->currentIndex()];
  miutil::miString model = m_modelgroup[indexMGR].modelNames[indexM];

  getFieldGroups(model, refTimeComboBox->currentText().toStdString(), indexMGR, indexM,
      fieldGroupCheckBox->isChecked(), vfgi);

  int nvfgi = vfgi.size();

  //Translate level names if not cdmSyntax
  for (int i = 0; i < nvfgi; i++) {

    if( !vfgi[i].cdmSyntax ) {
      for(size_t ii = 0; ii <  vfgi[i].levelNames.size(); ii++ ) {

        vfgi[i].levelNames[ii] = FieldSpecTranslation::getOldLevel(vfgi[i].zaxis, vfgi[i].levelNames[ii]);
      }
      vfgi[i].defaultLevel = FieldSpecTranslation::getOldLevel(vfgi[i].zaxis, vfgi[i].defaultLevel);
    }

  }

  int i, indexFGR;

  if (nvfgi > 0) {
    for (i = 0; i < nvfgi; i++) {
      fieldGRbox->addItem(QString(vfgi[i].groupName.c_str()));
    }

    indexFGR = -1;
    i = 0;
    while (i < nvfgi && vfgi[i].groupName != lastFieldGroupName)
      i++;
    if (i < nvfgi) {
      indexFGR = i;
    } else {
      int l1, l2 = lastFieldGroupName.length(), lm = 0;
      for (i = 0; i < nvfgi; i++) {
        l1 = vfgi[i].groupName.length();
        if (l1 > l2)
          l1 = l2;
        if (l1 > lm && vfgi[i].groupName.substr(0, l1)
            == lastFieldGroupName.substr(0, l1)) {
          lm = l1;
          indexFGR = i;
        }
      }
    }
    if (indexFGR < 0)
      indexFGR = 0;
    lastFieldGroupName = vfgi[indexFGR].groupName;
    fieldGRbox->setCurrentIndex(indexFGR);
    fieldGRboxActivated(indexFGR);
  }

  if (!selectedFields.empty()) {
    // possibly enable changeModel button
    enableFieldOptions();
  }

  updateTime();

}

void FieldDialog::fieldGRboxActivated(int index)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::fieldGRboxActivated called"<<endl;
#endif

  fieldbox->clear();
  fieldbox->blockSignals(true);

  int i, j, n;
  int last = -1;

  if (vfgi.size() > 0) {

    int indexMGR = indexMGRtable[modelGRbox->currentIndex()];
    int indexM = modelbox->currentRow();
    int indexRefTime = refTimeComboBox->currentIndex();
    int indexFGR = index;

    lastFieldGroupName = vfgi[indexFGR].groupName;

    int nfield = vfgi[indexFGR].fieldNames.size();
    for (i = 0; i < nfield; i++) {
      fieldbox->addItem(QString(vfgi[indexFGR].fieldNames[i].c_str()));
    }

    countSelected.resize(nfield);
    for (i = 0; i < nfield; ++i)
      countSelected[i] = 0;

    n = selectedFields.size();
    int ml;

    for (i = 0; i < n; ++i) {
      //mark fields in FieldBox if current selection (modelgroup, model,refTime) match a selectedField
      if (!selectedFields[i].inEdit &&
          selectedFields[i].indexMGR == indexMGR &&
          selectedFields[i].indexM == indexM &&
          selectedFields[i].indexRefTime == indexRefTime ) {
        j = 0;
        if (selectedFields[i].modelName == vfgi[indexFGR].modelName) {
          while (j < nfield && selectedFields[i].fieldName
              != vfgi[indexFGR].fieldNames[j])
            j++;
          if (j < nfield) {
            //Check if level match
            if ((ml = vfgi[indexFGR].levelNames.size()) > 0) {
              int l = 0;
              while (l < ml && vfgi[indexFGR].levelNames[l]
                                                         != selectedFields[i].level)
                l++;
              if (l == ml)
                j = nfield;
            }
            // Check if idnum match
            if ((ml = vfgi[indexFGR].idnumNames.size()) > 0) {
              int l = 0;
              while (l < ml && vfgi[indexFGR].idnumNames[l]
                                                         != selectedFields[i].idnum)
                l++;
              if (l == ml)
                j = nfield;
            }
            if (j < nfield) {
              fieldbox->item(j)->setSelected(true);
              fieldbox->setCurrentRow(j);
              countSelected[j]++;
              last = i;
            }
          }
        }
      }
    }
  }

  if (last >= 0 && selectedFieldbox->item(last)) {
    selectedFieldbox->setCurrentRow(last);
    selectedFieldbox->item(last)->setSelected(true);
    enableFieldOptions();
  } else if (selectedFields.empty()) {
    enableWidgets("none");
  }

  fieldbox->blockSignals(false);

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::fieldGRboxActivated returned"<<endl;
#endif
}

void FieldDialog::setLevel()
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::setLevel called"<<endl;
#endif

  int i = selectedFieldbox->currentRow();
  int n = 0, l = 0;
  if (i >= 0 && selectedFields[i].level.exists()) {
    lastLevel = selectedFields[i].level;
    n = selectedFields[i].levelOptions.size();
    l = 0;
    while (l < n && selectedFields[i].levelOptions[l] != lastLevel)
      l++;
    if (l == n)
      l = 0; // should not happen!
  }

  levelSlider->blockSignals(true);

  if (n > 0) {
    currentLevels = selectedFields[i].levelOptions;
    levelSlider->setRange(0, n - 1);
    levelSlider->setValue(l);
    levelSlider->setEnabled(true);
    QString qstr = lastLevel.c_str();
    levelLabel->setText(qstr);
  } else {
    currentLevels.clear();
    // keep slider in a fixed position when disabled
    levelSlider->setEnabled(false);
    levelSlider->setValue(1);
    levelSlider->setRange(0, 1);
    levelLabel->clear();
  }

  levelSlider->blockSignals(false);

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::setLevel returned"<<endl;
#endif
  return;
}

void FieldDialog::levelPressed()
{
  levelInMotion = true;
}

void FieldDialog::levelChanged(int index)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::levelChanged called: "<<index<<endl;
#endif

  int n = currentLevels.size();
  if (index >= 0 && index < n) {
    QString qstr = currentLevels[index].c_str();
    levelLabel->setText(qstr);
    lastLevel = currentLevels[index];
  }

  if (!levelInMotion)
    updateLevel();

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::levelChanged returned"<<endl;
#endif
  return;
}

vector<miutil::miString> FieldDialog::changeLevel(int increment, int type)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::changeLevel called: "<<increment<<endl;
#endif
  // called from MainWindow levelUp/levelDown

  miutil::miString level;
  vector<miutil::miString> vlevels;
  int n = selectedFields.size();

  //For some reason (?) vertical levels and extra levels are sorted i opposite directions
  if ( type == 0 ) {
    increment *= -1;
  }

  //find first plot with levels, use use this plot to select next level
  int i = 0;
  if ( type==0 ) { //vertical levels
    while ( i < n && selectedFields[i].levelOptions.size() == 0) i++;
    if( i != n ) {
      vlevels = selectedFields[i].levelOptions;
      level = selectedFields[i].level;
    }
  } else { // extra levels
    while ( i < n && selectedFields[i].idnumOptions.size() == 0) i++;
    if( i != n ) {
      vlevels = selectedFields[i].idnumOptions;
      level = selectedFields[i].idnum;
    }
  }


  //plot with levels exists, find next level
  if( i != n ) {
    miutil::miString level_incremented;
    int m = vlevels.size();
    int current = 0;
    while (current < m && vlevels[current] != level) current++;
    if (current < m) {
      level_incremented = vlevels[current + increment];
    }

    //loop through all plots to see if is possible to

    if ( type == 0 ) { //vertical levels
      for (int j = 0; j < n; j++) {
        if (selectedFields[j].levelmove && selectedFields[j].level == level) {
          selectedFields[j].level = level_incremented;
          //update dialog
          if(j==selectedFieldbox->currentRow()){
            levelSlider->blockSignals(true);
            levelSlider->setValue(current + increment);
            levelSlider->blockSignals(false);
            levelChanged(current + increment);
          }
        } else {
          selectedFields[j].levelmove = false;
        }
      }
    } else { // extra levels
      for (int j = 0; j < n; j++) {
        if (selectedFields[j].idnummove && selectedFields[j].idnum == level) {
          selectedFields[j].idnum = level_incremented;
          //update dialog
          if(j==selectedFieldbox->currentRow()){
            idnumSlider->blockSignals(true);
            idnumSlider->setValue(current + increment);
            idnumSlider->blockSignals(false);
            idnumChanged(current + increment);
          }
        } else {
          selectedFields[j].idnummove = false;
        }
      }
    }

  }

  return getOKString(false);
}

void FieldDialog::updateLevel()
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::updateLevel called"<<endl;
#endif

  int i = selectedFieldbox->currentRow();

  if (i >= 0 && i < int(selectedFields.size())) {
    selectedFields[i].level = lastLevel;
    updateTime();
  }

  levelInMotion = false;

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::updateLevel returned"<<endl;
#endif
  return;
}

void FieldDialog::setIdnum()
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::setIdnum called"<<endl;
#endif

  int i = selectedFieldbox->currentRow();
  int n = 0, l = 0;
  if (i >= 0 && selectedFields[i].idnum.exists()) {
    lastIdnum = selectedFields[i].idnum;
    n = selectedFields[i].idnumOptions.size();
    l = 0;
    while (l < n && selectedFields[i].idnumOptions[l] != lastIdnum)
      l++;
    if (l == n)
      l = 0; // should not happen!
  }

  idnumSlider->blockSignals(true);

  if (n > 0) {
    currentIdnums = selectedFields[i].idnumOptions;
    idnumSlider->setRange(0, n - 1);
    idnumSlider->setValue(l);
    idnumSlider->setEnabled(true);
    QString qstr = lastIdnum.c_str();
    idnumLabel->setText(qstr);
  } else {
    currentIdnums.clear();
    // keep slider in a fixed position when disabled
    idnumSlider->setEnabled(false);
    idnumSlider->setValue(1);
    idnumSlider->setRange(0, 1);
    idnumLabel->clear();
  }

  idnumSlider->blockSignals(false);

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::setIdnum returned"<<endl;
#endif
  return;
}

void FieldDialog::idnumPressed()
{
  idnumInMotion = true;
}

void FieldDialog::idnumChanged(int index)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::idnumChanged called"<<endl;
#endif

  int n = currentIdnums.size();
  if (index >= 0 && index < n) {
    QString qstr = currentIdnums[index].c_str();
    idnumLabel->setText(qstr);
    lastIdnum = currentIdnums[index];
  }

  if (!idnumInMotion)
    updateIdnum();

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::idnumChanged returned"<<endl;
#endif
  return;
}


void FieldDialog::updateIdnum()
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::updateIdnum called"<<endl;
#endif

  if (selectedFieldbox->currentRow() >= 0) {
    unsigned int i = selectedFieldbox->currentRow();
    if (i < selectedFields.size()) {
      selectedFields[i].idnum = lastIdnum;
      updateTime();
    }
  }

  idnumInMotion = false;

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::updateIdnum returned"<<endl;
#endif
  return;
}

void FieldDialog::fieldboxChanged(QListWidgetItem* item)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::fieldboxChanged called:"<<fieldbox->count()<<endl;
#endif

  if (!fieldGRbox->count())
    return;
  if (!fieldbox->count())
    return;

  if (historyOkButton->isEnabled())
    deleteAllSelected();

  int i, j, jp, n;
  int indexMGR = indexMGRtable[modelGRbox->currentIndex()];
  int indexM = modelbox->currentRow();
  int indexFGR = fieldGRbox->currentIndex();

  // Note: multiselection list, current item may only be the last...

  int nf = vfgi[indexFGR].fieldNames.size();

  int last = -1;
  int lastdelete = -1;

  for (int indexF = 0; indexF < nf; ++indexF) {

    if (fieldbox->item(indexF)->isSelected() && countSelected[indexF] == 0) {

      SelectedField sf;
      sf.inEdit = false;
      sf.external = false;
      sf.forecastSpec = false;
      sf.indexMGR = indexMGR;
      sf.indexM = indexM;
      sf.modelName = vfgi[indexFGR].modelName;
      sf.fieldName = vfgi[indexFGR].fieldNames[indexF];
      sf.levelOptions = vfgi[indexFGR].levelNames;
      sf.idnumOptions = vfgi[indexFGR].idnumNames;
      sf.refTime = vfgi[indexFGR].refTime;
      sf.zaxis = vfgi[indexFGR].zaxis;
      sf.extraaxis = vfgi[indexFGR].extraaxis;
      sf.grid = vfgi[indexFGR].grid;
      if ( int(vfgi[indexFGR].units.size()) > indexF ) {
        sf.unit = vfgi[indexFGR].units[indexF];
      }
      sf.cdmSyntax = vfgi[indexFGR].cdmSyntax;
      sf.plotDefinition = fieldGroupCheckBox->isChecked();
      sf.minus = false;

      if (!vfgi[indexFGR].defaultLevel.empty()) {
        n = vfgi[indexFGR].levelNames.size();
        i = 0;
        while (i < n && vfgi[indexFGR].levelNames[i] != lastLevel)
          i++;
        if (i < n)
          sf.level = lastLevel;
        else
          sf.level = vfgi[indexFGR].defaultLevel;
      }

      if (!vfgi[indexFGR].defaultIdnum.empty()) {
        n = vfgi[indexFGR].idnumNames.size();
        i = 0;
        while (i < n && vfgi[indexFGR].idnumNames[i] != lastIdnum)
          i++;
        if (i < n)
          sf.idnum = lastIdnum;
        else
          sf.idnum = vfgi[indexFGR].defaultIdnum;
      }

      sf.hourOffset = 0;
      sf.hourDiff = 0;

      sf.fieldOpts = getFieldOptions(sf.fieldName, false, sf.inEdit);

      selectedFields.push_back(sf);

      countSelected[indexF]++;

      miutil::miString text = sf.modelName + " " + sf.fieldName + " " + sf.refTime;
      selectedFieldbox->addItem(QString(text.c_str()));
      last = selectedFields.size() - 1;

    } else if (!fieldbox->item(indexF)->isSelected() && countSelected[indexF]
                                                                      > 0) {
      miutil::miString fieldName = vfgi[indexFGR].fieldNames[indexF];
      n = selectedFields.size();
      j = jp = -1;
      int jsel = -1, isel = selectedFieldbox->currentRow();
      for (i = 0; i < n; i++) {
        if (selectedFields[i].indexMGR == indexMGR && selectedFields[i].indexM
            == indexM && selectedFields[i].fieldName == fieldName) {
          jp = j;
          j = i;
          if (i == isel)
            jsel = isel;
        }
      }
      if (j >= 0) { // anything else is a program error!
        // remove item in selectedFieldbox,
        // a field may be selected (by copy) more than once,
        // the selected or the last of those are removed
        if (jsel >= 0 && jp >= 0) {
          jp = j;
          j = jsel;
        }
        if (jp >= 0) {
          fieldbox->blockSignals(true);
          fieldbox->setCurrentRow(indexF);
          fieldbox->item(indexF)->setSelected(true);
          fieldbox->blockSignals(false);
          lastdelete = jp;
          selectedFieldbox->takeItem(jp);
        } else {
          lastdelete = j;
          selectedFieldbox->takeItem(j);
        }
        countSelected[indexF]--;
        for (i = j; i < n - 1; i++)
          selectedFields[i] = selectedFields[i + 1];
        selectedFields.pop_back();
      }
    }
  }

  if (last < 0 && lastdelete >= 0 && selectedFields.size() > 0) {
    last = lastdelete;
    if (last >= int(selectedFields.size()))
      last = selectedFields.size() - 1;
  }

  if (last >= 0 && selectedFieldbox->item(last)) {
    selectedFieldbox->setCurrentRow(last);
    selectedFieldbox->item(last)->setSelected(true);
    enableFieldOptions();
  } else if (selectedFields.size() == 0) {
    enableWidgets("none");
  }

  updateTime();

  //first field can't be minus
  if (selectedFieldbox->count() > 0 && selectedFields[0].minus) {
    minusButton->setChecked(false);
  }

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::fieldboxChanged returned"<<endl;
#endif
  return;
}

void FieldDialog::enableFieldOptions()
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::enableFieldOptions called"<<endl;
#endif

  setDefaultFieldOptions();

  int nc, i, n;

  int index = selectedFieldbox->currentRow();
  int lastindex = selectedFields.size() - 1;

  if (index < 0 || index > lastindex) {
    cerr << "POGRAM ERROR.1 in FieldDialog::enableFieldOptions" << endl;
    cerr << "       index,selectedFields.size: " << index << " "
        << selectedFields.size() << endl;
    enableWidgets("none");
    return;
  }

  if (selectedFields[index].inEdit) {
    changeModelButton->setEnabled(false);
    deleteButton->setEnabled(false);
    copyField->setEnabled(false);
  } else {
    upFieldButton->setEnabled((index > numEditFields));
    downFieldButton->setEnabled((index < lastindex));
    if (vfgi.size() == 0)
      changeModelButton->setEnabled(false);
    else if (selectedFields[index].indexMGR == modelGRbox->currentIndex()
        && selectedFields[index].indexM == modelbox->currentRow())
      changeModelButton->setEnabled(false);
    else
      changeModelButton->setEnabled(true);
    deleteButton->setEnabled(true);
    copyField->setEnabled(true);
  }

  setLevel();
  setIdnum();
  minusButton->setEnabled(index > 0 && !selectedFields[index - 1].minus);
  deleteAll->setEnabled(true);
  resetOptionsButton->setEnabled(true);

  if (selectedFields[index].fieldOpts == currentFieldOpts
      && selectedFields[index].inEdit == currentFieldOptsInEdit
      && !selectedFields[index].minus)
    return;

  currentFieldOpts = selectedFields[index].fieldOpts;
  currentFieldOptsInEdit = selectedFields[index].inEdit;

  //###############################################################################
//    cerr << "FieldDialog::enableFieldOptions: "
//         << selectedFields[index].fieldName << endl;
//    cerr << "             " << selectedFields[index].fieldOpts << endl;
  //###############################################################################


  if (selectedFields[index].minus && !minusButton->isChecked())
    minusButton->setChecked(true);
  else if (!selectedFields[index].minus && minusButton->isChecked())
    minusButton->setChecked(false);

  //hourOffset
  if (currentFieldOptsInEdit) {
    hourOffsetSpinBox->setValue(0);
    hourOffsetSpinBox->setEnabled(false);
  } else {
    i = selectedFields[index].hourOffset;
    hourOffsetSpinBox->setValue(i);
    hourOffsetSpinBox->setEnabled(true);
  }

  //hourDiff
  if (currentFieldOptsInEdit) {
    hourDiffSpinBox->setValue(0);
    hourDiffSpinBox->setEnabled(false);
  } else {
    i = selectedFields[index].hourDiff;
    hourDiffSpinBox->setValue(i);
    hourDiffSpinBox->setEnabled(true);
  }

  if (selectedFields[index].minus)
    return;

  vpcopt = cp->parse(selectedFields[index].fieldOpts);

  //   cerr <<endl;
  //   cerr <<"FieldOpt string: "<<selectedFields[index].fieldOpts<<endl;
  //   cerr <<endl;
  /*******************************************************
   n=vpcopt.size();
   bool err=false;
   bool listall= true;
   for (j=0; j<n; j++)
   if (vpcopt[j].key=="unknown") err=true;
   if (err || listall) {
   cerr << "FieldDialog::enableFieldOptions: "
   << selectedFields[index].fieldName << endl;
   cerr << "             " << selectedFields[index].fieldOpts << endl;
   for (j=0; j<n; j++) {
   cerr << "  parse " << j << " : key= " << vpcopt[j].key
   << "  idNumber= " << vpcopt[j].idNumber << endl;
   cerr << "            allValue: " << vpcopt[j].allValue << endl;
   for (k=0; k<vpcopt[j].strValue.size(); k++)
   cerr << "               " << k << "    strValue: " << vpcopt[j].strValue[k] << endl;
   for (k=0; k<vpcopt[j].floatValue.size(); k++)
   cerr << "               " << k << "  floatValue: " << vpcopt[j].floatValue[k] << endl;
   for (k=0; k<vpcopt[j].intValue.size(); k++)
   cerr << "               " << k << "    intValue: " << vpcopt[j].intValue[k] << endl;
   }
   }
   *******************************************************/

  int nr_linetypes = linetypes.size();
  enableWidgets("contour");

  //unit
  if ((nc = cp->findKey(vpcopt, "unit")) >= 0) {
    updateFieldOptions("unit", vpcopt[nc].allValue, -1);
    unitLineEdit->setText(vpcopt[nc].allValue.c_str());
  } else {
    updateFieldOptions("unit", "remove");
    unitLineEdit->clear();
  }

  //dimension (1dim = contour,..., 2dim=wind,...)
    if ((nc = cp->findKey(vpcopt, "dim")) >= 0) {
      if (!vpcopt[nc].intValue.empty() && vpcopt[nc].intValue[0] < int(plottypes_dim.size())) {
          plottypes = plottypes_dim[vpcopt[nc].intValue[0]];
      } else if( plottypes_dim.size() > 0 ){
        plottypes = plottypes_dim[0];
      }
    } else if( plottypes_dim.size() > 0 ){
      plottypes = plottypes_dim[0];
    }
    plottypeComboBox->clear();
    int nr_plottypes = plottypes.size();
    for( int i=0; i<nr_plottypes; i++ ){
      plottypeComboBox->addItem(QString(plottypes[i].c_str()));
    }

      //plottype
    if ((nc = cp->findKey(vpcopt, "plottype")) >= 0) {
      miutil::miString value = vpcopt[nc].allValue;
      i = 0;
      while (i < nr_plottypes && value != plottypes[i])
        i++;
      if (i == nr_plottypes) {
        i=0;
      }
      plottypeComboBox->setCurrentIndex(i);
      updateFieldOptions("plottype", value, -1);
      enableWidgets(plottypes[i]);
    } else {
      updateFieldOptions("plottype", plottypes[0], -1);
      plottypeComboBox->setCurrentIndex(0);
      enableWidgets(plottypes[0]);
    }


  // colour(s)
  if ((nc = cp->findKey(vpcopt, "colour_2")) >= 0) {
    int nr_colours = colour2ComboBox->count();
    if (vpcopt[nc].allValue.downcase() == "off" ) {
      updateFieldOptions("colour_2", "off");
      colour2ComboBox->setCurrentIndex(0);
    } else {
      QString col = vpcopt[nc].allValue.c_str();
      i = 0;
      while (i < nr_colours && col != colour2ComboBox->itemText(i) )
        i++;
      if (i == nr_colours) {
        Colour::defineColourFromString(vpcopt[nc].allValue);
        ExpandColourBox(colour2ComboBox,Colour(vpcopt[nc].allValue));
      }
      updateFieldOptions("colour_2", vpcopt[nc].allValue);
      colour2ComboBox->setCurrentIndex(i);
    }
  }

  if ((nc = cp->findKey(vpcopt, "colour")) >= 0) {
    int nr_colours = colorCbox->count();
    if (vpcopt[nc].allValue.downcase() == "off" ) {
      updateFieldOptions("colour", "off");
      colorCbox->setCurrentIndex(0);
    } else {
      QString col = vpcopt[nc].allValue.c_str();
      i = 1;
      while (i < nr_colours && col != colorCbox->itemText(i))
        i++;
      if (i == nr_colours) {
        Colour::defineColourFromString(vpcopt[nc].allValue);
        ExpandColourBox(colorCbox,Colour(vpcopt[nc].allValue));
      }
      updateFieldOptions("colour", vpcopt[nc].allValue);
      colorCbox->setCurrentIndex(i);
    }
  }

  // 3 colours
  if ((nc = cp->findKey(vpcopt, "colours")) >= 0) {
    int nr_colours = threeColourBox[0]->count();
    vector<miutil::miString> colours = vpcopt[nc].allValue.split(",");
    if (colours.size() == 3) {
      for (int j = 0; j < 3; j++) {
        i = 1;
        QString col = colours[j].c_str();
        while (i < nr_colours && col != threeColourBox[j]->itemText(i) )
          i++;
        if (i == nr_colours) {
          Colour::defineColourFromString(colours[j]);
          ExpandColourBox(threeColourBox[j], Colour(colours[j]));
        }
        threeColourBox[j]->setCurrentIndex(i);
      }
      threeColoursChanged();
    }
  }

  //contour shading
  if ((nc = cp->findKey(vpcopt, "palettecolours")) >= 0) {
    if (vpcopt[nc].allValue.downcase() == "off" ) {
      updateFieldOptions("palettecolours", "off");
      shadingComboBox->setCurrentIndex(0);
    } else {
      vector<miutil::miString> tokens = vpcopt[nc].allValue.split(",");
      vector<miutil::miString> stokens = tokens[0].split(";");
      if (stokens.size() == 2)
        shadingSpinBox->setValue(atoi(stokens[1].cStr()));
      else
        shadingSpinBox->setValue(0);
      int nr_cs = csInfo.size();
      miutil::miString str;
      i = 0;
      while (i < nr_cs && stokens[0] != csInfo[i].name)
        i++;
      if (i == nr_cs) {
        ColourShading::defineColourShadingFromString(vpcopt[nc].allValue);
        ExpandPaletteBox(shadingComboBox,ColourShading(vpcopt[nc].allValue));
        ColourShading::ColourShadingInfo info;
        info.name=vpcopt[nc].allValue;
        info.colour=ColourShading::getColourShading(vpcopt[nc].allValue);
        csInfo.push_back(info);
      }
      str = vpcopt[nc].allValue;//tokens[0];
      shadingComboBox->setCurrentIndex(i + 1);
      updateFieldOptions("palettecolours", str, -1);
    }
  }
  //pattern
  if ((nc = cp->findKey(vpcopt, "patterns")) >= 0) {
    miutil::miString value = vpcopt[nc].allValue;
    int nr_p = patternInfo.size();
    miutil::miString str;
    i = 0;
    while (i < nr_p && value != patternInfo[i].name)
      i++;
    if (i == nr_p) {
      str = "off";
      patternComboBox->setCurrentIndex(0);
    } else {
      str = patternInfo[i].name;
      patternComboBox->setCurrentIndex(i + 1);
    }
    updateFieldOptions("patterns", str, -1);
  } else {
    updateFieldOptions("patterns", "off", -1);
    patternComboBox->setCurrentIndex(0);
  }

  //pattern colour
  if ((nc = cp->findKey(vpcopt, "patterncolour")) >= 0) {
    int nr_colours = patternColourBox->count();
    QString col = vpcopt[nc].allValue.c_str();
    i = 0;
    while (i < nr_colours && col != patternColourBox->itemText(i) )
      i++;
    if (i == nr_colours) {
      Colour::defineColourFromString(vpcopt[nc].allValue);
      ExpandColourBox(patternColourBox,Colour(vpcopt[nc].allValue));
    }
    updateFieldOptions("patterncolour", vpcopt[nc].allValue);
    patternColourBox->setCurrentIndex(i);
  }

  //table
  nc = cp->findKey(vpcopt, "table");
  if (nc >= 0) {
    bool on = vpcopt[nc].allValue == "1";
    tableCheckBox->setChecked(on);
    tableCheckBoxToggled(on);
  }

  //repeat
  nc = cp->findKey(vpcopt, "repeat");
  if (nc >= 0) {
    bool on = vpcopt[nc].allValue == "1";
    repeatCheckBox->setChecked(on);
    repeatCheckBoxToggled(on);
  }

  //alpha shading
  if ((nc = cp->findKey(vpcopt, "alpha")) >= 0) {
    if (!vpcopt[nc].intValue.empty())
      i = vpcopt[nc].intValue[0];
    else
      i = 255;
    alphaSpinBox->setValue(i);
    alphaSpinBox->setEnabled(true);
  }

  // linetype
  if ((nc = cp->findKey(vpcopt, "linetype")) >= 0) {
    i = 0;
    while (i < nr_linetypes && vpcopt[nc].allValue != linetypes[i])
      i++;
    if (i == nr_linetypes)
      i = 0;
    updateFieldOptions("linetype", linetypes[i]);
    lineTypeCbox->setCurrentIndex(i);
    if ((nc = cp->findKey(vpcopt, "linetype_2")) >= 0) {
      i = 0;
      while (i < nr_linetypes && vpcopt[nc].allValue != linetypes[i])
        i++;
      if (i == nr_linetypes)
        i = 0;
      updateFieldOptions("linetype_2", linetypes[i]);
      linetype2ComboBox->setCurrentIndex(i);
    } else {
      linetype2ComboBox->setCurrentIndex(0);
    }
  }

  // linewidth
  if ((nc = cp->findKey(vpcopt, "linewidth")) >= 0  && !vpcopt[nc].intValue.empty()) {
    i = vpcopt[nc].intValue[0];
    int nr_linewidths = lineWidthCbox->count();
    if (  i  > nr_linewidths )  {
      ExpandLinewidthBox(lineWidthCbox, i);
    }
    updateFieldOptions("linewidth", miutil::miString(i));
    lineWidthCbox->setCurrentIndex(i-1);
  }

  if ((nc = cp->findKey(vpcopt, "linewidth_2")) >= 0  && !vpcopt[nc].intValue.empty()) {
    int nr_linewidths = linewidth2ComboBox->count();
    i = vpcopt[nc].intValue[0];
    if ( i  > nr_linewidths )  {
      ExpandLinewidthBox(linewidth2ComboBox, i);
    }
    updateFieldOptions("linewidth_2", miutil::miString(i));
    linewidth2ComboBox->setCurrentIndex(i-1);
  }


  // line interval (isoline contouring)
  if ((nc = cp->findKey(vpcopt, "line.interval")) >= 0 || (nc = cp->findKey(
      vpcopt, "line.values")) >= 0) {
    if ((nc = cp->findKey(vpcopt, "line.interval")) >= 0
        && (!vpcopt[nc].floatValue.empty())) {
      float ekv = vpcopt[nc].floatValue[0];
      lineintervals = numberList(lineintervalCbox, ekv);
      numberList(interval2ComboBox, ekv);
    }
    if ((nc = cp->findKey(vpcopt, "line.interval_2")) >= 0) {
      if (!vpcopt[nc].floatValue.empty()){
        float ekv = vpcopt[nc].floatValue[0];
        numberList(interval2ComboBox, ekv);
      }
    }
  }

  // wind/vector density
  if ((nc = cp->findKey(vpcopt, "density")) >= 0) {
    miutil::miString s;
    if (!vpcopt[nc].strValue.empty()) {
      s = vpcopt[nc].strValue[0];
    } else {
      s = "0";
      updateFieldOptions("density", s);
    }
    if (s == "0") {
      i = 0;
    } else {
      i = densityStringList.indexOf(QString(s.cStr()));
      if (i == -1) {
        densityStringList << QString(s.cStr());
        densityCbox->addItem(QString(s.cStr()));
        i = densityCbox->count() - 1;
      }
    }
    densityCbox->setCurrentIndex(i);
  }

  // vectorunit (vector length unit)
  if ((nc = cp->findKey(vpcopt, "vector.unit")) >= 0) {
    float e;
    if (!vpcopt[nc].floatValue.empty())
      e = vpcopt[nc].floatValue[0];
    else
      e = 5;
    vectorunit = numberList(vectorunitCbox, e);
  }

  // extreme.type (L+H, C+W or none)
  if ((nc = cp->findKey(vpcopt, "extreme.type")) >= 0) {
    i = 0;
    n= extremeType.size();
    while (i < n && vpcopt[nc].allValue != extremeType[i]) {
      i++;
    }
    if (i == n) {
      i = 0;
    }
    updateFieldOptions("extreme.type", extremeType[i]);
    extremeTypeCbox->setCurrentIndex(i);
  }

  if ((nc = cp->findKey(vpcopt, "extreme.size")) >= 0) {
    float e;
    if (!vpcopt[nc].floatValue.empty()) {
      e = vpcopt[nc].floatValue[0];
    } else {
      e = 1.0;
    }
    i = (int(e * 100. + 0.5)) / 5 * 5;
    extremeSizeSpinBox->setValue(i);
  }

  if ((nc = cp->findKey(vpcopt, "extreme.radius")) >= 0) {
    float e;
    if (!vpcopt[nc].floatValue.empty()) {
      e = vpcopt[nc].floatValue[0];
    } else {
      e = 1.0;
    }
    i = (int(e * 100. + 0.5)) / 5 * 5;
    extremeRadiusSpinBox->setValue(i);
  }

  if ((nc = cp->findKey(vpcopt, "line.smooth")) >= 0 && !vpcopt[nc].intValue.empty()) {
    i = vpcopt[nc].intValue[0];
    lineSmoothSpinBox->setValue(i);
  } else {
    lineSmoothSpinBox->setValue(0);
  }

  if (currentFieldOptsInEdit) {
    fieldSmoothSpinBox->setValue(0);
    fieldSmoothSpinBox->setEnabled(false);
  } else if ((nc = cp->findKey(vpcopt, "field.smooth")) >= 0) {
    if (!vpcopt[nc].intValue.empty())
      i = vpcopt[nc].intValue[0];
    else
      i = 0;
    fieldSmoothSpinBox->setValue(i);
  } else if (fieldSmoothSpinBox->isEnabled()) {
    fieldSmoothSpinBox->setValue(0);
  }

  if ((nc = cp->findKey(vpcopt, "label.size")) >= 0) {
    float e;
    if (!vpcopt[nc].floatValue.empty())
      e = vpcopt[nc].floatValue[0];
    else
      e = 1.0;
    i = (int(e * 100. + 0.5)) / 5 * 5;
    labelSizeSpinBox->setValue(i);
  } else if (labelSizeSpinBox->isEnabled()) {
    labelSizeSpinBox->setValue(100);
    labelSizeSpinBox->setEnabled(false);
  }

  if ((nc = cp->findKey(vpcopt, "precision")) >= 0) {
    if (!vpcopt[nc].floatValue.empty() && vpcopt[nc].floatValue[0] < valuePrecisionBox->count() ) {
      valuePrecisionBox->setCurrentIndex(vpcopt[nc].floatValue[0]);
    } else {
      valuePrecisionBox->setCurrentIndex(0);
    }
  }

  nc = cp->findKey(vpcopt, "grid.value");
  if (nc >= 0) {
    if (vpcopt[nc].allValue == "-1") {
      nc = -1;
    } else {
      bool on = vpcopt[nc].allValue == "1";
      gridValueCheckBox->setChecked(on);
      gridValueCheckBox->setEnabled(true);
    }
  }
  if (nc < 0 && gridValueCheckBox->isEnabled()) {
    gridValueCheckBox->setChecked(false);
    gridValueCheckBox->setEnabled(false);
  }

  if ((nc = cp->findKey(vpcopt, "grid.lines")) >= 0) {
    if (!vpcopt[nc].intValue.empty())
      i = vpcopt[nc].intValue[0];
    else
      i = 0;
    gridLinesSpinBox->setValue(i);
  }

  //   if ((nc=cp->findKey(vpcopt,"grid.lines.max"))>=0) {
  //     if (!vpcopt[nc].intValue.empty()) i=vpcopt[nc].intValue[0];
  //     else i=0;
  //     gridLinesMaxSpinBox->setValue(i);
  //     gridLinesMaxSpinBox->setEnabled(true);
  //   } else if (gridLinesMaxSpinBox->isEnabled()) {
  //     gridLinesMaxSpinBox->setValue(0);
  //     gridLinesMaxSpinBox->setEnabled(false);
  //   }

  // undefined masking
  int iumask = 0;
  if ((nc = cp->findKey(vpcopt, "undef.masking")) >= 0) {
    if (vpcopt[nc].intValue.size() == 1) {
      iumask = vpcopt[nc].intValue[0];
      if (iumask < 0 || iumask >= int(undefMasking.size()))
        iumask = 0;
    } else {
      iumask = 0;
    }
    undefMaskingCbox->setCurrentIndex(iumask);
    undefMaskingActivated(iumask);
  }

  // undefined masking colour
  if ((nc = cp->findKey(vpcopt, "undef.colour")) >= 0) {
    int nr_colours = undefColourCbox->count();
    QString col = vpcopt[nc].allValue.c_str();
    i = 0;
    while (i < nr_colours && col != undefColourCbox->itemText(i) )
      i++;
    if (i == nr_colours) {
      Colour::defineColourFromString(vpcopt[nc].allValue);
      ExpandColourBox(undefColourCbox,Colour(vpcopt[nc].allValue));
    }
    updateFieldOptions("undef.colour", vpcopt[nc].allValue);
    undefColourCbox->setCurrentIndex(i);
  }

  // undefined masking linewidth
  if ((nc = cp->findKey(vpcopt, "undef.linewidth")) >= 0&& !vpcopt[nc].intValue.empty()) {
    int nr_linewidths = undefLinewidthCbox->count();
    i = vpcopt[nc].intValue[0];
    if ( i  > nr_linewidths )  {
      ExpandLinewidthBox(undefLinewidthCbox, i);
    }
    updateFieldOptions("undef.linewidth", miutil::miString(i));
    undefLinewidthCbox->setCurrentIndex(i-1);
  }

  // undefined masking linetype
  if ((nc = cp->findKey(vpcopt, "undef.linetype")) >= 0) {
    i = 0;
    while (i < nr_linetypes && vpcopt[nc].allValue != linetypes[i])
      i++;
    if (i == nr_linetypes) {
      i = 0;
      updateFieldOptions("undef.linetype", linetypes[i]);
    }
    undefLinetypeCbox->setCurrentIndex(i);
  }

  if ((nc = cp->findKey(vpcopt, "frame")) >= 0 ) {
    frameCheckBox->setChecked(vpcopt[nc].allValue != "0");
  }

  if ((nc = cp->findKey(vpcopt, "zero.line")) >= 0) {
    bool on = vpcopt[nc].allValue == "1";
    zeroLineCheckBox->setChecked(on);
  }

  if ((nc = cp->findKey(vpcopt, "value.label")) >= 0) {
    bool on = vpcopt[nc].allValue == "1";
    valueLabelCheckBox->setChecked(on);
  }

  // base
  if ((nc = cp->findKey(vpcopt, "base")) >= 0) {
    float e;
    if (!vpcopt[nc].floatValue.empty()){
      e = vpcopt[nc].floatValue[0];
      baseList(zero1ComboBox, e);
    }
    if ((nc = cp->findKey(vpcopt, "base_2")) >= 0) {
      if (!vpcopt[nc].floatValue.empty()) {
        e = vpcopt[nc].floatValue[0];
        baseList(zero2ComboBox, e);
      }
    }
  }

  if (( nc = cp->findKey(vpcopt, "minvalue")) >= 0 ) {
    if (vpcopt[nc].allValue != "off") {
      float value = atof(vpcopt[nc].allValue.cStr());
      baseList(min1ComboBox, value, true);
    }
  }

  if ((nc = cp->findKey(vpcopt, "minvalue_2")) >= 0) {
    if (vpcopt[nc].allValue != "off") {
      float value = atof(vpcopt[nc].allValue.cStr());
      baseList(min2ComboBox, value, true);
    }
  }

  if (( nc = cp->findKey(vpcopt, "maxvalue")) >= 0 ) {
    if (vpcopt[nc].allValue != "off") {
      float value = atof(vpcopt[nc].allValue.cStr());
      baseList(max1ComboBox, value, true);
    }
  }

  if ((nc = cp->findKey(vpcopt, "maxvalue_2")) >= 0) {
    if (vpcopt[nc].allValue != "off") {
      float value = atof(vpcopt[nc].allValue.cStr());
      baseList(max2ComboBox, value, true);
    }
  }


#ifdef DEBUGPRINT
  cerr<<"FieldDialog::enableFieldOptions returned"<<endl;
#endif
}

void FieldDialog::setDefaultFieldOptions()
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::setDefaultFieldOptions called"<<endl;
#endif

  // show levels for the current field group
//  setLevel();

  currentFieldOpts.clear();

  deleteButton->setEnabled( false );
  deleteAll->setEnabled( false );
  copyField->setEnabled( false );
  changeModelButton->setEnabled( false );
  upFieldButton->setEnabled( false );
  downFieldButton->setEnabled( false );
  resetOptionsButton->setEnabled( false );
  minusButton->setEnabled( false );

  unitLineEdit->clear();
  plottypeComboBox->setCurrentIndex(0);
  colorCbox->setCurrentIndex(1);
  fieldSmoothSpinBox->setValue(0);
  gridValueCheckBox->setChecked(false);
  gridLinesSpinBox->setValue(0);
  hourOffsetSpinBox->setValue(0);
  hourDiffSpinBox->setValue(0);
  undefMaskingCbox->setCurrentIndex(0);
  undefColourCbox->setCurrentIndex(1);
  undefLinewidthCbox->setCurrentIndex(0);
  undefLinetypeCbox->setCurrentIndex(0);
  frameCheckBox->setChecked(true);
  for (int i = 0; i < 3; i++) {
    threeColourBox[i]->setCurrentIndex(0);
  }

  lineTypeCbox->setCurrentIndex(0);
  lineSmoothSpinBox->setValue(0);
  zeroLineCheckBox->setChecked(true);
  colour2ComboBox->setCurrentIndex(0);
  interval2ComboBox->setCurrentIndex(0);
  linewidth2ComboBox->setCurrentIndex(0);
  linetype2ComboBox->setCurrentIndex(0);
  valueLabelCheckBox->setChecked(true);

  extremeTypeCbox->setCurrentIndex(0);
  extremeSizeSpinBox->setValue(100);
  extremeRadiusSpinBox->setValue(100);

//  lineintervalCbox->setCurrentIndex(0);
  tableCheckBox->setChecked(false);
  repeatCheckBox->
  setChecked(false);
  shadingComboBox->setCurrentIndex(0);
  shadingcoldComboBox->setCurrentIndex(0);
  shadingSpinBox->setValue(0);
  shadingcoldSpinBox->setValue(0);
  patternComboBox->setCurrentIndex(0);
  patternColourBox->setCurrentIndex(0);
  alphaSpinBox->setValue(0);

  lineWidthCbox->setCurrentIndex(0);
  labelSizeSpinBox->setValue(0);
  valuePrecisionBox->setCurrentIndex(0);

  densityCbox->setCurrentIndex(0);

  vectorunitCbox->setCurrentIndex(0);

  lineintervals = numberList(lineintervalCbox, 10);
  lineintervalCbox->setCurrentIndex(0);
  baseList(zero1ComboBox, 0);
  baseList(zero2ComboBox, 0);
  baseList(min2ComboBox, 0, true);
  baseList(min1ComboBox, 0, true);
  baseList(max1ComboBox, 0, true);
  baseList(max2ComboBox, 0, true);
  min1ComboBox->setCurrentIndex(0);
  min2ComboBox->setCurrentIndex(0);
  max1ComboBox->setCurrentIndex(0);
  max2ComboBox->setCurrentIndex(0);

  hourOffsetSpinBox->setValue(0);
  hourDiffSpinBox->setValue(0);



#ifdef DEBUGPRINT
  cerr<<"Leaving FieldDialog::setDefaultFieldOptions"<<endl;
#endif
}

void FieldDialog::enableWidgets(miutil::miString plottype)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::enableWidgets called:"<<plottype<<endl;
#endif
  bool enable = (plottype != "none");

  //used for all plottypes
  unitLineEdit->setEnabled(enable);
  plottypeComboBox->setEnabled(enable);
  colorCbox->setEnabled(enable);
  fieldSmoothSpinBox->setEnabled(enable);
  gridValueCheckBox->setEnabled(enable);
  gridLinesSpinBox->setEnabled(enable);
  hourOffsetSpinBox->setEnabled(enable);
  hourDiffSpinBox->setEnabled(enable);
  undefMaskingCbox->setEnabled(enable);
  undefColourCbox->setEnabled(enable);
  undefLinewidthCbox->setEnabled(enable);
  undefLinetypeCbox->setEnabled(enable);
  frameCheckBox->setEnabled(enable);
  zero1ComboBox->setEnabled(enable);
  min1ComboBox->setEnabled(enable);
  max1ComboBox->setEnabled(enable);
  for (int i = 0; i < 3; i++) {
    threeColourBox[i]->setEnabled(enable);
  }

  enable= enableMap[plottype].contourWidgets;
  lineTypeCbox->setEnabled(enable);
  lineSmoothSpinBox->setEnabled(enable);
  zeroLineCheckBox->setEnabled(enable);
  colour2ComboBox->setEnabled(enable);
  interval2ComboBox->setEnabled(enable);
  zero2ComboBox->setEnabled(enable);
  min2ComboBox->setEnabled(enable);
  max2ComboBox->setEnabled(enable);
  linewidth2ComboBox->setEnabled(enable);
  linetype2ComboBox->setEnabled(enable);
  valueLabelCheckBox->setEnabled(enable);

  enable =  enableMap[plottype].extremeWidgets;
  extremeTypeCbox->setEnabled(enable);
  extremeSizeSpinBox->setEnabled(enable);
  extremeRadiusSpinBox->setEnabled(enable);

  enable =  enableMap[plottype].shadingWidgets;
  lineintervalCbox->setEnabled(enable);
  tableCheckBox->setEnabled(enable);
  repeatCheckBox->setEnabled(enable);
  shadingComboBox->setEnabled(enable);
  shadingcoldComboBox->setEnabled(enable);
  shadingSpinBox->setEnabled(enable);
  shadingcoldSpinBox->setEnabled(enable);
  patternComboBox->setEnabled(enable);
  patternColourBox->setEnabled(enable);
  alphaSpinBox->setEnabled(enable);

  enable = enableMap[plottype].lineWidgets;
  lineWidthCbox->setEnabled(enable);

  enable = enableMap[plottype].fontWidgets;
  labelSizeSpinBox->setEnabled(enable);
  valuePrecisionBox->setEnabled(enable);

  enable = enableMap[plottype].densityWidgets;
  densityCbox->setEnabled(enable);

  enable = enableMap[plottype].unitWidgets;
  vectorunitCbox->setEnabled(enable);


#ifdef DEBUGPRINT
  cerr<<"FieldDialog::enableWidgets returned"<<endl;
#endif
}

vector<miutil::miString> FieldDialog::numberList(QComboBox* cBox, float number)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::numberList called: "<<number<<endl;
#endif

  cBox->clear();

  vector<miutil::miString> vnumber;

  const int nenormal = 18;
  const float enormal[nenormal] =
  { 1., 1.5,2., 2.5, 3.,3.5, 4.,4.5, 5.,5.5, 6.,6.5, 7.,7.5, 8.,8.5, 9.,9.5 };
  float e, elog, ex, d, dd;
  int i, j, k, n, ielog, nupdown;

  e = number;
  if (e <= 0)
    e = 1.0;
  elog = log10f(e);
  if (elog >= 0.)
    ielog = int(elog);
  else
    ielog = int(elog - 0.99999);
  ex = powf(10., ielog);
  n = 0;
  d = fabsf(e - enormal[0] * ex);
  for (i = 1; i < nenormal; ++i) {
    dd = fabsf(e - enormal[i] * ex);
    if (d > dd) {
      d = dd;
      n = i;
    }
  }
  nupdown = nenormal * 2 / 3;
  vnumber.push_back("off");
  for (i = n - nupdown; i <= n + nupdown; ++i) {
    j = i / nenormal;
    k = i % nenormal;
    if (i < 0)
      j--;
    if (k < 0)
      k += nenormal;
    ex = powf(10., ielog + j);
    vnumber.push_back(miutil::miString(enormal[k] * ex));
  }
  n = 1 + nupdown * 2;

  QString qs;
  for (i = 0; i < n; ++i) {
    cBox->addItem(QString(vnumber[i].cStr()));
  }

  cBox->setCurrentIndex(nupdown + 1);

  return vnumber;
}

void FieldDialog::baseList(QComboBox* cBox, float base, bool onoff)
{

  float ekv=10;
  if (lineintervalCbox->currentIndex() >0 && !lineintervalCbox->currentText().isNull()) {
    ekv = atof(lineintervalCbox->currentText().toAscii());
  }

  int n;
  if (base < 0.)
    n = int(base / ekv - 0.5);
  else
    n = int(base / ekv + 0.5);
  if (fabsf(base - ekv * float(n)) > 0.01 * ekv) {
    base = ekv * float(n);
  }
  n = 21;
  int k = n / 2;
  int j = -k - 1;

  cBox->clear();

  if (onoff)
    cBox->addItem(tr("Off"));

  for (int i = 0; i < n; ++i) {
    j++;
    float e = base + ekv * float(j);
    if (fabs(e) < ekv / 2)
      cBox->addItem("0");
    else {
      miutil::miString estr(e);
      cBox->addItem(estr.cStr());
    }
  }

  if (onoff)
    cBox->setCurrentIndex(k + 1);
  else
    cBox->setCurrentIndex(k);

}

void FieldDialog::selectedFieldboxClicked(QListWidgetItem * item)
{
  int index = selectedFieldbox->row(item);

  // may get here when there is none selected fields (the last is removed)
  if (index < 0 || selectedFields.size() == 0)
    return;

  enableFieldOptions();

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::selectedFieldboxClicked returned"<<endl;
#endif
  return;
}

void FieldDialog::unitEditingFinished()
{
  updateFieldOptions("unit", unitLineEdit->text().toStdString());
}

void FieldDialog::plottypeComboBoxActivated(int index)
{
  updateFieldOptions("plottype", plottypes[index]);
  enableWidgets(plottypes[index]);
}

void FieldDialog::colorCboxActivated(int index)
{
  if (index == 0)
    updateFieldOptions("colour", "off");
  else
    updateFieldOptions("colour",colorCbox->currentText().toStdString());
}

void FieldDialog::lineWidthCboxActivated(int index)
{
  updateFieldOptions("linewidth", miutil::miString(index + 1));
}

void FieldDialog::lineTypeCboxActivated(int index)
{
  updateFieldOptions("linetype", linetypes[index]);
}

void FieldDialog::lineintervalCboxActivated(int index)
{
  if (index == 0) {
    updateFieldOptions("line.interval", "remove");
  } else {
    updateFieldOptions("line.interval", lineintervals[index]);
    // update the list (with selected value in the middle)
    float a = atof(lineintervals[index].c_str());
    lineintervals = numberList(lineintervalCbox, a);
  }
}

void FieldDialog::densityCboxActivated(int index)
{
  if (index == 0)
    updateFieldOptions("density", "0");
  else
    updateFieldOptions("density", densityCbox->currentText().toStdString());
}

void FieldDialog::vectorunitCboxActivated(int index)
{
  updateFieldOptions("vector.unit", vectorunit[index]);
  // update the list (with selected value in the middle)
  float a = atof(vectorunit[index].c_str());
  vectorunit = numberList(vectorunitCbox, a);
}

void FieldDialog::extremeTypeActivated(int index)
{
  updateFieldOptions("extreme.type", extremeType[index]);
}

void FieldDialog::extremeSizeChanged(int value)
{
  miutil::miString str = miutil::miString(float(value) * 0.01);
  updateFieldOptions("extreme.size", str);
}

void FieldDialog::extremeRadiusChanged(int value)
{
  miutil::miString str = miutil::miString(float(value) * 0.01);
  updateFieldOptions("extreme.radius", str);
}

void FieldDialog::lineSmoothChanged(int value)
{
  miutil::miString str = miutil::miString(value);
  updateFieldOptions("line.smooth", str);
}

void FieldDialog::fieldSmoothChanged(int value)
{
  miutil::miString str = miutil::miString(value);
  updateFieldOptions("field.smooth", str);
}

void FieldDialog::labelSizeChanged(int value)
{
  miutil::miString str = miutil::miString(float(value) * 0.01);
  updateFieldOptions("label.size", str);
}

void FieldDialog::valuePrecisionBoxActivated( int index )
{
  miutil::miString str = miutil::miString(index);
  updateFieldOptions("precision", str);
}


void FieldDialog::gridValueCheckBoxToggled(bool on)
{
  if (on)
    updateFieldOptions("grid.value", "1");
  else
    updateFieldOptions("grid.value", "0");
}

void FieldDialog::gridLinesChanged(int value)
{
  miutil::miString str = miutil::miString(value);
  updateFieldOptions("grid.lines", str);
}

// void FieldDialog::gridLinesMaxChanged(int value)
// {
//   miutil::miString str= miutil::miString( value );
//   updateFieldOptions("grid.lines.max",str);
// }


void FieldDialog::hourOffsetChanged(int value)
{
  int n = selectedFieldbox->currentRow();
  selectedFields[n].hourOffset = value;
  updateTime();
}

void FieldDialog::hourDiffChanged(int value)
{
  int n = selectedFieldbox->currentRow();
  selectedFields[n].hourDiff = value;
  updateTime();
}

void FieldDialog::undefMaskingActivated(int index)
{
  updateFieldOptions("undef.masking", miutil::miString(index));
  undefColourCbox->setEnabled(index > 0);
  undefLinewidthCbox->setEnabled(index > 1);
  undefLinetypeCbox->setEnabled(index > 1);
}

void FieldDialog::undefColourActivated(int index)
{
  updateFieldOptions("undef.colour", undefColourCbox->currentText().toStdString());
}

void FieldDialog::undefLinewidthActivated(int index)
{
  updateFieldOptions("undef.linewidth", miutil::miString(index + 1));
}

void FieldDialog::undefLinetypeActivated(int index)
{
  updateFieldOptions("undef.linetype", linetypes[index]);
}

void FieldDialog::frameCheckBoxToggled(bool on)
{
  if (on)
    updateFieldOptions("frame", "1");
  else
    updateFieldOptions("frame", "0");
}

void FieldDialog::zeroLineCheckBoxToggled(bool on)
{
  if (on)
    updateFieldOptions("zero.line", "1");
  else
    updateFieldOptions("zero.line", "0");
}

void FieldDialog::valueLabelCheckBoxToggled(bool on)
{
  if (on)
    updateFieldOptions("value.label", "1");
  else
    updateFieldOptions("value.label", "0");
}

void FieldDialog::colour2ComboBoxToggled(int index)
{
  if (index == 0) {
    updateFieldOptions("colour_2", "off");
    enableType2Options(false);
    colour2ComboBox->setEnabled(true);
  } else {
    updateFieldOptions("colour_2", colour2ComboBox->currentText().toStdString());
    enableType2Options(true); //check if needed
    //turn of 3 colours (not possible to combine threeCols and col_2)
    threeColourBox[0]->setCurrentIndex(0);
    threeColoursChanged();
  }
}

void FieldDialog::tableCheckBoxToggled(bool on)
{
  if (on)
    updateFieldOptions("table", "1");
  else
    updateFieldOptions("table", "0");
}

void FieldDialog::patternComboBoxToggled(int index)
{
  if (index == 0) {
    updateFieldOptions("patterns", "off");
  } else {
    updateFieldOptions("patterns", patternInfo[index - 1].name);
  }
  updatePaletteString();
}

void FieldDialog::patternColourBoxToggled(int index)
{
  if (index == 0) {
    updateFieldOptions("patterncolour", "remove");
  } else {
    updateFieldOptions("patterncolour", patternColourBox->currentText().toStdString());
  }
  updatePaletteString();
}

void FieldDialog::repeatCheckBoxToggled(bool on)
{
  if (on)
    updateFieldOptions("repeat", "1");
  else
    updateFieldOptions("repeat", "0");
}

void FieldDialog::threeColoursChanged()
{

  if (threeColourBox[0]->currentIndex() == 0
      || threeColourBox[1]->currentIndex() == 0
      || threeColourBox[2]->currentIndex() == 0) {

    updateFieldOptions("colours", "remove");

  } else {

    //turn of colour_2 (not possible to combine threeCols and col_2)
    colour2ComboBox->setCurrentIndex(0);
    colour2ComboBoxToggled(0);

    miutil::miString str = miutil::miString(threeColourBox[0]->currentText().toStdString()) + ","
        + threeColourBox[1]->currentText().toStdString() + "," + threeColourBox[2]->currentText().toStdString();

    updateFieldOptions("colours", "remove");
    updateFieldOptions("colours", str);
  }
}

void FieldDialog::shadingChanged()
{
  updatePaletteString();
}

void FieldDialog::updatePaletteString()
{
  if (patternComboBox->currentIndex() > 0 && patternColourBox->currentIndex()
      > 0) {
    updateFieldOptions("palettecolours", "off", -1);
    return;
  }

  int index1 = shadingComboBox->currentIndex();
  int index2 = shadingcoldComboBox->currentIndex();
  int value1 = shadingSpinBox->value();
  int value2 = shadingcoldSpinBox->value();

  if (index1 == 0 && index2 == 0) {
    updateFieldOptions("palettecolours", "off", -1);
    return;
  }

  miutil::miString str;
  if (index1 > 0) {
    str = csInfo[index1 - 1].name;
    if (value1 > 0)
      str += ";" + miutil::miString(value1);
    if (index2 > 0)
      str += ",";
  }
  if (index2 > 0) {
    str += csInfo[index2 - 1].name;
    if (value2 > 0)
      str += ";" + miutil::miString(value2);
  }
  updateFieldOptions("palettecolours", str, -1);
}

void FieldDialog::alphaChanged(int index)
{
  updateFieldOptions("alpha", miutil::miString(index));
}

void FieldDialog::interval2ComboBoxToggled(int index)
{
  miutil::miString str = interval2ComboBox->currentText().toStdString();
  updateFieldOptions("line.interval_2", str);
  // update the list (with selected value in the middle)
  float a = atof(str.c_str());
  numberList(interval2ComboBox, a);
}

void FieldDialog::zero1ComboBoxToggled(int index)
{
  if (!zero1ComboBox->currentText().isNull()) {
    miutil::miString str = zero1ComboBox->currentText().toStdString();
    float a = atof(str.cStr());
    baseList(zero1ComboBox, a);
    str = zero1ComboBox->currentText().toStdString();
    updateFieldOptions("base", str);
  }
}

void FieldDialog::zero2ComboBoxToggled(int index)
{
  if (!zero2ComboBox->currentText().isNull()) {
    miutil::miString str = zero2ComboBox->currentText().toStdString();
    updateFieldOptions("base_2", str);
    float a = atof(str.cStr());
    str = zero2ComboBox->currentText().toStdString();
    baseList(zero2ComboBox, a);
  }
}

void FieldDialog::min1ComboBoxToggled(int index)
{
  if (index == 0)
    updateFieldOptions("minvalue", "off");
  else if (!min1ComboBox->currentText().isNull()) {
    miutil::miString str = min1ComboBox->currentText().toStdString();
    float a = atof(str.cStr());
    baseList(min1ComboBox, a, true);
    updateFieldOptions("minvalue", min1ComboBox->currentText().toStdString());
  }
}

void FieldDialog::max1ComboBoxToggled(int index)
{
  if (index == 0)
    updateFieldOptions("maxvalue", "off");
  else if (!max1ComboBox->currentText().isNull()) {
    miutil::miString str = max1ComboBox->currentText().toStdString();
    float a = atof(str.cStr());
    baseList(max1ComboBox, a, true);
    updateFieldOptions("maxvalue", max1ComboBox->currentText().toStdString());
  }
}

void FieldDialog::min2ComboBoxToggled(int index)
{

  if (index == 0)
    updateFieldOptions("minvalue_2", "remove");
  else if (!min2ComboBox->currentText().isNull()) {
    miutil::miString str = min2ComboBox->currentText().toStdString();
    float a = atof(str.cStr());
    baseList(min2ComboBox, a, true);
    updateFieldOptions("minvalue_2", min2ComboBox->currentText().toStdString());
  }
}

void FieldDialog::max2ComboBoxToggled(int index)
{
  if (index == 0)
    updateFieldOptions("maxvalue_2", "remove");
  else if (!max2ComboBox->currentText().isNull()) {
    miutil::miString str = max2ComboBox->currentText().toStdString();
    float a = atof(str.cStr());
    baseList(max2ComboBox, a, true);
    updateFieldOptions("maxvalue_2", max2ComboBox->currentText().toStdString());
  }
}

void FieldDialog::linewidth1ComboBoxToggled(int index)
{
  lineWidthCbox->setCurrentIndex(index);
  updateFieldOptions("linewidth", miutil::miString(index + 1));
}

void FieldDialog::linewidth2ComboBoxToggled(int index)
{
  updateFieldOptions("linewidth_2", miutil::miString(index + 1));
}

void FieldDialog::linetype1ComboBoxToggled(int index)
{
  lineTypeCbox->setCurrentIndex(index);
  updateFieldOptions("linetype", linetypes[index]);
}

void FieldDialog::linetype2ComboBoxToggled(int index)
{
  updateFieldOptions("linetype_2", linetypes[index]);
}

void FieldDialog::enableType2Options(bool on)
{
  colour2ComboBox->setEnabled(on);

  //enable the rest only if colour2 is on
  on = (colour2ComboBox->currentIndex() != 0);

  interval2ComboBox->setEnabled(on);
  zero2ComboBox->setEnabled(on);
  min2ComboBox->setEnabled(on);
  max2ComboBox->setEnabled(on);
  linewidth2ComboBox->setEnabled(on);
  linetype2ComboBox->setEnabled(on);

  if (on) {
    if (!interval2ComboBox->currentText().isNull())
      updateFieldOptions("line.interval_2",
          interval2ComboBox->currentText().toStdString());
    if (!zero2ComboBox->currentText().isNull())
      updateFieldOptions("base_2", zero2ComboBox->currentText().toStdString());
    if (!min2ComboBox->currentText().isNull() && min2ComboBox->currentIndex()
        > 0)
      updateFieldOptions("minvalue_2",
          min2ComboBox->currentText().toStdString());
    if (!max2ComboBox->currentText().isNull() && max2ComboBox->currentIndex()
        > 0)
      updateFieldOptions("maxvalue_2",
          max2ComboBox->currentText().toStdString());
    updateFieldOptions("linewidth_2", miutil::miString(
        linewidth2ComboBox->currentIndex() + 1));
    updateFieldOptions("linetype_2",
        linetypes[linetype2ComboBox->currentIndex()]);
  } else {
    colour2ComboBox->setCurrentIndex(0);
    updateFieldOptions("colour_2", "off");
    updateFieldOptions("line.interval_2", "remove");
    updateFieldOptions("base_2", "remove");
    updateFieldOptions("value.range_2", "remove");
    updateFieldOptions("linewidth_2", "remove");
    updateFieldOptions("linetype_2", "remove");
  }
}

void FieldDialog::updateFieldOptions(const miutil::miString& name,
    const miutil::miString& value, int valueIndex)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::updateFieldOptions  name= " << name
      << "  value= " << value <<endl;
#endif

  if (currentFieldOpts.empty())
    return;

  int n = selectedFieldbox->currentRow();

  if (value == "remove")
    cp->removeValue(vpcopt, name);
  else
    cp->replaceValue(vpcopt, name, value, valueIndex);

  currentFieldOpts = cp->unParse(vpcopt);
  selectedFields[n].fieldOpts = currentFieldOpts;

  // not update private settings if external/QuickMenu command...
  if (!selectedFields[n].external) {
    if (selectedFields[n].inEdit && !selectedFields[n].editPlot) {
      editFieldOptions[selectedFields[n].fieldName]
                       = currentFieldOpts;
    } else {
      fieldOptions[selectedFields[n].fieldName] = currentFieldOpts;
    }
  }

}

void FieldDialog::getFieldGroups(const miutil::miString& model, const std::string& refTime, int& indexMGR,
    int& indexM, bool plotOptions, vector<FieldGroupInfo>& vfg)
{


  miutil::miString modelName;

  QApplication::setOverrideCursor(Qt::WaitCursor);

  m_ctrl->getFieldGroups(model, modelName, refTime, plotOptions, vfg);

  QApplication::restoreOverrideCursor();

  if (indexMGR >= 0 && indexM >= 0) {
    // field groups will be shown, translate the name parts
    map<miutil::miString, miutil::miString>::const_iterator pt, ptend =
        fgTranslations.end();
    size_t pos;
    for (unsigned int n = 0; n < vfg.size(); n++) {
      for (pt = fgTranslations.begin(); pt != ptend; pt++) {
        if ((pos = vfg[n].groupName.find(pt->first)) != string::npos)
          vfg[n].groupName.string::replace(pos, pt->first.size(), pt->second);
      }
    }
  } else {
    int i, n, ng = m_modelgroup.size();
    indexMGR = 0;
    indexM = -1;
    while (indexMGR < ng && indexM < 0) {
      n = m_modelgroup[indexMGR].modelNames.size();
      i = 0;
      modelName = modelName;
      while (i < n && modelName != m_modelgroup[indexMGR].modelNames[i]) {

        //        cout << " getFieldGroups, checking group:" << indexMGR << " model:"
        //            << m_modelgroup[indexMGR].modelNames[i] << " against " << modelName
        //            << endl;

        i++;
      }
      if (i < n)
        indexM = i;
      else
        indexMGR++;
    }
    if (indexMGR == ng) {
      indexMGR = -1;
      vfg.clear();
    }
  }
}

vector<miutil::miString> FieldDialog::getOKString( bool resetLevelMove )
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::getOKString called"<<endl;
#endif

  if ( resetLevelMove) {
    int n = selectedFields.size();
    for (int i = 0; i < n; i++) {
      selectedFields[i].levelmove = true;
      selectedFields[i].idnummove = true;
    }
  }

  if (historyOkButton->isEnabled())
    historyOk();

  vector<miutil::miString> vstr;
  if (selectedFields.size() == 0)
    return vstr;

  bool allTimeSteps = allTimeStepButton->isChecked();

  vector<miutil::miString> hstr;

  int n = selectedFields.size();

  for (int i = 0; i < n; i++) {

    //Skip edit strings when the strings are used to change level
    if (!resetLevelMove && selectedFields[i].inEdit && selectedFields[i].editPlot) {
      continue;
    }

    ostringstream ostr;

    if (selectedFields[i].minus)
      continue;
    bool minus = false;
    if (i + 1 < n && selectedFields[i + 1].minus) {
      minus = true;
      ostr << "( ";
    }

    ostr << getParamString(i);
    if (minus) {
      ostr << " - ";
      ostr << getParamString(i+1);
      ostr << " )";
    }

    ostr << " " << selectedFields[i].fieldOpts;

    // for local and global history and for QuickMenues
    if (allTimeSteps)
      ostr << " allTimeSteps=on";

    if (selectedFields[i].inEdit && !selectedFields[i].editPlot) {
      ostr << " overlay=1";
    }

    if (selectedFields[i].time.exists()) {
      ostr << " time=" << selectedFields[i].time;
    }

    miutil::miString str;

    if (selectedFields[i].inEdit && selectedFields[i].editPlot) {

      str = "EDITFIELD " + ostr.str();

    } else {

      str = "FIELD " + ostr.str();

      // the History string (but not if quickmenu command)
      if (!selectedFields[i].external)
        hstr.push_back(ostr.str());

    }

    // the OK string
    vstr.push_back(str);

    //#############################################################
    //cerr << "OK: " << str << endl;
    //#############################################################
  }

  // could check if a previous equal command should be deleted...
  // check if previous command was equal (the easiest...)
  if (hstr.size() > 0) {
    bool newcommand = true;
    int hs = commandHistory.size();
    if (hs > 0) {
      hs--;
      if (commandHistory[hs].size() == hstr.size()) {
        newcommand = false;
        n = hstr.size();
        for (int i = 0; i < n; i++)
          if (commandHistory[hs][i] != hstr[i])
            newcommand = true;
      }
    }
    if (newcommand)
      commandHistory.push_back(hstr);
    //###############################################################
    //if (newcommand) cerr << "NEW COMMAND !!!!!!!!!!!!!" << endl;
    //else            cerr << "SAME COMMAND !!!!!!!!!!!!!" << endl;
    //###############################################################
  }
  //###############################################################
  //if (hstr.size()>0) {
  //  cerr << "OK-----------------------------" << endl;
  //  n= hstr.size();
  //  bool eq;
  //  vector< vector<miutil::miString> >::iterator p, pend= commandHistory.end();
  //  for (p=commandHistory.begin(); p!=pend; p++) {
  //    if (p->size()==n) {
  //	eq= true;
  //	for (i=0; i<n; i++)
  //	  if (p[i]!=hstr[i]) eq= false;
  //	if (eq) cerr << "FOUND" << endl;
  //  }
  //  }
  //  cerr << "-------------------------------" << endl;
  //}
  //###############################################################

  historyPos = commandHistory.size();

  historyBackButton->setEnabled(true);
  historyForwardButton->setEnabled(false);

  return vstr;
}

std::string FieldDialog::getParamString(int i)
{

  ostringstream ostr;

  if( selectedFields[i].cdmSyntax ) {
    if (selectedFields[i].inEdit && selectedFields[i].editPlot)
      ostr << " model="<< editName;
    else
      ostr <<" model="<< selectedFields[i].modelName;

    if ( !selectedFields[i].refTime.empty() ) {
      ostr <<" reftime="<<selectedFields[i].refTime;
    }

    if (selectedFields[i].plotDefinition) {
      ostr << " plot=" << selectedFields[i].fieldName;
    } else {
      ostr << " parameter=" << selectedFields[i].fieldName;
    }

    if (selectedFields[i].level.exists()) {
      ostr << " vcoord=" << selectedFields[i].zaxis;
      ostr << " vlevel=" << selectedFields[i].level;
    }
    if (selectedFields[i].idnum.exists()) {
      ostr << " ecoor="<< selectedFields[i].extraaxis;
      ostr << " elevel=" << selectedFields[i].idnum;
    }
    //    if (selectedFields[i].grid.exists()) {
    //ostr << " grid="<< selectedFields[i].grid;
    //    }
    if (selectedFields[i].hourOffset != 0)
      ostr << " hour.offset=" << selectedFields[i].hourOffset;

    if (selectedFields[i].hourDiff != 0)
      ostr << " hour.diff=" << selectedFields[i].hourDiff;

  } else {

    if (selectedFields[i].inEdit && selectedFields[i].editPlot)
      ostr << editName;
    else
      ostr << selectedFields[i].modelName;

    ostr << " " << selectedFields[i].fieldName;

    if (selectedFields[i].level.exists())
      ostr << " level=" << selectedFields[i].level;
    if (selectedFields[i].idnum.exists())
      ostr << " idnum=" << selectedFields[i].idnum;

    if (selectedFields[i].hourOffset != 0)
      ostr << " hour.offset=" << selectedFields[i].hourOffset;

    if (selectedFields[i].hourDiff != 0)
      ostr << " hour.diff=" << selectedFields[i].hourDiff;

  }

  return ostr.str();

}

miutil::miString FieldDialog::getShortname()
{
  // AC: simple version for testing...the shortname could perhaps
  // be made in getOKString?

  miutil::miString name;
  int n = selectedFields.size();
  ostringstream ostr;
  miutil::miString pmodelName;
  bool fielddiff = false, paramdiff = false, leveldiff = false;

  for (int i = numEditFields; i < n; i++) {
    miutil::miString modelName = selectedFields[i].modelName;
    miutil::miString fieldName = selectedFields[i].fieldName;
    miutil::miString level = selectedFields[i].level;
    miutil::miString idnum = selectedFields[i].idnum;

    //difference field
    if (i < n - 1 && selectedFields[i + 1].minus) {

      miutil::miString modelName2 = selectedFields[i + 1].modelName;
      miutil::miString fieldName2 = selectedFields[i + 1].fieldName;
      miutil::miString level_2 = selectedFields[i + 1].level;
      miutil::miString idnum_2 = selectedFields[i + 1].idnum;

      fielddiff = (modelName != modelName2);
      paramdiff = (fieldName != fieldName2);
      leveldiff = (!level.exists() || level != level_2 || idnum != idnum_2);

      if (modelName != pmodelName || modelName2 != pmodelName) {

        if (i > numEditFields)
          ostr << "  ";
        if (fielddiff)
          ostr << "( ";
        ostr << modelName;
        pmodelName = modelName;
      }

      if (!fielddiff && paramdiff)
        ostr << " ( ";
      if (!fielddiff || (fielddiff && paramdiff) || (fielddiff && leveldiff))
        ostr << " " << fieldName;

      if (selectedFields[i].level.exists()) {
        if (!fielddiff && !paramdiff)
          ostr << " ( " << selectedFields[i].level;
        else if ((fielddiff || paramdiff) && leveldiff)
          ostr << " " << selectedFields[i].level;
      }
      if (selectedFields[i].idnum.exists() && leveldiff)
        ostr << " " << selectedFields[i].idnum;

    } else if (selectedFields[i].minus) {
      ostr << " - ";

      if (fielddiff) {
        ostr << modelName;
        pmodelName.clear();
      }

      if (fielddiff && !paramdiff && !leveldiff)
        ostr << " ) " << fieldName;
      else if (paramdiff || (fielddiff && leveldiff))
        ostr << " " << fieldName;

      if (selectedFields[i].level.exists()) {
        if (!leveldiff && paramdiff)
          ostr << " )";
        ostr << " " << selectedFields[i].level;
        if (selectedFields[i].idnum.exists())
          ostr << " " << selectedFields[i].idnum;
        if (leveldiff)
          ostr << " )";
      } else {
        ostr << " )";
      }

      fielddiff = paramdiff = leveldiff = false;

    } else { // Ordinary field

      if (i > numEditFields)
        ostr << "  ";

      if (modelName != pmodelName) {
        ostr << modelName;
        pmodelName = modelName;
      }

      ostr << " " << fieldName;

      if (selectedFields[i].level.exists())
        ostr << " " << selectedFields[i].level;
      if (selectedFields[i].idnum.exists())
        ostr << " " << selectedFields[i].idnum;
    }
  }

  if (n > 0)
    name = "<font color=\"#000099\">" + ostr.str() + "</font>";

  return name;
}

bool FieldDialog::levelsExists(bool up, int type)
{

  //returns true if there exist plots with levels available: up/down of type (0=vertical/ 1=extra/eps)

  int n = selectedFields.size();

  if ( type == 0 ) {

    int i = 0;
    while ( i < n && selectedFields[i].levelOptions.size() == 0) i++;
    if( i == n ) {
      return false;
    } else {
      int m = selectedFields[i].levelOptions.size();
      if ( up ) {
        return ( selectedFields[i].level != selectedFields[i].levelOptions[0]);
      } else {
        return ( selectedFields[i].level != selectedFields[i].levelOptions[m-1]);
      }
    }

  } else {

    int i = 0;
    while ( i < n && selectedFields[i].idnumOptions.size() == 0) i++;
    if( i == n ) {
      return false;
    } else {
      int m = selectedFields[i].idnumOptions.size();
      if ( up ) {
        return ( selectedFields[i].idnum != selectedFields[i].idnumOptions[m-1]);
      } else {
        return ( selectedFields[i].idnum != selectedFields[i].idnumOptions[0]);
      }
    }

  }

  return false;

}

void FieldDialog::historyBack()
{
  showHistory(-1);
}

void FieldDialog::historyForward()
{
  showHistory(1);
}

void FieldDialog::showHistory(int step)
{

  int hs = commandHistory.size();
  if (hs == 0) {
    historyPos = -1;
    historyBackButton->setEnabled(false);
    historyForwardButton->setEnabled(false);
    historyOkButton->setEnabled(false);
    return;
  }

  historyPos += step;
  if (historyPos < -1)
    historyPos = -1;
  if (historyPos > hs)
    historyPos = hs;

  if (historyPos < 0 || historyPos >= hs) {

    // enable model/field boxes and show edit fields
    deleteAllSelected();

  } else {

    if (!historyOkButton->isEnabled()) {
      deleteAllSelected(); // some cleanup before browsing history
      if (numEditFields > 0)
        enableWidgets("none");
    }

    // not show edit fields during history browsing
    selectedFieldbox->clear();

    bool minus = false;
    miutil::miString history_str;
    vector<miutil::miString> vstr;
    int n = commandHistory[historyPos].size();

    for (int i = 0; i < n; i++) {

      if (history_str.empty())
        history_str = commandHistory[historyPos][i];

      //if (field1 - field2)
      miutil::miString field1, field2;
      if (fieldDifference(history_str, field1, field2))
        history_str = field1;

      vector<ParsedCommand> vpc = cp->parse(history_str);

      /*******************************************************
       int mmm= (vpc.size()<10) ? vpc.size() : 10;
       cerr<<"--------------------------------"<<endl;
       for (int j=0; j<mmm; j++) {
       cerr << "  parse " << j << " : key= " << vpc[j].key
       << "  idNumber= " << vpc[j].idNumber << endl;
       cerr << "            allValue: " << vpc[j].allValue << endl;
       for (int k=0; k<vpc[j].strValue.size(); k++)
       cerr << "               " << k << "    strValue: " << vpc[j].strValue[k] << endl;
       for (int k=0; k<vpc[j].floatValue.size(); k++)
       cerr << "               " << k << "  floatValue: " << vpc[j].floatValue[k] << endl;
       for (int k=0; k<vpc[j].intValue.size(); k++)
       cerr << "               " << k << "    intValue: " << vpc[j].intValue[k] << endl;
       }
       *******************************************************/
      miutil::miString str;
      if (minus)
        str = "  -  ";

      if (vpc.size() > 1 && vpc[0].key == "unknown") {
        str += vpc[0].allValue; // modelName
        if (vpc[1].key == "unknown") {
          str += (" " + vpc[1].allValue); // fieldName
          int nc;
          if ((nc = cp->findKey(vpc, "level")) >= 0)
            str += (" " + vpc[nc].allValue);
          if ((nc = cp->findKey(vpc, "idnum")) >= 0)
            str += (" " + vpc[nc].allValue);
          vstr.push_back(str);
        }
      }

      //reset
      minus = false;
      history_str.clear();

      //if ( field1 - field2 )
      if (field2.exists()) {
        minus = true;
        history_str = field2;
        i--;
      }
    }

    int nvstr = vstr.size();
    if (nvstr > 0) {
      for (int i = 0; i < nvstr; i++) {
        selectedFieldbox->addItem(QString(vstr[i].c_str()));
      }
      deleteAll->setEnabled(true);
    }

    historyOkButton->setEnabled(true);

  }

  historyBackButton->setEnabled(historyPos > -1);
  historyForwardButton->setEnabled(historyPos < hs);
}

void FieldDialog::historyOk()
{
#ifdef DEBUGPRINT
  cerr << "FieldDialog::historyOk()" << endl;
#endif

  if (historyPos < 0 || historyPos >= int(commandHistory.size())) {
    vector<miutil::miString> vstr;
    putOKString(vstr, false, false);
  } else {
    putOKString(commandHistory[historyPos], false, false);
  }
}

void FieldDialog::putOKString(const vector<miutil::miString>& vstr,
    bool checkOptions, bool external)
{
#ifdef DEBUGPRINT
#endif

  deleteAllSelected();

  bool allTimeSteps = false;

  int nc = vstr.size();

  if (nc == 0) {
    updateTime();
    return;
  }

  miutil::miString vfg2_model, model, field, level, idnum, fOpts;
  int indexMGR, indexM, indexFGR;
  bool minus = false;
  miutil::miString str;

  for (int ic = 0; ic < nc; ic++) {
    if (str.empty())
      str = vstr[ic];
    //######################################################################
//            cerr << "P.OK>> " << vstr[ic] << endl;
    //######################################################################

    //if prefix, remove it
    if (str.length() > 6 && str.substr(0, 6) == "FIELD ")
      str = str.substr(6, str.size() - 6);

    //if (field1 - field2)
    miutil::miString field1, field2;
    if (fieldDifference(str, field1, field2))
      str = field1;

    SelectedField sf;
    sf.external = external; // from QuickMenu
    bool decodeOK = false;
    sf.cdmSyntax = str.contains("model=");

    if (checkOptions) {
      str = checkFieldOptions(str, sf.cdmSyntax);
      if (str.empty())
        continue;
    }

    if ( sf.cdmSyntax ) {
      decodeOK = decodeString_cdmSyntax(str, sf, allTimeSteps);
    } else {
      decodeOK = decodeString_oldSyntax(str, sf, allTimeSteps);
    }

    //old string from quickMenu or bdiana, rewrite string in new syntax
    if( ! decodeOK && !sf.cdmSyntax) {
        //First try - decode model string - model + refhour + refoffset
      std::string oldString = str;
      str = FieldSpecTranslation::getNewFieldString(oldString, true);
      sf.cdmSyntax = true;

      if (checkOptions) {
        str = checkFieldOptions(str, sf.cdmSyntax);
        if (str.empty())
          continue;
      }
      decodeOK = decodeString_cdmSyntax(str, sf, allTimeSteps);

      if( ! decodeOK ) {
      // Second try - do not decode model string
        str = FieldSpecTranslation::getNewFieldString(oldString,false);

        if (checkOptions) {
          str = checkFieldOptions(str, sf.cdmSyntax);
          if (str.empty())
            continue;
        }
        decodeOK = decodeString_cdmSyntax(str, sf, allTimeSteps);
      }
    }

    if ( decodeOK ) {

      selectedFields.push_back(sf);

      miutil::miString text = sf.modelName + " " + sf.fieldName + " " + sf.refTime;
      selectedFieldbox->addItem(QString(text.c_str()));

      selectedFieldbox->setCurrentRow(selectedFieldbox->count() - 1);
      selectedFieldbox->item(selectedFieldbox->count() - 1)->setSelected(true);

      //############################################################################
//           cerr << "  ok: " << str << " " << fOpts << endl;
      //############################################################################
    }

    if (minus) {
      minus = false;
      minusField(true);
    }

    if (!field2.empty()) {
      str = field2;
      ic--;
      minus = true;
    } else {
      str.clear();
    }

  }

  int m = selectedFields.size();

  if (m > 0) {
    if (vfgi.size() > 0) {
      indexMGR = indexMGRtable[modelGRbox->currentIndex()];
      indexM = modelbox->currentRow();
      indexFGR = fieldGRbox->currentIndex();
      int n = vfgi[indexFGR].fieldNames.size();
      bool change = false;
      for (int i = 0; i < m; i++) {
        if (selectedFields[i].indexMGR == indexMGR && selectedFields[i].indexM
            == indexM) {
          bool groupOK = true;
          if ( selectedFields[i].cdmSyntax ) {
            if ( selectedFields[i].zaxis != vfgi[indexFGR].zaxis
                || selectedFields[i].extraaxis != vfgi[indexFGR].extraaxis ) {
//                || selectedFields[i].grid != vfgi[indexFGR].grid ) {
              groupOK = false;
            }
          } else { //old syntax
            int ml;
            if ((ml = vfgi[indexFGR].levelNames.size()) > 0) {
              int l = 0;
              while (l < ml && vfgi[indexFGR].levelNames[l]
                                                         != selectedFields[i].level)
                l++;
              if (l == ml)
                groupOK = false;
            }
            if ((ml = vfgi[indexFGR].idnumNames.size()) > 0) {
              int l = 0;
              while (l < ml && vfgi[indexFGR].idnumNames[l]
                                                         != selectedFields[i].idnum)
                l++;
              if (l == ml)
                groupOK = false;
            }
          }
          if ( groupOK ) {
            int j = 0;
            while (j < n && vfgi[indexFGR].fieldNames[j]
                                                      != selectedFields[i].fieldName)
              j++;
            if (j < n) {
              countSelected[j]++;
              fieldbox->item(j)->setSelected(true);
              change = true;
            }
          }
        }
      }
      if (change)
        fieldboxChanged(fieldbox->currentItem());
    }
    selectedFieldbox->setCurrentRow(0);
    selectedFieldbox->item(0)->setSelected(true);
    enableFieldOptions();
  }

  if (m > 0 && allTimeSteps != allTimeStepButton->isChecked()) {
    allTimeStepButton->setChecked(allTimeSteps);
    allTimeStepToggled(allTimeSteps);
  } else {
    updateTime();
  }

#ifdef DEBUGPRINT
  cerr << "FieldDialog::putOKString finished" << endl;
#endif
}

bool FieldDialog::decodeString_cdmSyntax( const miutil::miString& fieldString, SelectedField& sf, bool& allTimeSteps )
{

  vector<ParsedCommand> vpc;

  vpc = cp->parse(fieldString);

   //######################################################################
  //    for (int j = 0; j < vpc.size(); j++) {
  //      cerr << "   " << j << " : " << vpc[j].key << " = " << vpc[j].strValue[0]
  //          << "   " << vpc[j].allValue << endl;
  //    }
  //######################################################################

  sf.inEdit = false;

  int refOffset = 0;
  int refHour = -1;

  for (size_t j = 0; j < vpc.size(); j++) {
    if (vpc[j].key == "model") {
      sf.modelName = vpc[j].allValue;
    } else if (vpc[j].key == "reftime") {
      sf.refTime = vpc[j].allValue;
    } else if (vpc[j].key == "refoffset" && !vpc[j].intValue.empty())  {
      refOffset = vpc[j].intValue[0];
    } else if (vpc[j].key == "refhour" && !vpc[j].intValue.empty())  {
      refHour = vpc[j].intValue[0];
    } else if (vpc[j].key == "plot") {
      sf.fieldName = vpc[j].allValue;
    } else if (vpc[j].key == "parameter") {
      sf.fieldName = vpc[j].allValue;
      sf.plotDefinition = false;
    } else if (vpc[j].key == "level") {
      sf.level = vpc[j].allValue;
    } else if (vpc[j].key == "vlevel") {
      sf.level = vpc[j].allValue;
    } else if (vpc[j].key == "idnum") {
      sf.idnum = vpc[j].allValue;
    } else if (vpc[j].key == "vcoord") {
      sf.zaxis = vpc[j].allValue;
    } else if (vpc[j].key == "ecoor") {
      sf.extraaxis = vpc[j].allValue;
    } else if (vpc[j].key == "grid") {
      sf. grid = vpc[j].allValue;
    } else if (vpc[j].key == "vccor") {
      sf. zaxis = vpc[j].allValue;
    } else if (vpc[j].key == "hour.offset" && !vpc[j].intValue.empty()) {
      sf. hourOffset = vpc[j].intValue[0];
    } else if (vpc[j].key == "hour.diff" && !vpc[j].intValue.empty()) {
      sf. hourDiff = vpc[j].intValue[0];
    } else if (vpc[j].key == "allTimeSteps" && vpc[j].allValue == "on") {
      allTimeSteps = true;
    } else if (vpc[j].key != "unknown") {
      if (!sf.fieldOpts.empty())
        sf.fieldOpts += " ";
      sf.fieldOpts += (vpc[j].key + "=" + vpc[j].allValue);
      if (vpc[j].idNumber == 2)
        sf.forecastSpec = true;
    }
  }

  //find referencetime
  if ( sf.refTime.empty() ) {
    sf.refTime = m_ctrl->getBestFieldReferenceTime(sf.modelName, refOffset, refHour);
  }

  //######################################################################
//  cerr << " ->" << sf.modelName << " " << sf.fieldName << " l= " << sf.level << " l2= "
//      << sf.idnum << endl;
  //######################################################################

  vector<FieldGroupInfo> vfg;

  //find index of modelgroup and model. Keep name of model and reuse info if same model
  int indexMGR = -1;
  int indexM = -1;
  getFieldGroups(sf.modelName, sf.refTime, indexMGR, indexM, sf.plotDefinition, vfg);

  //find index of fieldgroup and field
  bool fieldFound = false;
  int nvfg = vfg.size();
  int indexFGR = 0;
  while (indexFGR < nvfg) {
//         cout << "Searching for correct fieldgroup: "<< sf.zaxis<< " : "<<vfg[indexFGR].zaxis<<endl;
    if (sf.zaxis == vfg[indexFGR].zaxis
        && sf.extraaxis == vfg[indexFGR].extraaxis ) {
        //&& sf.grid == vfg[indexFGR].grid ) {
      int m = vfg[indexFGR].fieldNames.size();
      int indexF = 0;
      while (indexF < m && vfg[indexFGR].fieldNames[indexF] != sf.fieldName){
//                  cout << " .. skipping field:" << vfg[indexFGR].fieldNames[indexF] << endl;
        indexF++;
      }

      if (indexF < m) {
        fieldFound = true;
        break;
      }

    }
    indexFGR++;
  }

  if (fieldFound) {

    sf.indexMGR = indexMGR;
    sf.indexM = indexM;
    sf.levelOptions = vfg[indexFGR].levelNames;
    sf.idnumOptions = vfg[indexFGR].idnumNames;
    sf.minus = false;
    return true;
  }

  //############################################################################
//     else cerr << "  error" << endl;
  //############################################################################

  return false;
}

bool FieldDialog::decodeString_oldSyntax( const miutil::miString& fieldString, SelectedField& sf, bool& allTimeSteps )
{

  miutil::miString vfg2_model, model, field, level, idnum, fOpts;
  int hourOffset, hourDiff;
  int indexMGR, indexM, indexFGR, indexF;
  bool forecastSpec;

  vector<ParsedCommand> vpc;

  vpc = cp->parse(fieldString);

  model.clear();
  field.clear();
  level.clear();
  idnum.clear();
  fOpts.clear();
  hourOffset = 0;
  hourDiff = 0;
  forecastSpec = false;

  //######################################################################
  //    for (int j = 0; j < vpc.size(); j++) {
  //      cerr << "   " << j << " : " << vpc[j].key << " = " << vpc[j].strValue[0]
  //          << "   " << vpc[j].allValue << endl;
  //    }
  //######################################################################

  if (vpc.size() > 1 && vpc[0].key == "unknown") {
    model = vpc[0].allValue; // modelName
    if (vpc[1].key == "unknown") {
      field = vpc[1].allValue; // fieldName
      for (unsigned int j = 2; j < vpc.size(); j++) {
        if (vpc[j].key == "level")
          level = vpc[j].allValue;
        else if (vpc[j].key == "idnum")
          idnum = vpc[j].allValue;
        else if (vpc[j].key == "hour.offset" && !vpc[j].intValue.empty())
          hourOffset = vpc[j].intValue[0];
        else if (vpc[j].key == "hour.diff" && !vpc[j].intValue.empty())
          hourDiff = vpc[j].intValue[0];
        else if (vpc[j].key == "allTimeSteps" && vpc[j].allValue == "on")
          allTimeSteps = true;
        else if (vpc[j].key != "unknown") {
          if (!fOpts.empty())
            fOpts += " ";
          fOpts += (vpc[j].key + "=" + vpc[j].allValue);
          if (vpc[j].idNumber == 2)
            forecastSpec = true;
        }
      }
    }
  }

  //######################################################################
//     cerr << " ->" << model << " " << field << " l= " << level << " l2= "
//        << idnum << endl;
  //######################################################################
  vector<FieldGroupInfo> vfg2;
  int nvfg = 0;

//  if (model != vfg2_model) {
    indexMGR = indexM = -1;
    getFieldGroups(model, "",indexMGR, indexM, true, vfg2);
//    vfg2_model = model;
    nvfg = vfg2.size();
//  }

  indexF = -1;
  indexFGR = -1;
  int j = 0;
  bool ok = false;

  while (!ok && j < nvfg) {
    //      cout << "Searching for correct model, index:" << j << " has model:" << vfg2[j].modelName << endl;

    // Old syntax: Model, new syntax: Model(gridnr)
    miutil::miString modelName = vfg2[j].modelName;
    if (vfg2[j].modelName.contains("(") && !model.contains("(")) {
      modelName = modelName.substr(0, modelName.find(("(")));
    }
    if (!vfg2[j].modelName.contains("(") && model.contains("(")) {
      model = model.substr(0, model.find(("(")));
    }

    if (modelName == model) {
      //        cout << "Found model:" << modelName << " in index:" << j << endl;
      int m = vfg2[j].fieldNames.size();
      int i = 0;
      while (i < m && vfg2[j].fieldNames[i] != field){
        //          cout << " .. skipping field:" << vfg2[j].fieldNames[i] << endl;
        i++;
      }

      if (i < m) {
        ok = true;
        int m;
        if ((m = vfg2[j].levelNames.size()) > 0 && !level.empty()) {
          //            cout << " .. level is not empty" << endl;
          int l = 0;
          while (l < m && vfg2[j].levelNames[l] != level)
            l++;
          if (l == m && cp->isInt(level)) {
            level += "hPa";
            l = 0;
            while (l < m && vfg2[j].levelNames[l] != level)
              l++;
            if (l < m)
              level = vfg2[j].levelNames[l];
          }
          if (l == m){
            //             cout << " .. did not find level:" << level << " ok=false" << endl;
            ok = false;
          }
        } else if (!vfg2[j].levelNames.empty()) {
          ok = false;
        } else {
          level.clear();
        }
        if ((m = vfg2[j].idnumNames.size()) > 0 && !idnum.empty()) {
          int l = 0;
          while (l < m && vfg2[j].idnumNames[l] != idnum)
            l++;
          if (l == m)
            ok = false;
        } else if (!vfg2[j].idnumNames.empty()) {
          ok = false;
        } else {
          idnum.clear();
        }
        if (ok) {
          indexFGR = j;
          indexF = i;
        }
      }
    }
    j++;
  }

  if (indexFGR >= 0 && indexF >= 0) {
    sf.inEdit = false;
    sf.forecastSpec = forecastSpec; // only if external
    sf.indexMGR = indexMGR;
    sf.indexM = indexM;
    sf.modelName = vfg2[indexFGR].modelName;
    sf.fieldName = vfg2[indexFGR].fieldNames[indexF];
    sf.levelOptions = vfg2[indexFGR].levelNames;
    sf.idnumOptions = vfg2[indexFGR].idnumNames;
    sf.level = level;
    sf.idnum = idnum;
    sf.hourOffset = hourOffset;
    sf.hourDiff = hourDiff;
    sf.fieldOpts = fOpts;
    sf.minus = false;
    return true;
  }
  //############################################################################
//     else cerr << "  error" << endl;
  //############################################################################
  return false;

}


bool FieldDialog::fieldDifference(const miutil::miString& str,
    miutil::miString& field1, miutil::miString& field2) const
{
  size_t beginOper = str.find("( ");
  if (beginOper != string::npos) {
    size_t oper = str.find(" - ", beginOper + 3);
    if (oper != string::npos) {
      size_t endOper = str.find(" )", oper + 4);
      if (endOper != string::npos) {
        size_t end = str.size();
        if (beginOper > 1 && endOper < end - 2) {
          field1 = str.substr(0, beginOper) + str.substr(beginOper + 2, oper
              - beginOper - 2) + str.substr(endOper + 2, end - endOper - 1);
          field2 = str.substr(0, beginOper - 1) + str.substr(oper + 2, endOper
              - oper - 2);
        } else if (endOper < end - 2) {
          field1 = str.substr(beginOper + 2, oper - beginOper - 2)
                  + str.substr(endOper + 2, end - endOper - 1);
          field2 = str.substr(oper + 3, endOper - oper - 3);
        } else {
          field1 = str.substr(0, beginOper) + str.substr(beginOper + 2, oper
              - beginOper - 2);
          field2 = str.substr(0, beginOper) + str.substr(oper + 3, endOper
              - oper - 3);
        }
        return true;
      }
    }
  }
  return false;
}

void FieldDialog::getEditPlotOptions(map<miutil::miString, map<
    miutil::miString, miutil::miString> >& po)
{

  //map<paramater, map <option, value> >

  map<miutil::miString, map<miutil::miString, miutil::miString> >::iterator p =
      po.begin();
  //loop through parameters
  for (; p != po.end(); p++) {
    miutil::miString options;
    miutil::miString parameter = p->first;
    if (editFieldOptions.count(parameter)) {
      options = editFieldOptions[parameter];
    } else if (fieldOptions.count(parameter)) {
      options = fieldOptions[parameter];
    } else if (setupFieldOptions.count(parameter)) {
      options = setupFieldOptions[parameter];
    } else {
      continue; //parameter not found
    }

    vector<ParsedCommand> parsedComm = cp->parse(options);
    map<miutil::miString, miutil::miString>::iterator q = p->second.begin();
    //loop through options
    for (; q != p->second.end(); q++) {
      miutil::miString opt = q->first.downcase();
      unsigned int i = 0;
      while (i < parsedComm.size() && parsedComm[i].key != opt)
        i++;
      //option found and value inserted
      if (i < parsedComm.size()) {
        q->second = parsedComm[i].allValue;
      }
    }
  }

}

vector<miutil::miString> FieldDialog::writeLog()
{

  vector<miutil::miString> vstr;

  // write history

  int i, n, h, hf = 0, hs = commandHistory.size();

  // avoid eternal history
  if (hs > 100)
    hf = hs - 100;

  for (h = hf; h < hs; h++) {
    n = commandHistory[h].size();
    for (i = 0; i < n; i++)
      vstr.push_back(commandHistory[h][i]);
    vstr.push_back("----------------");
  }
  vstr.push_back("================");

  // write used field options

  map<miutil::miString, miutil::miString>::iterator pfopt, pfend =
      fieldOptions.end();

  for (pfopt = fieldOptions.begin(); pfopt != pfend; pfopt++) {
    miutil::miString sopts = getFieldOptions(pfopt->first, true);
    // only logging options if different from setup
    if (sopts != pfopt->second)
      vstr.push_back(pfopt->first + " " + pfopt->second);
  }

  //write edit/profet field options
  if (editFieldOptions.size() > 0) {
    vstr.push_back("--- EDIT ---");
    pfend = editFieldOptions.end();
    for (pfopt = editFieldOptions.begin(); pfopt != pfend; pfopt++) {
      miutil::miString sopts = getFieldOptions(pfopt->first, true);
      // only logging options if different from setup
      if (sopts != pfopt->second) {
        vstr.push_back(pfopt->first + " " + pfopt->second);
      }
    }
  }

  vstr.push_back("================");

  return vstr;
}

void FieldDialog::readLog(const vector<miutil::miString>& vstr,
    const miutil::miString& thisVersion, const miutil::miString& logVersion)
{

  miutil::miString str, fieldname, fopts, sopts;
  vector<miutil::miString> hstr;
  size_t pos, end;

  int nopt, nlog;
  bool changed;

  int nvstr = vstr.size();
  int ivstr = 0;

  // history of commands,
  // many checks in case program and/or diana.setup has changed

  for (; ivstr < nvstr; ivstr++) {
    if (vstr[ivstr].substr(0, 4) == "====")
      break;
    if (vstr[ivstr].substr(0, 4) == "----") {
      if (hstr.size() > 0) {
        commandHistory.push_back(hstr);
        hstr.clear();
      }
    } else {
      hstr.push_back(vstr[ivstr]);
    }
  }
  ivstr++;

  // field options:
  // do not destroy any new options in the program
  bool editOptions = false;
  for (; ivstr < nvstr; ivstr++) {
    if (vstr[ivstr].substr(0, 4) == "====")
      break;
    if (vstr[ivstr] == "--- EDIT ---") {
      editOptions = true;
      continue;
    }
    str = vstr[ivstr];
    end = str.length();
    pos = str.find_first_of(' ');
    if (pos > 0 && pos < end) {
      fieldname = str.substr(0, pos);
      pos++;
      fopts = str.substr(pos);

      // get options from setup
      sopts = getFieldOptions(fieldname, true);

      if (!sopts.empty()) {
        // update options from setup, if necessary
        vector<ParsedCommand> vpopt = cp->parse(sopts);
        vector<ParsedCommand> vplog = cp->parse(fopts);
        nopt = vpopt.size();
        nlog = vplog.size();
        changed = false;
        for (int i = 0; i < nopt; i++) {
          int j = 0;
          while (j < nlog && vplog[j].key != vpopt[i].key)
            j++;
          if (j < nlog) {
            if (vplog[j].allValue != vpopt[i].allValue) {
              cp->replaceValue(vpopt[i], vplog[j].allValue, -1);
              changed = true;
            }
          }
        }
        for (int i = 0; i < nlog; i++) {
          int j = 0;
          while (j < nopt && vpopt[j].key != vplog[i].key)
            j++;
          if (j == nopt) {
            cp->replaceValue(vpopt, vplog[i].key, vplog[i].allValue);
            changed = true;
          }
        }
        if (changed) {
          if (editOptions) {
            editFieldOptions[fieldname] = cp->unParse(vpopt);
          } else {
            fieldOptions[fieldname] = cp->unParse(vpopt);
          }
        }

      }
    }
  }
  ivstr++;

  historyPos = commandHistory.size();

  historyBackButton->setEnabled(historyPos > 0);
  historyForwardButton->setEnabled(false);
}

miutil::miString FieldDialog::checkFieldOptions(const miutil::miString& str, bool cdmSyntax)
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::checkFieldOptions:"<<str<<endl;
#endif

  //merging fieldOptions from str whith current fieldOptions from same field

  miutil::miString newstr;

  miutil::miString fieldname;

  vector<ParsedCommand> vplog = cp->parse(str);
  int nlog = vplog.size();

  // find fieldname
  if( cdmSyntax ) {
    for (unsigned int j = 0; j < vplog.size(); j++) {
      if (vplog[j].key == "plot" || vplog[j].key == "parameter" ) {
        fieldname = vplog[j].allValue;
        break;
      }
    }
  } else {
    if (nlog >= 2 && vplog[0].key == "unknown" && vplog[1].key == "unknown") {
      fieldname = vplog[1].allValue;
    }
  }

  if ( fieldname.exists() ) {
    miutil::miString fopts = getFieldOptions(fieldname, true);

    if (!fopts.empty()) {
      vector<ParsedCommand> vpopt = cp->parse(fopts);
      int nopt = vpopt.size();
      //##################################################################
//      cerr << "    nopt= " << nopt << "  nlog= " << nlog << endl;
//      for (int j=0; j<nlog; j++)
//        cerr << "        log " << j << " : id " << vplog[j].idNumber
//        << "  " << vplog[j].key << " = " << vplog[j].allValue << endl;
      //##################################################################

      // model + field, old syntax
      if ( !cdmSyntax ) {
        newstr += vplog[0].allValue + " " + vplog[1].allValue;
      }

      for (int i = 0; i < nlog; i++) {
        if (vplog[i].idNumber == 1)
          newstr += (" " + vplog[i].key + "=" + vplog[i].allValue);
      }

      //loop through current options, replace the value if the new string has same option with different value
      for (int j = 0; j < nopt; j++) {
        int i = 0;
        while (i < nlog && vplog[i].key != vpopt[j].key)
          i++;
        if (i < nlog) {
          // there is no option with variable no. of values, YET !!!!!
          if (vplog[i].allValue != vpopt[j].allValue)
            cp->replaceValue(vpopt[j], vplog[i].allValue, -1);
        }
      }

      //loop through new options, add new option if it is not a part og current options
      for (int i = 2; i < nlog; i++) {
        if (vplog[i].key != "level" && vplog[i].key != "idnum") {
          int j = 0;
          while (j < nopt && vpopt[j].key != vplog[i].key)
            j++;
          if (j == nopt) {
            cp->replaceValue(vpopt, vplog[i].key, vplog[i].allValue);
          }
        }
      }

      newstr += " ";
      newstr += cp->unParse(vpopt);
      // from quickmenu, keep "forecast.hour=..." and "forecast.hour.loop=..."
      for (int i = 2; i < nlog; i++) {
        if (vplog[i].idNumber == 2 || vplog[i].idNumber == 3
            || vplog[i].idNumber == -1) {
          newstr += (" " + vplog[i].key + "=" + vplog[i].allValue);
        }
      }
    }
  }

  return newstr;
}

void FieldDialog::deleteSelected()
{
#ifdef DEBUGPRINT
  cerr<<"FieldDialog::deleteSelected called"<<endl;
#endif

  int index = selectedFieldbox->currentRow();

  int ns = selectedFields.size() - 1;

  if (index < 0 || index > ns)
    return;
  if (selectedFields[index].inEdit)
    return;

  int indexF = -1;
  int ml;

  if (vfgi.size() > 0) {
    int indexMGR = indexMGRtable[modelGRbox->currentIndex()];
    int indexM = modelbox->currentRow();
    int indexFGR = fieldGRbox->currentIndex();
    if (selectedFields[index].indexMGR == indexMGR
        && selectedFields[index].indexM == indexM) {
      int n = vfgi[indexFGR].fieldNames.size();
      int i = 0;
      while (i < n && vfgi[indexFGR].fieldNames[i]
                                                != selectedFields[index].fieldName)
        i++;
      if (i < n) {
        if ((ml = vfgi[indexFGR].levelNames.size()) > 0) {
          int l = 0;
          while (l < ml && vfgi[indexFGR].levelNames[l]
                                                     != selectedFields[index].level)
            l++;
          if (l == ml)
            i = n;
        }
        if ((ml = vfgi[indexFGR].idnumNames.size()) > 0) {
          int l = 0;
          while (l < ml && vfgi[indexFGR].idnumNames[l]
                                                     != selectedFields[index].idnum)
            l++;
          if (l == ml)
            i = n;
        }
        if (i < n)
          indexF = i;
      }
    }
  }

  if (indexF >= 0) {
    countSelected[indexF]--;
    if (countSelected[indexF] == 0) {
      fieldbox->item(indexF)->setSelected(false);
    }
  }
  selectedFieldbox->takeItem(index);
  for (int i = index; i < ns; i++) {
    selectedFields[i] = selectedFields[i + 1];
  }
  selectedFields.pop_back();

  if (selectedFields.size() > 0) {
    if (index >= int(selectedFields.size()))
      index = selectedFields.size() - 1;
    selectedFieldbox->setCurrentRow(index);
    selectedFieldbox->item(index)->setSelected(true);
    enableFieldOptions();
  } else {
    setDefaultFieldOptions();
    enableWidgets("none");
    setLevel();
    setIdnum();
  }

  updateTime();

  //first field can't be minus
  if (selectedFieldbox->count() > 0 && selectedFields[0].minus)
    minusButton->setChecked(false);

#ifdef DEBUGPRINT
  cerr<<"FieldDialog::deleteSelected returned"<<endl;
#endif
  return;
}

void FieldDialog::deleteAllSelected()
{
#ifdef DEBUGPRINT
  cerr<<" FieldDialog::deleteAllSelected() called"<<endl;
#endif

  // calls:
  //  1: button clicked (while selecting fields)
  //  2: button clicked when history is shown (get out of history)
  //  3: from putOKString when accepting history command
  //  4: from putOKString when inserting QuickMenu command
  //     (possibly when dialog in "history" state...)
  //  5: variable useArchive changed

  int n = fieldbox->count();
  for (int i = 0; i < n; i++)
    countSelected[i] = 0;

  if (n > 0) {
    fieldbox->blockSignals(true);
    fieldbox->clearSelection();
    fieldbox->blockSignals(false);
  }

  selectedFields.resize(numEditFields);
  selectedFieldbox->clear();
  minusButton->setChecked(false);
  minusButton->setEnabled(false);
  historyOkButton->setEnabled(false);

  if (numEditFields > 0) {
    // show edit fields
    for (int i = 0; i < numEditFields; i++) {
      miutil::miString str = editName + " " + selectedFields[i].fieldName + " " + selectedFields[i].refTime;
      selectedFieldbox->addItem(QString(str.c_str()));
    }
    selectedFieldbox->setCurrentRow(0);
    selectedFieldbox->item(0)->setSelected(true);
    enableFieldOptions();
  } else {
    setDefaultFieldOptions();
    enableWidgets("none");
    setLevel();
    setIdnum();
  }

  updateTime();

#ifdef DEBUGPRINT
  cerr<<" FieldDialog::deleteAllSelected() returned"<<endl;
#endif
  return;
}

void FieldDialog::copySelectedField()
{
#ifdef DEBUGPRINT
  cerr<<" FieldDialog::copySelectedField called"<<endl;
#endif

  if (selectedFieldbox->count() == 0)
    return;

  int n = selectedFields.size();
  if (n == 0)
    return;

  int index = selectedFieldbox->currentRow();

  if (vfgi.size() > 0) {
    int indexMGR = indexMGRtable[modelGRbox->currentIndex()];
    int indexM = modelbox->currentRow();
    int indexFGR = fieldGRbox->currentIndex();
    if (selectedFields[index].indexMGR == indexMGR
        && selectedFields[index].indexM == indexM) {
      int n = vfgi[indexFGR].fieldNames.size();
      int i = 0;
      while (i < n && vfgi[indexFGR].fieldNames[i]
                                                != selectedFields[index].fieldName)
        i++;
      if (i < n) {
        int ml = vfgi[indexFGR].levelNames.size();
        if (ml > 0 && selectedFields[index].level.exists()) {
          int l = 0;
          while (l < ml && vfgi[indexFGR].levelNames[l]
                                                     != selectedFields[index].level)
            l++;
          if (l == ml)
            i = n;
        } else if (ml > 0 || selectedFields[index].level.exists()) {
          i = n;
        }
        ml = vfgi[indexFGR].idnumNames.size();
        if (ml > 0 && selectedFields[index].idnum.exists()) {
          int l = 0;
          while (l < ml && vfgi[indexFGR].idnumNames[l]
                                                     != selectedFields[index].idnum)
            l++;
          if (l == ml)
            i = n;
        } else if (ml > 0 || selectedFields[index].idnum.exists()) {
          i = n;
        }
        if (i < n)
          countSelected[i]++;
      }
    }
  }

  selectedFields.push_back(selectedFields[index]);
  selectedFields[n].hourOffset = 0;

  selectedFieldbox->addItem(selectedFieldbox->item(index)->text());
  selectedFieldbox->setCurrentRow(n);
  selectedFieldbox->item(n)->setSelected(true);
  enableFieldOptions();

#ifdef DEBUGPRINT
  cerr<<" FieldDialog::copySelectedField returned"<<endl;
#endif
  return;
}

void FieldDialog::changeModel()
{
#ifdef DEBUGPRINT
  cerr<<" FieldDialog::changeModel called"<<endl;
#endif

  if (selectedFieldbox->count() == 0)
    return;
  int n = selectedFields.size();
  if (n == 0)
    return;

  int index = selectedFieldbox->currentRow();
  if (index < 0 || index >= n)
    return;

  if (modelGRbox->count() == 0 || modelbox->count() == 0)
    return;

  int indexMGR = modelGRbox->currentIndex();
  int indexM = modelbox->currentRow();
  if (indexMGR < 0 || indexM < 0)
    return;
  indexMGR = indexMGRtable[indexMGR];

  int indexFGR = fieldGRbox->currentIndex();
  if (indexFGR < 0 || indexFGR >= int(vfgi.size()))
    return;

  miutil::miString oldModel = selectedFields[index].modelName;
  miutil::miString oldRefTime = selectedFields[index].refTime;
  miutil::miString newModel = vfgi[indexFGR].modelName;
  miutil::miString newRefTime = vfgi[indexFGR].refTime;
  if ( (oldModel == newModel) && (oldRefTime == newRefTime) )
    return;
  //ignore (gridnr)
  newModel = newModel.substr(0, newModel.find("("));
  oldModel = oldModel.substr(0, oldModel.find("("));
  fieldbox->blockSignals(true);

  int nvfgi = vfgi.size();
  int gbest, fbest, gnear, fnear;

  for (int i = 0; i < n; i++) {
    miutil::miString selectedModel = selectedFields[i].modelName;
    selectedModel = selectedModel.substr(0, selectedModel.find("("));
    miutil::miString selectedRefTime = selectedFields[i].refTime;
    if ( (selectedModel == oldModel) && (selectedRefTime == oldRefTime) ) {
      // check if field exists for the new model
      gbest = fbest = gnear = fnear = -1;
      int j = 0;
      while (gbest < 0 && j < nvfgi) {
        miutil::miString model = vfgi[j].modelName;
        model = model.substr(0, model.find("("));
        miutil::miString refTime = vfgi[j].refTime;
        if ( (model == newModel) && (refTime == newRefTime) ) {
          int m = vfgi[j].fieldNames.size();
          int k = 0;
          while (k < m && vfgi[j].fieldNames[k] != selectedFields[i].fieldName)
            k++;
          if (k < m) {
            int ml = vfgi[j].levelNames.size();
            if (ml > 0 && !selectedFields[i].level.empty()) {
              int l = 0;
              while (l < ml && vfgi[j].levelNames[l] != selectedFields[i].level)
                l++;
              if (l == ml)
                k = m;
            } else if (ml > 0 || !selectedFields[i].level.empty()) {
              k = m;
            }
            ml = vfgi[j].idnumNames.size();
            if (ml > 0 && !selectedFields[i].idnum.empty()) {
              int l = 0;
              while (l < ml && vfgi[j].idnumNames[l] != selectedFields[i].idnum)
                l++;
              if (l == ml)
                k = m;
            } else if (ml > 0 || !selectedFields[i].idnum.empty()) {
              k = m;
            }
            if (k < m) {
              gbest = j;
              fbest = k;
            } else if (gnear < 0) {
              gnear = j;
              fnear = k;
            }
          }
        }
        j++;
      }
      if (gbest >= 0 || gnear >= 0) {
        if (gbest < 0) {
          gbest = gnear;
          fbest = fnear;
        }
        if (indexFGR == gbest) {
          countSelected[fbest]++;
          if (countSelected[fbest] == 1 && fbest > 0 && fbest
              < fieldbox->count()) {
            fieldbox->setCurrentRow(fbest);
            fieldbox->item(fbest)->setSelected(true);
          }
        }
        selectedFields[i].indexMGR = indexMGR;
        selectedFields[i].indexM = indexM;
        selectedFields[i].modelName = vfgi[gbest].modelName;
        selectedFields[i].refTime = vfgi[gbest].refTime;
        selectedFields[i].levelOptions = vfgi[gbest].levelNames;
        selectedFields[i].idnumOptions = vfgi[gbest].idnumNames;
        selectedFields[i].refTime = vfgi[indexFGR].refTime;
        selectedFields[i].zaxis = vfgi[indexFGR].zaxis;
        selectedFields[i].extraaxis = vfgi[indexFGR].extraaxis;
        selectedFields[i].grid = vfgi[indexFGR].grid;
        selectedFields[i].cdmSyntax = vfgi[indexFGR].cdmSyntax;
        selectedFields[i].plotDefinition = fieldGroupCheckBox->isChecked();

        miutil::miString str = selectedFields[i].modelName + " "
            + selectedFields[i].fieldName;
        selectedFieldbox->item(i)->setText(QString(str.c_str()));
      }
    }
  }

  fieldbox->blockSignals(false);

  selectedFieldbox->setCurrentRow(index);
  selectedFieldbox->item(index)->setSelected(true);
  enableFieldOptions();

  updateTime();

#ifdef DEBUGPRINT
  cerr<<" FieldDialog::changeModel returned"<<endl;
#endif
  return;
}

void FieldDialog::upField()
{

  if (selectedFieldbox->count() == 0)
    return;
  int n = selectedFields.size();
  if (n == 0)
    return;

  int index = selectedFieldbox->currentRow();
  if (index < 1 || index >= n)
    return;

  SelectedField sf = selectedFields[index];
  selectedFields[index] = selectedFields[index - 1];
  selectedFields[index - 1] = sf;

  QString qstr1 = selectedFieldbox->item(index - 1)->text();
  QString qstr2 = selectedFieldbox->item(index)->text();
  selectedFieldbox->item(index - 1)->setText(qstr2);
  selectedFieldbox->item(index)->setText(qstr1);

  //some fields can't be minus
  for (int i = 0; i < n; i++) {
    selectedFieldbox->setCurrentRow(i);
    if (selectedFields[i].minus && (i == 0 || selectedFields[i - 1].minus))
      minusButton->setChecked(false);
  }

  selectedFieldbox->setCurrentRow(index - 1);
}

void FieldDialog::downField()
{

  if (selectedFieldbox->count() == 0)
    return;
  int n = selectedFields.size();
  if (n == 0)
    return;

  int index = selectedFieldbox->currentRow();
  if (index < 0 || index >= n - 1)
    return;

  SelectedField sf = selectedFields[index];
  selectedFields[index] = selectedFields[index + 1];
  selectedFields[index + 1] = sf;

  QString qstr1 = selectedFieldbox->item(index)->text();
  QString qstr2 = selectedFieldbox->item(index + 1)->text();
  selectedFieldbox->item(index)->setText(qstr2);
  selectedFieldbox->item(index + 1)->setText(qstr1);

  //some fields can't be minus
  for (int i = 0; i < n; i++) {
    selectedFieldbox->setCurrentRow(i);
    if (selectedFields[i].minus && (i == 0 || selectedFields[i - 1].minus))
      minusButton->setChecked(false);
  }

  selectedFieldbox->setCurrentRow(index + 1);
}

void FieldDialog::resetOptions()
{
  if (selectedFieldbox->count() == 0)
    return;
  int n = selectedFields.size();
  if (n == 0)
    return;

  int index = selectedFieldbox->currentRow();
  if (index < 0 || index >= n)
    return;

  miutil::miString fopts = getFieldOptions(selectedFields[index].fieldName,
      true);
  if (fopts.empty())
    return;

  selectedFields[index].fieldOpts = fopts;
  selectedFields[index].hourOffset = 0;
  selectedFields[index].hourDiff = 0;
  enableWidgets("none");
  currentFieldOpts.clear();
  enableFieldOptions();
}

miutil::miString FieldDialog::getFieldOptions(
    const miutil::miString& fieldName, bool reset, bool edit) const
{
  miutil::miString fieldname = fieldName;

  map<miutil::miString, miutil::miString>::const_iterator pfopt;

  if (!reset) {
    if (edit) {
      // try private profet options
      pfopt = editFieldOptions.find(fieldname);
      if (pfopt != editFieldOptions.end())
        return pfopt->second;
    }

    // try private options used
    pfopt = fieldOptions.find(fieldname);
    if (pfopt != fieldOptions.end())
      return pfopt->second;
  }

  // following only searches for original options from the setup file

  pfopt = setupFieldOptions.find(fieldname);
  if (pfopt != setupFieldOptions.end())
    return pfopt->second;

  // test known suffixes and prefixes to the original name.

  map<miutil::miString, miutil::miString>::const_iterator pfend =
      setupFieldOptions.end();

  set<std::string>::const_iterator ps;
  size_t l, lname = fieldname.length();

  ps = fieldSuffixes.begin();

  while (pfopt == pfend && ps != fieldSuffixes.end()) {
    if ((l = (*ps).length()) < lname && fieldname.substr(lname - l) == (*ps))
      pfopt = setupFieldOptions.find(fieldname.substr(0, lname - l));
    ps++;
  }

  if (pfopt != pfend)
    return pfopt->second;

  ps = fieldPrefixes.begin();

  while (pfopt == pfend && ps != fieldPrefixes.end()) {
    if ((l = (*ps).length()) < lname && fieldname.substr(0, l) == (*ps))
      pfopt = setupFieldOptions.find(fieldname.substr(l));
    ps++;
  }

  if (pfopt != pfend)
    return pfopt->second;

  //default
  PlotOptions po;
  return po.toString();
}

void FieldDialog::minusField(bool on)
{

  int i = selectedFieldbox->currentRow();

  if (i < 0 || i >= selectedFieldbox->count())
    return;

  QString qstr = selectedFieldbox->currentItem()->text();

  if (on) {
    if (!selectedFields[i].minus) {
      selectedFields[i].minus = true;
      selectedFieldbox->blockSignals(true);
      selectedFieldbox->item(i)->setText("  -  " + qstr);
      selectedFieldbox->blockSignals(false);
    }
    enableWidgets("none");
    //next field can't be minus
    if (selectedFieldbox->count() > i + 1 && selectedFields[i + 1].minus) {
      selectedFieldbox->setCurrentRow(i + 1);
      minusButton->setChecked(false);
      selectedFieldbox->setCurrentRow(i);
    }
  } else {
    if (selectedFields[i].minus) {
      selectedFields[i].minus = false;
      selectedFieldbox->blockSignals(true);
      selectedFieldbox->item(i)->setText(qstr.remove(0, 5));
      selectedFieldbox->blockSignals(false);
      currentFieldOpts.clear();
      enableFieldOptions();
    }
  }

}

void FieldDialog::updateTime()
{

  vector<miutil::miTime> fieldtime;
  int m;

  if ((m = selectedFields.size()) > 0) {

    vector<FieldRequest> request;
    FieldRequest ftr;

    int nr = 0;

    for (int i = 0; i < m; i++) {
      if (!selectedFields[i].inEdit) {
        request.push_back(ftr);
        request[nr].modelName = selectedFields[i].modelName;
        request[nr].paramName = selectedFields[i].fieldName;
        request[nr].plevel = selectedFields[i].level;
        request[nr].elevel = selectedFields[i].idnum;
        request[nr].hourOffset = selectedFields[i].hourOffset;
        request[nr].forecastSpec = 0;
        request[nr].refTime = selectedFields[i].refTime;
        request[nr].zaxis = selectedFields[i].zaxis;
        request[nr].eaxis = selectedFields[i].extraaxis;
        request[nr].grid = selectedFields[i].grid;
        request[nr].plotDefinition = selectedFields[i].plotDefinition;
        request[nr].allTimeSteps = allTimeStepButton->isChecked();
        if (selectedFields[i].forecastSpec) {
          vector<ParsedCommand> vpc = cp->parse(selectedFields[i].fieldOpts);
          int nvpc = vpc.size();
          int j = 0;
          while (j < nvpc && vpc[j].idNumber != 2)
            j++;
          if (j < nvpc) {
            if (vpc[j].key == "forecast.hour")
              request[nr].forecastSpec = 1;
            else if (vpc[j].key == "forecast.hour.loop")
              request[nr].forecastSpec = 2;
            request[nr].forecast = vpc[j].intValue;
          }
        }
        nr++;
      }
    }

    if (nr > 0) {
      fieldtime = m_ctrl->getFieldTime(request);
    }
  }

#ifdef DEBUGREDRAW
  cerr<<"FieldDialog::updateTime emit emitTimes  fieldtime.size="
      <<fieldtime.size()<<endl;
#endif
  emit emitTimes("field", fieldtime);

  //  allTimeStepButton->setChecked(false);
}

void FieldDialog::addField(miutil::miString str)
{
  //  cerr <<"void FieldDialog::addField(miutil::miString str) "<<endl;
  bool remove = false;
  vector<miutil::miString> token = str.split(1, " ", true);
  if (token.size() == 2 && token[0] == "REMOVE") {
    str = token[1];
    remove = true;
  }

  vector<miutil::miString> vstr = getOKString();

  //remove option overlay=1 from all strings
  //(should be a more general setOption()
  for (unsigned int i = 0; i < vstr.size(); i++) {
    vstr[i].replace("overlay=1", "");
  }

  vector<miutil::miString>::iterator p = vstr.begin();
  for (; p != vstr.end(); p++) {
    if ((*p).contains(str)) {
      p = vstr.erase(p);
      if (p == vstr.end())
        break;
    }
  }
  if (!remove) {
    vstr.push_back(str);
  }
  putOKString(vstr);

}

void FieldDialog::fieldEditUpdate(miutil::miString str)
{

#ifdef DEBUGREDRAW
  if (str.empty()) cerr<<"FieldDialog::fieldEditUpdate STOP"<<endl;
  else cerr<<"FieldDialog::fieldEditUpdate START "<<str<<endl;
#endif

  int i, j, m, n = selectedFields.size();

  if (str.empty()) {

    // remove fixed edit field(s)
    vector<int> keep;
    m = selectedField2edit_exists.size();
    bool change = false;
    for (i = 0; i < n; i++) {
      if (!selectedFields[i].inEdit) {
        keep.push_back(i);
      } else if (i < m && selectedField2edit_exists[i]) {
        selectedFields[i] = selectedField2edit[i];
        miutil::miString text = selectedFields[i].modelName + " "
            + selectedFields[i].fieldName;
        QString qtext = text.c_str();
        selectedFieldbox->item(i)->setText(qtext);
        keep.push_back(i);
        change = true;
      }
    }
    m = keep.size();
    if (m < n) {
      for (i = 0; i < m; i++) {
        j = keep[i];
        selectedFields[i] = selectedFields[j];
        QString qstr = selectedFieldbox->item(j)->text();
        selectedFieldbox->item(i)->setText(qstr);
      }
      selectedFields.resize(m);
      if (m == 0) {
        selectedFieldbox->clear();
      } else {
        for (i = m; i < n; i++)
          selectedFieldbox->takeItem(i);
      }
    }

    numEditFields = 0;
    selectedField2edit.clear();
    selectedField2edit_exists.clear();
    if (change) {
      updateTime();
#ifdef DEBUGREDRAW
      cerr<<"FieldDialog::fieldEditUpdate emit FieldApply"<<endl;
#endif
      emit FieldApply();
    }

  } else {

    // add edit field (and remove the original field)
    int indrm = -1;
    SelectedField sf;

    vector<miutil::miString> vstr = str.split(' ');

    if (vstr.size() == 1) {
      // In original edit, str=fieldName if the field is not already read
      sf.fieldName = vstr[0];
    } else {
      miutil::miString modelName;
      miutil::miString fieldName;
      if ( str.contains("model=") ) {
        //edit field from profet
        bool allTimeSteps;
        decodeString_cdmSyntax(str, sf, allTimeSteps);
      } else if (vstr.size() >= 2) {
        // new edit field
        sf.modelName = vstr[0];
        sf.fieldName = vstr[1];
      }
      for (i = 0; i < n; i++) {
        if (!selectedFields[i].inEdit) {
          if (selectedFields[i].modelName == sf.modelName
              && selectedFields[i].fieldName == sf.fieldName
              && selectedFields[i].refTime == sf.refTime)
            break;
        }
      }
      if (i < n) {
        sf = selectedFields[i];
        indrm = i;
      }
    }


  map<miutil::miString, miutil::miString>::const_iterator pfo;
  if ((pfo = editFieldOptions.find(sf.fieldName))
      != editFieldOptions.end()) {
    sf.fieldOpts = pfo->second;
    } else if ((pfo = fieldOptions.find(sf.fieldName))
        != fieldOptions.end()) {
      sf.fieldOpts = pfo->second;
    }

    // Searching for time=
    unsigned int j = 0;
    while (j < vstr.size() && !vstr[j].downcase().contains("time="))
      j++;
    if (j < vstr.size()) {
      vector<miutil::miString> stokens = vstr[j].split("=");
      if (stokens.size() == 2) { //Profet edit, using FieldPlot
        sf.time = stokens[1];
        sf.editPlot = false;
      }
    } else { //Orig edit, using EditManager
      sf.editPlot = true;
    }

    sf.inEdit = true;
    sf.external = false;
    sf.indexMGR = -1;
    sf.indexM = -1;
    sf.hourOffset = 0;
    sf.hourDiff = 0;
    sf.minus = false;
    if (indrm >= 0) {
      selectedField2edit.push_back(selectedFields[indrm]);
      selectedField2edit_exists.push_back(true);
      n = selectedFields.size();
      for (i = indrm; i < n - 1; i++)
        selectedFields[i] = selectedFields[i + 1];
      selectedFields.pop_back();
      selectedFieldbox->takeItem(indrm);
    } else {
      SelectedField sfdummy;
      selectedField2edit.push_back(sfdummy);
      selectedField2edit_exists.push_back(false);
    }

    vector<ParsedCommand> vpopt = cp->parse(sf.fieldOpts);
    cp->replaceValue(vpopt, "field.smooth", "0", 0);
    sf.fieldOpts = cp->unParse(vpopt);

    n = selectedFields.size();
    SelectedField sfdummy;
    selectedFields.push_back(sfdummy);
    for (i = n; i > numEditFields; i--)
      selectedFields[i] = selectedFields[i - 1];
    selectedFields[numEditFields] = sf;

    miutil::miString text = editName + " " + sf.fieldName;
    selectedFieldbox->insertItem(numEditFields, QString(text.c_str()));
    selectedFieldbox->setCurrentRow(numEditFields);
    if (!sf.editPlot) {
      selectedFields[numEditFields].fieldOpts = getFieldOptions(
          selectedFields[numEditFields].fieldName, false,
          selectedFields[numEditFields].inEdit);
    }
    numEditFields++;

    updateTime();
  }

  if (!selectedFields.empty())
    enableFieldOptions();
  else
    enableWidgets("none");
}

void FieldDialog::allTimeStepToggled(bool on)
{
  updateTime();

}

void FieldDialog::applyClicked()
{
  if (historyOkButton->isEnabled())
    historyOk();
  emit FieldApply();
}

void FieldDialog::applyhideClicked()
{
  if (historyOkButton->isEnabled())
    historyOk();
  emit FieldHide();
  emit FieldApply();
}

void FieldDialog::hideClicked()
{
  emit FieldHide();
}

void FieldDialog::helpClicked()
{
  emit showsource("ug_fielddialogue.html");
}

void FieldDialog::closeEvent(QCloseEvent* e)
{
  emit FieldHide();
}


