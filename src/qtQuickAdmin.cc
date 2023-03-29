/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2018 met.no

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

#include "qtQuickAdmin.h"
#include "qtQuickEditOptions.h"
#include "qtUtility.h"
#include "diUtilities.h"
#include "diLocalSetupParser.h"

#include "util/string_util.h"
#include <puTools/miStringFunctions.h>

#include <QPushButton>
#include <QTreeWidget>
#include <QLabel>
#include <QFrame>
#include <QInputDialog>
#include <QFile>
#include <QFileDialog>
#include <QTextEdit>
#include <QRegExp>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPixmap>
#include <QVBoxLayout>
#include <QShortcut>

#include "up12x12.xpm"
#include "down12x12.xpm"
#include "filenew.xpm"
#include "fileopen.xpm"
#include "editcopy.xpm"
#include "editcut.xpm"
#include "editpaste.xpm"

#define MILOGGER_CATEGORY "diana.QuickAdmin"
#include <miLogger/miLogging.h>


class QuickTreeWidgetItem: public QTreeWidgetItem {
private:
  int menu;
  int item;
public:
  QuickTreeWidgetItem(QTreeWidget * parent, QStringList t, int m, int i) :
    QTreeWidgetItem(parent, t), menu(m), item(i)
    {
    }
  QuickTreeWidgetItem(QTreeWidgetItem * parent, QStringList t, int m, int i) :
    QTreeWidgetItem(parent, t), menu(m), item(i)
    {
    }

  int Menu() const
  {
    return menu;
  }
  int Item() const
  {
    return item;
  }
};

QuickAdmin::QuickAdmin(QWidget* parent, const std::vector<quickMenu>& qm)
    : QDialog(parent)
    , menus(qm)
    , autochange(false)
    , active_list(-1)
    , active_item(-1)
    , copyMenu(-1)
    , copyElement(-1)
{
  setModal(true);

  setWindowTitle(tr("Edit quickmenus"));

  menutree = new QTreeWidget(this);
  menutree->setHeaderHidden(true);
  menutree->setSortingEnabled(false);
  menutree->setColumnCount(1);

  connect(menutree, &QTreeWidget::itemClicked, this, &QuickAdmin::selectionChanged);

  // up
  upButton = PixmapButton(QPixmap(up12x12_xpm), this, 14, 12);
  upButton->setEnabled(false);
  upButton->setShortcut(tr("Ctrl+Up"));
  connect(upButton, &QPushButton::clicked, this, &QuickAdmin::upClicked);

  // down
  downButton = PixmapButton(QPixmap(down12x12_xpm), this, 14, 12);
  downButton->setEnabled(false);
  downButton->setShortcut(tr("Ctrl+Down"));
  connect(downButton, &QPushButton::clicked, this, &QuickAdmin::downClicked);

  // Command buttons for menu-elements
  // new
  newButton = new QPushButton(QPixmap(filenew_xpm), tr("&New"), this);
  newButton->setEnabled(false);
  connect(newButton, &QPushButton::clicked, this, &QuickAdmin::newClicked);

  // new from file
  newfileButton = new QPushButton(QPixmap(fileopen_xpm), tr("New from &file.."), this);
  newfileButton->setEnabled(false);
  connect(newfileButton, &QPushButton::clicked, this, &QuickAdmin::newfileClicked);

  // rename
  renameButton = new QPushButton(tr("&Change name.."), this);
  renameButton->setEnabled(false);
  connect(renameButton, &QPushButton::clicked, this, &QuickAdmin::renameClicked);

  // erase
  eraseButton = new QPushButton(QPixmap(editcut_xpm), "", this);
  eraseButton->setEnabled(false);
  connect(eraseButton, &QPushButton::clicked, this, &QuickAdmin::eraseClicked);

  // copy
  copyButton = new QPushButton(QPixmap(editcopy_xpm), "", this);
  copyButton->setEnabled(false);
  connect(copyButton, &QPushButton::clicked, this, &QuickAdmin::copyClicked);

  // paste
  pasteButton = new QPushButton(QPixmap(editpaste_xpm), "", this);
  pasteButton->setEnabled(false);
  connect(pasteButton, &QPushButton::clicked, this, &QuickAdmin::pasteClicked);

  updateButtonTexts();

  // a horizontal frame line
  QFrame* line = new QFrame(this);
  line->setFrameStyle(QFrame::HLine | QFrame::Sunken);

  // create commands-area
  comedit = new QTextEdit(this);
  comedit->setLineWrapMode(QTextEdit::NoWrap);
  comedit->setFont(QFont("Courier", 12, QFont::Normal));
  comedit->setReadOnly(false);
  comedit->setMaximumHeight(150);
  connect(comedit, SIGNAL(textChanged()), SLOT(comChanged()));

  QLabel* comlabel = new QLabel(tr("Command field"), this);
  comlabel->setMinimumSize(comlabel->sizeHint());

  // edit options
  optionButton = new QPushButton(tr("&Dynamic options.."), this);
  optionButton->setEnabled(false);
  connect(optionButton, SIGNAL(clicked()), SLOT(optionClicked()));

  // a horizontal frame line
  QFrame* line2 = new QFrame(this);
  line2->setFrameStyle(QFrame::HLine | QFrame::Sunken);

  // last row of buttons
  QPushButton* ok = new QPushButton(tr("&OK"), this);
  QPushButton* cancel = new QPushButton(tr("&Cancel"), this);
  connect(ok, SIGNAL(clicked()), SLOT(accept()));
  connect(cancel, SIGNAL(clicked()), SLOT(reject()));


  QBoxLayout* vl1 = new QVBoxLayout();
  vl1->addWidget(upButton);
  vl1->addWidget(downButton);

  QHBoxLayout* hl1 = new QHBoxLayout();
  hl1->addWidget(menutree);
  hl1->addLayout(vl1);

  QGridLayout* gl = new QGridLayout();
  gl->addWidget(newButton, 0, 0);
  gl->addWidget(newfileButton, 0, 1);
  gl->addWidget(renameButton, 0, 2);
  gl->addWidget(eraseButton, 1, 0);
  gl->addWidget(copyButton, 1, 1);
  gl->addWidget(pasteButton, 1, 2);

  QHBoxLayout* hl = new QHBoxLayout();
  hl->addWidget(optionButton);
  hl->addStretch(1);

  QHBoxLayout* hl4 = new QHBoxLayout();
  hl4->addStretch();
  hl4->addWidget(ok);
  hl4->addStretch();
  hl4->addWidget(cancel);
  hl4->addStretch();

  // top layout
  QVBoxLayout* vlayout = new QVBoxLayout(this);
  vlayout->addLayout(hl1);
  vlayout->addLayout(gl);
  vlayout->addWidget(line);
  vlayout->addWidget(comlabel);
  vlayout->addWidget(comedit);
  vlayout->addLayout(hl);
  vlayout->addWidget(line2);
  vlayout->addLayout(hl4);

  updateWidgets();
  resize(500, 550);
}

void QuickAdmin::updateButtonTexts()
{
  if (active_item == -1) {
    newButton->setText(tr("&New menu.."));
    copyButton->setText(tr("Copy menu"));
    eraseButton->setText(tr("Remove menu.."));
  } else {
    newButton->setText(tr("&New plot.."));
    copyButton->setText(tr("Copy plot"));
    eraseButton->setText(tr("Remove plot"));
  }
  if (copyMenu == -1)
    pasteButton->setText(tr("Paste"));
  else if (copyElement == -1)
    pasteButton->setText(tr("Paste menu"));
  else
    pasteButton->setText(tr("Paste plot"));

  // need to set shortcuts after changing text
  copyButton->setShortcut(tr("Ctrl+C"));
  eraseButton->setShortcut(tr("Ctrl+X"));
  pasteButton->setShortcut(tr("Ctrl+V"));
}

void QuickAdmin::selectionChanged(QTreeWidgetItem* p, int)
{
  if (!p)
    return;
  QuickTreeWidgetItem* qp = (QuickTreeWidgetItem*)(p);
  active_list = qp->Menu();
  active_item = qp->Item();
  updateButtonTexts();
  updateCommand();

  const int nitems = menus[active_list].menuitems.size();

  if (menus[active_list].type == quickMenu::QM_USER) {
    upButton->setEnabled(active_item > 0);
    downButton->setEnabled(active_item >= 0 && active_item < nitems - 1);
    newButton->setEnabled(true);
    newfileButton->setEnabled(true);
    renameButton->setEnabled(true);
    eraseButton->setEnabled(true);
    copyButton->setEnabled(true);
    pasteButton->setEnabled(copyMenu != -1);
    optionButton->setEnabled(true);

  } else if (menus[active_list].is_history()) {
    upButton->setEnabled(active_item > 0);
    downButton->setEnabled(active_item >= 0 && active_item < nitems - 1);
    newButton->setEnabled(active_item == -1);
    newfileButton->setEnabled(true);
    renameButton->setEnabled(false);
    eraseButton->setEnabled(active_item != -1);
    copyButton->setEnabled(true);
    pasteButton->setEnabled(copyMenu != -1 && active_item == -1);
    optionButton->setEnabled(false);

  } else { // standard menus
    upButton->setEnabled(false);
    downButton->setEnabled(false);
    newButton->setEnabled(false);
    newfileButton->setEnabled(false);
    renameButton->setEnabled(false);
    eraseButton->setEnabled(false);
    copyButton->setEnabled(true);
    pasteButton->setEnabled(false);
    optionButton->setEnabled(false);
  }
}

void QuickAdmin::updateWidgets()
{
  menutree->clear();
  QTreeWidgetItem* active = nullptr;

  const int n = menus.size();
  for (int i = 0; i < n; i++) {
    const quickMenu& qm = menus[i];
    QString mname = QString::fromStdString(qm.name);
    QuickTreeWidgetItem* pp = new QuickTreeWidgetItem(menutree, QStringList(mname), i, -1);
    if (active_list == i && active_item == -1)
      active = pp;
    const int m = qm.menuitems.size();
    for (int j = 0; j < m; j++) {
      const quickMenuItem& qmi = qm.menuitems[j];
      QString qstr = QString::fromStdString(qmi.name);
      qstr.replace(QRegExp("</*font[^>]*>"), "");
      QTreeWidgetItem *tmp = new QuickTreeWidgetItem(pp, QStringList(qstr), i, j);
      if (active_list == i && active_item == j)
        active = tmp;
    }
  }

  if (active) {
    active->setSelected(true);
    menutree->scrollToItem(active, QAbstractItemView::EnsureVisible);
    selectionChanged( active, 0 );
  } else {
    comedit->setEnabled(false);
  }
}

const std::vector<quickMenu>& QuickAdmin::getMenus() const
{
  return menus;
}

void QuickAdmin::helpClicked()
{
  Q_EMIT showsource("quickadmin", "");
}

void QuickAdmin::upClicked()
{
  if (active_item != -1) {
    quickMenu& q = menus[active_list];
    std::swap(q.item(active_item - 1), q.item(active_item));
    active_item--;
  }
  updateWidgets();
}

void QuickAdmin::downClicked()
{
  if (active_item != -1) {
    quickMenu& q = menus[active_list];
    std::swap(q.item(active_item + 1), q.item(active_item));
    active_item++;
  }
  updateWidgets();
}

static void initQmName(quickMenu& qm)
{
  miutil::trim(qm.name);
  // TODO check that name is not empty now
  miutil::replace(qm.name, ",", " ");
  qm.filename = qm.name + ".quick";
  miutil::replace(qm.filename, "..", "__");
  diutil::replace_chars(qm.filename, " \t\n\r,;:/\\(){}[]$`'\"*+?~&%#!|><", '_');
  qm.filename = LocalSetupParser::basicValue("homedir") + "/" + qm.filename;
}

void QuickAdmin::newClicked()
{
  bool ok = false;
  if (active_item == -1) {
    QString text = QInputDialog::getText(this, tr("Make new menu"),
        tr("Make new menu with name:"), QLineEdit::Normal, QString::null, &ok);
    if (ok && !text.trimmed().isEmpty()) {
      quickMenu tmp;
      tmp.type = quickMenu::QM_USER;
      tmp.name = text.toStdString();
      initQmName(tmp);
      tmp.item_index = 0;

      menus.insert(menus.begin() + active_list + 1, tmp);
    }

  } else {
    QString text = QInputDialog::getText(this, tr("Make new plot"),
        tr("Make new plot with name:"), QLineEdit::Normal, QString::null, &ok);
    if (ok && !text.isEmpty()) {
      quickMenuItem tmp;
      tmp.name = text.toStdString();
      menus[active_list].menuitems.insert(menus[active_list].menuitems.begin() + active_item, tmp);
    }
  }
  if (ok)
    updateWidgets();
}

void QuickAdmin::newfileClicked()
{
  QString filter = tr("Menus (*.quick);;All (*.*)");
  QString s(QFileDialog::getOpenFileName(this, tr("Add new menu from file"), "./", filter));
  if (s.isEmpty())
    return;

  quickMenu tmp;
  tmp.type = quickMenu::QM_USER;
  tmp.filename = s.toStdString();
  tmp.item_index = 0;
  if (tmp.read()) {
    initQmName(tmp);
    menus.insert(menus.begin() + active_list + 1, tmp);
  }
  updateWidgets();
}

void QuickAdmin::renameClicked()
{
  bool ok = false;
  if (active_item == -1) {
    QString text =
        QInputDialog::getText(this, tr("Change menu name"), tr("New name:"), QLineEdit::Normal, QString::fromStdString(menus[active_list].name), &ok);
    text = text.trimmed();
    if (ok && !text.isEmpty())
      menus[active_list].name = text.toStdString();
    initQmName(menus[active_list]);

  } else {
    QString text = QInputDialog::getText(this, tr("Change plot name"), tr("New name:"), QLineEdit::Normal,
                                         QString::fromStdString(menus[active_list].menuitems[active_item].name), &ok);
    text = text.trimmed();
    if (ok && !text.isEmpty())
      menus[active_list].menuitems[active_item].name = text.toStdString();
  }
  if (ok)
    updateWidgets();
}

void QuickAdmin::eraseClicked()
{
  copyClicked();
  if (active_item == -1) {
    // remove file
    QFile qmfile(QString::fromStdString(menus[active_list].filename));
    if (!qmfile.remove()) {
      METLIBS_LOG_INFO("Could not remove file: '" << menus[active_list].filename << "'");
    }
    menus.erase(menus.begin() + active_list);

  } else {
    menus[active_list].menuitems.erase(menus[active_list].menuitems.begin() + active_item);
    if (active_item == int(menus[active_list].menuitems.size())) {
      if (active_item == 0)
        active_item = -1;
      else
        active_item--;
    }
  }
  updateWidgets();
}

void QuickAdmin::copyClicked()
{
  copyMenu = active_list;
  copyElement = active_item;

  if (copyElement == -1) {
    MenuCopy = menus[copyMenu];
  } else {
    MenuItemCopy = menus[copyMenu].menuitems[copyElement];
  }

  updateButtonTexts();

  if (menus[active_list].type == quickMenu::QM_USER || (active_list == 0 && active_item == -1))
    pasteButton->setEnabled(true);
}

void QuickAdmin::pasteClicked()
{
  if (copyElement == -1) {
    METLIBS_LOG_DEBUG("Paste menu:" << copyMenu << " after menu:" << active_list);
    quickMenu tmp = MenuCopy;
    tmp.type = quickMenu::QM_USER;
    tmp.name += "2";
    initQmName(tmp);
    tmp.item_index = 0;
    menus.insert(menus.begin() + active_list + 1, tmp);
    active_list++;
    active_item = -1; // move to start of menu
  } else {
    quickMenuItem tmp = MenuItemCopy;
    int pos = (active_item != -1 ? active_item + 1 : 0);
    menus[active_list].menuitems.insert(menus[active_list].menuitems.begin() + pos, tmp);
    active_item = pos;
  }
  copyMenu = -1;
  copyElement = -1;

  pasteButton->setEnabled(false);
  updateButtonTexts();
  updateWidgets();
}

void QuickAdmin::updateCommand()
{
  autochange = true;
  QString ts;
  bool enable = false;
  if (active_list >= 0 && active_item >= 0) {
    const quickMenu& q = menus[active_list];
    enable = (q.type != quickMenu::QM_SHARED);
    for (const std::string& c : q.menuitems[active_item].command)
      ts += QString::fromStdString(c) + "\n";
  }
  comedit->setEnabled(enable);
  comedit->setText(ts);
  autochange = false;
}

void QuickAdmin::comChanged()
{
  if (autochange)
    return;
  if (active_item < 0 || active_list < 0)
    return;
  std::vector<std::string> s;
  std::string str = comedit->toPlainText().toStdString();
  if (!str.empty())
    s.push_back(str);
  menus[active_list].menuitems[active_item].command = s;
}

void QuickAdmin::optionClicked()
{
  std::unique_ptr<QuickEditOptions> qeo(new QuickEditOptions(this, menus[active_list].opt));
  if (qeo->exec()) {
    menus[active_list].opt = qeo->getOptions();
  }
}
