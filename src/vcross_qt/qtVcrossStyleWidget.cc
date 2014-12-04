
#include "qtVcrossStyleWidget.h"

#include "diLinetype.h"
#include "qtUtility.h"

#include <diField/diMetConstants.h>
#include <puTools/miStringFunctions.h>

#include <QApplication>

#define MILOGGER_CATEGORY "diana.VcrossStyleWidget"
#include <miLogger/miLogging.h>

#include "vcross_style_widget.ui.h"

//#define DISABLE_EXTREMES 1
#define DISABLE_EXTREME_LIMITS 1
#define DISABLE_PATTERNS 1
#define DISABLE_LINE_SMOOTHING 1
#define DISABLE_HOUROFFSET 1

namespace {

std::vector<std::string> numberList(QComboBox* cBox, float number)
{
  const float enormal[] = { 1., 2., 2.5, 3., 4., 5., 6., 7., 8., 9., -1 };
  return diutil::numberList(cBox, number, enormal, false);
}

std::string baseList(QComboBox* cBox, float base, float ekv, bool onoff=false)
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
    cBox->addItem(qApp->translate("VcrossStyleWidget", "Off"));

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

} // namespace

// ========================================================================

VcrossStyleWidget::VcrossStyleWidget(QWidget* parent)
  : QWidget(parent)
  , ui(new Ui_VcrossStyleWidget)
  , cp(new CommandParser())
{
  setupUi();

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

void VcrossStyleWidget::setupUi()
{
  ui->setupUi(this);

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

  ui->colorCbox->addItem(tr("Off"));
  installColours(ui->colorCbox, colourInfo, true);
  ui->shadingComboBox->setCurrentIndex(0);

  installLinewidths(ui->lineWidthCbox);
  installLinetypes(ui->lineTypeCbox);
  ui->extremeSizeSpinBox->setWrapping(true);
  ui->labelSizeSpinBox->setWrapping(true);

  ui->shadingComboBox->addItem(tr("Off"));
  installPalette(ui->shadingComboBox, csInfo);
  ui->shadingComboBox->setCurrentIndex(0);

  ui->shadingSpinBox->setSpecialValueText(tr("Auto"));

  ui->shadingcoldComboBox->addItem(tr("Off"));
  installPalette(ui->shadingcoldComboBox, csInfo);
  ui->shadingcoldComboBox->setCurrentIndex(0);

  ui->shadingcoldSpinBox->setSpecialValueText(tr("Auto"));

  connect(ui->colorCbox, SIGNAL(activated(int)),
      SLOT(colorCboxActivated(int)));
  connect(ui->lineWidthCbox, SIGNAL(activated(int)),
      SLOT(lineWidthCboxActivated(int)));
  connect(ui->lineTypeCbox, SIGNAL(activated(int)),
      SLOT(lineTypeCboxActivated(int)));
  connect(ui->lineintervalCbox, SIGNAL(activated(int)),
      SLOT(lineintervalCboxActivated(int)));
  connect(ui->densityCbox, SIGNAL(activated(int)),
      SLOT(densityCboxActivated(int)));
  connect(ui->vectorunitCbox, SIGNAL(activated(int)),
      SLOT(vectorunitCboxActivated(int)));
  connect(ui->resetOptionsButton, SIGNAL(clicked()),
      SLOT(resetOptions()));

  // mark min/max values
#ifndef DISABLE_EXTREMES
  connect(ui->extremeValueCheckBox, SIGNAL(toggled(bool)),
      SLOT(extremeValueCheckBoxToggled(bool)));

  connect(ui->extremeSizeSpinBox, SIGNAL(valueChanged(int)),
      SLOT(extremeSizeChanged(int)));

#ifndef DISABLE_EXTREME_LIMITS
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
#endif // DISABLE_EXTREME_LIMITS
#endif // DISABLE_EXTREMES

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

  connect(ui->labelSizeSpinBox, SIGNAL(valueChanged(int)),
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
  connect(ui->zeroLineCheckBox, SIGNAL(toggled(bool)),
      SLOT(zeroLineCheckBoxToggled(bool)));

  // enable/disable numbers on isolines
  connect(ui->valueLabelCheckBox, SIGNAL(toggled(bool)),
      SLOT(valueLabelCheckBoxToggled(bool)));

#ifndef DISABLE_PATTERNS
  QLabel* patternLabel    = new QLabel( tr("Pattern"),     advFrame );
#endif

  //  tableCheckBox = new QCheckBox(tr("Table"), advFrame);
  //  tableCheckBox->setEnabled(false);
  //  connect( tableCheckBox, SIGNAL( toggled(bool) ),
  //	   SLOT(tableCheckBoxToggled(bool) ) );

  connect(ui->repeatCheckBox, SIGNAL(toggled(bool)),
      SLOT(repeatCheckBoxToggled(bool)));
  connect(ui->shadingComboBox, SIGNAL(activated(int)),
      SLOT(shadingChanged()));
  connect(ui->shadingSpinBox, SIGNAL(valueChanged(int)),
      SLOT(shadingChanged()));
  connect(ui->shadingcoldComboBox, SIGNAL(activated(int)),
      SLOT(shadingChanged()));
  connect(ui->shadingcoldSpinBox, SIGNAL(valueChanged(int)),
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

  connect(ui->alphaSpinBox, SIGNAL(valueChanged(int)),
      SLOT(alphaChanged(int)));
  connect(ui->zero1ComboBox, SIGNAL(activated(int)),
      SLOT(zero1ComboBoxToggled(int)));
  connect(ui->min1ComboBox, SIGNAL(activated(int)),
      SLOT(min1ComboBoxToggled(int)));
  connect(ui->max1ComboBox, SIGNAL( activated(int)),
      SLOT(max1ComboBoxToggled(int)));
}

void VcrossStyleWidget::enableFieldOptions()
{
  METLIBS_LOG_SCOPE(LOGVAL(currentFieldOpts));

  disableFieldOptions();

  ui->resetOptionsButton->setEnabled(true);

  vpcopt = cp->parse(currentFieldOpts);

  int nc, i;
  float e;

  // colour(s)
  if ((nc=cp->findKey(vpcopt,"colour"))>=0) {
    ui->shadingComboBox->setEnabled(true);
    ui->shadingcoldComboBox->setEnabled(true);
    if (!ui->colorCbox->isEnabled()) {
      ui->colorCbox->setEnabled(true);
    }
    i=0;
    if(miutil::to_lower(vpcopt[nc].allValue) == "off" ||
        miutil::to_lower(vpcopt[nc].allValue) == "av" ){
      updateFieldOptions("colour","off");
      ui->colorCbox->setCurrentIndex(0);
    } else {
      while (i<nr_colors
          && miutil::to_lower(vpcopt[nc].allValue)!=colourInfo[i].name) i++;
      if (i==nr_colors) i=0;
      updateFieldOptions("colour",colourInfo[i].name);
      ui->colorCbox->setCurrentIndex(i+1);
    }
  } else if (ui->colorCbox->isEnabled()) {
    ui->colorCbox->setEnabled(false);
  }

  //contour shading
  if ((nc=cp->findKey(vpcopt,"palettecolours"))>=0) {
    ui->shadingComboBox->setEnabled(true);
    ui->shadingSpinBox->setEnabled(true);
    ui->shadingcoldComboBox->setEnabled(true);
    ui->shadingcoldSpinBox->setEnabled(true);
    //    tableCheckBox->setEnabled(true);
#ifndef DISABLE_PATTERNS
    ui->patternComboBox->setEnabled(true);
#endif
    ui->repeatCheckBox->setEnabled(true);
    ui->alphaSpinBox->setEnabled(true);
    std::vector<std::string> tokens = miutil::split(vpcopt[nc].allValue,",");
    std::vector<std::string> stokens = miutil::split(tokens[0],";");
    if(stokens.size()==2)
      ui->shadingSpinBox->setValue(atoi(stokens[1].c_str()));
    else
      ui->shadingSpinBox->setValue(0);
    int nr_cs = csInfo.size();
    std::string str;
    i=0;
    while (i<nr_cs && stokens[0]!=csInfo[i].name) i++;
    if (i==nr_cs) {
      str = "off";
      ui->shadingComboBox->setCurrentIndex(0);
      ui->shadingcoldComboBox->setCurrentIndex(0);
    }else {
      str = tokens[0];
      ui->shadingComboBox->setCurrentIndex(i+1);
    }
    if(tokens.size()==2){
      std::vector<std::string> stokens = miutil::split(tokens[1],";");
      if(stokens.size()==2)
        ui->shadingcoldSpinBox->setValue(atoi(stokens[1].c_str()));
      ui->shadingcoldSpinBox->setValue(0);
      i=0;
      while (i<nr_cs && stokens[0]!=csInfo[i].name) i++;
      if (i==nr_cs) {
        ui->shadingcoldComboBox->setCurrentIndex(0);
      }else {
        str += "," + tokens[1];
        ui->shadingcoldComboBox->setCurrentIndex(i+1);
      }
    } else {
      ui->shadingcoldComboBox->setCurrentIndex(0);
    }
    updateFieldOptions("palettecolours",str,-1);
  } else {
    updateFieldOptions("palettecolours","off",-1);
    ui->shadingComboBox->setCurrentIndex(0);
    ui->shadingComboBox->setEnabled(false);
    ui->shadingcoldComboBox->setCurrentIndex(0);
    ui->shadingcoldComboBox->setEnabled(false);
    //    tableCheckBox->setEnabled(false);
    //    updateFieldOptions("table","remove");
#ifndef DISABLE_PATTERNS
    patternComboBox->setEnabled(false);
    updateFieldOptions("patterns","remove");
#endif
    ui->repeatCheckBox->setEnabled(false);
    updateFieldOptions("repeat","remove");
    ui->alphaSpinBox->setEnabled(false);
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
#endif // DISABLE_PATTERNS

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
    ui->repeatCheckBox->setChecked( on );
    ui->repeatCheckBox->setEnabled(true);
    repeatCheckBoxToggled(on);
  }

  //alpha shading
  if ((nc=cp->findKey(vpcopt,"alpha"))>=0) {
    if (vpcopt[nc].intValue.size()>0) i=vpcopt[nc].intValue[0];
    else i=255;
    ui->alphaSpinBox->setValue(i);
    ui->alphaSpinBox->setEnabled(true);
  } else {
    ui->alphaSpinBox->setValue(255);
    updateFieldOptions("alpha","remove");
  }

  // linetype
  if ((nc=cp->findKey(vpcopt,"linetype"))>=0) {
    ui->lineTypeCbox->setEnabled(true);
    i=0;
    while (i<nr_linetypes && vpcopt[nc].allValue!=linetypes[i]) i++;
    if (i==nr_linetypes) {
      i=0;
      updateFieldOptions("linetype",linetypes[i]);
    }
    ui->lineTypeCbox->setCurrentIndex(i);
  } else if (ui->lineTypeCbox->isEnabled()) {
    ui->lineTypeCbox->setEnabled(false);
  }

  // linewidth
  if ((nc=cp->findKey(vpcopt,"linewidth"))>=0) {
    ui->lineWidthCbox->setEnabled(true);
    i=0;
    while (i<nr_linewidths && vpcopt[nc].allValue!=miutil::from_number(i+1)) i++;
    if (i==nr_linewidths) {
      i=0;
      updateFieldOptions("linewidth",miutil::from_number(i+1));
    }
    ui->lineWidthCbox->setCurrentIndex(i);
  } else if (ui->lineWidthCbox->isEnabled()) {
    ui->lineWidthCbox->setEnabled(false);
  }

  // line interval (isoline contouring)
  float ekv=-1.;
  if ((nc=cp->findKey(vpcopt,"line.interval"))>=0) {
    if (vpcopt[nc].floatValue.size()>0) ekv=vpcopt[nc].floatValue[0];
    else ekv= 10.;
    lineintervals= numberList(ui->lineintervalCbox, ekv);
    ui->lineintervalCbox->setEnabled(true);
  } else if (ui->lineintervalCbox->isEnabled()) {
    ui->lineintervalCbox->clear();
    ui->lineintervalCbox->setEnabled(false);
  }

  // wind/vector density
  if ((nc=cp->findKey(vpcopt,"density"))>=0) {
    if (!ui->densityCbox->isEnabled()) {
      ui->densityCbox->addItems(densityStringList);
      ui->densityCbox->setEnabled(true);
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
        ui->densityCbox->addItem(QString(s.c_str()));
        i=ui->densityCbox->count()-1;
      }
    }
    ui->densityCbox->setCurrentIndex(i);
  } else if (ui->densityCbox->isEnabled()) {
    ui->densityCbox->clear();
    ui->densityCbox->setEnabled(false);
  }

  // vectorunit (vector length unit)
  if ((nc=cp->findKey(vpcopt,"vector.unit"))>=0) {
    if (vpcopt[nc].floatValue.size()>0) e= vpcopt[nc].floatValue[0];
    else e=5;
    vectorunit= numberList(ui->vectorunitCbox, e);
  } else if (ui->vectorunitCbox->isEnabled()) {
    ui->vectorunitCbox->clear();
  }

#ifndef DISABLE_EXTREMES
  if ((nc=cp->findKey(vpcopt,"extreme.size"))>=0) {
    if (vpcopt[nc].floatValue.size()>0)
      e = vpcopt[nc].floatValue[0];
    else
      e = 1.0;
    i = int((e*100.+0.5)/5) * 5;
    ui->extremeSizeSpinBox->setValue(i);
  } else {
    ui->extremeSizeSpinBox->setValue(100);
  }

#ifndef DISABLE_EXTREME_LIMITS
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
#endif // DISABLE_EXTREME_LIMITS

  // extreme.type (value or none)
  if ((nc=cp->findKey(vpcopt,"extreme.type"))>=0) {
    if( miutil::to_lower(vpcopt[nc].allValue) == "value" ) {
      ui->extremeValueCheckBox->setChecked(true);
    } else {
      ui->extremeValueCheckBox->setChecked(false);
    }
    updateFieldOptions("extreme.type", vpcopt[nc].allValue);
#ifndef DISABLE_EXTREME_LIMITS
    extremeLimitMinComboBox->setEnabled(true);
    extremeLimitMaxComboBox->setEnabled(true);
#endif
    ui->extremeSizeSpinBox->setEnabled(true);
  }
  ui->extremeValueCheckBox->setEnabled(true);
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
    ui->labelSizeSpinBox->setValue(i);
    ui->labelSizeSpinBox->setEnabled(true);
  } else if (ui->labelSizeSpinBox->isEnabled()) {
    ui->labelSizeSpinBox->setValue(100);
    ui->labelSizeSpinBox->setEnabled(false);
  }

  // base
  std::string base;
  if (ekv>0. && (nc=cp->findKey(vpcopt,"base"))>=0) {
    if (!vpcopt[nc].floatValue.empty())
      e = vpcopt[nc].floatValue[0];
    else
      e = 0.0;
    ui->zero1ComboBox->setEnabled(true);
    base = baseList(ui->zero1ComboBox, vpcopt[nc].floatValue[0], ekv/2.0);
    if (not base.empty())
      cp->replaceValue(vpcopt[nc],base, 0);
    base.clear();
  } else if (ui->zero1ComboBox->isEnabled()) {
    ui->zero1ComboBox->clear();
    ui->zero1ComboBox->setEnabled(false);
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
      ui->zeroLineCheckBox->setChecked( on );
      ui->zeroLineCheckBox->setEnabled(true);
    }
  }
  if (nc<0 && ui->zeroLineCheckBox->isEnabled()) {
    ui->zeroLineCheckBox->setChecked( true );
    ui->zeroLineCheckBox->setEnabled( false );
  }

  nc=cp->findKey(vpcopt,"value.label");
  if (nc>=0) {
    bool on= vpcopt[nc].allValue=="1";
    ui->valueLabelCheckBox->setChecked( on );
    ui->valueLabelCheckBox->setEnabled(true);
  }
  if (nc<0 && ui->valueLabelCheckBox->isEnabled()) {
    ui->valueLabelCheckBox->setChecked( true );
    ui->valueLabelCheckBox->setEnabled( false );
  }


  nc=cp->findKey(vpcopt,"minvalue");
  if (nc>=0) {
    ui->min1ComboBox->setEnabled(true);
    float value;
    if(vpcopt[nc].allValue=="off")
      value=atof(base.c_str());
    else
      value = atof(vpcopt[nc].allValue.c_str());
    baseList(ui->min1ComboBox,value,ekv,true);
    if(vpcopt[nc].allValue=="off")
      ui->min1ComboBox->setCurrentIndex(0);
  } else {
    ui->min1ComboBox->setEnabled( false );
  }

  nc=cp->findKey(vpcopt,"maxvalue");
  if (nc>=0) {
    ui->max1ComboBox->setEnabled(true);
    float value;
    if(vpcopt[nc].allValue=="off")
      value=atof(base.c_str());
    else
      value = atof(vpcopt[nc].allValue.c_str());
    baseList(ui->max1ComboBox,value,ekv,true);
    if(vpcopt[nc].allValue=="off")
      ui->max1ComboBox->setCurrentIndex(0);
  } else {
    ui->max1ComboBox->setEnabled( false );
  }
}

void VcrossStyleWidget::disableFieldOptions()
{
  METLIBS_LOG_SCOPE();

  ui->resetOptionsButton->setEnabled(false);

  ui->colorCbox->setEnabled(false);
  ui->shadingComboBox->setCurrentIndex(0);
  ui->shadingComboBox->setEnabled(false);
  ui->shadingSpinBox->setValue(0);
  ui->shadingSpinBox->setEnabled(false);
  ui->shadingcoldComboBox->setCurrentIndex(0);
  ui->shadingcoldComboBox->setEnabled(false);
  ui->shadingcoldSpinBox->setValue(0);
  ui->shadingcoldSpinBox->setEnabled(false);
  //  tableCheckBox->setEnabled(false);
#ifndef DISABLE_PATTERNS
  patternComboBox->setEnabled(false);
  patternColourBox->setEnabled(false);
#endif
  ui->repeatCheckBox->setEnabled(false);
  ui->alphaSpinBox->setValue(255);
  ui->alphaSpinBox->setEnabled(false);

  ui->lineTypeCbox->setEnabled(false);

  ui->lineWidthCbox->setEnabled(false);

  ui->lineintervalCbox->clear();
  ui->lineintervalCbox->setEnabled(false);

  ui->densityCbox->clear();
  ui->densityCbox->setEnabled(false);

  ui->vectorunitCbox->clear();

#ifndef DISABLE_EXTREMES
  ui->extremeValueCheckBox->setChecked(false);
  ui->extremeValueCheckBox->setEnabled(false);

  ui->extremeSizeSpinBox->setValue(100);
  ui->extremeSizeSpinBox->setEnabled(false);

#ifndef DISABLE_EXTREME_LIMITS
  extremeLimitMinComboBox->setCurrentIndex(0);
  extremeLimitMinComboBox->setEnabled(false);
  extremeLimitMaxComboBox->setCurrentIndex(0);
  extremeLimitMaxComboBox->setEnabled(false);
#endif // DISABLE_EXTREME_LIMITS
#endif // DISABLE_EXTREMES

#ifndef DISABLE_LINE_SMOOTHING
  lineSmoothSpinBox->setValue(0);
  lineSmoothSpinBox->setEnabled(false);
#endif

  ui->zeroLineCheckBox->setChecked( true );
  ui->zeroLineCheckBox->setEnabled(false);

  ui->valueLabelCheckBox->setChecked( true );
  ui->valueLabelCheckBox->setEnabled(false);

  ui->labelSizeSpinBox->setValue(100);
  ui->labelSizeSpinBox->setEnabled(false);

  ui->zero1ComboBox->clear();
  ui->zero1ComboBox->setEnabled(false);

  ui->min1ComboBox->setEnabled(false);
  ui->max1ComboBox->setEnabled(false);

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
  lineintervals= numberList(ui->lineintervalCbox, a);
}

void VcrossStyleWidget::densityCboxActivated(int index)
{
  if (index==0)
    updateFieldOptions("density","0");
  else
    updateFieldOptions("density", ui->densityCbox->currentText().toStdString());
}

void VcrossStyleWidget::vectorunitCboxActivated(int index)
{
  updateFieldOptions("vector.unit",vectorunit[index]);
  // update the list (with selected value in the middle)
  float a= atof(vectorunit[index].c_str());
  vectorunit= numberList(ui->vectorunitCbox, a);
}

void VcrossStyleWidget::extremeValueCheckBoxToggled(bool on)
{
#ifndef DISABLE_EXTREMES
  if (on) {
    updateFieldOptions("extreme.type","Value");
#ifndef DISABLE_EXTREME_LIMITS
    extremeLimitMinComboBox->setEnabled(true);
    extremeLimitMaxComboBox->setEnabled(true);
#endif // DISABLE_EXTREME_LIMITS
    ui->extremeSizeSpinBox->setEnabled(true);
  } else {
    updateFieldOptions("extreme.type","None");
#ifndef DISABLE_EXTREME_LIMITS
    extremeLimitMinComboBox->setEnabled(false);
    extremeLimitMaxComboBox->setEnabled(false);
#endif // DISABLE_EXTREME_LIMITS
    ui->extremeSizeSpinBox->setEnabled(false);
  }
  extremeLimitsChanged();
#endif
}

void VcrossStyleWidget::extremeLimitsChanged()
{
#ifndef DISABLE_EXTREME_LIMITS
  std::string extremeString = "remove";
  std::ostringstream ost;
  if (ui->extremeLimitMinComboBox->isEnabled() && ui->extremeLimitMinComboBox->currentIndex() > 0 ) {
    ost << ui->extremeLimitMinComboBox->currentText().toStdString();
    if (ui->extremeLimitMaxComboBox->currentIndex() > 0) {
      ost <<","<<ui->extremeLimitMaxComboBox->currentText().toStdString();
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
  const int n = ui->selectedFieldbox->currentRow();
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
  if (ui->patternComboBox->currentIndex()>0 && ui->patternColourBox->currentIndex()>0) {
    updateFieldOptions("palettecolours","off",-1);
    return;
  }
#endif

  int index1 = ui->shadingComboBox->currentIndex();
  int index2 = ui->shadingcoldComboBox->currentIndex();
  int value1 = ui->shadingSpinBox->value();
  int value2 = ui->shadingcoldSpinBox->value();

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
  if (!ui->zero1ComboBox->currentText().isNull()) {
    std::string str = ui->zero1ComboBox->currentText().toStdString();
    updateFieldOptions("base",str);
    float a = atof(str.c_str());
    float b = ui->lineintervalCbox->currentText().toInt();
    baseList(ui->zero1ComboBox,a,b,true);
  }
}

void VcrossStyleWidget::min1ComboBoxToggled(int index)
{
  if( index == 0 )
    updateFieldOptions("minvalue","off");
  else if(!ui->min1ComboBox->currentText().isNull() ){
    std::string str = ui->min1ComboBox->currentText().toStdString();
    updateFieldOptions("minvalue",str);
    float a = atof(str.c_str());
    float b = 1.0;
    if(!ui->lineintervalCbox->currentText().isNull() )
      b = ui->lineintervalCbox->currentText().toInt();
    baseList(ui->min1ComboBox,a,b,true);
  }
}

void VcrossStyleWidget::max1ComboBoxToggled(int index)
{
  if( index == 0 )
    updateFieldOptions("maxvalue","off");
  else if(!ui->max1ComboBox->currentText().isNull() ){
    std::string str = ui->max1ComboBox->currentText().toStdString();
    updateFieldOptions("maxvalue", ui->max1ComboBox->currentText().toStdString());
    float a = atof(str.c_str());
    float b = 1.0;
    if(!ui->lineintervalCbox->currentText().isNull() )
      b = ui->lineintervalCbox->currentText().toInt();
    baseList(ui->max1ComboBox,a,b,true);
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
  disableFieldOptions();
  enableFieldOptions();
}