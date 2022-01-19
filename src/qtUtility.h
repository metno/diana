/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2021 met.no

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
#ifndef _qtutility_h
#define _qtutility_h

#include "diCommonTypes.h"
#include "diColourShading.h"
#include "diPattern.h"
#include "util/diKeyValue.h"

#include <QCursor>
#include <QLabel>
#include <QPixmap>

#include <string>
#include <vector>

class QAbstractItemView;
class QAction;
class QWidget;
class QPushButton;
class QComboBox;
class QListWidget;
class QLabel;
class QLCDNumber;
class QCheckBox;
class QSlider;
class QColor;

class Linetype;

int getIndex(const std::vector<std::string>& vstr, const std::string& def_str);
int getIndex(const std::vector<std::string>& vstr, const std::string& def_str);
int getIndex(const std::vector<Colour::ColourInfo>& cInfo, const std::string& def_str);

// create a QPixmap showing palettecolours and colour from options
QPixmap createPixmapForStyle(const miutil::KeyValue_v &options);

// Labels

QLabel* TitleLabel(const QString& name, QWidget* parent);

// PushButtons

QPushButton* PixmapButton( const QPixmap& pixmap, QWidget* parent,
			 int deltaWidth=0, int deltaHeight=0);

// ComboBox

void installCombo(QComboBox* box, const std::vector<std::string>& vstr, bool Enabled=true, int defItem=0);

QComboBox* ComboBox(QWidget* parent, const std::vector<std::string>& vstr,
    bool Enabled=true, int defItem=0);

void installCombo(QComboBox* box, QColor* pixcolor, int nr_colors, bool Enabled=true, int defItem=0);

QComboBox* ComboBox(QWidget* parent, QColor* pixcolor, int nr_colors,
    bool Enabled=true, int defItem=0);

QComboBox* ColourBox(QWidget* parent, const std::vector<Colour::ColourInfo>&,
    bool Enabled=true, int defItem=0,
    const std::string& firstItem="", bool name=false);

QComboBox* ColourBox(QWidget* parent,
    bool Enabled=true, int defItem=0,
    const std::string& firstItem="", bool name=false);

void installColours(QComboBox* box, const std::vector<Colour::ColourInfo>& cInfo, bool name=false);

void ExpandColourBox(QComboBox* box, const Colour& col);

void SetCurrentItemColourBox(QComboBox* box, const std::string& value);

QPixmap pixmapForColourShading(const std::vector<Colour>& colour);

void installPalette(QComboBox* box, const std::vector<ColourShading::ColourShadingInfo>& csInfo, bool name=false);

QComboBox* PaletteBox(QWidget* parent,
		      const std::vector<ColourShading::ColourShadingInfo>&,
		      bool Enabled=true, int defItem=0,
		      const std::string& firstItem="", bool name=false);

void ExpandPaletteBox( QComboBox* box, const ColourShading& palette );

QComboBox* PatternBox(QWidget* parent, const std::vector<Pattern::PatternInfo>&,
		      bool Enabled=true, int defItem=0,
		      const std::string& firstItem="", bool name=false);

void installLinetypes(QComboBox* box);
QComboBox* LinetypeBox(QWidget* parent,
		    bool Enabled=true, int defItem=0);

void installLinewidths(QComboBox* box, int nr_linewidths=12);
QComboBox* LinewidthBox(QWidget* parent,
			bool Enabled=true,
			int nr_linewidths=12,
			int defItem=0);

void ExpandLinewidthBox( QComboBox* box,
    int new_nr_linewidths);

QComboBox* PixmapBox(QWidget* parent, std::vector<std::string>& markerName);

// Div

QLCDNumber* LCDNumber(uint numDigits, QWidget* parent=0);

QSlider* Slider( int minValue, int maxValue, int pageStep, int value,
		 Qt::Orientation orient, QWidget* parent, int width );

QSlider* Slider( int minValue, int maxValue, int pageStep, int value,
		 Qt::Orientation orient, QWidget* parent );

QPixmap linePixmap(int linewidth);
QPixmap linePixmap(const Linetype& pattern, int linewidth);
QPixmap linePixmap(const Linetype& pattern, int linewidth, const Colour& col);

namespace diutil {

class OverrideCursor {
public:
  OverrideCursor(const QCursor& cursor = QCursor(Qt::WaitCursor));
  ~OverrideCursor();
};

/*! Fill combobox with values around 'number'.
 */
std::vector<std::string> numberList(QComboBox* cBox, float number, const float* enormal, bool onoff);

/** select all rows in an item view */
void selectAllRows(QAbstractItemView* view);

void appendText(QString& text, const QString& append, const QString& separator=" ");
QString appendedText(const QString& text, const QString& append, const QString& separator=" ");

/** Add a shortcut hint to the current tooltip (if there is a shortcut).
 *
 *  It is not god to call this function several times without
 *  resetting the tooltip text inbetween.
 */
void addShortcutToTooltip(QAction* action);

class BlockSignals
{
public:
  BlockSignals(QObject* object)
      : object_(object) { if (object_) object->blockSignals(true); }
  ~BlockSignals() { unblock(); }

  void unblock() { if (object_) { object_->blockSignals(false); object_ = nullptr; }}

private:
  QObject* object_;
};

inline QColor QC(const Colour& c) { return QColor(c.R(), c.G(), c.B(), c.A()); }
inline Colour fromQColor(const QColor& c, bool alpha=true) { return Colour::fromF(c.redF(), c.greenF(), c.blueF(), alpha ? c.alphaF() : 1); }

} // namespace diutil

#endif
