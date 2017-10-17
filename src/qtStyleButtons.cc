#include "qtStyleButtons.h"

#include "diLinetype.h"
#include "qtUtility.h"

#include <QDialog>
#include <QGridLayout>
#include <QListWidget>
#include <QPixmap>

#include <memory>
#include <vector>

#include "linestyledialog.ui.h"

#define MILOGGER_CATEGORY "diana.StyleButtons"
#include <miLogger/miLogging.h>

namespace {

void set_selected(QListView* box, int row, int column=0)
{
  const QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current;
  QModelIndex idx = box->model()->index(row, column);
  box->selectionModel()->select(idx, flags);
  box->scrollTo(idx, QAbstractItemView::EnsureVisible);
}

void get_selected(QListView* box, std::string& text)
{
  const QModelIndexList selected = box->selectionModel()->selectedIndexes();
  if (selected.size() == 1)
    text = box->model()->data(selected.front(), Qt::DisplayRole).toString().toStdString();
}

// FIXME copied from qtUtility ------------------------- BEGIN

void installLinetypes(QListWidget* box)
{
  const std::vector<std::string>& slinetypenames = Linetype::getLinetypeNames();
  for (const std::string& ltname : slinetypenames) {
    const Linetype lt(ltname); // this makes a copy after looking up in the list of line types
    const QPixmap pmapLinetype(linePixmap(lt, 3));
    QListWidgetItem* item = new QListWidgetItem(box);
    item->setIcon(QIcon(pmapLinetype));
    item->setText(QString::fromStdString(lt.name));
    item->setData(Qt::UserRole, lt.bmap);
    box->addItem(item);
  }
}

void installLinewidths(QListWidget* box, int nr_linewidths=12)
{
  for (int i=0; i < nr_linewidths; i++) {
    QPixmap pmapLinewidth(linePixmap(i+1));
    QListWidgetItem* item = new QListWidgetItem(box);
    item->setIcon(QIcon(pmapLinewidth));
    item->setText(QString::number(i+1));
    item->setData(Qt::UserRole, i+1);
    box->addItem(item);
  }
}

static void ExpandColorBox(QListWidget* box, const QColor& pixcolor, const QString& name)
{
  QPixmap pmap(20, 20);
  pmap.fill(pixcolor);
  QListWidgetItem* item = new QListWidgetItem(box);
  item->setIcon(QIcon(pmap));
  item->setText(name);
  item->setData(Qt::UserRole, pixcolor);
  box->addItem(item);
}

void installColors(QListWidget* box, const std::vector<Colour::ColourInfo>& cInfo, bool name)
{
  const size_t nr_colors= cInfo.size();
  for (size_t t=0; t<nr_colors; t++) {
    const QColor pixcolor(cInfo[t].rgb[0],cInfo[t].rgb[1],cInfo[t].rgb[2] );
    ExpandColorBox(box, pixcolor, name ? QString::fromStdString(cInfo[t].name) : QString());
  }
}

void SetCurrentItemColorBox(QListWidget* box, const std::string& value)
{
  int nr_colors = box->count();
  QString col = value.c_str();
  int i = 0;
  while (i < nr_colors && col != box->item(i)->text())
    i++;
  if (i == nr_colors) {
    Colour::defineColourFromString(value);
    Colour c(value);
    const QColor pixcolor(c.R(), c.G(), c.B());
    ExpandColorBox(box, pixcolor, col);
  }
  set_selected(box, i);
}

// FIXME copied from qtUtility ------------------------- END

} // namespace

LineStyleButton::LineStyleButton(QWidget* parent)
  : QPushButton(parent)
  , m_enableColor(true)
  , m_enableWidth(true)
  , m_enableType(true)
  , m_linecolor("black")
  , m_linewidth("1")
  , m_linetype("solid")
{
  connect(this, &QPushButton::clicked, this, &LineStyleButton::showEditor);
  updatePixmap();
}

void LineStyleButton::setWhat(const QString& w)
{
  m_what = w;
}

void LineStyleButton::setLineColor(const std::string& c)
{
  m_linecolor = c;
  updatePixmap();
}

void LineStyleButton::setLineWidth(const std::string& lt)
{
  m_linewidth = lt;
  updatePixmap();
}

void LineStyleButton::setLineType(const std::string& lt)
{
  m_linetype = lt;
  updatePixmap();
}

void LineStyleButton::showEditor()
{
  QDialog popup(this);
  Ui_LineStyleDialog ui;
  ui.setupUi(&popup);
  if (!m_what.isEmpty())
    popup.setWindowTitle(tr("Style for %1 lines").arg(m_what));
  else
    popup.setWindowTitle(tr("Line style"));

  if (m_enableColor) {
    installColors(ui.linecolor, Colour::getColourInfo(), true);
    SetCurrentItemColorBox(ui.linecolor, m_linecolor);
  } else {
    ui.linecolor->setEnabled(false);
  }

  if (m_enableWidth) {
    installLinewidths(ui.linewidth);
    int m_linewIndex = atoi(m_linewidth.c_str()) - 1;
    if (m_linewIndex < 0 || m_linewIndex >= ui.linewidth->count())
      m_linewIndex = 0;
    set_selected(ui.linewidth, m_linewIndex);
  } else {
    ui.linewidth->setEnabled(false);
  }
  if (m_enableType) {
    installLinetypes(ui.linetype);
    int m_linetIndex = getIndex(Linetype::getLinetypeNames(), m_linetype);
    if (m_linetIndex < 0 || m_linetIndex >= ui.linetype->count())
      m_linetIndex = 0;
    set_selected(ui.linetype, m_linetIndex);
  } else {
    ui.linetype->setEnabled(false);
  }

  popup.move(mapToGlobal(QPoint(0,0)));

  if (popup.exec() == QDialog::Accepted) {
    get_selected(ui.linecolor, m_linecolor);
    get_selected(ui.linewidth, m_linewidth);
    get_selected(ui.linetype, m_linetype);
    updatePixmap();
    Q_EMIT changed();
  }
}

void LineStyleButton::updatePixmap()
{
  METLIBS_LOG_SCOPE();
  const Colour col(m_linecolor);
  QPixmap pmap(linePixmap(Linetype(m_linetype), std::atoi(m_linewidth.c_str()), col));
  setIcon(QIcon(pmap));

  setToolTip(tr("Color: %1\nWidth: %2\nType: %3")
             .arg(QString::fromStdString(m_linecolor))
             .arg(QString::fromStdString(m_linewidth))
             .arg(QString::fromStdString(m_linetype)));
}
