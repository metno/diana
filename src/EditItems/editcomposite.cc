/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2014 met.no

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

#include "drawingstylemanager.h"
#include "editcomposite.h"
#include "editpolyline.h"
#include "editsymbol.h"
#include "edittext.h"

#include <QAction>
#include <QDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

namespace EditItem_Composite {

Composite::Composite(int id)
  : DrawingItem_Composite::Composite(id)
{
  editAction = new QAction(tr("Edit item"), this);
  connect(editAction, SIGNAL(triggered()), SLOT(editItem()));
}

Composite::~Composite()
{
}

QList<QAction *> Composite::actions(const QPoint &pos) const
{
  QList<QAction *> acts;
  acts << editAction;
  return acts;
}

void Composite::createElements()
{
  DrawingItem_Composite::Composite::createElements();
  editAction->setEnabled(isEditable(this));
}

bool Composite::isEditable(DrawingItemBase *element) const
{
  if (dynamic_cast<EditItem_Text::Text *>(element))
    return true;

  bool editable = false;
  EditItem_Composite::Composite *c = dynamic_cast<EditItem_Composite::Composite *>(element);
  if (c) {
    foreach (DrawingItemBase *child, c->elements_)
      editable |= isEditable(child);
    return editable;
  } else
    return false;
}

DrawingItemBase *Composite::cloneSpecial(bool setUniqueId) const
{
  Composite *item = new Composite(setUniqueId ? -1 : id());
  copyBaseData(item);
  // ### copy special data from this into item ... TBD
  return item;
}

void Composite::mousePress(QMouseEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
                           QSet<QSharedPointer<DrawingItemBase> > *items,
                           const QSet<QSharedPointer<DrawingItemBase> > *selItems,
                           bool *multiItemOp)
{
  if (event->button() == Qt::LeftButton) {
    pressedCtrlPointIndex_ = -1;
    resizing_ = (pressedCtrlPointIndex_ >= 0);
    moving_ = !resizing_;
    basePoints_ = points_;
    baseMousePos_ = event->pos();

    if (multiItemOp)
      *multiItemOp = moving_; // i.e. a move operation would apply to all selected items
  }
}

/**
 * Processes a mouse press event when the object is incomplete.
 * This implementation only handles left button clicks, adding a single point to the
 * internal list on the first click.
 * Any additional clicks cause the object to be marked as complete.
 */
void Composite::incompleteMousePress(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  repaintNeeded = true;

  if (event->button() == Qt::LeftButton && points_.isEmpty()) {
    // Create two points: one for the current mouse position and another to be
    // updated during the following move events.
    points_.append(QPointF(event->pos()));
    points_.append(QPointF(event->pos()));

    // Ensure that the manager gets the keyboard focus so that key events are
    // delivered to this item.
    EditItemManager::instance()->setFocus(true);

    // Update the geographic points and the control points.
    setLatLonPoints(DrawingManager::instance()->getLatLonPoints(*this));
    createElements();
    updateRect();
  }
}

void Composite::incompleteMouseRelease(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  repaintNeeded = true;
  complete = true;
}

void Composite::move(const QPointF &pos)
{
  const QPointF delta = pos - baseMousePos_;
  QList<QPointF> newPoints;
  for (int i = 0; i < points_.size(); ++i)
    newPoints.append(basePoints_.at(i) + delta);

  setPoints(newPoints);
  updateControlPoints();
}

void Composite::resize(const QPointF &)
{
}

void Composite::updateControlPoints()
{
}

void Composite::drawHoverHighlighting(bool, bool) const
{
  QRectF bbox = boundingRect();

  glColor3ub(255, 0, 0);
  glBegin(GL_LINE_LOOP);
  glVertex2f(bbox.bottomLeft().x(), bbox.bottomLeft().y());
  glVertex2f(bbox.bottomRight().x(), bbox.bottomRight().y());
  glVertex2f(bbox.topRight().x(), bbox.topRight().y());
  glVertex2f(bbox.topLeft().x(), bbox.topLeft().y());
  glEnd();
}

void Composite::drawIncomplete() const
{
  QRectF bbox = boundingRect();

  glColor3ub(0, 255, 0);
  glBegin(GL_LINE_LOOP);
  glVertex2f(bbox.bottomLeft().x(), bbox.bottomLeft().y());
  glVertex2f(bbox.bottomRight().x(), bbox.bottomRight().y());
  glVertex2f(bbox.topRight().x(), bbox.topRight().y());
  glVertex2f(bbox.topLeft().x(), bbox.topLeft().y());
  glEnd();
}

void Composite::editItem()
{
  QDialog dialog;
  QVBoxLayout *layout = new QVBoxLayout(&dialog);

  CompositeEditor *editor = new CompositeEditor(this);
  layout->addWidget(editor);

  layout->addStretch();

  QDialogButtonBox *box = new QDialogButtonBox();
  box->addButton(QDialogButtonBox::Ok);
  box->addButton(QDialogButtonBox::Cancel);
  layout->addWidget(box);

  connect(box, SIGNAL(accepted()), &dialog, SLOT(accept()));
  connect(box, SIGNAL(rejected()), &dialog, SLOT(reject()));

  dialog.setWindowTitle(tr("Edit Item"));
  if (dialog.exec() == QDialog::Accepted)
    editor->applyChanges();
}

DrawingItemBase *Composite::newCompositeItem() const
{
  return new EditItem_Composite::Composite();
}

DrawingItemBase *Composite::newPolylineItem() const
{
  return new EditItem_PolyLine::PolyLine();
}

DrawingItemBase *Composite::newSymbolItem() const
{
  return new EditItem_Symbol::Symbol();
}

DrawingItemBase *Composite::newTextItem() const
{
  return new EditItem_Text::Text();
}

CompositeEditor::CompositeEditor(Composite *item)
{
  this->item = item;
  createElements(DrawingItemBase::Composite,
    Drawing(item)->property("style:type").toString());
}

void CompositeEditor::createElements(const DrawingItemBase::Category &category, const QString &name)
{
  DrawingStyleManager *dsm = DrawingStyleManager::instance();
  QVariantMap style = dsm->getStyle(category, name);
  objects = style.value("objects").toStringList();
  values = style.value("values").toStringList();
  styles = style.value("styles").toStringList();
  QString layout = style.value("layout").toString();

  QString background = style.value("fillcolour", "white").toString();
  setStyleSheet(QString("QWidget { background-color: %1 }").arg(background));

  QLayout *layout_;

  if (layout == "vertical")
    layout_ = new QVBoxLayout(this);
  else if (layout == "diagonal")
    layout_ = new QGridLayout(this);
  else
    layout_ = new QHBoxLayout(this);

  // If the number of objects, styles and values do not match then return.
  if ((objects.size() != values.size()) || (objects.size() != styles.size()))
    return;

  // Create the elements.
  DrawingManager *dm = DrawingManager::instance();

  for (int i = 0; i < objects.size(); ++i) {

    DrawingItemBase *element = item->elementAt(i);
    QString style = element->property("style:type").toString();
    QWidget *child;

    if (objects.at(i) == "text") {
      QString currentText = element->property("text").toStringList().join("\n");

      if (values.at(i) == "X") {
        QComboBox *textCombo = new QComboBox();
        textCombo->setEditable(true);
        textCombo->addItems(dsm->getComplexTextList());

        int index = dsm->getComplexTextList().indexOf(currentText);
        if (index == -1) {
          textCombo->insertItem(0, currentText);
          index = 0;
        }

        textCombo->setCurrentIndex(index);

        // Record the index of this component in the list of objects.
        textCombo->setProperty("index", i);
        connect(textCombo, SIGNAL(currentIndexChanged(const QString &)), SLOT(updateText(const QString &)));
        connect(textCombo, SIGNAL(editTextChanged(const QString &)), SLOT(updateText(const QString &)));
        child = textCombo;

      } else {
        QLineEdit *textEdit = new QLineEdit();
        textEdit->setText(currentText);
        // Record the index of this component in the list of objects.
        textEdit->setProperty("index", i);
        connect(textEdit, SIGNAL(textChanged(const QString &)), SLOT(updateText(const QString &)));
        child = textEdit;
      }

    } else if (objects.at(i) == "symbol") {
      QLabel *label = new QLabel();

      QSize size = dm->getSymbolSize(style);
      label->setPixmap(QPixmap::fromImage(dm->getSymbolImage(style, size.width(), size.height())));
      child = label;

    } else if (objects.at(i) == "composite") {
      // Create a child editor for the composite item and append its index to
      // a list for use when changes are applied.
      CompositeEditor* childEditor = new CompositeEditor(static_cast<EditItem_Composite::Composite *>(element));
      childEditors.append(childEditor);
      child = childEditor;

    } else
      continue;

    if (layout == "diagonal") {
      static_cast<QGridLayout *>(layout_)->addWidget(child, i, i);
    } else
      layout_->addWidget(child);
  }
}

CompositeEditor::~CompositeEditor()
{
}

void CompositeEditor::applyChanges()
{
  // Apply the changes to the immediate child elements.
  QHashIterator<int, QVariantList> it(changes);
  while (it.hasNext()) {
    it.next();
    int index = it.key();
    QVariantList value = it.value();
    item->elementAt(index)->setProperty(value.at(0).toString(), value.at(1));
  }

  // Apply the changes to composite child elements.
  foreach (CompositeEditor *childEditor, childEditors)
    childEditor->applyChanges();

  // Arrange the elements, taking the changes into account.
  item->arrangeElements();
  item->readExtraProperties();
}

void CompositeEditor::updateSymbol(QAction *action)
{
  int index = action->data().toInt();
  if (objects.at(index) == "symbol") {
    QWidget *w = editors.value(index);
    // Update the button icon immediately.
    QToolButton *button = static_cast<QToolButton *>(w);
    button->setIcon(action->icon());
    // Record the new style in the changes map.
    QVariantList ch;
    ch << "style:type" << action->iconText();
    changes[index] = ch;
  }
}

void CompositeEditor::updateText(const QString &text)
{
  bool ok = false;
  int index = sender()->property("index").toInt(&ok);
  if (ok && objects.at(index) == "text") {
    if (item) {
      // The element's text property accepts a QStringList. If we inadvertently
      // pass a QString then we'll get a hard-to-debug crash.
      QStringList lines;
      lines << text;
      QVariantList ch;
      ch << "text" << lines;
      changes[index] = ch;
    }
  }
}

} // namespace EditItem_Composite
