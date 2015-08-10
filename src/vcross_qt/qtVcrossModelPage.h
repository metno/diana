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

#ifndef QTVCROSSMODELPAGE_H
#define QTVCROSSMODELPAGE_H

#include "vcross_v2/VcrossQtManager.h"

#include <QWidget>

#include <memory>

class QSortFilterProxyModel;
class QStringListModel;

class Ui_VcrossModelPage;

class VcrossModelPage : public QWidget {
  Q_OBJECT

public:
  VcrossModelPage(QWidget* parent=0);
  void setManager(vcross::QtManager_p vm);

public:
  void initialize(bool forward);
  bool isComplete() const;
  QString selected() const;

Q_SIGNALS:
  void completeStatusChanged(bool complete);
  void requestNext();

private Q_SLOTS:
  void checkComplete();
  void listActivated();
  void onFilter(const QString& text);

private:
  vcross::QtManager_p vcrossm;
  std::auto_ptr<Ui_VcrossModelPage> ui;
  QStringListModel* modelNames;
  QSortFilterProxyModel* modelSorter;

};

#endif // QTVCROSSMODELPAGE_H
