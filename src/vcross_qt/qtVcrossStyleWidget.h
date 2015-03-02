
#ifndef VcrossStyleWidget_h
#define VcrossStyleWidget_h 1

#include "diCommandParser.h"
#include "diColourShading.h"
#include "diPattern.h"

#include <QStringList>
#include <QWidget>

#include <memory>
#include <string>
#include <vector>

class Ui_VcrossStyleWidget;

class VcrossStyleWidget : public QWidget {
  Q_OBJECT;

public:
  VcrossStyleWidget(QWidget* parent);

  void setOptions(const std::string& fopt, const std::string& defaultopt);
  const std::string& options() const;
  bool valid() const;

public Q_SLOTS:
  void resetOptions();

Q_SIGNALS:
  void canResetOptions(bool enableReset);

private:
  void setupUi();

  void disableFieldOptions();
  void enableFieldOptions();
  void updateFieldOptions(const std::string& name, const std::string& value, int valueIndex=0);

private Q_SLOTS:
  void colorCboxActivated( int index );
  void lineWidthCboxActivated( int index );
  void lineTypeCboxActivated( int index );
  void lineintervalCboxActivated( int index );
  void densityCboxActivated( int index );
  void vectorunitCboxActivated( int index );
  void vectorthicknessChanged(int value);

  void extremeValueCheckBoxToggled(bool on);
  void extremeSizeChanged(int value);
  void extremeLimitsChanged();
  void lineSmoothChanged(int value);
  void labelSizeChanged(int value);
  void hourOffsetChanged(int value);
  //void undefMaskingActivated(int index);
  //void undefColourActivated(int index);
  //void undefLinewidthActivated(int index);
  //void undefLinetypeActivated(int index);
  void zeroLineCheckBoxToggled(bool on);
  void valueLabelCheckBoxToggled(bool on);
  void tableCheckBoxToggled(bool on);
  void repeatCheckBoxToggled(bool on);
  void shadingChanged();
  void patternComboBoxToggled(int index);
  void patternColourBoxToggled(int index);
  void alphaChanged(int index);
  void zero1ComboBoxToggled(int index);
  void min1ComboBoxToggled(int index);
  void max1ComboBoxToggled(int index);
  void updatePaletteString();

private:
  std::auto_ptr<Ui_VcrossStyleWidget> ui;

  std::string currentFieldOpts;
  std::string defaultOptions;

  std::vector<Colour::ColourInfo> colourInfo;
  std::vector<ColourShading::ColourShadingInfo> csInfo;
  std::vector<Pattern::PatternInfo> patternInfo;

  std::vector<std::string> linetypes;
  std::vector<std::string> lineintervals;
  QStringList      densityStringList;
  std::vector<std::string> vectorunit;
  QStringList extremeLimits;

  std::auto_ptr<CommandParser> cp;
  std::vector<ParsedCommand> vpcopt;

  std::vector<std::string> undefMasking;

  int        nr_colors;
  int        nr_linewidths;
  int        nr_linetypes;
};

#endif // VcrossStyleWidget_h
