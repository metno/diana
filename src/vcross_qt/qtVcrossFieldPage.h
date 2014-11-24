/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2014 met.no

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

#ifndef QTVCROSSFIELDPAGE_H
#define QTVCROSSFIELDPAGE_H

#include <QWizardPage>

namespace vcross {
class QtManager;
}
class QLabel;
class QLineEdit;
class QListView;
class QSortFilterProxyModel;
class QStringListModel;
class QWidget;

class VcrossFieldPage : public QWizardPage {
  Q_OBJECT;

public:
  VcrossFieldPage(QWidget* parent=0);

  void initializePage();
  bool isComplete() const;
  
  QStringList selectedFields() const;

private Q_SLOTS:
  void onFilterTextChanged(const QString& text);

private:
  void retranslateUI();

private:
  QLineEdit* fieldFilter;
  QListView* fieldList;
  QStringListModel* fieldNames;
  QSortFilterProxyModel* fieldSorter;
};

#endif // QTVCROSSFIELDPAGE_H
