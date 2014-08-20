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

#ifndef EIMTESTDIALOG_H
#define EIMTESTDIALOG_H

// #define ENABLE_EIM_TESTDIALOG
#ifdef ENABLE_EIM_TESTDIALOG

#include <QDialog>
#include <QSharedPointer>
#include <QSet>
#include <QVariant>
#include <EditItems/drawingitembase.h>

class QTextEdit;
class QCheckBox;
class QComboBox;
class QString;
class QPushButton;

class EIMTestDialog : public QDialog
{
  Q_OBJECT
public:
  EIMTestDialog(QWidget *);

private:
  QTextEdit *textEdit_;
  QCheckBox *enabledCheckBox_;
  QComboBox *selectComboBox_;
  int nEvents_;
  QSet<int> itemIds_;
  void updateSelectComboBox();
  void appendText(const QString &);

private slots:
  void changeEnabledState(int);
  void clear();
  void prepareCrossSectionPlacement();
  void handleItemChange(const QVariantMap &);
  void handleItemRemoval(int);
  void selectItem();
};

#endif // ENABLE_EIM_TESTDIALOG
#endif // EIMTESTDIALOG_H
