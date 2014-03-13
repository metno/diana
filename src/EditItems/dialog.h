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

#ifndef EDITITEMSDIALOG_H
#define EDITITEMSDIALOG_H

#include <QLabel>
#include <QScrollArea>
#include <QStandardItemModel>
#include "qtDataDialog.h"
#include "EditItems/layers.h"

class QListView;
class QVBoxLayout;
class QToolButton;
class EditItemManager;

namespace EditItems {

class CheckableLabel : public QLabel
{
  Q_OBJECT
public:
  CheckableLabel(bool, const QPixmap &, const QString &, const QString &, bool = true);
  void setChecked(bool);
  bool isChecked() { return checked_; }
private:
  bool checked_;
  QPixmap pixmap_;
  QString checkedToolTip_;
  QString uncheckedToolTip_;
  bool clickable_;
  void mousePressEvent(QMouseEvent *);
signals:
  void mouseClicked(QMouseEvent *);
  void checked(bool);
};

class NameLabel : public QLabel
{
  Q_OBJECT
public:
  NameLabel(const QString &);
private:
  void mousePressEvent(QMouseEvent *);
  void mouseDoubleClickEvent(QMouseEvent *);
signals:
  void mouseClicked(QMouseEvent *);
  void mouseDoubleClicked(QMouseEvent *);
};

class LayerWidget : public QWidget
{
  Q_OBJECT
public:
  LayerWidget(const QSharedPointer<Layer> &, QWidget * = 0);
  ~LayerWidget();
  QSharedPointer<Layer> layer();
  QString name() const;
  void setName(const QString &);
  bool isLayerVisible() const; // Note that isVisible() and setVisible() are already used in QWidget!
  void setLayerVisible(bool);  // ---''---
  bool hasUnsavedChanges() const;
  void setUnsavedChanges(bool);
  void setCurrent(bool);
  void editName();
private:
  QSharedPointer<Layer> layer_;
  CheckableLabel *visibleLabel_;
  CheckableLabel *unsavedChangesLabel_;
  NameLabel *nameLabel_;
private slots:
  void handleVisibilityChanged(bool);
signals:
  void mouseClicked(QMouseEvent *);
  void mouseDoubleClicked(QMouseEvent *);
  void visibilityChanged(bool);
};

class ScrollArea : public QScrollArea
{
public:
  ScrollArea(QWidget * = 0);
private:
  void keyPressEvent(QKeyEvent *);
};

class Dialog : public DataDialog
{
  Q_OBJECT

public:
  Dialog(QWidget *, Controller *);

  virtual std::string name() const;
  virtual void updateDialog();
  virtual std::vector<std::string> getOKString();
  virtual void putOKString(const std::vector<std::string> &);

private:
  QVBoxLayout *activeLayersLayout_;
  ScrollArea *activeLayersScrollArea_;
  QToolButton *addEmptyButton_;
  QToolButton *mergeVisibleButton_;
  QToolButton *showAllButton_;
  QToolButton *hideAllButton_;
  QToolButton *duplicateCurrentButton_;
  QToolButton *removeCurrentButton_;
  QToolButton *moveCurrentUpButton_;
  QToolButton *moveCurrentDownButton_;
  QToolButton *editCurrentButton_;
  QToolButton *importFilesButton_;
  QToolButton *loadFileButton_;
  QToolButton *createToolButton(const QIcon &, const QString &, const char *) const;

  EditItemManager *editm_;
  QWidget *createAvailableLayersPane();
  QWidget *createActiveLayersPane();
  void initialize(LayerWidget *);
  void keyPressEvent(QKeyEvent *);
  int currentPos() const;
  LayerWidget *current();
  void setCurrentIndex(int);
  void setCurrent(LayerWidget *);
  void updateCurrent();
  LayerWidget *atPos(int);
  void ensureVisible(LayerWidget *);
  void ensureCurrentVisible();
  void add(const QSharedPointer<Layer> &, bool = false);
  void duplicate(LayerWidget *);
  void remove(LayerWidget *, bool = false);
  void remove(int);
  void move(LayerWidget *, bool);
  void moveUp(LayerWidget *);
  void moveUp(int);
  void moveDown(LayerWidget *);
  void moveDown(int);
  void setAllVisible(bool);
  QList<LayerWidget *> visibleLayerWidgets();
  QList<LayerWidget *> allLayerWidgets();
  QList<QSharedPointer<Layer> > layers(const QList<LayerWidget *> &);
  void toggleEditingMode(bool);

private slots:
  void updateButtons();
  void updateModel();
  virtual void updateTimes();
  void toggleDrawingMode(bool);
  void mouseClicked(QMouseEvent *);
  void mouseDoubleClicked(QMouseEvent *);
  void ensureCurrentVisibleTimeout();
  void addEmpty();
  void mergeVisible();
  void showAll();
  void hideAll();
  void duplicateCurrent();
  void removeCurrent();
  void moveCurrentUp();
  void moveCurrentDown();
  void editCurrent();
  void handleLayersStateUpdate();
  void importChosenFiles();
  void loadFile();
  void handleLayersUpdated();

private:
  /// List of available drawings from the setup file.
  QListView *drawingList;
  QStandardItemModel drawingModel;
};

} // namespace

#endif // EDITITEMSDIALOG_H
