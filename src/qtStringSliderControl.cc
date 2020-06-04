/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2020 met.no

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

#include "qtStringSliderControl.h"

#include "qtUtility.h"

#include <QLabel>
#include <QSlider>

namespace {

const QString NONE("---");

} // namespace

StringSliderControl::StringSliderControl(bool reverse, QSlider* slider, QLabel* label, QObject* parent)
    : QObject(parent)
    , slider_(slider)
    , label_(label)
    , reverse_(reverse)
    , in_motion_(false)
{
  label_->clear();
  slider_->setInvertedAppearance(reverse_);
  setValues(values_); // empty

  connect(slider_, static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged), this, &StringSliderControl::onSliderChanged);
  connect(slider_, &QSlider::sliderPressed, this, &StringSliderControl::onSliderPressed);
  connect(slider_, &QSlider::sliderReleased, this, &StringSliderControl::onSliderReleased);
}

StringSliderControl::~StringSliderControl() {}

void StringSliderControl::setValues(const std::vector<std::string>& values, const std::string& value)
{
  current_ = value;
  if (values.empty())
    current_.clear();
  else if (current_.empty())
    current_ = (reverse_ ? values_.back() : values.front());
  setValues(values);
}

void StringSliderControl::setValues(const std::vector<std::string>& values)
{
  values_ = values;
  diutil::BlockSignals block(slider_);
  if (!values_.empty()) {
    size_t index = (std::find(values_.begin(), values_.end(), current_) - values_.begin());
    if (index >= values.size()) {
      index = 0;
      current_ = values[index];
    }
    slider_->setEnabled(values.size() > 1);
    slider_->setRange(0, values.size() - 1);
    slider_->setValue(index);
  } else {
    current_.clear();
    slider_->setEnabled(false);
    slider_->setRange(0, 1);
    slider_->setValue(1);
  }
  updateLabelText();
}

void StringSliderControl::setValue(const std::string& value)
{
  const auto it = std::find(values_.begin(), values_.end(), value);
  if (it != values_.end()) {
    int index = it - values_.begin();
    {
      diutil::BlockSignals block(slider_);
      slider_->setValue(index);
    }
    onSliderChanged(index);
  }
}

void StringSliderControl::onSliderPressed()
{
  in_motion_ = true;
}

void StringSliderControl::onSliderReleased()
{
  in_motion_ = false;
  /*Q_EMIT*/ valueChanged(current_);
}

void StringSliderControl::onSliderChanged(int index)
{
  const int n = values_.size();
  if (index >= 0 && index < n) {
    current_ = values_[index];
  } else {
    current_.clear();
  }
  updateLabelText();

  if (!in_motion_)
    /*Q_EMIT*/ valueChanged(current_);
}

void StringSliderControl::updateLabelText()
{
  label_->setText(current_.empty() ? NONE : QString::fromStdString(current_));
}
