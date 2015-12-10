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
#include <QComboBox>
#include <QListWidget>
#include <QToolButton>

#define COMPOSITE_WIDTH 112
#define COMPOSITE_HEIGHT 96

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
  QBrush background;
  if (option.state & QStyle::State_Selected)
    background = QBrush(qApp->palette().color(QPalette::Highlight));
  else
    background = QBrush(Qt::white);

  painter->fillRect(option.rect, background);
  painter->fillRect(option.rect.adjusted(4, 4, -4, -4), Qt::white);
  painter->drawImage(option.rect.center() - QPoint(image.width()/2, image.height()/2), image);
}

QSize CompositeDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  return QSize(COMPOSITE_WIDTH, COMPOSITE_HEIGHT);
}

ToolBar *ToolBar::instance()
{
  if (!ToolBar::self_)
    ToolBar::self_ = new ToolBar();
  return ToolBar::self_;
}

ToolBar *ToolBar::self_ = 0;

ToolBar::ToolBar(QWidget *parent)
    : QToolBar(QApplication::translate("EditItems::ToolBar", "Paint Operations") + " (NEW)", parent)
{
  DrawingStyleManager *dsm = DrawingStyleManager::instance();

  QActionGroup *actionGroup = new QActionGroup(this);
  QHash<EditItemManager::Action, QAction *> actions = EditItemManager::instance()->actions();

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
  polyLineAction_ = actions[EditItemManager::CreatePolyLine];
  addAction(polyLineAction_);
  actionGroup->addAction(polyLineAction_);

  // Create a combo box containing specific polyline types.
  polyLineCombo_ = new QComboBox();
  connect(polyLineCombo_, SIGNAL(currentIndexChanged(int)), this, SLOT(setPolyLineType(int)));
  connect(polyLineCombo_, SIGNAL(currentIndexChanged(int)), polyLineAction_, SLOT(trigger()));
  addWidget(polyLineCombo_);

  // Create an entry for each style. Use the name as an internal identifier
  // since we may decide to use tr() on the visible name at some point.
  QStringList styles = dsm->styles(DrawingItemBase::PolyLine);
  styles.sort();
  foreach (QString name, styles)
    polyLineCombo_->addItem(name, name);

  polyLineCombo_->setCurrentIndex(0);
  setPolyLineType(0);

  // *** create symbol ***
  symbolAction_ = actions[EditItemManager::CreateSymbol];
  addAction(symbolAction_);
  actionGroup->addAction(actions[EditItemManager::CreateSymbol]);

  // Create a combo box containing specific symbols.
  symbolCombo_ = new QComboBox();
  symbolCombo_->view()->setTextElideMode(Qt::ElideRight);
  connect(symbolCombo_, SIGNAL(currentIndexChanged(int)), this, SLOT(setSymbolType(int)));
  connect(symbolCombo_, SIGNAL(currentIndexChanged(int)), symbolAction_, SLOT(trigger()));
  addWidget(symbolCombo_);

  DrawingManager *dm = DrawingManager::instance();

  foreach (QString section, dm->symbolSectionNames()) {
    // Create an entry for each symbol, noting that each of these names
    // includes the section in the form of "section|name".
    QStringList names = dm->symbolNames(section);
    addSymbols(section, names);
  }

  symbolCombo_->setCurrentIndex(0);
  setSymbolType(0);

  // *** create text ***
  textAction_ = actions[EditItemManager::CreateText];
  addAction(textAction_);
  actionGroup->addAction(textAction_);

  // Create a combo box containing specific text types.
  textCombo_ = new QComboBox();
  connect(textCombo_, SIGNAL(currentIndexChanged(int)), this, SLOT(setTextType(int)));
  connect(textCombo_, SIGNAL(currentIndexChanged(int)), textAction_, SLOT(trigger()));
  addWidget(textCombo_);

  // Create an entry for each style. Use the name as an internal identifier
  // since we may decide to use tr() on the visible name at some point.
  styles = dsm->styles(DrawingItemBase::Text);
  styles.sort();
  foreach (QString name, styles)
    textCombo_->addItem(name, name);

  textCombo_->setCurrentIndex(0);
  setTextType(0);

  // *** create composite ***
  compositeAction_ = actions[EditItemManager::CreateComposite];
  compositeButton_ = new QToolButton();
  compositeButton_->setDefaultAction(compositeAction_);
  connect(compositeButton_, SIGNAL(clicked()), SLOT(showComposites()));
  addWidget(compositeButton_);

  // Create an entry for each style. Use the name as an internal identifier
  // since we may decide to use tr() on the visible name at some point.
  styles = dsm->styles(DrawingItemBase::Composite);
  styles.sort();

  compositeDelegate_ = new CompositeDelegate(this);

  compositeDialog_ = new QDialog(this);
  compositeDialog_->setWindowFlags(Qt::Popup);

  QListWidget *compositeView = new QListWidget();

  // Fill the model with items for the composite objects.
  int i = 0;

  foreach (QString name, styles) {
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

  connect(compositeView, SIGNAL(itemActivated(QListWidgetItem *)), compositeDialog_, SLOT(accept()));
  connect(compositeView, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), SLOT(setCompositeType(QListWidgetItem *)));

  QVBoxLayout *layout = new QVBoxLayout(compositeDialog_);
  layout->setMargin(0);
  layout->addWidget(compositeView);

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
  const int index = polyLineCombo_->findText(type);
  if (index >= 0) {
    polyLineCombo_->setCurrentIndex(index);
    polyLineAction_->trigger();
  }
}

void ToolBar::setPolyLineType(int index)
{
  // Obtain the style identifier from the style action and store it in the
  // main polyline action for later retrieval by the EditItemManager.
  polyLineAction_->setData(polyLineCombo_->itemData(index));
}

void ToolBar::setCreateSymbolAction(const QString &type)
{
  const int index = symbolCombo_->findText(type);
  if (index >= 0) {
    symbolCombo_->setCurrentIndex(index);
    symbolAction_->trigger();
  }
}

void ToolBar::setSymbolType(int index)
{
  // Obtain the style identifier from the style action and store it in the
  // main symbol action for later retrieval by the EditItemManager.
  QVariant data = symbolCombo_->itemData(index);
  bool isSection = symbolCombo_->itemData(index, Qt::UserRole + 1).toBool();

  // If a section heading was selected then select the item following it if
  // possible.
  if (isSection && index < (symbolCombo_->count() - 1))
    symbolCombo_->setCurrentIndex(index + 1);
  else
    symbolAction_->setData(data);
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
  QAbstractItemModel *model = symbolCombo_->model();
  QFont sectionFont = font();
  sectionFont.setWeight(QFont::Bold);

  int row = symbolCombo_->findData(QVariant(section));

  if (row == -1) {
    symbolCombo_->addItem(section, section);
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
      row = symbolCombo_->findData(QVariant(sections_.at(s + 1)));
    }
  }

  foreach (const QString &name, names) {
    // Use the name as an internal identifier since we may decide to use tr()
    // on the visible name at some point.
    // Remove any section information from the name for clarity.
    QString visibleName = name.split("|").last();
    QIcon icon(QPixmap::fromImage(DrawingManager::instance()->getSymbolImage(name, 32, 32)));
    symbolCombo_->insertItem(row, icon, visibleName, name);

    // Set the section role to false.
    QModelIndex index = model->index(row, 0);
    model->setData(index, false, Qt::UserRole + 1);
    row++;
  }
}

void ToolBar::setTextType(int index)
{
  // Obtain the style identifier from the style action and store it in the
  // main text action for later retrieval by the EditItemManager.
  textAction_->setData(textCombo_->itemData(index));
}

void ToolBar::setCompositeType(QListWidgetItem *item)
{
  // Obtain the style identifier from the index in the selection and store
  // it in the main text action for later retrieval by the EditItemManager.
  compositeAction_->setData(item->data(Qt::UserRole));
}

void ToolBar::showComposites()
{
  compositeDialog_->move(mapToGlobal(compositeButton_->pos() - QPoint(0, COMPOSITE_HEIGHT * 3)));
  compositeDialog_->resize(COMPOSITE_WIDTH * 1.5, COMPOSITE_HEIGHT * 3);
  compositeDialog_->exec();
}

} // namespace
