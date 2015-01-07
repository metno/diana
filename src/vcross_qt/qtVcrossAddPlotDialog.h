/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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

#ifndef QTVCROSSADDPLOTDIALOG_H
#define QTVCROSSADDPLOTDIALOG_H

#include <QDialog>
#include <memory>

class QSortFilterProxyModel;
class QStringListModel;

namespace vcross {
class QtManager;
}
class VcrossSelectionManager;
class Ui_VcrossAddPlotDialog;

class VcrossAddPlotDialog : public QDialog {
  Q_OBJECT

public:
  VcrossAddPlotDialog(QWidget* parent, VcrossSelectionManager* vsm);

public Q_SLOTS:
  void restart();

private:
  void setupUi();

  QString selectedModel() const;
  QStringList selectedPlots() const;

  void initializeModelPage(bool forward);
  bool isModelComplete() const;

  void initializePlotPage(bool forward);
  bool isPlotComplete() const;

private Q_SLOTS:
  void onBack();
  void onNext();
  void onAdd();

  void checkModelComplete();
  void checkPlotComplete();

  void onModelFilter(const QString& text);
  void onPlotFilter(const QString& text);

private:
  enum { ModelPage, PlotPage };

  VcrossSelectionManager* selectionManager;
  std::auto_ptr<Ui_VcrossAddPlotDialog> ui;

  QStringListModel* modelNames;
  QSortFilterProxyModel* modelSorter;

  QStringListModel* plotNames;
  QSortFilterProxyModel* plotSorter;
};

#endif // QTVCROSSADDPLOTDIALOG_H
