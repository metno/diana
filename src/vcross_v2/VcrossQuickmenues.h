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

#ifndef VCROSSQUICKMENUS_H
#define VCROSSQUICKMENUS_H

#include "VcrossQtManager.h"
#include <QObject>
#include <vector>

namespace vcross {

/**
   \brief Managing Vertical Crossection data sources and plotting
*/
class VcrossQuickmenues : public QObject {
  Q_OBJECT;

public:
  typedef std::vector<std::string> string_v;

  VcrossQuickmenues(QtManager_p manager);
  ~VcrossQuickmenues();

  void selectFields(const string_v& fields);

  static void parse(QtManager_p manager, const string_v& vstr);

  void parse(const string_v& vstr);
  string_v get() const;

Q_SIGNALS:
  void quickmenuUpdate(const std::string& title, const std::vector<std::string>& qm);

private Q_SLOTS:
  void onFieldChangeBegin(bool fromScript);
  void onFieldAdded(int position);
  void onFieldRemoved(int position);
  void onFieldOptionsChanged(int position);
  void onFieldVisibilityChanged(int position);
  void onFieldChangeEnd();

  void onCrossectionChanged(int currentIndex);

private:
  std::string getQuickMenuTitle() const;

  void emitQmIfNotInGroup();
  void emitQm();

private:
  QtManager_p mManager;
  bool mInFieldChangeGroup; //!< true if grouped changes
  bool mUpdatesFromScript;  //!< true if field changes come from a script
  bool mFieldsChanged;      //!< true if not only visibility changed
};

} // namespace vcross

#endif // VCROSSQUICKMENUS_H
