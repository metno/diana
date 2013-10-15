/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2013 met.no

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
#ifndef _datadialog_h
#define _datadialog_h

#include <QDialog>
#include <vector>
#include <puTools/miString.h>
#include <puTools/miTime.h>

using namespace std;

class Controller;
class QAction;

class DataDialog : public QDialog
{
  Q_OBJECT

public:
  DataDialog(QWidget *parent, Controller *ctrl);
  virtual ~DataDialog();

  virtual QAction *action() const;
  virtual std::string name() const = 0;

  /// Update the dialog after re-reading the setup file.
  virtual void updateDialog() = 0;
  /// Returns the vector of command strings in use.
  virtual std::vector<miutil::miString> getOKString() = 0;
  /// Sets new command strings to be represented in the dialog.
  virtual void putOKString(const std::vector<miutil::miString>& vstr) = 0;

signals:
  void emitTimes(const miutil::miString &, const std::vector<miutil::miTime> &);
  void emitTimes(const miutil::miString &, const std::vector<miutil::miTime> &, bool);
  void applyData();
  void hideData();
  void showsource(const std::string, const std::string="");

protected:
  Controller *m_ctrl;
  QAction *m_action;
};

#endif