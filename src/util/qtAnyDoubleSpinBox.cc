/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2022 met.no

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

#include "qtAnyDoubleSpinBox.h"

#include <QLineEdit>

#include <cmath>

#define MILOGGER_CATEGORY "diana.FieldDialogStyle"
#include <miLogger/miLogging.h>

namespace {
double parseText(const QString& text)
{
  bool ok = false;
  auto value = text.toDouble(&ok);
  return ok ? value : std::nan("");
}
} // namespace

namespace diutil {

inline double mag10(double v)
{
  return std::floor(std::log10(v));
}

AnyDoubleSpinBox::AnyDoubleSpinBox(QWidget* parent)
    : QAbstractSpinBox(parent)
    , value_(0)
    , min_(std::nan(""))
    , max_(min_)
{
  setKeyboardTracking(false);
  lineEdit()->setText(QString::number(value_));
  connect(lineEdit(), &QLineEdit::editingFinished, this, &AnyDoubleSpinBox::lineEditTextEdited);
}

void AnyDoubleSpinBox::setMinimum(double min)
{
  if (min != min_) {
    min_ = min;
    setValue(value_); // apply new minimum
  }
}

void AnyDoubleSpinBox::setMaximum(double max)
{
  if (max != max_) {
    max_ = max;
    setValue(value_); // apply new maximum
  }
}

void AnyDoubleSpinBox::setRange(double min, double max)
{
  setMinimum(min);
  setMaximum(max);
}

void AnyDoubleSpinBox::setValue(double value)
{
  if (std::isnan(value) && specialValueText().isEmpty())
    value = 0;

  if (!std::isnan(value)) {
    if (!std::isnan(min_) && value < min_)
      value = min_;
    else if (!std::isnan(max_) && value > max_)
      value = max_;
  }

  if (value_ != value) {
    value_ = value;
    lineEdit()->setText(std::isnan(value_) ? specialValueText() : QString::number(value_));
    valueChanged(value_);
  }
}

bool AnyDoubleSpinBox::isOff() const
{
  return std::isnan(value());
}

void AnyDoubleSpinBox::stepBy(int steps)
{
  if (steps == 0)
    return;
  if (value_ == 0 || std::isnan(value_)) {
    setValue(0.1 * steps);
  } else {
    const double valua = std::abs(value_);
    const double factor = std::pow(10, 1 - mag10(valua));
    const double valur = std::round(valua * factor) / factor;
    const double step = std::pow(10, mag10(valur) - 1);
    setValue(value_ + steps * step);
  }
}

void AnyDoubleSpinBox::lineEditTextEdited()
{
  setValue(parseText(lineEdit()->text()));
}

QAbstractSpinBox::StepEnabled AnyDoubleSpinBox::stepEnabled() const
{
  if (std::isnan(value_))
    return StepEnabled(StepUpEnabled | StepDownEnabled);

  StepEnabled se;
  if (std::isnan(min_) || value_ > min_)
    se |= StepDownEnabled;
  if (std::isnan(max_) || value_ < max_)
    se |= StepUpEnabled;
  return se;
}

} // namespace diutil
