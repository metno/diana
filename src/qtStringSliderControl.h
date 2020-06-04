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

#ifndef diana_qtStringSliderControl_h
#define diana_qtStringSliderControl_h

#include <QObject>

#include <string>
#include <vector>

class QLabel;
class QSlider;

class StringSliderControl : public QObject
{
  Q_OBJECT;

public:
  StringSliderControl(bool reverse, QSlider* slider, QLabel* label, QObject* parent = 0);
  ~StringSliderControl();

  void setValues(const std::vector<std::string>& values, const std::string& value);
  void setValues(const std::vector<std::string>& values);
  void setValue(const std::string& value);

  const std::string& value() const { return current_; }

Q_SIGNALS:
  void valueChanged(const std::string& value);

private Q_SLOTS:
  void onSliderPressed();
  void onSliderReleased();
  void onSliderChanged(int index);

private:
  void updateLabelText();

private:
  QSlider* slider_;
  QLabel* label_;

  std::vector<std::string> values_;
  bool reverse_;

  bool in_motion_;
  std::string current_;
};

#endif // diana_qtStringSliderControl_h
