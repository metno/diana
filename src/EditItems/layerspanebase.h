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

#ifndef LAYERSPANEBASE_H
#define LAYERSPANEBASE_H

#include <QWidget>
//#define QT_SHAREDPOINTER_TRACK_POINTERS
#include <QSharedPointer>
#include <QString>
#include <QMouseEvent>
#include <QKeyEvent>

#include "edit.xpm"
#include "hideall.xpm"
#include "movedown.xpm"
#include "moveup.xpm"
#include "showall.xpm"
#include "visible.xpm"
#include "unsavedchanges.xpm"

class QVBoxLayout;
class QHBoxLayout;
class QToolButton;
class QLabel;
class QMenu;
class QAction;

namespace EditItems {

class Layer;
class LayerManager;
class ScrollArea;
class CheckableLabel;
class ClickableLabel;

class LayerWidget : public QWidget
{
  Q_OBJECT
public:
  LayerWidget(LayerManager *, const QSharedPointer<Layer> &, bool, bool = true, bool = true, QWidget * = 0);
  ~LayerWidget();
  QSharedPointer<Layer> layer() const;
  QString name() const;
  void setName(const QString &);
  bool isLayerVisible() const; // Note that isVisible() and setVisible() are already used in QWidget!
  void setLayerVisible(bool);  // ---''---
  bool hasUnsavedChanges() const;
  void setUnsavedChanges(bool);
  void setSelected(bool = true);
  void editName();
  void setState(const QSharedPointer<Layer> &);
  void showInfo(bool);
  bool isRemovable() const;
  bool isAttrsEditable() const;
private:
  LayerManager *layerMgr_;
  QSharedPointer<Layer> layer_;
  CheckableLabel *visibleLabel_;
  CheckableLabel *unsavedChangesLabel_;
  ClickableLabel *nameLabel_;
  ClickableLabel *infoLabel_;
  bool removable_;
  bool attrsEditable_;
public slots:
  void updateLabels();
private slots:
  void handleVisibilityChanged(bool);
signals:
  void mouseClicked(QMouseEvent *);
  void mouseDoubleClicked(QMouseEvent *);
  void visibilityChanged(bool);
};

class LayersPaneBase : public QWidget
{
  Q_OBJECT
public:
  void showInfo(bool);
  void init();
  QString saveVisible(const QString &) const;
  QString saveSelected(const QString &) const;

protected:
  LayersPaneBase(LayerManager *, const QString &, bool);

protected: // ### some of these may be private ... TBD
  QVBoxLayout *layout_;
  ScrollArea *scrollArea_;
  QToolButton *showAllButton_;
  QToolButton *hideAllButton_;
  QToolButton *moveUpButton_;
  QToolButton *moveDownButton_;
  QToolButton *editButton_;
  QToolButton *importFilesButton_;
  bool showInfo_;

  virtual void initLayerWidget(LayerWidget *);
  void keyPressEvent(QKeyEvent *);
  QList<int> selectedPos() const;
  void selectIndex(int, bool = true);
  void select(const QList<LayerWidget *> &, bool = true);
  void select(LayerWidget *, bool = true);
  void selectExclusive(LayerWidget *);
  LayerWidget *atPos(int);
  void ensureVisible(LayerWidget *);
  void remove(const QList<LayerWidget *> &, bool = false, bool = true);
  void move(LayerWidget *, bool);
  void moveUp(LayerWidget *);
  void moveUp(int);
  void moveDown(LayerWidget *);
  void moveDown(int);
  void editAttrs(LayerWidget *);
  void setAllVisible(bool);
  QList<LayerWidget *> widgets(bool = false, bool = false) const;
  QList<LayerWidget *> visibleWidgets() const;
  QList<LayerWidget *> selectedWidgets() const;
  QList<LayerWidget *> allWidgets() const;
  QList<QSharedPointer<Layer> > layers(const QList<LayerWidget *> &) const;

protected:
  LayerManager *layerMgr_;
  QHBoxLayout *bottomLayout_; // populated by subclass
  virtual void updateButtons();
  virtual void addContextMenuActions(QMenu &) const {}
  virtual bool handleContextMenuAction(const QAction *, const QList<LayerWidget *> &) { return false; }
  virtual bool handleKeyPressEvent(QKeyEvent *) { return false; }
  void getLayerCounts(int &, int &, int &, int &) const;
  void getSelectedLayersItemCounts(int &, int &) const;

protected slots: // ### some of these may be private ... TBD
  void mouseClicked(QMouseEvent *);
  void mouseDoubleClicked(QMouseEvent *);
  void showAll();
  void hideAll();
  void moveSingleSelectedUp();
  void moveSingleSelectedDown();
  void editAttrsOfSingleSelected();
  void handleWidgetsUpdate();
  void updateWidgetStructure();

private slots:
  void ensureVisibleTimeout();

private:
  LayerWidget *visibleLayerWidget_;
  bool multiSelectable_;

  QString saveLayers(const QList<QSharedPointer<Layer> > &, const QString &) const;

signals:
  void updated();
};

} // namespace

#endif // LAYERSPANEBASE_H
