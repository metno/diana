/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006 met.no

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

#include "diana_config.h"

#include "qtTextView.h"
#include <QTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

TextWidget::TextWidget(QWidget* parent, const std::string& text, int id_)
  : QWidget(parent)
{
  id = id_;

  textEdit = new QTextEdit(this);
  textEdit->setReadOnly(true);
  textEdit->setText(QString(text.c_str()));

  QVBoxLayout *vlayout = new QVBoxLayout(this);
  vlayout->addWidget(textEdit);
}

void TextWidget::setText(std::string text)
{
  QString str = text.c_str();
  if(!str.isNull())
    textEdit->setText(str);
}

TextView::TextView(QWidget* parent)
  : QDialog(parent)
{
  tabwidget = new QTabWidget(this);

  QPushButton* printButton = new QPushButton(tr("Print"), this);
  connect(printButton, SIGNAL(clicked()), SLOT(printSlot()));

  QPushButton* hideButton = new QPushButton(tr("Hide"), this);
  connect(hideButton, SIGNAL(clicked()), SLOT(hide()));

  QHBoxLayout* hlayout = new QHBoxLayout();
  hlayout->addWidget(printButton);
  hlayout->addWidget(hideButton);
  QVBoxLayout* vlayout = new QVBoxLayout(this);
  vlayout->addWidget(tabwidget);
  vlayout->addLayout(hlayout);
}

void TextView::setText(int id, const std::string& name,
		      const std::string& text)
{

  if (idmap.count(id) == 0) {
    TextWidget* widget = new TextWidget(this, text, id);
    idmap[id] = widget;
    tabwidget->addTab(widget, name.c_str());
  }
  idmap[id]->setText(text);
}

void TextView::deleteTab(int id)
{
  if (idmap.count(id) != 0) {
    tabwidget->removeTab(tabwidget->indexOf(idmap[id]));
    idmap.erase(id);
  }

  if (idmap.size() == 0)
    hide();
}
void TextView::deleteTab()
{
  if (idmap.size() != 0) {
    std::map<int,TextWidget*>::iterator p = idmap.begin();
    for (; p!=idmap.end(); p++){
      tabwidget->removeTab(tabwidget->indexOf(idmap[p->first]));
      idmap.erase(p->first);
    }
  }

  if (idmap.size() == 0)
    hide();
}

void TextView::printSlot()
{
  emit printClicked(tabwidget->currentIndex());
}
