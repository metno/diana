/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013-2018 met.no

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

#include "diPlotCommand.h"
#include "diTimeTypes.h"

#include <QDialog>

#include <vector>

class Controller;
class QAction;
class QPushButton;
class QAbstractButton;

class ShowMoreDialog : public QDialog
{
  Q_OBJECT

public:
  ShowMoreDialog(QWidget* parent=0);

  virtual bool showsMore();

public Q_SLOTS:
  virtual void showMore(bool more);

protected:
  virtual void doShowMore(bool more);

private:
  Qt::Orientation orientation;
  QSize sizeLess, sizeMore;
};


class DataDialog : public ShowMoreDialog
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
  virtual PlotCommand_cpv getOKString() = 0;

  /// Set new command strings, representing them in the dialog.
  virtual void putOKString(const PlotCommand_cpv& vstr) = 0;

  void setVisible(bool show) override;

public Q_SLOTS:
  /// Unsets the dialog action in order to make the dialog ready for opening.
  void unsetAction();

  /// Update the times that the dialog knows about.
  virtual void updateTimes() = 0;

  /// Update the dialog after re-reading the setup file.
  virtual void updateDialog() = 0;

Q_SIGNALS:
  void sendTimes(const std::string& datatype, const plottimes_t& times, bool use);
  void applyData();
  void hideData();
  void showsource(const std::string, const std::string="");
  void updated();

protected:
  void emitTimes(const plottimes_t& times, bool use) { sendTimes(name(), times, use); }
  void emitTimes(const plottimes_t& times) { sendTimes(name(), times, true); }

  void closeEvent(QCloseEvent* event) override;
  QLayout* createStandardButtons(bool refresh);
  void indicateUnappliedChanges(bool);

  void setRefreshEnabled(bool enabled);

private:
  QAbstractButton* createButton(const QString& tooltip, const QIcon& icon);

private slots:
  void applyhideClicked();
  void helpClicked();

protected:
  Controller* m_ctrl;
  QAction* m_action;
  std::string helpFileName;

private:
  QAbstractButton* applyhideButton;
  QPushButton* applyButton;
  QAbstractButton* refreshButton;
  QAbstractButton* helpButton;
};

#endif
