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

#include "diEditItemManager.h"
#include "EditItems/editcomposite.h"
#include "EditItems/drawingstylemanager.h"
#include "EditItems/toolbar.h"

#include <QActionGroup>
#include <QApplication>
#include <QDockWidget>
#include <QListWidget>
#include <QMainWindow>

#define COMPOSITE_WIDTH 112
#define COMPOSITE_HEIGHT 96

namespace {

int findItem(QListWidget* lw, const QString& item)
{
  QList<QListWidgetItem *> items = lw->findItems(item, Qt::MatchExactly | Qt::MatchCaseSensitive);
  if (items.isEmpty())
    return -1;
  else
    return lw->row(items.back());
}

} // namespace

namespace EditItems {

CompositeDelegate::CompositeDelegate(QObject *parent)
 : QStyledItemDelegate(parent)
{
}

CompositeDelegate::~CompositeDelegate()
{
}

void CompositeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  QImage image = DrawingStyleManager::instance()->toImage(DrawingItemBase::Composite, index.data(Qt::UserRole).toString());
  if (option.state & QStyle::State_Selected) {
    painter->fillRect(option.rect, qApp->palette().color(QPalette::Highlight));
    painter->fillRect(option.rect.adjusted(4, 4, -4, -4), Qt::white);
  } else {
    painter->fillRect(option.rect, Qt::gray);
    painter->fillRect(option.rect.adjusted(1, 1, -1, -1), Qt::white);
  }

  painter->drawImage(option.rect.center() - QPoint(image.width()/2, image.height()/2), image);
}

QSize CompositeDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  return QSize(COMPOSITE_WIDTH, COMPOSITE_HEIGHT);
}

ToolBar *ToolBar::instance(QWidget * parent)
{
  if (!ToolBar::self_)
    ToolBar::self_ = new ToolBar(parent);
  return ToolBar::self_;
}

ToolBar *ToolBar::self_ = 0;

ToolBar::ToolBar(QWidget *parent)
    : QToolBar(QApplication::translate("EditItems::ToolBar", "Paint Operations") + " (NEW)", parent)
{
  DrawingStyleManager *dsm = DrawingStyleManager::instance();

  QActionGroup *actionGroup = new QActionGroup(this);
  QHash<EditItemManager::Action, QAction *> actions = EditItemManager::instance()->actions();

  QAction *hideAction = new QAction(this);
  hideAction->setToolTip(tr("Hide or show all drawing dialogs"));
  hideAction->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogResetButton));

  connect( hideAction, SIGNAL( triggered() ) , SLOT( show_hide_all() ) );
  addAction(hideAction);
  actionGroup->addAction(hideAction);

  QAction *undoAction = actions[EditItemManager::Undo];
  addAction(undoAction);
  actionGroup->addAction(undoAction);

  QAction *redoAction = actions[EditItemManager::Redo];
  addAction(redoAction);
  actionGroup->addAction(redoAction);

  // *** select ***
  selectAction_ = actions[EditItemManager::Select];
  addAction(selectAction_);
  actionGroup->addAction(selectAction_);

  // *** create polyline ***
  // *** create composite ***
  polyLineWidget = new QDockWidget("PolyLines",parent,Qt::WindowStaysOnTopHint);

  polyLineWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
  polyLineWidget->setObjectName("PolyLines");
  QMainWindow * pwin = dynamic_cast<QMainWindow *>(parent);
  pwin->addDockWidget(Qt::LeftDockWidgetArea, polyLineWidget);
  polyLineWidget->hide();
  connect(this, SIGNAL(visibilityChanged(bool)), this, SLOT(showPolyLines()));

  polyLineAction_ = actions[EditItemManager::CreatePolyLine];
  addAction(polyLineAction_);
  actionGroup->addAction(polyLineAction_);
  // Create a combo box containing specific polyline types.
  polyLineList_ = new QListWidget(polyLineWidget);
  polyLineList_->setSelectionMode(QAbstractItemView::SingleSelection);
  connect(polyLineList_, SIGNAL( itemClicked( QListWidgetItem * )), this, SLOT(setPolyLineType(QListWidgetItem *)));
  connect(polyLineList_, SIGNAL( itemClicked( QListWidgetItem * )), polyLineAction_, SLOT(trigger()));
  connect(polyLineAction_, SIGNAL(triggered()), polyLineList_, SLOT(raise()));
  polyLineWidget->setWidget(polyLineList_);

  // Create an entry for each style. Use the name as an internal identifier
  // since we may decide to use tr() on the visible name at some point.
  QStringList styles = dsm->styles(DrawingItemBase::PolyLine);
  styles.sort();
  for (const QString& name : styles) {
    QListWidgetItem * item = new QListWidgetItem(name);
    item->setData(Qt::UserRole,name);
    polyLineList_->addItem(item);
  }

  polyLineList_->setCurrentRow(0);
  polyLineList_->item(0)->setSelected(true);
  setPolyLineType(polyLineList_->item(0));

  // *** create symbol ***
  symbolWidget = new QDockWidget("Symbols",parent,Qt::WindowStaysOnTopHint);

  symbolWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
  symbolWidget->setObjectName("symbols");
  pwin = dynamic_cast<QMainWindow *>(parent);
  pwin->addDockWidget(Qt::RightDockWidgetArea, symbolWidget);
  symbolWidget->hide();
  connect(this, SIGNAL(visibilityChanged(bool)), this, SLOT(showSymbols()));
  symbolAction_ = actions[EditItemManager::CreateSymbol];
  addAction(symbolAction_);
  actionGroup->addAction(actions[EditItemManager::CreateSymbol]);

  // Create a combo box containing specific symbols.
  symbolList_ = new QListWidget(symbolWidget);
  symbolList_->setSelectionMode(QAbstractItemView::SingleSelection);
  connect(symbolList_, SIGNAL( itemClicked( QListWidgetItem * )), this, SLOT(setSymbolType(QListWidgetItem *)));
  connect(symbolList_, SIGNAL( itemClicked( QListWidgetItem * )), symbolAction_, SLOT(trigger()));
  symbolWidget->setWidget(symbolList_);

  DrawingManager *dm = DrawingManager::instance();

  for (const QString& section : dm->symbolSectionNames()) {
    // Create an entry for each symbol, noting that each of these names
    // includes the section in the form of "section|name".
    QStringList names = dm->symbolNames(section);
    addSymbols(section, names);
  }

  symbolList_->setCurrentRow(0);
  symbolList_->item(0)->setSelected(true);
  setSymbolType(symbolList_->item(0));

  // *** create text ***
  textWidget = new QDockWidget("Texts",parent,Qt::WindowStaysOnTopHint);

  textWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
  textWidget->setObjectName("texts");
  pwin = dynamic_cast<QMainWindow *>(parent);
  pwin->addDockWidget(Qt::RightDockWidgetArea, textWidget);
  textWidget->hide();
  connect(this, SIGNAL(visibilityChanged(bool)), this, SLOT(showTexts()));
  textAction_ = actions[EditItemManager::CreateText];
  addAction(textAction_);
  actionGroup->addAction(textAction_);

  // Create a combo box containing specific text types.
  textList_ = new QListWidget();
  textList_->setSelectionMode(QAbstractItemView::SingleSelection);
  connect(textList_, SIGNAL( itemClicked( QListWidgetItem * )), this, SLOT(setTextType(QListWidgetItem *)));
  connect(textList_, SIGNAL( itemClicked( QListWidgetItem * )), textAction_, SLOT(trigger()));
  textWidget->setWidget(textList_);

  // Create an entry for each style. Use the name as an internal identifier
  // since we may decide to use tr() on the visible name at some point.
  styles = dsm->styles(DrawingItemBase::Text);
  styles.sort();
  for (const QString& name : styles) {
    if (dsm->getStyle(DrawingItemBase::Text, name).value("hide").toBool() == false) {
      QListWidgetItem * item = new QListWidgetItem(name);
      item->setData(Qt::UserRole,name);
      textList_->addItem(item);
    }
  }

  if (textList_->count() > 0) {
    textList_->setCurrentRow(0);
    QListWidgetItem* i0 = textList_->item(0);
    i0->setSelected(true);
    setTextType(i0);
  }

  // *** create composite **

  // Create an entry for each style. Use the name as an internal identifier
  // since we may decide to use tr() on the visible name at some point.
  styles = dsm->styles(DrawingItemBase::Composite);
  styles.sort();
  
  compositeWidget_ = new QDockWidget("Composites",parent,Qt::WindowStaysOnTopHint);

  compositeWidget_->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
  compositeWidget_->setObjectName("Composites");
  pwin = dynamic_cast<QMainWindow *>(parent);
  pwin->addDockWidget(Qt::RightDockWidgetArea, compositeWidget_);
  compositeWidget_->hide();
  connect(this, SIGNAL(visibilityChanged(bool)), this, SLOT(showComposites()));
  compositeAction_ = actions[EditItemManager::CreateComposite];
  addAction(compositeAction_);
  actionGroup->addAction(compositeAction_);

  compositeDelegate_ = new CompositeDelegate(this);

  compositeView = new QListWidget();

  // Fill the model with items for the composite objects.
  int i = 0;

  for (const QString& name : styles) {
    if (dsm->getStyle(DrawingItemBase::Composite, name).value("hide").toBool() == false) {
      QListWidgetItem *item = new QListWidgetItem(name);
      item->setData(Qt::UserRole, name);
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      item->setSizeHint(QSize(COMPOSITE_WIDTH, COMPOSITE_HEIGHT));
      compositeView->addItem(item);
      i++;
    }
  }

  compositeView->setSelectionMode(QAbstractItemView::SingleSelection);
  compositeView->setItemDelegate(compositeDelegate_);
  
  connect(compositeView, SIGNAL( itemClicked( QListWidgetItem * )), this, SLOT(setCompositeType(QListWidgetItem *)));
  connect(compositeView, SIGNAL( itemClicked( QListWidgetItem * )), compositeAction_, SLOT(trigger()));

  compositeWidget_->setWidget(compositeView);

  compositeView->setCurrentItem(compositeView->item(0));

  // Select the select action by default.
  selectAction_->trigger();
}

void ToolBar::setSelectAction()
{
  selectAction_->trigger();
}

void ToolBar::setCreatePolyLineAction(const QString &type)
{
  const int index = findItem(polyLineList_, type);
  if (index >= 0) {
    polyLineList_->setCurrentRow(index);
    polyLineList_->item(index)->setSelected(true);
    setPolyLineType(polyLineList_->item(index));
    polyLineAction_->trigger();
  }
}

void ToolBar::setPolyLineType(QListWidgetItem *item)
{
  // Obtain the style identifier from the style action and store it in the
  // main polyline action for later retrieval by the EditItemManager.
  polyLineAction_->setData(item->data(Qt::UserRole));
  polyLineAction_->setToolTip(item->text());
}

void ToolBar::setCreateSymbolAction(const QString &type)
{
  const int index = findItem(symbolList_, type);
  if (index >= 0) {
    symbolList_->setCurrentRow(index);
    symbolList_->item(index)->setSelected(true);
    setSymbolType(symbolList_->item(index));
    symbolAction_->trigger();
  }
}

void ToolBar::setSymbolType(QListWidgetItem *item)
{
  // Obtain the style identifier from the style action and store it in the
  // main symbol action for later retrieval by the EditItemManager.
  QVariant data = item->data(Qt::UserRole);
  bool isSection = item->data(Qt::UserRole + 1).toBool();

  // If a section heading was selected then select the item following it if
  // possible.
  int index = symbolList_->row(item);
  if (isSection && index < (symbolList_->count() - 1)) {
    symbolList_->setCurrentRow(index + 1);
    symbolList_->item(index + 1)->setSelected(true);
  }
  else {
    symbolAction_->setData(data);
    symbolAction_->setToolTip(item->text());
  }
}

/**
 * Adds the symbol with the given name to the symbol combo box under the
 * section specified.
 */
void ToolBar::addSymbol(const QString &section, const QString &name)
{
  QStringList names;
  names << QString("%1|%2").arg(section).arg(name);
  addSymbols(section, names);
}

/**
 * Adds the symbols with the given names to the symbol combo box under the
 * section specified. Note that the names are themselves combinations of
 * the section and name, taking the form of "section|name".
 */
void ToolBar::addSymbols(const QString &section, const QStringList &names)
{
  QAbstractItemModel *model = symbolList_->model();
  QFont sectionFont = font();
  sectionFont.setWeight(QFont::Bold);

  int row = findItem(symbolList_, section);
  if (row == -1) {
    symbolList_->addItem(section);
    row = model->rowCount() - 1;

    QModelIndex index = model->index(row, 0);
    model->setData(index, sectionFont, Qt::FontRole);
    model->setData(index, palette().brush(QPalette::AlternateBase), Qt::BackgroundRole);
    model->setData(index, true, Qt::UserRole + 1);

    // Insert the internal section name into a list to keep track of which
    // sections are present. At the moment, the internal name is the visible name.
    sections_.append(section);
    row++;

  } else {
    // The section already exists, so find the next section if there is one
    // and insert new symbols before that.
    int s = sections_.indexOf(section);

    if (s == sections_.size() - 1) {
      // This is the last section, so insert new symbols at the end of the
      // combo box model.
      row = model->rowCount();
    } else {
      // Find the row of the next section heading in the model.
      row = findItem(symbolList_, sections_.at(s + 1));
    }
  }

  for (const QString &name : names) {
    // Use the name as an internal identifier since we may decide to use tr()
    // on the visible name at some point.
    // Remove any section information from the name for clarity.
    QString visibleName = name.split("|").last();
    QIcon icon(QPixmap::fromImage(DrawingManager::instance()->getSymbolImage(name, 32, 32)));

    QListWidgetItem * item =  new QListWidgetItem(icon, visibleName);
    item->setData(Qt::UserRole, name);

    symbolList_->insertItem(row, item);

    // Set the section role to false.
    QModelIndex index = model->index(row, 0);
    model->setData(index, false, Qt::UserRole + 1);
    row++;
  }
}

void ToolBar::setTextType(QListWidgetItem *item)
{
  // Obtain the style identifier from the style action and store it in the
  // main text action for later retrieval by the EditItemManager.
  textAction_->setData(item->data(Qt::UserRole));
  textAction_->setToolTip(item->text());
}

void ToolBar::setCompositeType(QListWidgetItem *item)
{
  // Obtain the style identifier from the index in the selection and store
  // it in the main text action for later retrieval by the EditItemManager.
  compositeAction_->setData(item->data(Qt::UserRole));
  compositeAction_->setToolTip(item->text());
}

void ToolBar::showComposites()
{
  compositeWidget_->setVisible(this->isVisible());
}

void ToolBar::showPolyLines()
{
  polyLineWidget->setVisible(this->isVisible());
}

void ToolBar::showSymbols()
{
  symbolWidget->setVisible(this->isVisible());
}

void ToolBar::showTexts()
{
  textWidget->setVisible(this->isVisible());
}


void ToolBar::show_hide_all()
{
  int count_visible = 0;

  if ( polyLineWidget->isVisible() )
    count_visible++;
  if ( symbolWidget->isVisible() )
    count_visible++;
  if ( textWidget->isVisible() )
    count_visible++;
  if ( compositeWidget_->isVisible() )
    count_visible++;

  bool show = count_visible < 2;

  polyLineWidget->setVisible(show);
  symbolWidget->setVisible(show);
  textWidget->setVisible(show);
  compositeWidget_->setVisible(show);

}

} // namespace EditItems
