
#include "qtVcrossStyleWidget.h"

#include "diLinetype.h"
#include "qtUtility.h"

#include <diField/diMetConstants.h>
#include <puTools/miStringFunctions.h>

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

#define MILOGGER_CATEGORY "diana.VcrossStyleWidget"
#include <miLogger/miLogging.h>

#define DISABLE_EXTREMES 1
#define DISABLE_PATTERNS 1
#define DISABLE_LINE_SMOOTHING 1
#define DISABLE_HOUROFFSET 1

namespace {
QWidget* makeSeparator(QWidget* parent, bool horizontal)
{
  QFrame* line = new QFrame(parent);
  line->setFrameShape(horizontal ? QFrame::HLine : QFrame::VLine);
  line->setFrameShadow(QFrame::Sunken);
  return line;
}
} // namespace

// ========================================================================

VcrossStyleWidget::VcrossStyleWidget(QWidget* parent)
  : QTabWidget(parent)
  , cp(new CommandParser())
{
  addTab(createBasicTab(), tr("Basic"));
  addTab(createAdvancedTab(), tr("Advanced"));

  // add options to the cp's keyDataBase
  cp->addKey("model",      "",1,CommandParser::cmdString);
  cp->addKey("field",      "",1,CommandParser::cmdString);
  cp->addKey("hour.offset","",1,CommandParser::cmdInt);

  // add more plot options to the cp's keyDataBase
  cp->addKey("colour",         "",0,CommandParser::cmdString);
  cp->addKey("colours",        "",0,CommandParser::cmdString);
  cp->addKey("linewidth",      "",0,CommandParser::cmdInt);
  cp->addKey("linetype",       "",0,CommandParser::cmdString);
  cp->addKey("line.interval",  "",0,CommandParser::cmdFloat);
  cp->addKey("density",        "",0,CommandParser::cmdInt);
  cp->addKey("vector.unit",    "",0,CommandParser::cmdFloat);
  cp->addKey("extreme.type",   "",0,CommandParser::cmdString);
  cp->addKey("extreme.size",   "",0,CommandParser::cmdFloat);
  cp->addKey("extreme.limits", "",0,CommandParser::cmdString);
  cp->addKey("line.smooth",    "",0,CommandParser::cmdInt);
  cp->addKey("zero.line",      "",0,CommandParser::cmdInt);
  cp->addKey("value.label",    "",0,CommandParser::cmdInt);
  cp->addKey("label.size",     "",0,CommandParser::cmdFloat);
  cp->addKey("base",           "",0,CommandParser::cmdFloat);
  //cp->addKey("undef.masking",  "",0,CommandParser::cmdInt);
  //cp->addKey("undef.colour",   "",0,CommandParser::cmdString);
  //cp->addKey("undef.linewidth","",0,CommandParser::cmdInt);
  //cp->addKey("undef.linetype", "",0,CommandParser::cmdString);
  cp->addKey("palettecolours", "",0,CommandParser::cmdString);
  cp->addKey("minvalue",       "",0,CommandParser::cmdFloat);
  cp->addKey("maxvalue",       "",0,CommandParser::cmdFloat);
  cp->addKey("table",          "",0,CommandParser::cmdInt);
  cp->addKey("patterns",       "",0,CommandParser::cmdString);
  cp->addKey("patterncolour",  "",0,CommandParser::cmdString);
  cp->addKey("repeat",      "",0,CommandParser::cmdInt);
  cp->addKey("alpha",          "",0,CommandParser::cmdInt);
}

void VcrossStyleWidget::setOptions(const std::string& fopt, const std::string& defaultopt)
{
  defaultOptions = defaultopt;
  currentFieldOpts = fopt;
  enableFieldOptions();
}

const std::string& VcrossStyleWidget::options() const
{
  if (not currentFieldOpts.empty())
    return currentFieldOpts;
  return defaultOptions;
}

bool VcrossStyleWidget::valid() const
{
  return not options().empty();
}

QWidget* VcrossStyleWidget::createBasicTab()
{
  // Colours
  colourInfo = Colour::getColourInfo();
  nr_colors = colourInfo.size();

  csInfo = ColourShading::getColourShadingInfo();
  patternInfo = Pattern::getAllPatternInfo();

  // linewidths
  nr_linewidths= 12;

  // linetypes
  linetypes = Linetype::getLinetypeNames();
  nr_linetypes = linetypes.size();

  // density (of arrows etc, 0=automatic)
  densityStringList << "Auto";
  for (int i=0;  i<10; i++)
    densityStringList << QString::number(i);
  for (int i=10;  i<60; i+=10)
    densityStringList << QString::number(i);
  densityStringList << QString::number(100);

  QWidget* basic = new QWidget(this);

  // colorCbox
  colorlabel= new QLabel(tr("Colour"), basic);
  colorCbox= ColourBox(basic, colourInfo, false, 0, tr("off").toStdString(), true);
  colorCbox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
  colorCbox->setEnabled(false);

  connect(colorCbox, SIGNAL(activated(int)),
      SLOT(colorCboxActivated(int)));

  // linewidthcbox
  linewidthlabel= new QLabel(tr("Line width"), basic);
  lineWidthCbox = LinewidthBox(basic, false);
  lineWidthCbox->setEnabled(false);

  connect(lineWidthCbox, SIGNAL(activated(int)),
      SLOT(lineWidthCboxActivated(int)));

  // linetypecbox
  linetypelabel= new QLabel(tr("Line type"), basic);
  lineTypeCbox = LinetypeBox(basic, false);
  lineTypeCbox->setEnabled(false);

  connect(lineTypeCbox, SIGNAL(activated(int)),
      SLOT(lineTypeCboxActivated(int)));

  // lineinterval
  lineintervallabel = new QLabel(tr("Line interval"), basic);
  lineintervalCbox = new QComboBox(basic);
  lineintervalCbox->setEnabled(false);

  connect(lineintervalCbox, SIGNAL(activated(int)),
      SLOT(lineintervalCboxActivated(int)));

  // density
  densitylabel = new QLabel(tr("Density"), basic);
  densityCbox = new QComboBox(basic);
  densityCbox->setEnabled( false );

  connect(densityCbox, SIGNAL(activated(int)),
      SLOT(densityCboxActivated(int)));

  // vectorunit
  vectorunitlabel = new QLabel(tr("Unit"), basic);
  vectorunitCbox = new QComboBox(basic);

  connect(vectorunitCbox, SIGNAL(activated(int)),
      SLOT(vectorunitCboxActivated(int)));

  QGridLayout* optlayout = new QGridLayout();
  optlayout->addWidget(colorlabel,       0, 0);
  optlayout->addWidget(colorCbox,        0, 1);
  optlayout->addWidget(linewidthlabel,   1, 0);
  optlayout->addWidget(lineWidthCbox,    1, 1);
  optlayout->addWidget(linetypelabel,    2, 0);
  optlayout->addWidget(lineTypeCbox,     2, 1);
  optlayout->addWidget(lineintervallabel,3, 0);
  optlayout->addWidget(lineintervalCbox, 3, 1);
  optlayout->addWidget(densitylabel,     4, 0);
  optlayout->addWidget(densityCbox,      4, 1);
  optlayout->addWidget(vectorunitlabel,  5, 0);
  optlayout->addWidget(vectorunitCbox,   5, 1);

  resetOptionsButton = NormalPushButton( tr("R"), this );
  resetOptionsButton->setEnabled(false);
  optlayout->addWidget(resetOptionsButton, 6, 1);
  optlayout->setRowStretch(optlayout->rowCount(), 1);

  basic->setLayout(optlayout);

  return basic;
}

QWidget* VcrossStyleWidget::createAdvancedTab()
{
  METLIBS_LOG_SCOPE();

  QWidget* advFrame = new QWidget(this);

  // mark min/max values
#ifndef DISABLE_EXTREMES
  extremeValueCheckBox= new QCheckBox(tr("Min/max values"), advFrame);
  extremeValueCheckBox->setChecked(false);
  extremeValueCheckBox->setEnabled(false);
  connect(extremeValueCheckBox, SIGNAL(toggled(bool)),
      SLOT(extremeValueCheckBoxToggled(bool)));

  //QLabel* extremeTypeLabelHead= new QLabel( "Min,max", advFrame );
  QLabel* extremeSizeLabel= new QLabel(tr("Size"), advFrame);
  extremeSizeSpinBox= new QSpinBox(advFrame);
  extremeSizeSpinBox->setMinimum(5);
  extremeSizeSpinBox->setMaximum(300);
  extremeSizeSpinBox->setSingleStep(5);
  extremeSizeSpinBox->setWrapping(true);
  extremeSizeSpinBox->setSuffix("%");
  extremeSizeSpinBox->setValue(100);
  extremeSizeSpinBox->setEnabled(false);
  connect(extremeSizeSpinBox, SIGNAL(valueChanged(int)),
      SLOT(extremeSizeChanged(int)));

  extremeLimits<<"Off";
  for (int i=0; i<MetNo::Constants::nLevelTable; i++)
    extremeLimits << QString::number(MetNo::Constants::pLevelTable[i]);

  QLabel* extremeLimitMinLabel = new QLabel(tr("Level low"), advFrame);
  extremeLimitMinComboBox= new QComboBox(advFrame);
  extremeLimitMinComboBox->addItems(extremeLimits);
  extremeLimitMinComboBox->setEnabled(false);
  connect(extremeLimitMinComboBox, SIGNAL(activated(int)),
      SLOT(extremeLimitsChanged()));
  
  QLabel* extremeLimitMaxLabel = new QLabel(tr("Level high"), advFrame);
  extremeLimitMaxComboBox= new QComboBox(advFrame);
  extremeLimitMaxComboBox->addItems(extremeLimits);
  extremeLimitMaxComboBox->setEnabled(false);
  connect(extremeLimitMaxComboBox, SIGNAL(activated(int)),
      SLOT(extremeLimitsChanged()));

  QGridLayout* extremeLayout = new QGridLayout();
  extremeLayout->addWidget(extremeSizeLabel,        0, 0);
  extremeLayout->addWidget(extremeLimitMinLabel,    0, 1);
  extremeLayout->addWidget(extremeLimitMaxLabel,    0, 2);
  extremeLayout->addWidget(extremeSizeSpinBox,      1, 0);
  extremeLayout->addWidget(extremeLimitMinComboBox, 1, 1);
  extremeLayout->addWidget(extremeLimitMaxComboBox, 1, 2);
#endif

#ifndef DISABLE_LINE_SMOOTHING
  // line smoothing
  QLabel* lineSmoothLabel= new QLabel(tr("Smooth lines"), advFrame);
  lineSmoothSpinBox= new QSpinBox(advFrame);
  lineSmoothSpinBox->setMinimum(0);
  lineSmoothSpinBox->setMaximum(50);
  lineSmoothSpinBox->setSingleStep(2);
  lineSmoothSpinBox->setSpecialValueText(tr("Off"));
  lineSmoothSpinBox->setValue(0);
  lineSmoothSpinBox->setEnabled(false);
  connect(lineSmoothSpinBox, SIGNAL(valueChanged(int)),
      SLOT(lineSmoothChanged(int)));
#endif

  QLabel* labelSizeLabel= new QLabel(tr("Digit size"), advFrame);
  labelSizeSpinBox= new QSpinBox(advFrame);
  labelSizeSpinBox->setMinimum(5);
  labelSizeSpinBox->setMaximum(399);
  labelSizeSpinBox->setSingleStep(5);
  labelSizeSpinBox->setWrapping(true);
  labelSizeSpinBox->setSuffix("%");
  labelSizeSpinBox->setValue(100);
  labelSizeSpinBox->setEnabled(false);
  connect(labelSizeSpinBox, SIGNAL(valueChanged(int)),
      SLOT(labelSizeChanged(int)));

#ifndef DISABLE_HOUROFFSET
  QLabel* hourOffsetLabel= new QLabel(tr("Time offset"), advFrame);
  hourOffsetSpinBox= new QSpinBox(advFrame);
  hourOffsetSpinBox->setMinimum(-72);
  hourOffsetSpinBox->setMaximum(72);
  hourOffsetSpinBox->setSuffix(tr(" hour(s)"));
  hourOffsetSpinBox->setValue(0);
  hourOffsetSpinBox->setEnabled(false);
  connect(hourOffsetSpinBox, SIGNAL(valueChanged(int)),
      SLOT(hourOffsetChanged(int)));
#endif

  // Undefined masking
  ////QLabel* undefMaskingLabel= new QLabel( "Udefinert", advFrame );
  //undefMaskingCbox= new QComboBox( false, advFrame );
  //undefMaskingCbox->setEnabled( false );
  //undefMasking.push_back("Umarkert");
  //undefMasking.push_back("Farget");
  //undefMasking.push_back("Linjer");
  //connect( undefMaskingCbox, SIGNAL( activated(int) ),
  //	   SLOT(undefMaskingActivated(int) ) );

  // Undefined masking colour
  ////QLabel* undefColourLabel= new QLabel( COLOR, advFrame );
  //undefColourCbox= new QComboBox( false, advFrame );
  //undefColourCbox->setEnabled( false );
  //connect( undefColourCbox, SIGNAL( activated(int) ),
  //	   SLOT(undefColourActivated(int) ) );

  // Undefined masking linewidth (if used)
  ////QLabel* undefLinewidthLabel= new QLabel( LINEWIDTH, advFrame );
  //undefLinewidthCbox= new QComboBox( false, advFrame );
  //undefLinewidthCbox->setEnabled( false );
  //connect( undefLinewidthCbox, SIGNAL( activated(int) ),
  //	   SLOT(undefLinewidthActivated(int) ) );

  // Undefined masking linetype (if used)
  ////QLabel* undefLinetypeLabel= new QLabel( LINETYPE, advFrame );
  //undefLinetypeCbox= new QComboBox( false, advFrame );
  //undefLinetypeCbox->setEnabled( false );
  //connect( undefLinetypeCbox, SIGNAL( activated(int) ),
  //	   SLOT(undefLinetypeActivated(int) ) );

  // enable/disable zero line (isoline with value=0)
  zeroLineCheckBox= new QCheckBox(tr("Zero-line"), advFrame);
  zeroLineCheckBox->setChecked(true);
  zeroLineCheckBox->setEnabled(false);
  connect(zeroLineCheckBox, SIGNAL(toggled(bool)),
      SLOT(zeroLineCheckBoxToggled(bool)));

  // enable/disable numbers on isolines
  valueLabelCheckBox= new QCheckBox(tr("Number on line"), advFrame);
  valueLabelCheckBox->setChecked(true);
  valueLabelCheckBox->setEnabled(false);
  connect(valueLabelCheckBox, SIGNAL(toggled(bool)),
      SLOT(valueLabelCheckBoxToggled(bool)));

  QLabel* shadingLabel    = new QLabel( tr("Palette"),     advFrame );
  QLabel* shadingcoldLabel= new QLabel( tr("Palette (-)"), advFrame );
#ifndef DISABLE_PATTERNS
  QLabel* patternLabel    = new QLabel( tr("Pattern"),     advFrame );
#endif
  QLabel* alphaLabel      = new QLabel( tr("Alpha"),       advFrame );
  QLabel* baseLabel       = new QLabel( tr("Basis value"), advFrame);
  QLabel* minLabel        = new QLabel( tr("Min"),         advFrame);
  QLabel* maxLabel        = new QLabel( tr("Max"),         advFrame);


  //  tableCheckBox = new QCheckBox(tr("Table"), advFrame);
  //  tableCheckBox->setEnabled(false);
  //  connect( tableCheckBox, SIGNAL( toggled(bool) ),
  //	   SLOT(tableCheckBoxToggled(bool) ) );

  repeatCheckBox = new QCheckBox(tr("Repeat"), advFrame);
  repeatCheckBox->setEnabled(false);
  connect(repeatCheckBox, SIGNAL(toggled(bool)),
      SLOT(repeatCheckBoxToggled(bool)));

  //shading
  shadingComboBox = PaletteBox(advFrame, csInfo, false, 0, tr("Off").toStdString());
  connect(shadingComboBox, SIGNAL(activated(int)),
      SLOT(shadingChanged()));

  shadingSpinBox = new QSpinBox(advFrame);
  shadingSpinBox->setMinimum(0);
  shadingSpinBox->setMaximum(99);
  shadingSpinBox->setSpecialValueText(tr("Auto"));
  shadingSpinBox->setEnabled(false);
  connect(shadingSpinBox, SIGNAL(valueChanged(int)),
      SLOT(shadingChanged()));

  shadingcoldComboBox = PaletteBox(advFrame, csInfo, false, 0, tr("Off").toStdString());
  connect(shadingcoldComboBox, SIGNAL(activated(int)),
      SLOT(shadingChanged()));

  shadingcoldSpinBox = new QSpinBox(advFrame);
  shadingcoldSpinBox->setMinimum(0);
  shadingcoldSpinBox->setMaximum(99);
  shadingcoldSpinBox->setSpecialValueText(tr("Auto"));
  shadingcoldSpinBox->setEnabled(false);
  connect(shadingcoldSpinBox, SIGNAL(valueChanged(int)),
      SLOT(shadingChanged()));

#ifndef DISABLE_PATTERNS
  //pattern
  patternComboBox = PatternBox(advFrame, patternInfo, false, 0, tr("Off").toStdString());
  connect(patternComboBox, SIGNAL(activated(int)),
      SLOT(patternComboBoxToggled(int)));

  //pattern colour
  patternColourBox = ColourBox(advFrame, colourInfo, false, 0, tr("Auto").toStdString());
  connect(patternColourBox, SIGNAL(activated(int)),
      SLOT(patternColourBoxToggled(int)));
#endif

  //alpha blending
  alphaSpinBox = new QSpinBox(advFrame);
  alphaSpinBox->setMinimum(0);
  alphaSpinBox->setMaximum(255);
  alphaSpinBox->setSingleStep(5);
  alphaSpinBox->setEnabled(false);
  alphaSpinBox->setValue(255);
  connect(alphaSpinBox, SIGNAL(valueChanged(int)),
      SLOT(alphaChanged(int)));
  //zero value
  zero1ComboBox= new QComboBox(advFrame);
  zero1ComboBox->setEnabled(false);
  connect(zero1ComboBox, SIGNAL(activated(int)),
      SLOT(zero1ComboBoxToggled(int)));

  //min
  min1ComboBox = new QComboBox(advFrame);
  min1ComboBox->setEnabled(false);

  //max
  max1ComboBox = new QComboBox(advFrame);
  max1ComboBox->setEnabled(false);

  connect( min1ComboBox, SIGNAL(activated(int)),
      SLOT(min1ComboBoxToggled(int)));
  connect( max1ComboBox, SIGNAL( activated(int)),
      SLOT(max1ComboBoxToggled(int)));


  QVBoxLayout *adv1Layout = new QVBoxLayout();
  int space= 6;
  adv1Layout->addStretch();
#ifndef DISABLE_EXTREMES
  adv1Layout->addWidget(extremeValueCheckBox);
  adv1Layout->addSpacing(space);
  adv1Layout->addLayout(extremeLayout);
  adv1Layout->addSpacing(space);
  adv1Layout->addWidget(makeSeparator(advFrame, true));
  adv1Layout->addSpacing(space);
#endif // DISABLE_EXTREMES
#ifndef DISABLE_LINE_SMOOTHING
  adv1Layout->addWidget(lineSmoothLabel);
  adv1Layout->addWidget(lineSmoothSpinBox);
  adv1Layout->addSpacing(space);
#endif
  adv1Layout->addWidget(labelSizeLabel);
  adv1Layout->addWidget(labelSizeSpinBox);
  adv1Layout->addSpacing(space);
#ifndef DISABLE_HOUROFFSET
  adv1Layout->addWidget(hourOffsetLabel);
  adv1Layout->addWidget(hourOffsetSpinBox);
  adv1Layout->addSpacing(space);
#endif
  adv1Layout->addWidget(zeroLineCheckBox);
  adv1Layout->addWidget(valueLabelCheckBox);
  adv1Layout->addSpacing(space);

  QGridLayout* adv2Layout = new QGridLayout();
  { int row = 0;
    adv2Layout->addWidget(makeSeparator(advFrame, true), row, 0,1,3);
    //  adv2Layout->addWidget( tableCheckBox,      1, 0 );
    row += 1;
    adv2Layout->addWidget( repeatCheckBox,     row, 0 );
    row += 1;
    adv2Layout->addWidget( shadingLabel,       row, 0 );
    adv2Layout->addWidget( shadingComboBox,    row, 1 );
    adv2Layout->addWidget( shadingSpinBox,     row, 2 );
    row += 1;
    adv2Layout->addWidget( shadingcoldLabel,   row, 0 );
    adv2Layout->addWidget( shadingcoldComboBox,row, 1 );
    adv2Layout->addWidget( shadingcoldSpinBox, row, 2 );
#ifndef DISABLE_PATTERNS
    row += 1;
    adv2Layout->addWidget( patternLabel,       row, 0 );
    adv2Layout->addWidget( patternComboBox,    row, 1 );
    adv2Layout->addWidget( patternColourBox,   row, 2 );
#endif
    row += 1;
    adv2Layout->addWidget( alphaLabel,         row, 0 );
    adv2Layout->addWidget( alphaSpinBox,       row, 1 );
    row += 1;
    adv2Layout->addWidget( baseLabel,          row, 0 );
    adv2Layout->addWidget( zero1ComboBox,      row, 1 );
    row += 1;
    adv2Layout->addWidget( minLabel,           row, 0 );
    adv2Layout->addWidget( min1ComboBox,       row, 1 );
    row += 1;
    adv2Layout->addWidget( maxLabel,           row, 0 );
    adv2Layout->addWidget( max1ComboBox,       row, 1 );
  }
  QVBoxLayout *advLayout = new QVBoxLayout();
  advLayout->addLayout(adv1Layout);
  advLayout->addLayout(adv2Layout);
  advLayout->addStretch();
  advFrame->setLayout(advLayout);

  return advFrame;
}

#if 0
void VcrossStyleWidget::retranslateUI()
{
  resetOptionsButton->setToolTip(tr("reset plot layout"));
#ifndef DISABLE_EXTREMES
  extremeSizeSpinBox->setToolTip(tr("Size of min/max marker"));
  extremeLimitMinComboBox->setToolTip(tr("Find min/max value above this vertical level (unit hPa)"));
  extremeLimitMaxComboBox->setToolTip(tr("Find min/max value below this vertical level (unit hPa)"));
#endif // DISABLE_EXTREMES
}
#endif

void VcrossStyleWidget::enableFieldOptions()
{
  METLIBS_LOG_SCOPE(LOGVAL(currentFieldOpts));

  disableFieldOptions();

  resetOptionsButton->setEnabled(true);

  vpcopt = cp->parse(currentFieldOpts);

  int nc, i;
  float e;

  // colour(s)
  if ((nc=cp->findKey(vpcopt,"colour"))>=0) {
    shadingComboBox->setEnabled(true);
    shadingcoldComboBox->setEnabled(true);
    if (!colorCbox->isEnabled()) {
      colorCbox->setEnabled(true);
    }
    i=0;
    if(miutil::to_lower(vpcopt[nc].allValue) == "off" ||
        miutil::to_lower(vpcopt[nc].allValue) == "av" ){
      updateFieldOptions("colour","off");
      colorCbox->setCurrentIndex(0);
    } else {
      while (i<nr_colors
          && miutil::to_lower(vpcopt[nc].allValue)!=colourInfo[i].name) i++;
      if (i==nr_colors) i=0;
      updateFieldOptions("colour",colourInfo[i].name);
      colorCbox->setCurrentIndex(i+1);
    }
  } else if (colorCbox->isEnabled()) {
    colorCbox->setEnabled(false);
  }

  //contour shading
  if ((nc=cp->findKey(vpcopt,"palettecolours"))>=0) {
    shadingComboBox->setEnabled(true);
    shadingSpinBox->setEnabled(true);
    shadingcoldComboBox->setEnabled(true);
    shadingcoldSpinBox->setEnabled(true);
    //    tableCheckBox->setEnabled(true);
#ifndef DISABLE_PATTERNS
    patternComboBox->setEnabled(true);
#endif
    repeatCheckBox->setEnabled(true);
    alphaSpinBox->setEnabled(true);
    std::vector<std::string> tokens = miutil::split(vpcopt[nc].allValue,",");
    std::vector<std::string> stokens = miutil::split(tokens[0],";");
    if(stokens.size()==2)
      shadingSpinBox->setValue(atoi(stokens[1].c_str()));
    else
      shadingSpinBox->setValue(0);
    int nr_cs = csInfo.size();
    std::string str;
    i=0;
    while (i<nr_cs && stokens[0]!=csInfo[i].name) i++;
    if (i==nr_cs) {
      str = "off";
      shadingComboBox->setCurrentIndex(0);
      shadingcoldComboBox->setCurrentIndex(0);
    }else {
      str = tokens[0];
      shadingComboBox->setCurrentIndex(i+1);
    }
    if(tokens.size()==2){
      std::vector<std::string> stokens = miutil::split(tokens[1],";");
      if(stokens.size()==2)
        shadingcoldSpinBox->setValue(atoi(stokens[1].c_str()));
      shadingcoldSpinBox->setValue(0);
      i=0;
      while (i<nr_cs && stokens[0]!=csInfo[i].name) i++;
      if (i==nr_cs) {
        shadingcoldComboBox->setCurrentIndex(0);
      }else {
        str += "," + tokens[1];
        shadingcoldComboBox->setCurrentIndex(i+1);
      }
    } else {
      shadingcoldComboBox->setCurrentIndex(0);
    }
    updateFieldOptions("palettecolours",str,-1);
  } else {
    updateFieldOptions("palettecolours","off",-1);
    shadingComboBox->setCurrentIndex(0);
    shadingComboBox->setEnabled(false);
    shadingcoldComboBox->setCurrentIndex(0);
    shadingcoldComboBox->setEnabled(false);
    //    tableCheckBox->setEnabled(false);
    //    updateFieldOptions("table","remove");
#ifndef DISABLE_PATTERNS
    patternComboBox->setEnabled(false);
    updateFieldOptions("patterns","remove");
#endif
    repeatCheckBox->setEnabled(false);
    updateFieldOptions("repeat","remove");
    alphaSpinBox->setEnabled(false);
    updateFieldOptions("alpha","remove");
  }

  //pattern
#ifndef DISABLE_PATTERNS
  if ((nc=cp->findKey(vpcopt,"patterns"))>=0) {
    patternComboBox->setEnabled(true);
    patternColourBox->setEnabled(true);
    std::string value = vpcopt[nc].allValue;
    //METLIBS_LOG_DEBUG("patterns:"<<value);
    int nr_p = patternInfo.size();
    std::string str;
    i=0;
    while (i<nr_p && value!=patternInfo[i].name) i++;
    if (i==nr_p) {
      str = "off";
      patternComboBox->setCurrentIndex(0);
    }else {
      str = patternInfo[i].name;
      patternComboBox->setCurrentIndex(i+1);
    }
    updateFieldOptions("patterns",str,-1);
  } else {
    updateFieldOptions("patterns","off",-1);
    patternComboBox->setCurrentIndex(0);
  }

  //pattern colour
  if ((nc=cp->findKey(vpcopt,"patterncolour"))>=0) {
    i=0;
    while (i<nr_colors && vpcopt[nc].allValue!=colourInfo[i].name) i++;
    if (i==nr_colors) {
      updateFieldOptions("patterncolour","remove");
      patternColourBox->setCurrentIndex(0);
    }else {
      updateFieldOptions("patterncolour",colourInfo[i].name);
      patternColourBox->setCurrentIndex(i+1);
    }
  }
#endif

  //table
  //  nc=cp->findKey(vpcopt,"table");
  //  if (nc>=0) {
  //    bool on= vpcopt[nc].allValue=="1";
  //    tableCheckBox->setChecked( on );
  //    tableCheckBox->setEnabled(true);
  //    tableCheckBoxToggled(on);
  //  }

  //repeat
  nc=cp->findKey(vpcopt,"repeat");
  if (nc>=0) {
    bool on= vpcopt[nc].allValue=="1";
    repeatCheckBox->setChecked( on );
    repeatCheckBox->setEnabled(true);
    repeatCheckBoxToggled(on);
  }

  //alpha shading
  if ((nc=cp->findKey(vpcopt,"alpha"))>=0) {
    if (vpcopt[nc].intValue.size()>0) i=vpcopt[nc].intValue[0];
    else i=255;
    alphaSpinBox->setValue(i);
    alphaSpinBox->setEnabled(true);
  } else {
    alphaSpinBox->setValue(255);
    updateFieldOptions("alpha","remove");
  }

  // linetype
  if ((nc=cp->findKey(vpcopt,"linetype"))>=0) {
    lineTypeCbox->setEnabled(true);
    i=0;
    while (i<nr_linetypes && vpcopt[nc].allValue!=linetypes[i]) i++;
    if (i==nr_linetypes) {
      i=0;
      updateFieldOptions("linetype",linetypes[i]);
    }
    lineTypeCbox->setCurrentIndex(i);
  } else if (lineTypeCbox->isEnabled()) {
    lineTypeCbox->setEnabled(false);
  }

  // linewidth
  if ((nc=cp->findKey(vpcopt,"linewidth"))>=0) {
    lineWidthCbox->setEnabled(true);
    i=0;
    while (i<nr_linewidths && vpcopt[nc].allValue!=miutil::from_number(i+1)) i++;
    if (i==nr_linewidths) {
      i=0;
      updateFieldOptions("linewidth",miutil::from_number(i+1));
    }
    lineWidthCbox->setCurrentIndex(i);
  } else if (lineWidthCbox->isEnabled()) {
    lineWidthCbox->setEnabled(false);
  }

  // line interval (isoline contouring)
  float ekv=-1.;
  if ((nc=cp->findKey(vpcopt,"line.interval"))>=0) {
    if (vpcopt[nc].floatValue.size()>0) ekv=vpcopt[nc].floatValue[0];
    else ekv= 10.;
    lineintervals= numberList( lineintervalCbox, ekv);
    lineintervalCbox->setEnabled(true);
  } else if (lineintervalCbox->isEnabled()) {
    lineintervalCbox->clear();
    lineintervalCbox->setEnabled(false);
  }

  // wind/vector density
  if ((nc=cp->findKey(vpcopt,"density"))>=0) {
    if (!densityCbox->isEnabled()) {
      densityCbox->addItems(densityStringList);
      densityCbox->setEnabled(true);
    }
    std::string s;
    if (!vpcopt[nc].strValue.empty()) {
      s= vpcopt[nc].strValue[0];
    } else {
      s= "0";
      updateFieldOptions("density",s);
    }
    if (s=="0") {
      i=0;
    } else {
      i = densityStringList.indexOf(QString(s.c_str()));
      if (i==-1) {
        densityStringList <<QString(s.c_str());
        densityCbox->addItem(QString(s.c_str()));
        i=densityCbox->count()-1;
      }
    }
    densityCbox->setCurrentIndex(i);
  } else if (densityCbox->isEnabled()) {
    densityCbox->clear();
    densityCbox->setEnabled(false);
  }

  // vectorunit (vector length unit)
  if ((nc=cp->findKey(vpcopt,"vector.unit"))>=0) {
    if (vpcopt[nc].floatValue.size()>0) e= vpcopt[nc].floatValue[0];
    else e=5;
    vectorunit= numberList( vectorunitCbox, e);
  } else if (vectorunitCbox->isEnabled()) {
    vectorunitCbox->clear();
  }

#ifndef DISABLE_EXTREMES
  if ((nc=cp->findKey(vpcopt,"extreme.size"))>=0) {
    if (vpcopt[nc].floatValue.size()>0) e=vpcopt[nc].floatValue[0];
    else e=1.0;
    i= (int(e*100.+0.5))/5 * 5;
    extremeSizeSpinBox->setValue(i);
  } else {
    extremeSizeSpinBox->setValue(100);
  }

  if ((nc=cp->findKey(vpcopt,"extreme.limits"))>=0) {
    std::string value = vpcopt[nc].allValue;
    std::vector<std::string> tokens = miutil::split(value,",");
    if ( tokens.size() > 0 ) {
      int index = extremeLimits.indexOf(tokens[0].c_str());
      if ( index > -1 ) {
        extremeLimitMinComboBox->setCurrentIndex(index);//todo
      }
      if ( tokens.size() > 1 ) {
        index = extremeLimits.indexOf(tokens[1].c_str());
        if ( index > -1 ) {
          extremeLimitMaxComboBox->setCurrentIndex(index);//todo
        }
      }
    }
  }

  // extreme.type (value or none)
  if ((nc=cp->findKey(vpcopt,"extreme.type"))>=0) {
    if( miutil::to_lower(vpcopt[nc].allValue) == "value" ) {
      extremeValueCheckBox->setChecked(true);
    } else {
      extremeValueCheckBox->setChecked(false);
    }
    updateFieldOptions("extreme.type", vpcopt[nc].allValue);
    extremeLimitMinComboBox->setEnabled(true);
    extremeLimitMaxComboBox->setEnabled(true);
    extremeSizeSpinBox->setEnabled(true);
  }
  extremeValueCheckBox->setEnabled(true);
  /*************************************************************************
  if (extreme && (nc=cp->findKey(vpcopt,"extreme.radius"))>=0) {
    if (vpcopt[nc].floatValue.size()>0) e=vpcopt[nc].floatValue[0];
    else e=1.0;
    i= (int(e*100.+0.5))/5 * 5;
    extremeRadiusSpinBox->setValue(i);
    extremeRadiusSpinBox->setEnabled(true);
  } else if (extremeRadiusSpinBox->isEnabled()) {
    extremeRadiusSpinBox->setValue(100);
    extremeRadiusSpinBox->setEnabled(false);
  }
   *************************************************************************/
#endif // DISABLE_EXTREMES

#ifndef DISABLE_LINE_SMOOTHING
  if ((nc=cp->findKey(vpcopt,"line.smooth"))>=0) {
    if (vpcopt[nc].intValue.size()>0) i=vpcopt[nc].intValue[0];
    else i=0;
    lineSmoothSpinBox->setValue(i);
    lineSmoothSpinBox->setEnabled(true);
  } else if (lineSmoothSpinBox->isEnabled()) {
    lineSmoothSpinBox->setValue(0);
    lineSmoothSpinBox->setEnabled(false);
  }
#endif // DISABLE_LINE_SMOOTHING

  if ((nc=cp->findKey(vpcopt,"label.size"))>=0) {
    if (vpcopt[nc].floatValue.size()>0) e=vpcopt[nc].floatValue[0];
    else e= 1.0;
    i= (int(e*100.+0.5))/5 * 5;
    labelSizeSpinBox->setValue(i);
    labelSizeSpinBox->setEnabled(true);
  } else if (labelSizeSpinBox->isEnabled()) {
    labelSizeSpinBox->setValue(100);
    labelSizeSpinBox->setEnabled(false);
  }

  // base
  std::string base;
  if (ekv>0. && (nc=cp->findKey(vpcopt,"base"))>=0) {
    if (!vpcopt[nc].floatValue.empty())
      e = vpcopt[nc].floatValue[0];
    else
      e = 0.0;
    zero1ComboBox->setEnabled(true);
    base = baseList(zero1ComboBox,vpcopt[nc].floatValue[0],ekv/2.0);
    if (not base.empty())
      cp->replaceValue(vpcopt[nc],base, 0);
    base.clear();
  } else if (zero1ComboBox->isEnabled()) {
    zero1ComboBox->clear();
    zero1ComboBox->setEnabled(false);
  }

#ifndef DISABLE_HOUROFFSET
  i= selectedFields[index].hourOffset;
  hourOffsetSpinBox->setValue(i);
  hourOffsetSpinBox->setEnabled(true);
#endif

  /*************************************************************************
  // undefined masking
  if ((nc=cp->findKey(vpcopt,"undef.masking"))>=0) {
    n= undefMasking.size();
    if (!undefMaskingCbox->isEnabled()) {
      const char** cvstr= new const char*[n];
      for (i=0; i<n; i++ )
        cvstr[i]=  undefMasking[i].c_str();
      undefMaskingCbox->insertStrList( cvstr, n );
      delete[] cvstr;
      undefMaskingCbox->setEnabled(true);
    }
    if (vpcopt[nc].intValue.size()==1) {
      i= vpcopt[nc].intValue[0];
      if (i<0 || i>=undefMasking.size()) i=0;
    } else {
      i= 0;
    }
    undefMaskingCbox->setCurrentIndex(i);
  } else if (undefMaskingCbox->isEnabled()) {
    undefMaskingCbox->clear();
    undefMaskingCbox->setEnabled(false);
  }

  // undefined masking colour
  if ((nc=cp->findKey(vpcopt,"undef.colour"))>=0) {
    if (!undefColourCbox->isEnabled()) {
      for( i=0; i<nr_colors; i++)
        undefColourCbox->insertItem( *pmapColor[i] );
      undefColourCbox->setEnabled(true);
    }
    i=0;
    while (i<nr_colors && vpcopt[nc].allValue!=colourInfo[i].name) i++;
    if (i==nr_colors) {
      i=0;
      updateFieldOptions("undef.colour",colourInfo[i].name);
    }
    undefColourCbox->setCurrentIndex(i);
  } else if (undefColourCbox->isEnabled()) {
    undefColourCbox->clear();
    undefColourCbox->setEnabled(false);
  }

  // undefined masking linewidth (if used)
  if ((nc=cp->findKey(vpcopt,"undef.linewidth"))>=0) {
    if (!undefLinewidthCbox->isEnabled()) {
      for( i=0; i < nr_linewidths; i++)
        undefLinewidthCbox->insertItem ( *pmapLinewidths[i] );
      undefLinewidthCbox->setEnabled(true);
    }
    i=0;
    while (i<nr_linewidths && vpcopt[nc].allValue!=linewidths[i]) i++;
    if (i==nr_linewidths) {
      i=0;
      updateFieldOptions("undef.linewidth",linewidths[i]);
    }
    undefLinewidthCbox->setCurrentIndex(i);
  } else if (undefLinewidthCbox->isEnabled()) {
    undefLinewidthCbox->clear();
    undefLinewidthCbox->setEnabled(false);
  }

  // undefined masking linetype (if used)
  if ((nc=cp->findKey(vpcopt,"undef.linetype"))>=0) {
    if (!undefLinetypeCbox->isEnabled()) {
      for( i=0; i < nr_linetypes; i++)
        undefLinetypeCbox->insertItem ( *pmapLinetypes[i] );
      undefLinetypeCbox->setEnabled(true);
    }
    i=0;
    while (i<nr_linetypes && vpcopt[nc].allValue!=linetypes[i]) i++;
    if (i==nr_linetypes) {
      i=0;
      updateFieldOptions("undef.linetype",linetypes[i]);
    }
    undefLinetypeCbox->setCurrentIndex(i);
  } else if (undefLinetypeCbox->isEnabled()) {
    undefLinetypeCbox->clear();
    undefLinetypeCbox->setEnabled(false);
  }
   *************************************************************************/

  nc=cp->findKey(vpcopt,"zero.line");
  if (nc>=0) {
    if (vpcopt[nc].allValue=="-1") {
      nc=-1;
    } else {
      bool on= vpcopt[nc].allValue=="1";
      zeroLineCheckBox->setChecked( on );
      zeroLineCheckBox->setEnabled(true);
    }
  }
  if (nc<0 && zeroLineCheckBox->isEnabled()) {
    zeroLineCheckBox->setChecked( true );
    zeroLineCheckBox->setEnabled( false );
  }

  nc=cp->findKey(vpcopt,"value.label");
  if (nc>=0) {
    bool on= vpcopt[nc].allValue=="1";
    valueLabelCheckBox->setChecked( on );
    valueLabelCheckBox->setEnabled(true);
  }
  if (nc<0 && valueLabelCheckBox->isEnabled()) {
    valueLabelCheckBox->setChecked( true );
    valueLabelCheckBox->setEnabled( false );
  }


  nc=cp->findKey(vpcopt,"minvalue");
  if (nc>=0) {
    min1ComboBox->setEnabled(true);
    float value;
    if(vpcopt[nc].allValue=="off")
      value=atof(base.c_str());
    else
      value = atof(vpcopt[nc].allValue.c_str());
    baseList(min1ComboBox,value,ekv,true);
    if(vpcopt[nc].allValue=="off")
      min1ComboBox->setCurrentIndex(0);
  } else {
    min1ComboBox->setEnabled( false );
  }

  nc=cp->findKey(vpcopt,"maxvalue");
  if (nc>=0) {
    max1ComboBox->setEnabled(true);
    float value;
    if(vpcopt[nc].allValue=="off")
      value=atof(base.c_str());
    else
      value = atof(vpcopt[nc].allValue.c_str());
    baseList(max1ComboBox,value,ekv,true);
    if(vpcopt[nc].allValue=="off")
      max1ComboBox->setCurrentIndex(0);
  } else {
    max1ComboBox->setEnabled( false );
  }
}

void VcrossStyleWidget::disableFieldOptions()
{
  METLIBS_LOG_SCOPE();

  resetOptionsButton->setEnabled(false);

  colorCbox->setEnabled(false);
  shadingComboBox->setCurrentIndex(0);
  shadingComboBox->setEnabled(false);
  shadingSpinBox->setValue(0);
  shadingSpinBox->setEnabled(false);
  shadingcoldComboBox->setCurrentIndex(0);
  shadingcoldComboBox->setEnabled(false);
  shadingcoldSpinBox->setValue(0);
  shadingcoldSpinBox->setEnabled(false);
  //  tableCheckBox->setEnabled(false);
#ifndef DISABLE_PATTERNS
  patternComboBox->setEnabled(false);
  patternColourBox->setEnabled(false);
#endif
  repeatCheckBox->setEnabled(false);
  alphaSpinBox->setValue(255);
  alphaSpinBox->setEnabled(false);

  lineTypeCbox->setEnabled(false);

  lineWidthCbox->setEnabled(false);

  lineintervalCbox->clear();
  lineintervalCbox->setEnabled(false);

  densityCbox->clear();
  densityCbox->setEnabled(false);

  vectorunitCbox->clear();

#ifndef DISABLE_EXTREMES
  extremeValueCheckBox->setChecked(false);
  extremeValueCheckBox->setEnabled(false);

  extremeSizeSpinBox->setValue(100);
  extremeSizeSpinBox->setEnabled(false);

  extremeLimitMinComboBox->setCurrentIndex(0);
  extremeLimitMinComboBox->setEnabled(false);
  extremeLimitMaxComboBox->setCurrentIndex(0);
  extremeLimitMaxComboBox->setEnabled(false);

  //extremeRadiusSpinBox->setValue(100);
  //extremeRadiusSpinBox->setEnabled(false);
#endif // DISABLE_EXTREMES

#ifndef DISABLE_LINE_SMOOTHING
  lineSmoothSpinBox->setValue(0);
  lineSmoothSpinBox->setEnabled(false);
#endif

  zeroLineCheckBox->setChecked( true );
  zeroLineCheckBox->setEnabled(false);

  valueLabelCheckBox->setChecked( true );
  valueLabelCheckBox->setEnabled(false);

  labelSizeSpinBox->setValue(100);
  labelSizeSpinBox->setEnabled(false);

  zero1ComboBox->clear();
  zero1ComboBox->setEnabled(false);

  min1ComboBox->setEnabled(false);
  max1ComboBox->setEnabled(false);

#ifndef DISABLE_HOUROFFSET
  hourOffsetSpinBox->setValue(0);
  hourOffsetSpinBox->setEnabled(false);
#endif

  //undefMaskingCbox->clear();
  //undefMaskingCbox->setEnabled(false);

  //undefColourCbox->clear();
  //undefColourCbox->setEnabled(false);

  //undefLinewidthCbox->clear();
  //undefLinewidthCbox->setEnabled(false);

  //undefLinetypeCbox->clear();
  //undefLinetypeCbox->setEnabled(false);
}


std::vector<std::string> VcrossStyleWidget::numberList( QComboBox* cBox, float number)
{
  const float enormal[] = { 1., 2., 2.5, 3., 4., 5., 6., 7., 8., 9., -1 };
  return diutil::numberList(cBox, number, enormal, false);
}

std::string VcrossStyleWidget::baseList(QComboBox* cBox, float base, float ekv, bool onoff)
{
  std::string str;

  int n;
  if (base<0.) n= int(base/ekv - 0.5);
  else         n= int(base/ekv + 0.5);
  if (fabsf(base-ekv*float(n))>0.01*ekv) {
    base= ekv*float(n);
    str = miutil::from_number(base);
  }
  n=21;
  int k=n/2;
  int j=-k-1;

  cBox->clear();

  if(onoff)
    cBox->addItem(tr("Off"));

  for (int i=0; i<n; ++i) {
    j++;
    float e= base + ekv*float(j);
    if(fabs(e)<ekv/2)
      cBox->addItem("0");
    else{
      const std::string estr = miutil::from_number(e);
      cBox->addItem(QString::fromStdString(estr));
    }
  }

  if(onoff)
    cBox->setCurrentIndex(k+1);
  else
    cBox->setCurrentIndex(k);

  return str;
}

void VcrossStyleWidget::colorCboxActivated(int index)
{
  if (index==0)
    updateFieldOptions("colour","off");
  else
    updateFieldOptions("colour",colourInfo[index-1].name);
}

void VcrossStyleWidget::lineWidthCboxActivated(int index)
{
  updateFieldOptions("linewidth", miutil::from_number(index+1));
}

void VcrossStyleWidget::lineTypeCboxActivated(int index)
{
  updateFieldOptions("linetype", linetypes[index]);
}

void VcrossStyleWidget::lineintervalCboxActivated(int index)
{
  updateFieldOptions("line.interval", lineintervals[index]);
  // update the list (with selected value in the middle)
  float a= atof(lineintervals[index].c_str());
  lineintervals= numberList( lineintervalCbox, a);
}

void VcrossStyleWidget::densityCboxActivated(int index)
{
  if (index==0)
    updateFieldOptions("density","0");
  else
    updateFieldOptions("density",densityCbox->currentText().toStdString());
}

void VcrossStyleWidget::vectorunitCboxActivated(int index)
{
  updateFieldOptions("vector.unit",vectorunit[index]);
  // update the list (with selected value in the middle)
  float a= atof(vectorunit[index].c_str());
  vectorunit= numberList( vectorunitCbox, a);
}

void VcrossStyleWidget::extremeValueCheckBoxToggled(bool on)
{
#ifndef DISABLE_EXTREMES
  if (on) {
    updateFieldOptions("extreme.type","Value");
    extremeLimitMinComboBox->setEnabled(true);
    extremeLimitMaxComboBox->setEnabled(true);
    extremeSizeSpinBox->setEnabled(true);
  } else {
    updateFieldOptions("extreme.type","None");
    extremeLimitMinComboBox->setEnabled(false);
    extremeLimitMaxComboBox->setEnabled(false);
    extremeSizeSpinBox->setEnabled(false);
  }
  extremeLimitsChanged();
#endif
}

void VcrossStyleWidget::extremeLimitsChanged()
{
#ifndef DISABLE_EXTREMES
  std::string extremeString = "remove";
  std::ostringstream ost;
  if (extremeLimitMinComboBox->isEnabled() && extremeLimitMinComboBox->currentIndex() > 0 ) {
    ost << extremeLimitMinComboBox->currentText().toStdString();
    if(  extremeLimitMaxComboBox->currentIndex() > 0 ) {
      ost <<","<<extremeLimitMaxComboBox->currentText().toStdString();
    }
    extremeString = ost.str();
  }
  updateFieldOptions("extreme.limits",extremeString);
#endif
}

void VcrossStyleWidget::extremeSizeChanged(int value)
{
#ifndef DISABLE_EXTREMES
  const std::string str = miutil::from_number(float(value)*0.01);
  updateFieldOptions("extreme.size", str);
#endif
}

void VcrossStyleWidget::lineSmoothChanged(int value)
{
#ifndef DISABLE_LINE_SMOOTHING
  const std::string str = miutil::from_number(value);
  updateFieldOptions("line.smooth",str);
#endif
}

void VcrossStyleWidget::labelSizeChanged(int value)
{
  const std::string str = miutil::from_number(float(value)*0.01);
  updateFieldOptions("label.size",str);
}

void VcrossStyleWidget::hourOffsetChanged(int value)
{
#ifndef DISABLE_HOUROFFSET
  const int n = selectedFieldbox->currentRow();
  selectedFields[n].hourOffset = value;
#endif
}

void VcrossStyleWidget::zeroLineCheckBoxToggled(bool on)
{
  updateFieldOptions("zero.line", on ? "1" : "0");
}

void VcrossStyleWidget::valueLabelCheckBoxToggled(bool on)
{
  updateFieldOptions("value.label", on ? "1" : "0");
}

void VcrossStyleWidget::tableCheckBoxToggled(bool on)
{
  updateFieldOptions("table", on ? "1" : "0");
}

void VcrossStyleWidget::patternComboBoxToggled(int index)
{
#ifndef DISABLE_PATTERNS
  if(index == 0){
    updateFieldOptions("patterns","off");
  } else {
    updateFieldOptions("patterns", patternInfo[index-1].name);
  }
#endif
  updatePaletteString();
}

void VcrossStyleWidget::patternColourBoxToggled(int index)
{
#ifndef DISABLE_PATTERNS
  if(index == 0){
    updateFieldOptions("patterncolour","remove");
  } else {
    updateFieldOptions("patterncolour",colourInfo[index-1].name);
  }
#endif
  updatePaletteString();
}

void VcrossStyleWidget::repeatCheckBoxToggled(bool on)
{
  updateFieldOptions("repeat", on ? "1" : "0");
}

void VcrossStyleWidget::shadingChanged()
{
  updatePaletteString();
}

void VcrossStyleWidget::updatePaletteString(){

#ifndef DISABLE_PATTERNS
  if(patternComboBox->currentIndex()>0 && patternColourBox->currentIndex()>0){
    updateFieldOptions("palettecolours","off",-1);
    return;
  }
#endif

  int index1 = shadingComboBox->currentIndex();
  int index2 = shadingcoldComboBox->currentIndex();
  int value1 = shadingSpinBox->value();
  int value2 = shadingcoldSpinBox->value();

  if(index1==0 && index2==0){
    updateFieldOptions("palettecolours","off",-1);
    return;
  }

  std::string str;
  if(index1>0){
    str = csInfo[index1-1].name;
    if(value1>0)
      str += ";" + miutil::from_number(value1);
    if(index2>0)
      str += ",";
  }
  if(index2>0){
    str += csInfo[index2-1].name;
    if(value2>0)
      str += ";" + miutil::from_number(value2);
  }
  updateFieldOptions("palettecolours",str,-1);
}

void VcrossStyleWidget::alphaChanged(int index)
{
  updateFieldOptions("alpha", miutil::from_number(index));
}

void VcrossStyleWidget::zero1ComboBoxToggled(int index)
{
  if(!zero1ComboBox->currentText().isNull() ){
    std::string str = zero1ComboBox->currentText().toStdString();
    updateFieldOptions("base",str);
    float a = atof(str.c_str());
    float b = lineintervalCbox->currentText().toInt();
    baseList(zero1ComboBox,a,b,true);
  }
}

void VcrossStyleWidget::min1ComboBoxToggled(int index)
{
  if( index == 0 )
    updateFieldOptions("minvalue","off");
  else if(!min1ComboBox->currentText().isNull() ){
    std::string str = min1ComboBox->currentText().toStdString();
    updateFieldOptions("minvalue",str);
    float a = atof(str.c_str());
    float b = 1.0;
    if(!lineintervalCbox->currentText().isNull() )
      b = lineintervalCbox->currentText().toInt();
    baseList(min1ComboBox,a,b,true);
  }
}

void VcrossStyleWidget::max1ComboBoxToggled(int index)
{
  if( index == 0 )
    updateFieldOptions("maxvalue","off");
  else if(!max1ComboBox->currentText().isNull() ){
    std::string str = max1ComboBox->currentText().toStdString();
    updateFieldOptions("maxvalue", max1ComboBox->currentText().toStdString());
    float a = atof(str.c_str());
    float b = 1.0;
    if(!lineintervalCbox->currentText().isNull() )
      b = lineintervalCbox->currentText().toInt();
    baseList(max1ComboBox,a,b,true);
  }
}

void VcrossStyleWidget::updateFieldOptions(const std::string& name,
    const std::string& value, int valueIndex)
{
  METLIBS_LOG_SCOPE("name= " << name << "  value= " << value);

  if (currentFieldOpts.empty())
    return;

  if(value == "remove")
    cp->removeValue(vpcopt,name);
  else
    cp->replaceValue(vpcopt,name,value,valueIndex);

  currentFieldOpts = cp->unParse(vpcopt);
}

void VcrossStyleWidget::resetOptions()
{
  currentFieldOpts = defaultOptions;
  enableFieldOptions();
}
