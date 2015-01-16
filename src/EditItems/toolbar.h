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

#ifndef EDITITEMSTOOLBAR_H
#define EDITITEMSTOOLBAR_H

#include <QToolBar>

class QComboBox;

namespace EditItems {

class ToolBar : public QToolBar
{
  Q_OBJECT
public:
  static ToolBar *instance();
  void setCreatePolyLineAction(const QString &);

private slots:
  void setPolyLineType(int index);
  void setSymbolType(int index);
  void setTextType(int index);
  void setCompositeType(int index);

private:
  ToolBar(QWidget * = 0);
  static ToolBar *self_; // singleton instance pointer

  QAction *selectAction_;
  QAction *polyLineAction_;
  QComboBox *polyLineCombo_;
  QAction *symbolAction_;
  QComboBox *symbolCombo_;
  QAction *textAction_;
  QComboBox *textCombo_;
  QAction *compositeAction_;
  QComboBox *compositeCombo_;

  void showEvent(QShowEvent *);
  void hideEvent(QHideEvent *);

signals:
  void visible(bool);
};

} // namespace

#endif // EDITITEMSTOOLBAR_H
