/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2014 met.no

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

#ifndef DIVCROSSINTERFACE_H
#define DIVCROSSINTERFACE_H 1

#include <puDatatypes/miCoordinates.h>
#include <puTools/miTime.h>

#include <QtPlugin>
#include <QStringList>
#include <QVariantMap>

#include <string>
#include <vector>

class LogFileIO;
struct LocationData;

/**
 * This class forwards signals used for vertical crossections.
 */
class VcrossInterface : public QObject {
  Q_OBJECT

public:
  virtual ~VcrossInterface() { }

Q_SIGNALS: // emitted by vcross
  //! vcross window was closed
  void VcrossHide();

  //! help window
  void requestHelpPage(const std::string& source, const std::string& tag="");

  //! help window
  void requestLoadCrossectionFiles(const QStringList& filenames);

  //! request starting the edit manager for vertical crossections
  void requestVcrossEditor(bool on);

  //! changed list of crossections and/or the style
  void crossectionSetChanged(const LocationData& locations);

  //! currently selected crossection has changed
  void crossectionChanged(const QString&);

  //! sent if quickmenu history needs to be updated, i.e. new plot (new fields, removed fields, ...)
  void quickMenuStrings(const std::string&, const std::vector<std::string>&);

  //! request to navigate to previous quickmenu vcross history item
  void vcrossHistoryPrevious();

  //! request to navigate to next quickmenu vcross history item
  void vcrossHistoryNext();

  //! time list from crossections updated
  void emitTimes(const std::string&, const std::vector<miutil::miTime>&);

  //! currently selected time has changed in vcross
  void setTime(const std::string&, const miutil::miTime&);

public Q_SLOTS:
  //! editmanager has changed something ...
  virtual void editManagerChanged(const QVariantMap &props) = 0;

  //! an object was deleted in editmanager
  virtual void editManagerRemoved(int id) = 0;

  //! editmanager was started / stopped
  virtual void editManagerEditing(bool editing) = 0;

public:
  //! raise/show/hide vcross window
  virtual void makeVisible(bool visible) = 0;

  virtual void parseSetup() = 0;

  //! request to change currently selected crossection
  virtual void changeCrossection(const std::string& csName) = 0;

  //! request to show timegraph for the given position
  virtual void showTimegraph(const LonLat& position) = 0;

  //! request to change currently selected time
  virtual void mainWindowTimeChanged(const miutil::miTime& t) = 0;

  virtual void parseQuickMenuStrings(const std::vector<std::string>& vstr) = 0;

  virtual void writeLog(LogFileIO& logfile) = 0;

  virtual void readLog(const LogFileIO& logfile, const std::string& thisVersion, const std::string& logVersion,
      int displayWidth, int displayHeight) = 0;
};

Q_DECLARE_INTERFACE(VcrossInterface, "metno.diana.VcrossInterface/1.0");

#endif
