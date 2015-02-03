/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015 met.no

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

#ifndef TESTVCROSSQTMANAGER_H
#define TESTVCROSSQTMANAGER_H 1

#include <QObject>
#include <string>
#include <vector>

namespace vcross {

class QtManager;

namespace test {
class ManagerSlots : public QObject {
  Q_OBJECT;

public:
  ManagerSlots(vcross::QtManager* manager);
  void reset();

private Q_SLOTS:
  void onFieldChangeBegin(bool fromScript);
  void onFieldAdded(int index);
  void onFieldRemoved(int index);
  void onFieldOptionsChanged(int index);
  void onFieldVisibilityChanged(int index);
  void onFieldChangeEnd();

  void onCrossectionListChanged();
  void onCrossectionIndexChanged(int current);

  void onTimeListChanged();
  void onTimeIndexChanged(int current);

public:
  bool beginScript, end;

  std::vector<int> added;
  std::vector<int> removed;
  std::vector<int> options;
  std::vector<int> visibility;

  int cslist, csindex, timelist, timeindex;
};
} // namespace test
} // namespace vcross

#endif // TESTVCROSSQTMANAGER_H
