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

#ifndef TESTVCROSSQUICKMENUS_H
#define TESTVCROSSQUICKMENUS_H 1

#include <QObject>
#include <string>
#include <vector>

namespace vcross {

class VcrossQuickmenues;

namespace test {
class QuickmenuSlots : public QObject {
  Q_OBJECT;

public:
  QuickmenuSlots(VcrossQuickmenues* qm);
  void reset();

private Q_SLOTS:
  void onQuickmenuesUpdate(const std::string& t, const std::vector<std::string>& qm);

public:
  typedef std::vector<std::string> string_v;
  typedef std::vector<string_v> string_vv;

  string_v titles;
  string_vv qmenues;
};
} // namespace test
} // namespace vcross

#endif // TESTVCROSSQUICKMENUS_H
