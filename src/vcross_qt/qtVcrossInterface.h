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

#ifndef _qtvcrossinterface_
#define _qtvcrossinterface_

#include "diVcrossInterface.h"
#include "vcross_v2/VcrossQuickmenues.h"

#include <QVariant>
#include <set>
#include <string>
#include <vector>

class VcrossWindow;

#ifndef Q_DECL_OVERRIDE
#define Q_DECL_OVERRIDE /* empty */
#endif

class VcrossWindowInterface : public VcrossInterface
{
  Q_OBJECT;
  Q_INTERFACES(VcrossInterface);

public:
  VcrossWindowInterface();
  ~VcrossWindowInterface();

  void makeVisible(bool visible) Q_DECL_OVERRIDE;
  void parseSetup() Q_DECL_OVERRIDE;
  void changeCrossection(const std::string& csName) Q_DECL_OVERRIDE;
  void showTimegraph(const LonLat& position) Q_DECL_OVERRIDE;
  void mainWindowTimeChanged(const miutil::miTime& t) Q_DECL_OVERRIDE;
  void parseQuickMenuStrings(const std::vector<std::string>& vstr) Q_DECL_OVERRIDE;
  void writeLog(LogFileIO& logfile) Q_DECL_OVERRIDE;
  void readLog(const LogFileIO& logfile, const std::string& thisVersion, const std::string& logVersion,
      int displayWidth, int displayHeight) Q_DECL_OVERRIDE;

public: /* Q_SLOT implementations */
  void editManagerChanged(const QVariantMap &props) Q_DECL_OVERRIDE;
  void editManagerRemoved(int id) Q_DECL_OVERRIDE;
  void editManagerEditing(bool editing) Q_DECL_OVERRIDE;

private Q_SLOTS:
  void onVcrossHide();
  void crossectionChangedSlot(int);
  void crossectionListChangedSlot();
  void timeChangedSlot(int);
  void timeListChangedSlot();

private:
  bool checkWindow();
  void loadPredefinedCS();

private:
  vcross::QtManager_p vcrossm;
  std::auto_ptr<vcross::VcrossQuickmenues> quickmenues;

  VcrossWindow* window;

  std::set<std::string> mPredefinedCsFiles;
};

#endif // _qt_vcrossmainwindow_
