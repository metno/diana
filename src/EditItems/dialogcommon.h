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

#ifndef EDITITEMSDIALOGCOMMON_H
#define EDITITEMSDIALOGCOMMON_H

#include <QLabel>
#include <QPixmap>
#include <QString>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QScrollArea>

class QToolButton;
class QIcon;

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

class ClickableLabel : public QLabel
{
  Q_OBJECT
public:
  ClickableLabel(const QString & = QString());
private:
  void mousePressEvent(QMouseEvent *);
  void mouseDoubleClickEvent(QMouseEvent *);
signals:
  void mouseClicked(QMouseEvent *);
  void mouseDoubleClicked(QMouseEvent *);
};

class ScrollArea : public QScrollArea
{
public:
  ScrollArea(QWidget * = 0);
private:
  void keyPressEvent(QKeyEvent *);
};

class Layer;
class LayerManager;

QToolButton *createToolButton(const QIcon &, const QString &, const QObject *, const char *);
QList<QSharedPointer<Layer> > createLayersFromFile(const QString &, LayerManager *, bool, QString *);
QList<QSharedPointer<Layer> > createLayersFromFile(LayerManager *, bool, QString *);
QString selectString(const QString &, const QString &, const QString &, const QStringList &, bool &);

} // namespace

#endif // EDITITEMSDIALOGCOMMON_H
