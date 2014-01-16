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

#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <QDialog>
#include <QLineEdit>
#include <QString>
#include <QVariantMap>

class QContextMenuEvent;
class QTextEdit;
class DrawingItemBase;
class QWidget;

namespace Properties {

class TextEditor : public QDialog
{
public:
  TextEditor(const QString &text);
  virtual ~TextEditor();

  QString text() const;

private:
  QTextEdit *textEdit_;
};

class SpecialLineEdit : public QLineEdit
{
  Q_OBJECT
public:
  SpecialLineEdit(const QString &);
private:
  QString propertyName_;
  QString propertyName() const;
  void contextMenuEvent(QContextMenuEvent *);
private slots:
  void openTextEdit();
};

class PropertiesEditor : public QDialog
{
public:
  static PropertiesEditor *instance();
  bool edit(DrawingItemBase *);

private:
  PropertiesEditor();

  static PropertiesEditor *instance_;
  QWidget *formWidget_;
};

} // namespace

#endif // PROPERTIES_H
