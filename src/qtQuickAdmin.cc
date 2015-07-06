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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtQuickAdmin.h"
#include "qtQuickEditOptions.h"
#include "qtUtility.h"
#include "diUtilities.h"

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

using namespace std;

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

QuickAdmin::QuickAdmin(QWidget* parent, vector<quickMenu>& qm, int fc, int lc) :
  QDialog(parent), menus(qm), autochange(true),  activeMenu(-1),
  activeElement(-1), copyMenu(-1), copyElement(-1),firstcustom(fc), lastcustom(lc)
{
  setModal(true);
  QFont m_font = QFont("Helvetica", 12, 75);

  QLabel* mainlabel = new QLabel("<em><b>" + tr("Edit quickmenus") + "</b></em>", this);
  mainlabel->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

  menutree = new QTreeWidget(this);
  menutree->setRootIsDecorated(true);
  menutree->setSortingEnabled(false);
  menutree->setColumnCount(1);

  connect(menutree, SIGNAL(itemClicked(QTreeWidgetItem * ,int)),
      this, SLOT(selectionChanged(QTreeWidgetItem * ,int)));

  // up
  QPixmap upPicture(up12x12_xpm);
  upButton = PixmapButton(upPicture, this, 14, 12);
  upButton->setEnabled(false);
  //  upButton->setAccel(Qt::CTRL+Qt::Key_Up);
  QShortcut* upButtonShortcut = new QShortcut(Qt::CTRL + Qt::Key_Up, this);
  connect(upButtonShortcut, SIGNAL(activated()), SLOT(upClicked()));
  connect(upButton, SIGNAL(clicked()), SLOT(upClicked()));
  // down
  QPixmap downPicture(down12x12_xpm);
  downButton = PixmapButton(downPicture, this, 14, 12);
  downButton->setEnabled(false);
  //  downButton->setAccel(Qt::CTRL+Qt::Key_Down);
  QShortcut* downButtonShortcut = new QShortcut(Qt::CTRL + Qt::Key_Down, this);
  connect(downButtonShortcut, SIGNAL(activated()), SLOT(downClicked()));
  connect(downButton, SIGNAL(clicked()), SLOT(downClicked()));

  // Command buttons for menu-elements
  // new
  newButton = new QPushButton(QPixmap(filenew_xpm), tr("&New"), this);
  newButton->setEnabled(false);
  connect(newButton, SIGNAL(clicked()), SLOT(newClicked()));

  // new from file
  newfileButton = new QPushButton(QPixmap(fileopen_xpm),
      tr("New from &file.."), this);
  newfileButton->setEnabled(false);
  connect(newfileButton, SIGNAL(clicked()), SLOT(newfileClicked()));

  // rename
  renameButton = new QPushButton(tr("&Change name.."), this);
  renameButton->setEnabled(false);
  connect(renameButton, SIGNAL(clicked()), SLOT(renameClicked()));

  // erase
  eraseButton = new QPushButton(QPixmap(editcut_xpm), tr("Remove"), this);
  eraseButton->setEnabled(false);
  //  eraseButton->setAccel(Qt::CTRL+Qt::Key_X);
  QShortcut* eraseButtonShortcut = new QShortcut(Qt::CTRL + Qt::Key_X, this);
  connect(eraseButtonShortcut, SIGNAL(activated()), SLOT(eraseClicked()));
  connect(eraseButton, SIGNAL(clicked()), SLOT(eraseClicked()));

  // copy
  copyButton = new QPushButton(QPixmap(editcopy_xpm), tr("Copy"), this);
  copyButton->setEnabled(false);
  //  copyButton->setAccel(Qt::CTRL+Qt::Key_C);
  QShortcut* copyButtonShortcut = new QShortcut(Qt::CTRL + Qt::Key_C, this);
  connect(copyButtonShortcut, SIGNAL(activated()), SLOT(upClicked()));
  connect(copyButton, SIGNAL(clicked()), SLOT(copyClicked()));

  // paste
  pasteButton = new QPushButton(QPixmap(editpaste_xpm), tr("Paste"), this);
  pasteButton->setEnabled(false);
  //  pasteButton->setAccel(Qt::CTRL+Qt::Key_V);
  QShortcut* pasteButtonShortcut = new QShortcut(Qt::CTRL + Qt::Key_V, this);
  connect(pasteButtonShortcut, SIGNAL(activated()), SLOT(pasteClicked()));
  connect(pasteButton, SIGNAL(clicked()), SLOT(pasteClicked()));

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

  vlayout->addWidget(mainlabel);
  vlayout->addLayout(hl1);
  vlayout->addLayout(gl);
  vlayout->addWidget(line);
  vlayout->addWidget(comlabel);
  vlayout->addWidget(comedit);
  vlayout->addLayout(hl);
  vlayout->addWidget(line2);
  vlayout->addLayout(hl4);

  vlayout->activate();

  updateWidgets();
  resize(500, 550);
  }

void QuickAdmin::selectionChanged(QTreeWidgetItem *p, int i)
{
  if (p) {
    QuickTreeWidgetItem* qp = (QuickTreeWidgetItem*) (p);
    activeMenu = qp->Menu();
    activeElement = qp->Item();

    if (activeElement == -1) {
      newButton->setText(tr("&New menu.."));
      copyButton->setText(tr("Copy menu"));
      eraseButton->setText(tr("Remove menu.."));
    } else {
      newButton->setText(tr("&New plot.."));
      copyButton->setText(tr("Copy plot"));
      eraseButton->setText(tr("Remove plot"));
    }
    updateCommand();

    int nitems = menus[activeMenu].menuitems.size();

    if (activeMenu >= firstcustom && activeMenu <= lastcustom) {
      // up
      upButton->setEnabled((activeElement > 0)
          || (activeElement == -1 && activeMenu > firstcustom));
      // down
      downButton->setEnabled((activeElement >= 0 && activeElement < nitems - 1)
          || (activeElement == -1 && activeMenu < lastcustom));
      // new, erase, copy, rename
      newButton->setEnabled(true);
      newfileButton->setEnabled(true);
      renameButton->setEnabled(true);
      eraseButton->setEnabled(true);
      copyButton->setEnabled(true);
      // paste
      if (copyMenu != -1)
        pasteButton->setEnabled(true);
      else
        pasteButton->setEnabled(false);
      // edit options
      optionButton->setEnabled(true);

    } else if (activeMenu == 0) { // history
      // up
      upButton->setEnabled(activeElement > 0);
      // down
      downButton->setEnabled(activeElement >= 0 && activeElement < nitems - 1);
      // new
      newButton->setEnabled(activeElement == -1);
      // new from file
      newfileButton->setEnabled(true);
      // rename
      renameButton->setEnabled(false);
      // erase
      if (activeElement != -1)
        eraseButton->setEnabled(true);
      else
        eraseButton->setEnabled(false);
      // copy
      copyButton->setEnabled(true);
      // paste
      pasteButton->setEnabled(copyMenu != -1 && activeElement == -1);
      // edit options
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
      // edit options
      optionButton->setEnabled(false);
    }
  }
}

void QuickAdmin::updateWidgets()
{

  menutree->clear();

  int n = menus.size();

  QTreeWidgetItem *active = 0;

  for (int i = 0; i < n; i++) {
    QString mname(menus[i].name.c_str());
    QuickTreeWidgetItem* pp = new QuickTreeWidgetItem(menutree, QStringList(mname), i, -1);
    if (activeMenu == i && activeElement == -1)
      active = pp;
    int m = menus[i].menuitems.size();
    for (int j = 0; j < m; j++) {
      QString qstr = QString::fromStdString(menus[i].menuitems[j].name);
      qstr.replace(QRegExp("</*font[^>]*>"), "");
      QTreeWidgetItem *tmp = new QuickTreeWidgetItem(pp, QStringList(qstr), i, j);
      if (activeMenu == i && activeElement == j)
        active = tmp;
    }
  }

  if (active) {
    active->setSelected(true);
    menutree->scrollToItem(active, QAbstractItemView::EnsureVisible);
    selectionChanged( active, 0 );
  }
}

vector<quickMenu> QuickAdmin::getMenus() const
{
  return menus;
}

void QuickAdmin::helpClicked()
{
  emit help("quickadmin");
}

void QuickAdmin::upClicked()
{
  if (activeElement == -1) {
    std::swap(menus[activeMenu - 1], menus[activeMenu]);
    activeMenu--;
  } else {
    std::swap(menus[activeMenu].menuitems[activeElement - 1],
        menus[activeMenu].menuitems[activeElement]);
    activeElement--;
  }
  updateWidgets();
}

void QuickAdmin::downClicked()
{
  if (activeElement == -1) {
    std::swap(menus[activeMenu + 1], menus[activeMenu]);
    activeMenu++;
  } else {
    std::swap(menus[activeMenu].menuitems[activeElement + 1],
        menus[activeMenu].menuitems[activeElement]);
    activeElement++;
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
}

void QuickAdmin::newClicked()
{
  bool ok = false;
  if (activeElement == -1) {
    QString text = QInputDialog::getText(this, tr("Make new menu"),
        tr("Make new menu with name:"), QLineEdit::Normal, QString::null, &ok);
    if (ok && !text.trimmed().isEmpty()) {
      quickMenu tmp;
      tmp.name = text.toStdString();
      initQmName(tmp);
      tmp.plotindex = 0;

      menus.insert(menus.begin() + activeMenu + 1, tmp);
      if (firstcustom < 0) {
        firstcustom = 1;
        lastcustom = 0;
      }
      lastcustom++;
    }

  } else {
    QString text = QInputDialog::getText(this, tr("Make new plot"),
        tr("Make new plot with name:"), QLineEdit::Normal, QString::null, &ok);
    if (ok && !text.isEmpty()) {
      quickMenuItem tmp;
      tmp.name = text.toStdString();
      menus[activeMenu].menuitems.insert(menus[activeMenu].menuitems.begin()
          + activeElement, tmp);
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
  tmp.filename = s.toStdString();
  tmp.plotindex = 0;
  if (readQuickMenu(tmp)) {
    initQmName(tmp);

    menus.insert(menus.begin() + activeMenu + 1, tmp);
    if (firstcustom < 0) {
      firstcustom = 1;
      lastcustom = 0;
    }
    lastcustom++;
  }
  updateWidgets();
}

void QuickAdmin::renameClicked()
{
  bool ok = false;
  if (activeElement == -1) {
    QString text = QInputDialog::getText(this, tr("Change menu name"), tr("New name:"),
        QLineEdit::Normal, QString::fromStdString(menus[activeMenu].name), &ok);
    text = text.trimmed();
    if (ok && !text.isEmpty())
      menus[activeMenu].name = text.toStdString();
    initQmName(menus[activeMenu]);

  } else {
    QString text = QInputDialog::getText(this, tr("Change plot name"), tr("New name:"),
        QLineEdit::Normal, QString::fromStdString(menus[activeMenu].menuitems[activeElement].name), &ok);
    text = text.trimmed();
    if (ok && !text.isEmpty())
      menus[activeMenu].menuitems[activeElement].name = text.toStdString();
  }
  if (ok)
    updateWidgets();
}

void QuickAdmin::eraseClicked()
{
  copyClicked();
  if (activeElement == -1) {
    // remove file
    QFile qmfile(QString::fromStdString(menus[activeMenu].filename));
    if (!qmfile.remove()) {
      METLIBS_LOG_INFO("Could not remove file: '" << menus[activeMenu].filename << "'");
    }
    menus.erase(menus.begin() + activeMenu);
    lastcustom--;
    if (lastcustom < firstcustom) {
      firstcustom = -1;
    }

  } else {
    menus[activeMenu].menuitems.erase(menus[activeMenu].menuitems.begin()
        + activeElement);
    if (activeElement == int(menus[activeMenu].menuitems.size())) {
      if (activeElement == 0)
        activeElement = -1;
      else
        activeElement--;
    }
  }
  updateWidgets();
}

void QuickAdmin::copyClicked()
{

  copyMenu = activeMenu;
  copyElement = activeElement;

  if (copyElement == -1) {
    //     METLIBS_LOG_DEBUG("Copy menu:" << copyMenu);
    MenuCopy = menus[copyMenu];
  } else {
    //     METLIBS_LOG_DEBUG("Copy item:" << copyElement
    // 	 << " from menu:" << copyMenu);
    MenuItemCopy = menus[copyMenu].menuitems[copyElement];
  }

  if (copyElement == -1)
    pasteButton->setText(tr("Paste menu"));
  else
    pasteButton->setText(tr("Paste plot"));

  //  pasteButton->setAccel(Qt::CTRL+Qt::Key_V);

  if ((activeMenu >= firstcustom && activeMenu <= lastcustom) || (activeMenu
      == 0 && activeElement == -1))
    pasteButton->setEnabled(true);
}

void QuickAdmin::pasteClicked()
{
  if (copyElement == -1) {
    //     METLIBS_LOG_DEBUG("Paste menu:" << copyMenu << " after menu:" << activeMenu);
    quickMenu tmp = MenuCopy;
    miutil::trim(tmp.name);
    miutil::replace(tmp.name, ",", " ");
    tmp.filename = tmp.name + "2.quick";
    miutil::replace(tmp.filename, " ", "_");
    tmp.plotindex = 0;

    menus.insert(menus.begin() + activeMenu + 1, tmp);
    if (firstcustom < 0) {
      firstcustom = 1;
      lastcustom = 0;
    }
    lastcustom++;
    activeMenu++;
    activeElement = -1; // move to start of menu
  } else {
    //     METLIBS_LOG_DEBUG("Paste item:" << copyElement
    //     	 << " from menu:" << copyMenu
    //      	 << " to item:" << activeElement << ", menu:" << activeMenu);
    quickMenuItem tmp = MenuItemCopy;
    int pos = (activeElement != -1 ? activeElement + 1 : 0);
    menus[activeMenu].menuitems.insert(menus[activeMenu].menuitems.begin()
        + pos, tmp);
    activeElement = pos;
  }
  copyMenu = -1;
  copyElement = -1;

  pasteButton->setText(tr("Paste"));
  pasteButton->setEnabled(false);
  //   pasteButton->setAccel(Qt::CTRL+Qt::Key_V);
  updateWidgets();
}

void QuickAdmin::updateCommand()
{
  autochange = true;
  if (activeMenu >= 0 && activeElement >= 0) {
    std::string ts;
    int n = menus[activeMenu].menuitems[activeElement].command.size();
    for (int i = 0; i < n; i++) {
      ts += menus[activeMenu].menuitems[activeElement].command[i];
      ts += "\n";
    }
    // set command into command-edit
    comedit->setText(ts.c_str());
  } else
    comedit->clear();
  autochange = false;
}

void QuickAdmin::comChanged()
{
  if (autochange)
    return;
  if (activeElement < 0 || activeMenu < 0)
    return;
  //   int ni= comedit->numLines();
  //   int ni= comedit->paragraphs();
  //   vector<std::string> s;
  //   for (int i=0; i<ni; i++){
  // //     std::string str= comedit->textLine(i).toStdString();
  //     std::string str= comedit->text(i).toStdString();
  //     miutil::trim(str);
  //     if (miutil::contains(str, "\n"))
  //       str.erase(str.end()-1);
  //     if ((not str.empty())) s.push_back(str);
  //   }

  //Qt4
  vector<string> s;
  string str = comedit->toPlainText().toStdString();
  if (not str.empty())
    s.push_back(str);
  menus[activeMenu].menuitems[activeElement].command = s;
}

void QuickAdmin::optionClicked()
{
  QuickEditOptions* qeo = new QuickEditOptions(this, menus[activeMenu].opt);
  //connect(qeo, SIGNAL(help(const char*)), this, SIGNAL(help(const char*)));
  if (qeo->exec()) {
    menus[activeMenu].opt = qeo->getOptions();
  }
}
