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

#ifndef EDITITEMSTOOLBAR_H
#define EDITITEMSTOOLBAR_H

#include <QStyledItemDelegate>
#include <QToolBar>

class QListBox;
class QListWidget;
class QListWidgetItem;
class QToolButton;
class QDockWidget;

namespace EditItems {

class CompositeDelegate : public QStyledItemDelegate
{
public:
  CompositeDelegate(QObject *parent = 0);
  ~CompositeDelegate();

  virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class ToolBar : public QToolBar
{
  Q_OBJECT

public:
  static ToolBar *instance(QWidget * parent);

public slots:
  void setSelectAction();
  void setCreatePolyLineAction(const QString &);
  void setPolyLineType(QListWidgetItem *item);
  void setCreateSymbolAction(const QString &type);
  void setSymbolType(QListWidgetItem *item);
  void setTextType(QListWidgetItem *item);
  void setCompositeType(QListWidgetItem *item);

  void addSymbol(const QString &section, const QString &name);
  void addSymbols(const QString &section, const QStringList &names);

private slots:
  void showComposites();
  void showPolyLines();
  void showSymbols();
  void showTexts();
  void show_hide_all();

private:
  ToolBar(QWidget * = 0);
  static ToolBar *self_; // singleton instance pointer
  QWidget * parent_;
  QAction *selectAction_;
  QAction *polyLineAction_;
  QDockWidget * polyLineWidget;
  QListWidget *polyLineList_;
  
  QAction *symbolAction_;
  QDockWidget * symbolWidget;
  QListWidget *symbolList_;
  
  QAction *textAction_;
  QDockWidget * textWidget;
  QListWidget *textList_;

  QAction *compositeAction_;
  CompositeDelegate *compositeDelegate_;
  QDockWidget *compositeWidget_;
  QListWidget *compositeView;

  QStringList sections_;
};

} // namespace

#endif // EDITITEMSTOOLBAR_H
