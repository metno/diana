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

#include "qtQuickMenu.h"

#include "diLocalSetupParser.h"
#include "diPlotCommandFactory.h"
#include "diUtilities.h"
#include "qtMainWindow.h"
#include "qtQuickAdmin.h"
#include "qtTreeFilterProxyModel.h"
#include "qtUtility.h"
#include "util/string_util.h"

#include <puTools/miStringFunctions.h>

#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegExp>
#include <QSignalMapper>
#include <QSpinBox>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QTimerEvent>
#include <QToolButton>
#include <QToolTip>
#include <QTreeView>
#include <QVBoxLayout>

#include "minus12x12.xpm"

#define MILOGGER_CATEGORY "diana.QuickMenu"
#include <miLogger/miLogging.h>


// ========================================================================

QuickMenu::QuickMenu(QWidget* parent, const std::vector<QuickMenuDefs>& qdefs)
    : QDialog(parent)
    , timerinterval(10)
    , timeron(false)
    , updating_options(false)
    , selected_list(0)
    , plotted_list(HISTORY_MAP)
    , plotted_item(0)
    , old_list(-1)
    , old_item(-1)
{
  setWindowTitle(tr("Quickmenu"));

  connect(parent, SIGNAL(instanceNameChanged(QString)),
      SLOT(onInstanceNameChanged(QString)));

  QBoxLayout* vlayout = new QVBoxLayout(this);

  QLabel* menulistlabel = TitleLabel(tr("&Menus"), this);
  menubox = new QTreeView(this);
  menubox->setHeaderHidden(true);
  menubox->setSelectionMode(QAbstractItemView::SingleSelection);
  menubox->setSelectionBehavior(QAbstractItemView::SelectRows);
  menuItems = new QStandardItemModel(this);
  menuFilter = new TreeFilterProxyModel(this);
  menuFilter->setDynamicSortFilter(true);
  menuFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
  menuFilter->setSourceModel(menuItems);
  menubox->setModel(menuFilter);
  connect(menubox->selectionModel(), &QItemSelectionModel::selectionChanged, this, &QuickMenu::menuboxSelectionChanged);
  connect(menubox, &QTreeView::activated, this, &QuickMenu::menuboxItemActivated);
  menuFilterEdit = new QLineEdit("", this);
  menuFilterEdit->setPlaceholderText(tr("Type to filter menu names"));
  connect(menuFilterEdit, &QLineEdit::textChanged, this, &QuickMenu::filterMenus);
  menulistlabel->setBuddy(menuFilterEdit);

  QToolButton* collapseAll = new QToolButton(this);
  collapseAll->setIcon(QIcon(QPixmap(minus12x12_xpm)));
  collapseAll->setToolTip(tr("Collapse all"));
  connect(collapseAll, &QToolButton::clicked, menubox, &QTreeView::collapseAll);

  QPushButton* adminbut = new QPushButton(tr("&Edit menus.."), this);
  adminbut->setToolTip(tr("Menu editor: Copy, change name and sortorder etc. on your own menus") );
  connect(adminbut, &QPushButton::clicked, this, &QuickMenu::adminButton);

  updatebut = new QPushButton(tr("&Update.."), this);
  updatebut->setToolTip(tr("Update command with current plot") );
  connect(updatebut, &QPushButton::clicked, this, &QuickMenu::updateButton);

  QBoxLayout* l2 = new QHBoxLayout();
  l2->addWidget(menulistlabel);
  l2->addWidget(menuFilterEdit, 1);
  l2->addWidget(collapseAll);
  l2->addWidget(updatebut);
  l2->addWidget(adminbut);

  // create menuitem list
  vlayout->addLayout(l2);
  vlayout->addWidget(menubox, 1);

  // Create variables/options layout manager
  QSignalMapper* mapper = new QSignalMapper(this);
  connect(mapper, SIGNAL(mapped(int)), this, SLOT(optionChanged(int)));
  QGridLayout* varlayout = new QGridLayout();
  for (int i = 0; i < maxoptions; i++) {
    optionlabel[i] = new QLabel("", this);
    optionmenu[i] = new QComboBox(this);
    optionmenu[i]->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    const int row = i / 4;
    const int col = 2 * (i % 4);
    mapper->setMapping(optionmenu[i], i);
    connect(optionmenu[i], SIGNAL(currentIndexChanged(int)), mapper, SLOT(map()));
    varlayout->addWidget(optionlabel[i], row, col, Qt::AlignRight);
    varlayout->addWidget(optionmenu[i], row, col + 1);
  }
  vlayout->addLayout(varlayout);

  // create commands-area
  comedit = new QTextEdit(this);
  comedit->setLineWrapMode(QTextEdit::NoWrap);
  comedit->setFont(QFont("Courier",12,QFont::Normal));
  comedit->setReadOnly(false);
  comedit->setMinimumHeight(150);
  connect(comedit, &QTextEdit::textChanged, this, &QuickMenu::comChanged);
  comlabel = new QLabel(tr("Command field"), this);
  comlabel->setMinimumSize(comlabel->sizeHint());
  comlabel->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

  comlabel->hide();
  comedit->hide();

  QHBoxLayout* l3= new QHBoxLayout();
  l3->addWidget(comlabel);
  l3->addStretch();

  vlayout->addLayout(l3,0);
  vlayout->addWidget(comedit, 1);

  // buttons and stuff at the bottom

  QPushButton* quickhide = new QPushButton(tr("&Hide"), this);
  connect(quickhide, &QPushButton::clicked, this, &QuickMenu::QuickHide);

  QPushButton* combut = new QPushButton(tr("&Command"), this);
  combut->setCheckable(true);
  connect(combut, &QPushButton::toggled, this, &QuickMenu::comButton);

  QPushButton* demobut = new QPushButton(tr("&Demo"), this);
  demobut->setCheckable(true);
  connect(demobut, &QPushButton::toggled, this, &QuickMenu::demoButton);

  QSpinBox* interval = new QSpinBox(this);
  interval->setMinimum(2);
  interval->setMaximum(360);
  interval->setSingleStep(2);
  interval->setValue(timerinterval);
  interval->setSuffix(" sec");
  connect(interval, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &QuickMenu::intervalChanged);

  QPushButton* qhelp = new QPushButton(tr("&Help"), this);
  connect(qhelp, &QPushButton::clicked, this, &QuickMenu::helpClicked);

  QPushButton* plothidebut = new QPushButton(tr("Apply+Hide"), this);
  connect(plothidebut, &QPushButton::clicked, this, &QuickMenu::plotActiveMenu);
  connect(plothidebut, &QPushButton::clicked, this, &QuickMenu::QuickHide);

  QPushButton* plotbut = new QPushButton(tr("&Apply"), this);
  plotbut->setDefault( true );
  connect(plotbut, &QPushButton::clicked, this, &QuickMenu::plotActiveMenu);

  QBoxLayout* l = new QHBoxLayout();
  l->addWidget(quickhide);
  l->addWidget(combut);
  l->addWidget(demobut);
  l->addWidget(interval);
  l->addWidget(qhelp);
  l->addWidget(plothidebut);
  l->addWidget(plotbut);
  vlayout->addLayout(l);

  fillHistoryMenus();
  fillPrivateMenus();
  fillStaticMenus(qdefs);
}

void QuickMenu::start()
{
  fillMenuList(false);
}

std::vector<std::string> QuickMenu::getUserMenus()
{
  std::vector<std::string> vs;
  for (const quickMenu& q : qm)
    if (q.type == quickMenu::QM_USER)
      vs.push_back(q.name);
  return vs;
}

void QuickMenu::addMenu(const std::string& name)
{
  quickMenu qtmp;
  qtmp.type = quickMenu::QM_USER;
  qtmp.name = name;
  miutil::trim(qtmp.name);
  qtmp.filename = LocalSetupParser::basicValue("homedir") + "/";
  qtmp.filename += qtmp.name + ".quick";
  qtmp.item_index = 0;
  qtmp.write();

  std::vector<quickMenu>::iterator itq = qm.begin();
  while (itq != qm.end() && itq->is_history())
    ++itq;
  itq = qm.insert(itq, qtmp);

  // update indexes
  const int insert = itq - qm.begin();
  if (plotted_list >= insert)
    plotted_list++;
  if (selected_list >= insert)
    selected_list++;

  fillMenuList();
}

bool QuickMenu::addToMenu(const std::string& name)
{
  if (!(isValidList(plotted_list) && qm[plotted_list].valid_item(plotted_item)))
    return false;
  for (quickMenu& q : qm) {
    if (q.type == quickMenu::QM_USER && q.name == name) {
      q.menuitems.push_back(qm[plotted_list].menuitems[plotted_item]);
      q.write();
      fillMenuList();
      return true;
    }
  }
  return false;
}

std::string QuickMenu::getCurrentName()
{
  if (isValidList(plotted_list)) {
    const quickMenu& qml = qm[plotted_list];
    if (qml.valid_item(plotted_item))
      return qml.menuitems[plotted_item].name;
  }
  return std::string();
}

void QuickMenu::addPlotToHistory(const std::string& name, const PlotCommand_cpv& commands, int list)
{
  METLIBS_LOG_SCOPE();
  if (!isValidList(list))
    return;

  std::vector<std::string> cmds;
  cmds.reserve(commands.size());
  for (PlotCommand_cp cmd : commands)
    cmds.push_back(cmd->toString());

  for (std::string& c : cmds)
    while (miutil::contains(c, "reftime"))
      c.erase(c.find("reftime="), 28);

  quickMenu& q = qm[list];
  std::deque<quickMenuItem>& qi = q.menuitems;

  // nothing to do if this is the same as the first (most recent) history item
  const bool same_as_first = !qi.empty() && (cmds == qi[0].command);
  if (!same_as_first) {
    // remove any duplicates
    std::deque<quickMenuItem>::iterator it = qi.begin();
    while (it != qi.end()) {
      const bool drop = it->command.empty() || (cmds == it->command);
      if (drop)
        it = qi.erase(it);
      else
        ++it;
    }

    // keep stack within bounds
    while (qi.size() >= maxplotsinstack)
      qi.pop_back();

    // update pointer
    q.item_index = 0;
    // push plot on stack
    qi.push_front(quickMenuItem());
    qi[0].name = miutil::trimmed(name);
    qi[0].command = cmds;

    fillMenuList();

    // switch to history-menu
    if (selected_list == list)
      selectList(list);

    plotted_list = list;
    plotted_item = 0;
  }
}

void QuickMenu::selectList(int i)
{
  METLIBS_LOG_SCOPE(LOGVAL(i));
  if (isValidList(i)) {
    selected_list = i;

    int parentRow = selected_list;
    int childRow = qm[selected_list].item_index;
    QModelIndex parent = menuItems->index(parentRow, 0);
    QModelIndex child = menuItems->index(childRow, 0, parent);
    menubox->setCurrentIndex(menuFilter->mapFromSource(child));
  }
  updateOptions();
  setCommand();
}

// called from quick-quick menu (Browsing)
bool QuickMenu::prevQPlot()
{
  return stepQPlot(+1);
}

// called from quick-quick menu (Browsing)
bool QuickMenu::nextQPlot()
{
  return stepQPlot(-1);
}

bool QuickMenu::isValidList(int list) const
{
  return (!qm.empty() && list >= 0 && list < int(qm.size()));
}

// called from quick-quick menu (Browsing)
bool QuickMenu::stepQPlot(int delta)
{
  METLIBS_LOG_SCOPE();
  if (!isValidList(selected_list))
    return false;

  quickMenu& q = qm[selected_list];
  if (!q.step_item(delta))
    return false;

  selectList(selected_list);
  return true;
}

// Go to previous History-plot
bool QuickMenu::prevHPlot(int list)
{
  return stepHistoryPlot(list, +1);
}

// Go to next History-plot
bool QuickMenu::nextHPlot(int list)
{
  return stepHistoryPlot(list, -1);
}

// Go to next History-plot
bool QuickMenu::stepHistoryPlot(int list, int delta)
{
  METLIBS_LOG_SCOPE();
  if (!isValidList(list))
    return false;

  quickMenu& q = qm[list];
  // if in History-menu or last plot from History:
  // Change plotindex and plot
  if (selected_list == list || plotted_list == list) {
    if (!q.step_item(delta))
      return false;
    if (selected_list == list)
      selectList(list);
  } else {
    // Jumping back to History (plotindex intact)
    if (!q.valid_item())
      return false;
  }

  Q_EMIT Apply(makeCommands(q.command()), true);
  plotted_list = list;
  plotted_item = q.item_index;

  return true;
}

// For browsing: go to previous quick-menu
bool QuickMenu::prevList()
{
  if (!isValidList(selected_list - 1))
    return false;
  selectList(selected_list - 1);
  return true;
}

// For browsing: go to next quick-menu
bool QuickMenu::nextList()
{
  if (!isValidList(selected_list + 1))
    return false;
  selectList(selected_list + 1);
  return true;
}

// for Browsing: get menu-details
void QuickMenu::getDetails(int& item_index, std::string& listname, std::string& itemname)
{
  if (isValidList(selected_list)) {
    quickMenu& q = qm[selected_list];
    item_index = q.item_index;
    listname = q.name;
    itemname = (q.valid_item() ? q.menuitems[item_index].name : "");
  } else {
    item_index = -1;
  }
}

bool QuickMenu::applyItem(const std::string& mlist, const std::string& item)
{
  //find list index
  int n=qm.size();
  int listIndex=0;
  while(listIndex<n && qm[listIndex].name != mlist){
    listIndex++;
  }
  if( listIndex==n ) {
    METLIBS_LOG_ERROR("list not found");
    return false;
  }

  //find item index
  int m=qm[listIndex].menuitems.size();
  int itemIndex=0;
  METLIBS_LOG_INFO(item);
  while(itemIndex<m && qm[listIndex].menuitems[itemIndex].name != item){
    METLIBS_LOG_INFO(qm[listIndex].menuitems[itemIndex].name);
    itemIndex++;
  }
  if( itemIndex==m  ) {
    METLIBS_LOG_ERROR("item not found");
    return false;
  }

  //set menu
  qm[listIndex].item_index = itemIndex;
  selectList(listIndex);
  return true;
}

void QuickMenu::applyPlot()
{
  METLIBS_LOG_SCOPE();
  plotActiveMenu();
}

void QuickMenu::adminButton()
{
  std::unique_ptr<QuickAdmin> admin(new QuickAdmin(this, qm));
  connect(admin.get(), &QuickAdmin::showsource, this, &QuickMenu::showsource);
  if (admin->exec()) {
    // get updated list of menus
    qm = admin->getMenus();

    old_list = old_item = -1;

    if (!isValidList(plotted_list)) { // previous plot now bad
      plotted_list = -1;
      plotted_item = -1;
    }
    fillMenuList();

    // save custom/history quickmenus to file
    for (const quickMenu& q : qm) {
      if (q.type != quickMenu::QM_SHARED)
        q.write();
    }
  }
}

QString QuickMenu::instanceNameSuffix() const
{
  DianaMainWindow* mw = static_cast<DianaMainWindow*>(parent());
  QString instanceNameSuffix = mw->instanceNameSuffix();
  if (!instanceNameSuffix.isEmpty())
    return "-" + instanceNameSuffix;
  else
    return instanceNameSuffix;
}

void QuickMenu::onInstanceNameChanged(const QString&)
{
  const std::string ins = instanceNameSuffix().toStdString();
  qm[HISTORY_MAP].filename = LocalSetupParser::basicValue("homedir") + "/History" + ins + ".quick";
  qm[HISTORY_VCROSS].filename = LocalSetupParser::basicValue("homedir") + "/History-vcross" + ins + ".quick";
}

void QuickMenu::fillHistoryMenus()
{
  quickMenu qtmp;
  qtmp.type = quickMenu::QM_USER;

  const std::string ins = instanceNameSuffix().toStdString();
  const std::string& home = LocalSetupParser::basicValue("homedir");

  // history quick-menus
  qm.push_back(qtmp);
  qm[HISTORY_MAP].type = quickMenu::QM_HISTORY_MAIN;
  qm[HISTORY_MAP].name = tr("History").toStdString();
  qm[HISTORY_MAP].filename = home + "/History" + ins + ".quick";
  qm[HISTORY_MAP].item_index = 0;
  qm[HISTORY_MAP].read();

  qm.push_back(qtmp);
  qm[HISTORY_VCROSS].type = quickMenu::QM_HISTORY_VCROSS;
  qm[HISTORY_VCROSS].name = tr("History-vcross").toStdString();
  qm[HISTORY_VCROSS].filename = home + "/History-vcross" + ins + ".quick";
  qm[HISTORY_VCROSS].item_index = 0;
  qm[HISTORY_VCROSS].read();
}

void QuickMenu::fillPrivateMenus()
{
  quickMenu qtmp;
  qtmp.type = quickMenu::QM_USER;

  // user-defined menus
  const std::string quickfile = LocalSetupParser::basicValue("homedir") + "/*.quick";
  for (const std::string& qf : diutil::glob(quickfile)) {
    if (miutil::contains(qf, "History"))
      continue; // History quickmenu files should not be included
    qtmp.name = "";
    qtmp.filename = qf;
    qtmp.item_index = 0;
    if (qtmp.read()) {
      bool collision = false;
      for (const quickMenu& q : qm) {
        if (q.is_history() && q.name == qtmp.name)
          collision = true;
      }
      if (!collision) // avoid mix with History
        qm.push_back(qtmp);
    }
  }
}

void QuickMenu::fillStaticMenus(const std::vector<QuickMenuDefs>& qdefs)
{
  quickMenu qtmp;
  qtmp.type = quickMenu::QM_SHARED;
  for (const QuickMenuDefs& qdef : qdefs) {
    for (const std::string& quickfile : diutil::glob(qdef.filename)) {
      qtmp.name = "";
      qtmp.filename = quickfile;
      qtmp.item_index = 0;
      if (qtmp.read())
        qm.push_back(qtmp);
    }
  }
}

void QuickMenu::updateButton()
{
  if (!isValidList(plotted_list) || plotted_item < 0)
    return;
  quickMenu& p = qm[plotted_list];
  if (!p.valid_item(plotted_item))
    return;
  quickMenuItem& pi = p.item(plotted_item);

  quickMenu& q = qm[selected_list];
  if (!q.valid_item())
    return;
  quickMenuItem& qi = q.item();

  std::vector<std::string> vs = p.item(plotted_item).command;
  if (vs.empty())
    return;

  bool changename = false;

  if (q.type == quickMenu::QM_SHARED) {
    QString mess = "<b>" + tr("Do you want to replace the content of this menuitem with current plot?") + "</b><br>" +
                   tr("This is a static/official menuitem, which can be reset to default value.");

    QMessageBox mb("Diana", mess, QMessageBox::Information, QMessageBox::Yes | QMessageBox::Default, QMessageBox::Cancel | QMessageBox::Escape, Qt::NoButton);
    mb.setButtonText(QMessageBox::Yes, tr("Yes"));
    mb.setButtonText(QMessageBox::Cancel, tr("Cancel"));
    switch (mb.exec()) {
    case QMessageBox::Yes:
      // Yes
      break;
    case QMessageBox::Cancel:
      // cancel operation
      return;
      break;
    }
  } else {
    QString mess = "<b>" + tr("Do you want to replace the content of this menuitem with current plot?") + "</b><br>" +
                   tr("The menu name can be automatically created from the underlying data in the plot");

    QMessageBox mb("Diana", mess, QMessageBox::Information, QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
                   QMessageBox::Cancel | QMessageBox::Escape);
    mb.setButtonText(QMessageBox::Yes, tr("Yes, make new menu name"));
    mb.setButtonText(QMessageBox::No, tr("Yes, keep menu name"));
    mb.setButtonText(QMessageBox::Cancel, tr("Cancel"));
    switch (mb.exec()) {
    case QMessageBox::Yes:
      // Yes
      changename = true;
      break;
    case QMessageBox::No:
      // Yes, but keep the name
      changename = false;
      break;
    case QMessageBox::Cancel:
      // cancel operation
      return;
      break;
    }
  }

  if (q.type == quickMenu::QM_SHARED) {
    qi.command = vs;
  } else {
    replaceDynamicQuickMenuOptions(qi.command, vs);
    qi.command = vs;
    if (changename)
      qi.name = pi.name;

    if (q.type == quickMenu::QM_USER)
      q.write();
  }

  selectList(selected_list);
}

void QuickMenu::readLog(const std::vector<std::string>& vstr, const std::string&, const std::string&)
{
  readQuickMenuLog(qm, vstr);
}

std::vector<std::string> QuickMenu::writeLog()
{
  // save any changes to the command
  saveChanges(-1,-1);
  for (const quickMenu& q : qm) {
    if (q.is_history()) {
      // history menu, write to file
      q.write();
    }
  }

  return writeQuickMenuLog(qm);
}

void QuickMenu::fillMenuList(bool select)
{
  menuItems->clear();
  if (qm.empty())
    return;

  for (const auto& qmi : qm) {
    QStandardItem* group = new QStandardItem(QString::fromStdString(qmi.name));
    group->setToolTip(QString::fromStdString(qmi.filename));
    group->setFlags(Qt::ItemIsEnabled); // not selectable
    menuItems->appendRow(group);
    for (const auto& mi : qmi.menuitems) {
      QString name = QString::fromStdString(mi.name);
      name.replace(QRegExp("</*font[^>]*>"), "");
      QStandardItem* child = new QStandardItem(name);
      child->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      group->appendRow(child);
    }
  }
  menubox->collapseAll();

  if (selected_list >= int(qm.size()))
    selected_list = qm.size() - 1;
  if (select) {
    // set active menu
    selectList(selected_list);
  } else {
    updateOptions();
    setCommand();
  }
}

void QuickMenu::filterMenus(const QString& filtertext)
{
  menuFilter->setFilterFixedString(filtertext);
  if (!filtertext.isEmpty()) {
    menubox->expandAll();
    if (menuFilter->rowCount() > 0)
      menubox->scrollTo(menuFilter->index(0, 0));
  }
}

void QuickMenu::updateOptions()
{
  METLIBS_LOG_SCOPE();
  // hide old options
  for (int i=0; i<maxoptions; i++){
    optionmenu[i]->adjustSize();
    optionlabel[i]->hide();
    optionmenu[i]->hide();
  }

  // add options
  updating_options = true;
  if (!isValidList(selected_list))
    return;
  quickMenu& q = qm[selected_list];
  const int n = std::min(q.opt.size(), (size_t)maxoptions);
  for (int i = 0; i < n; ++i) {
    quickMenuOption& o = q.opt[i];
    optionlabel[i]->setText(QString::fromStdString(o.key));
    optionlabel[i]->show();

    const int nopts = o.options.size();
    optionmenu[i]->clear();
    optionmenu[i]->setEnabled(nopts > 0);
    if (nopts > 0) {
      int defidx = -1;
      for (int j = 0; j < nopts; j++) {
        optionmenu[i]->addItem(QString::fromStdString(o.options[j]));
        if (o.options[j] == o.def)
          defidx = j;
      }
      if (defidx < 0) {
        defidx = 0;
        o.def = o.options[defidx];
      }
      optionmenu[i]->setCurrentIndex(defidx);
    }
    optionmenu[i]->adjustSize();
    optionmenu[i]->show();
  }
  updating_options = false;
  updatebut->setEnabled(q.type == quickMenu::QM_USER);
}

void QuickMenu::saveChanges(int list, int item)
{
  METLIBS_LOG_SCOPE();
  if ((old_list != list || old_list != item) && isValidList(old_list) && qm[old_list].valid_item(old_item)) {
    getCommand(qm[old_list].item(old_item).command);
  }
  old_list = list;
  old_item = item;
}

void QuickMenu::menuboxSelectionChanged(const QItemSelection&, const QItemSelection&)
{
  METLIBS_LOG_SCOPE();
  const QModelIndexList selected = menubox->selectionModel()->selectedIndexes();
  if (selected.size() != 1)
    return;
  const QModelIndex& filterIndex = selected.first();
  if (!filterIndex.isValid())
    return;
  const QModelIndex index = menuFilter->mapToSource(filterIndex);
  if (!index.isValid() || !index.parent().isValid())
    return;

  const int idx = index.row();
  selected_list = index.parent().row();
  saveChanges(selected_list, idx);

  if (!isValidList(selected_list))
    return;
  quickMenu& q = qm[selected_list];
  if (!q.valid_item(idx))
    return;
  q.item_index = idx;

  updateOptions();
  setCommand();
}

void QuickMenu::setCommand()
{
  METLIBS_LOG_SCOPE();
  QString ts;
  if (isValidList(selected_list)) {
    quickMenu& sm = qm[selected_list];
    if (sm.valid_item()) {
      for (const std::string& c : sm.item().command)
        ts += QString::fromStdString(c) + "\n";
    }
  }
  comedit->setText(ts);
}

void QuickMenu::comChanged()
{
  METLIBS_LOG_SCOPE();
  std::string ts = comedit->toPlainText().toStdString();
  const std::set<int> uo = qm[selected_list].used_options(ts);
  for (int i = 0; i < maxoptions; i++) {
    const bool enable = (uo.count(i) > 0);
    optionmenu[i]->setEnabled(enable);
    optionlabel[i]->setEnabled(enable);
  }
}

void QuickMenu::menuboxItemActivated(const QModelIndex&)
{
  plotActiveMenu();
}

void QuickMenu::getCommand(std::vector<std::string>& commands)
{
  METLIBS_LOG_SCOPE();
  const std::string text = comedit->toPlainText().toStdString();
  commands = miutil::split(text, 0, "\n");
  for (std::string& c : commands) {
    miutil::trim(c);
  }
}

void QuickMenu::varExpand(std::vector<std::string>& com)
{
  qm[selected_list].expand_options(com);
}

void QuickMenu::plotActiveMenu()
{
  METLIBS_LOG_SCOPE();
  std::vector<std::string> com;
  getCommand(com);
  METLIBS_LOG_DEBUG(LOGVAL(com.size()));

  if (!com.empty()) {
    varExpand(com);
    Q_EMIT Apply(makeCommands(com), true);
    plotted_list = selected_list;
    plotted_item = qm[selected_list].item_index;
  }
  saveChanges(-1,-1);
}

void QuickMenu::comButton(bool on)
{
  const int w = this->width();
  int h = this->height();
  if (on) {
    comlabel->show();
    comedit->show();
    h += comlabel->height() + comedit->height();
  } else {
    h -= comlabel->height() + comedit->height();
    comlabel->hide();
    comedit->hide();
  }
  this->resize(w, h);
}

void QuickMenu::demoButton(bool on)
{
  if (on) {
    qm[selected_list].item_index = 0;
    demoTimer= startTimer(timerinterval*1000);
    timeron = true;
    selectList(selected_list);
    plotActiveMenu();
  } else {
    killTimer(demoTimer);
    timeron = false;
  }
}

void QuickMenu::timerEvent(QTimerEvent *e)
{
  if (e->timerId()==demoTimer) {
    quickMenu& q = qm[selected_list];
    q.item_index++;
    if (q.item_index >= int(q.menuitems.size()))
      // wrap around
      q.item_index = 0;
    selectList(selected_list);
    plotActiveMenu();
  }
}

void QuickMenu::intervalChanged(int value)
{
  timerinterval= value;
  if (timeron) {
    killTimer(demoTimer);
    demoTimer= startTimer(timerinterval*1000);
  }
}

void QuickMenu::optionChanged(int option)
{
  if (updating_options)
    return;
  const int choice = optionmenu[option]->currentIndex();
  if (isValidList(selected_list)) {
    quickMenuOption& o = qm[selected_list].opt[option];
    if (choice >= 0 && choice < (int)o.options.size()) {
      o.def = o.options[choice];
    }
  }
}

void QuickMenu::helpClicked()
{
  Q_EMIT showsource("ug_quickmenu.html");
}

void QuickMenu::closeEvent(QCloseEvent*)
{
  Q_EMIT QuickHide();
}
