/*
  Diana - A Free Meteorological Visualisation Tool

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

#include <puTools/miTime.h>
#include <QDialog>
#include <vector>

class Controller;
class QAction;

class DataDialog : public QDialog
{
  Q_OBJECT

public:
  DataDialog(QWidget *parent, Controller *ctrl);
  virtual ~DataDialog();

  virtual QAction *action() const;
  /// Returns the name of the data source that the dialog displays. This should
  /// be the same as the name used by the corresponding manager.
  virtual std::string name() const = 0;

  /// Returns the vector of command strings in use.
  virtual std::vector<std::string> getOKString() = 0;
  /// Set new command strings, representing them in the dialog.
  virtual void putOKString(const std::vector<std::string>& vstr) = 0;

public slots:
  /// Unsets the dialog action in order to make the dialog ready for opening.
  void unsetAction();
  /// Update the times that the dialog knows about.
  virtual void updateTimes() = 0;
  /// Update the dialog after re-reading the setup file.
  virtual void updateDialog() = 0;

signals:
  void emitTimes(const std::string &, const std::vector<miutil::miTime> &);
  void emitTimes(const std::string &, const std::vector<miutil::miTime> &, bool);
  void applyData();
  void hideData();
  void showsource(const std::string, const std::string="");
  void updated();

private:
  QPushButton *applyhideButton;
  QPushButton *applyButton;

protected:
  virtual void closeEvent(QCloseEvent *event);
  QLayout *createStandardButtons();
  void indicateUnappliedChanges(bool);

  Controller *m_ctrl;
  QAction *m_action;
  std::string helpFileName;

private slots:
  void applyhideClicked();
  void helpClicked();
};

#endif
