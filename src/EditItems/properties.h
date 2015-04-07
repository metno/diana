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
#include <QHash>
#include <QLineEdit>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class QContextMenuEvent;
class DrawingItemBase;
class QWidget;
class QDialogButtonBox;

namespace Properties {

class SpecialLineEdit : public QLineEdit
{
  Q_OBJECT
public:
  SpecialLineEdit(const QString &, bool = false);
private:
  QString propertyName_;
  QString propertyName() const;
  void contextMenuEvent(QContextMenuEvent *);
  void mouseDoubleClickEvent(QMouseEvent *);
private slots:
  void openTextEdit();
};

class PropertiesEditor : public QDialog
{
public:
  static PropertiesEditor *instance();
  bool edit(QSharedPointer<DrawingItemBase> &, bool = false);
  void setPropertyRules(const QString &rule, const QStringList &values);

private:
  PropertiesEditor();
  QWidget *createEditor(const QString &propertyName, const QVariant &val, bool readOnly = false);

  static PropertiesEditor *instance_;
  QWidget *formWidget_;
  QDialogButtonBox *buttonBox_;
  QDialogButtonBox *readOnlyButtonBox_;

  QHash<QString, QStringList> rules_;
};

} // namespace

#endif // PROPERTIES_H
