
#ifndef VcrossStyleWidget_h
#define VcrossStyleWidget_h 1

#include "diCommandParser.h"
#include "diColourShading.h"
#include "diPattern.h"

#include <QStringList>
#include <QTabWidget>

#include <memory>
#include <string>
#include <vector>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;

class VcrossStyleWidget : public QTabWidget {
  Q_OBJECT;

public:
  VcrossStyleWidget(QWidget* parent);

  void setOptions(const std::string& fopt, const std::string& defaultopt);
  const std::string& options() const;
  bool valid() const;

  void initialize();
  bool isComplete() const;
  std::string getOptions();

private:
  QWidget* createBasicTab();
  QWidget* createAdvancedTab();

  void initFieldOptions();

  void disableFieldOptions();
  void enableFieldOptions();
  void updateFieldOptions(const std::string& name,
      const std::string& value, int valueIndex=0);
  std::vector<std::string> numberList( QComboBox* cBox, float number );
  std::string baseList( QComboBox* cBox, float base, float ekv, bool onoff=false);
  std::string checkFieldOptions(const std::string& str, bool fieldPrefix);

private Q_SLOTS:
  void resetOptions();
  void colorCboxActivated( int index );
  void lineWidthCboxActivated( int index );
  void lineTypeCboxActivated( int index );
  void lineintervalCboxActivated( int index );
  void densityCboxActivated( int index );
  void vectorunitCboxActivated( int index );

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
  std::string currentFieldOpts;
  std::string defaultOptions;

  std::vector<Colour::ColourInfo> colourInfo;
  std::vector<ColourShading::ColourShadingInfo> csInfo;
  std::vector<std::string> twoColourNames;
  std::vector<std::string> threeColourNames;
  std::vector<Pattern::PatternInfo> patternInfo;

  std::vector<std::string> linetypes;
  std::vector<std::string> lineintervals;
  QStringList      densityStringList;
  std::vector<std::string> vectorunit;
  QStringList extremeLimits;

  std::auto_ptr<CommandParser> cp;
  std::vector<ParsedCommand> vpcopt;

  QCheckBox*  extremeValueCheckBox;
  QSpinBox*  extremeSizeSpinBox;
  QComboBox* extremeLimitMaxComboBox;
  QComboBox* extremeLimitMinComboBox;
  QSpinBox*  lineSmoothSpinBox;
  QSpinBox*  labelSizeSpinBox;
  QSpinBox*  hourOffsetSpinBox;
  //QComboBox* undefMaskingCbox;
  //QComboBox* undefColourCbox;
  //QComboBox* undefLinewidthCbox;
  //QComboBox* undefLinetypeCbox;
  QCheckBox* zeroLineCheckBox;
  QCheckBox* valueLabelCheckBox;
  QCheckBox* tableCheckBox;
  QCheckBox* repeatCheckBox;
  QComboBox* shadingComboBox;
  QComboBox* shadingcoldComboBox;
  QSpinBox*  shadingSpinBox;
  QSpinBox*  shadingcoldSpinBox;
  QComboBox* patternComboBox;
  QComboBox* patternColourBox;
  QSpinBox*  alphaSpinBox;
  QComboBox* zero1ComboBox;
  QComboBox* min1ComboBox;
  QComboBox* max1ComboBox;
  QComboBox* interval2ComboBox;
  QComboBox* linewidth1ComboBox;
  QComboBox* linetype1ComboBox;
  QComboBox* type1ComboBox;

  std::vector<std::string> baseopts;
  std::vector<std::string> undefMasking;

  QPushButton*  resetOptionsButton;
  QLabel*    colorlabel;
  QComboBox* colorCbox;
  QPixmap**  pmapColor;
  QPixmap**  pmapTwoColors;
  QPixmap**  pmapThreeColors;
  int        nr_colors;

  QLabel*    linewidthlabel;
  QComboBox* lineWidthCbox;
  int        nr_linewidths;

  QLabel*    linetypelabel;
  QComboBox* lineTypeCbox;
  int        nr_linetypes;

  QLabel*    lineintervallabel;
  QComboBox* lineintervalCbox;

  QLabel*    densitylabel;
  QComboBox* densityCbox;
  const char** cdensities;
  int        nr_densities;

  QLabel*    vectorunitlabel;
  QComboBox* vectorunitCbox;
};

#endif // VcrossStyleWidget_h
